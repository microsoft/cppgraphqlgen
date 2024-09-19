// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "query/ProxyClient.h"

#include "schema/ProxySchema.h"
#include "schema/QueryObject.h"
#include "schema/QueryResultsObject.h"

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

#include <algorithm>
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

struct AsyncIoWorker : service::RequestState
{
	AsyncIoWorker()
		: worker { std::make_shared<service::await_worker_thread>() }
	{
	}

	const service::await_async worker;
};

class Results
{
public:
	explicit Results(response::Value&& data, std::vector<client::Error> errors) noexcept;

	service::AwaitableScalar<std::optional<std::string>> getData(
		service::FieldParams&& fieldParams) const;
	service::AwaitableScalar<std::optional<std::vector<std::optional<std::string>>>> getErrors(
		service::FieldParams&& fieldParams) const;

private:
	mutable response::Value m_data;
	mutable std::vector<client::Error> m_errors;
};

Results::Results(response::Value&& data, std::vector<client::Error> errors) noexcept
	: m_data { std::move(data) }
	, m_errors { std::move(errors) }
{
}

service::AwaitableScalar<std::optional<std::string>> Results::getData(
	service::FieldParams&& fieldParams) const
{
	auto asyncIoWorker = std::static_pointer_cast<AsyncIoWorker>(fieldParams.state);
	auto data = std::move(m_data);

	// Jump to a worker thread for the resolver where we can run a separate I/O context without
	// blocking the I/O context in Query::getRelay. This simulates how you might fan out to
	// additional async I/O tasks for sub-field resolvers.
	co_await asyncIoWorker->worker;

	net::io_context ioc;
	auto future = net::co_spawn(
		ioc,
		[](response::Value&& data) -> net::awaitable<std::optional<std::string>> {
			co_return (data.type() == response::Type::Null)
				? std::nullopt
				: std::make_optional(response::toJSON(std::move(data)));
		}(std::move(data)),
		net::use_future);

	ioc.run();

	co_return future.get();
}

service::AwaitableScalar<std::optional<std::vector<std::optional<std::string>>>> Results::getErrors(
	service::FieldParams&& fieldParams) const
{
	auto asyncIoWorker = std::static_pointer_cast<AsyncIoWorker>(fieldParams.state);
	auto errors = std::move(m_errors);

	// Jump to a worker thread for the resolver where we can run a separate I/O context without
	// blocking the I/O context in Query::getRelay. This simulates how you might fan out to
	// additional async I/O tasks for sub-field resolvers.
	co_await asyncIoWorker->worker;

	net::io_context ioc;
	auto future = net::co_spawn(
		ioc,
		[](std::vector<client::Error> errors)
			-> net::awaitable<std::optional<std::vector<std::optional<std::string>>>> {
			if (errors.empty())
			{
				co_return std::nullopt;
			}

			std::vector<std::optional<std::string>> results { errors.size() };

			std::transform(errors.begin(),
				errors.end(),
				results.begin(),
				[](auto& error) noexcept -> std::optional<std::string> {
					return error.message.empty()
						? std::nullopt
						: std::make_optional<std::string>(std::move(error.message));
				});

			co_return std::make_optional(results);
		}(std::move(errors)),
		net::use_future);

	ioc.run();

	co_return future.get();
}

class Query
{
public:
	explicit Query(std::string_view host, std::string_view port, std::string_view target,
		int version) noexcept;

	std::future<std::shared_ptr<proxy::object::QueryResults>> getRelay(
		proxy::QueryInput&& inputArg) const;

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
std::future<std::shared_ptr<proxy::object::QueryResults>> Query::getRelay(
	proxy::QueryInput&& inputArg) const
{
	response::Value payload { response::Type::Map };

	payload.emplace_back("query"s, response::Value { std::move(inputArg.query) });

	if (inputArg.operationName)
	{
		payload.emplace_back("operationName"s,
			response::Value { std::move(*inputArg.operationName) });
	}

	if (inputArg.variables)
	{
		payload.emplace_back("variables"s, response::Value { std::move(*inputArg.variables) });
	}

	std::string requestBody = response::toJSON(std::move(payload));

	net::io_context ioc;
	auto future = net::co_spawn(
		ioc,
		[](const char* host,
			const char* port,
			const char* target,
			int version,
			std::string requestBody)
			-> net::awaitable<std::shared_ptr<proxy::object::QueryResults>> {
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

			auto [data, errors] = client::parseServiceResponse(response::parseJSON(res.body()));

			co_return std::make_shared<proxy::object::QueryResults>(
				std::make_shared<Results>(std::move(data), std::move(errors)));
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

		using namespace proxy::client::query::relayQuery;

		auto query = GetRequestObject();
		auto variables = serializeVariables({ QueryInput { OperationType::QUERY,
			input,
			((argc > 1) ? std::make_optional(argv[1]) : std::nullopt),
			std::nullopt } });
		auto launch = service::await_async { std::make_shared<service::await_worker_queue>() };
		auto state = std::make_shared<AsyncIoWorker>();
		auto serviceResponse = client::parseServiceResponse(
			service->resolve({ query, GetOperationName(), std::move(variables), launch, state })
				.get());
		auto result = parseResponse(std::move(serviceResponse.data));
		auto errors = std::move(serviceResponse.errors);

		if (result.relay.data)
		{
			std::cout << "Data: " << *result.relay.data << std::endl;
		}

		if (result.relay.errors)
		{
			for (const auto& message : *result.relay.errors)
			{
				std::cerr << "Remote Error: "
						  << (message ? std::string_view { *message } : "<empty>"sv) << std::endl;
			}
		}

		if (!errors.empty())
		{
			std::cerr << "Errors executing query:" << std::endl << GetRequestText() << std::endl;

			for (const auto& error : errors)
			{
				std::cerr << "Error: " << error.message;

				if (!error.locations.empty())
				{
					bool firstLocation = true;

					std::cerr << ", Locations: [";

					for (const auto& location : error.locations)
					{
						if (!firstLocation)
						{
							std::cerr << ", ";
						}

						firstLocation = false;

						std::cerr << "(line: " << location.line << ", column: " << location.column
								  << ")";
					}

					std::cerr << "]";
				}

				if (!error.path.empty())
				{
					bool firstSegment = true;

					std::cerr << ", Path: ";

					for (const auto& segment : error.path)
					{
						std::visit(
							[firstSegment](const auto& value) noexcept {
								using _Type = std::decay_t<decltype(value)>;

								if constexpr (std::is_same_v<std::string, _Type>)
								{
									if (!firstSegment)
									{
										std::cerr << "/";
									}

									std::cerr << value;
								}
								else if constexpr (std::is_same_v<int, _Type>)
								{
									std::cerr << "[" << value << "]";
								}
							},
							segment);

						firstSegment = false;
					}
				}

				std::cerr << std::endl;
			}
		}
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
