// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "query/ProxyClient.h"

#include "schema/ProxySchema.h"
#include "schema/QueryObject.h"

#include "graphqlservice/JSONResponse.h"

#ifdef _MSC_VER
#include <SDKDDKVer.h>
#endif // _MSC_VER

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace std::literals;

using namespace graphql;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

constexpr auto c_host = "127.0.0.1"sv;
constexpr auto c_port = "8080"sv;
constexpr auto c_target = "/graphql"sv;
constexpr int c_version = 11; // HTTP 1.1

class Query
{
public:
	explicit Query(std::string_view host, std::string_view port, std::string_view target,
		int version) noexcept;

	std::future<std::string> getRelay(std::string&& queryArg,
		std::optional<std::string>&& operationNameArg,
		std::optional<std::string>&& variablesArg) const;

private:
	const std::string m_host;
	const std::string m_port;
	const std::string m_target;
	const int m_version;
};

Query::Query(
	std::string_view host, std::string_view port, std::string_view target, int version) noexcept
	: m_host { host }
	, m_port { port }
	, m_target { target }
	, m_version { version }
{
}

// Based on:
// https://www.boost.org/doc/libs/1_82_0/libs/beast/example/http/client/awaitable/http_client_awaitable.cpp
std::future<std::string> Query::getRelay(std::string&& queryArg,
	std::optional<std::string>&& operationNameArg, std::optional<std::string>&& variablesArg) const
{
	response::Value payload { response::Type::Map };

	payload.emplace_back("query"s, response::Value { std::move(queryArg) });

	if (operationNameArg)
	{
		payload.emplace_back("operationName"s, response::Value { std::move(*operationNameArg) });
	}

	if (variablesArg)
	{
		payload.emplace_back("variables"s, response::Value { std::move(*variablesArg) });
	}

	std::string requestBody = response::toJSON(std::move(payload));

	net::io_context ioc;
	auto future = net::co_spawn(
		ioc,
		[](const char* host,
			const char* port,
			const char* target,
			int version,
			std::string requestBody) -> net::awaitable<std::string> {
			// These objects perform our I/O. They use an executor with a default completion token
			// of use_awaitable. This makes our code easy, but will use exceptions as the default
			// error handling, i.e. if the connection drops, we might see an exception.
			auto resolver =
				net::use_awaitable.as_default_on(tcp::resolver(co_await net::this_coro::executor));
			auto stream = net::use_awaitable.as_default_on(
				beast::tcp_stream(co_await net::this_coro::executor));

			// Look up the domain name.
			const auto resolved = co_await resolver.async_resolve(host, port);

			// Set the timeout.
			stream.expires_after(std::chrono::seconds(30));

			// Make the connection on the IP address we get from a lookup
			stream.connect(resolved);

			// Set up an HTTP POST request message
			http::request<http::string_body> req { http::verb::post, target, version };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
			req.body() = std::move(requestBody);
			req.prepare_payload();

			// Set the timeout.
			stream.expires_after(std::chrono::seconds(30));

			// Send the HTTP request to the remote host
			co_await http::async_write(stream, req);

			// This buffer is used for reading and must be persisted
			beast::flat_buffer b;

			// Declare a container to hold the response
			http::response<http::string_body> res;

			// Receive the HTTP response
			co_await http::async_read(stream, b, res);

			// Gracefully close the socket
			beast::error_code ec;
			stream.socket().shutdown(tcp::socket::shutdown_both, ec);

			// not_connected happens sometimes
			// so don't bother reporting it.
			if (ec && ec != beast::errc::not_connected)
			{
				throw boost::system::system_error(ec, "shutdown");
			}

			co_return std::string { std::move(res.body()) };
		}(m_host.c_str(), m_port.c_str(), m_target.c_str(), m_version, std::move(requestBody)),
		net::use_future);

	ioc.run();

	return future;
}

int main(int argc, char** argv)
{
	auto service = std::make_shared<proxy::Operations>(std::make_shared<proxy::object::Query>(
		std::make_shared<Query>(c_host, c_port, c_target, c_version)));

	std::cout << "Created the service..." << std::endl;

	try
	{
		std::istream_iterator<char> start { std::cin >> std::noskipws }, end {};
		std::string input { start, end };

		std::cout << "Executing query..." << std::endl;

		using namespace client::query::relayQuery;

		auto query = GetRequestObject();
		auto variables = serializeVariables(
			{ input, ((argc > 1) ? std::make_optional(argv[1]) : std::nullopt) });
		auto launch = service::await_async { std::make_shared<service::await_worker_queue>() };
		auto serviceResponse = client::parseServiceResponse(
			service->resolve({ query, GetOperationName(), std::move(variables), launch }).get());
		auto result = client::query::relayQuery::parseResponse(std::move(serviceResponse.data));

		std::cout << result.relay << std::endl;
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
