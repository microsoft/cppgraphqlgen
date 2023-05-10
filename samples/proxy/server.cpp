// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "StarWarsData.h"

#include "graphqlservice/JSONResponse.h"

#ifdef _MSC_VER
#include <SDKDDKVer.h>
#endif // _MSC_VER

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <boost/config.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::literals;

using namespace graphql;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

using tcp_stream = typename beast::tcp_stream::rebind_executor<
	net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;

constexpr net::string_view c_host { "127.0.0.1" };
constexpr unsigned short c_port = 8080;
constexpr net::string_view c_target { "/graphql" };

// Based on:
// https://www.boost.org/doc/libs/1_82_0/libs/beast/example/http/server/awaitable/http_server_awaitable.cpp
int main()
{
	auto service = star_wars::GetService();

	std::cout << "Created the service..." << std::endl;

	const auto address = net::ip::make_address(c_host);

	// The io_context is required for all I/O.
	net::io_context ioc;

	// Spawn a listening port.
	net::co_spawn(
		ioc,
		[](tcp::endpoint endpoint, std::shared_ptr<service::Request> service)
			-> net::awaitable<void> {
			// Open the acceptor.
			auto acceptor =
				net::use_awaitable.as_default_on(tcp::acceptor(co_await net::this_coro::executor));
			acceptor.open(endpoint.protocol());

			// Allow address reuse.
			acceptor.set_option(net::socket_base::reuse_address(true));

			// Bind to the server address.
			acceptor.bind(endpoint);

			// Start listening for connections.
			acceptor.listen(net::socket_base::max_listen_connections);

			while (true)
			{
				net::co_spawn(
					acceptor.get_executor(),
					[](tcp_stream stream, std::shared_ptr<service::Request> service)
						-> net::awaitable<void> {
						// This buffer is required to persist across reads
						beast::flat_buffer buffer;

						bool keepAlive = false;

						// This lambda is used to send messages
						try
						{
							do
							{
								// Set the timeout.
								stream.expires_after(std::chrono::seconds(30));

								// Read a request
								http::request<http::string_body> req;
								co_await http::async_read(stream, buffer, req);

								// Handle the request
								http::response<http::string_body> msg;

								try
								{
									if (req.method() != http::verb::post
										|| req.target() != c_target)
									{
										throw std::runtime_error(
											"Only POST requests to /graphql are supported.");
									}
									else
									{
										std::ostringstream oss;

										oss << req.body();

										auto payload = response::parseJSON(oss.str());

										if (payload.type() != response::Type::Map)
										{
											throw std::runtime_error("Invalid request!");
										}

										const auto queryItr = payload.find("query"sv);

										if (queryItr == payload.end()
											|| queryItr->second.type() != response::Type::String)
										{
											throw std::runtime_error("Invalid request!");
										}

										peg::ast query;

										query = peg::parseString(
											queryItr->second.get<response::StringType>());

										if (!query.root)
										{
											throw std::runtime_error("Unknown error!");
										}

										const auto operationNameItr =
											payload.find("operationName"sv);
										const auto operationName =
											(operationNameItr != payload.end()
												&& operationNameItr->second.type()
													== response::Type::String)
											? std::string_view { operationNameItr->second
																	 .get<response::StringType>() }
											: std::string_view {};

										const auto variablesItr = payload.find("variables"sv);
										auto variables = (variablesItr != payload.end()
															 && variablesItr->second.type()
																 == response::Type::String)
											? response::parseJSON(operationNameItr->second
																	  .get<response::StringType>())
											: response::Value {};

										msg = http::response<http::string_body> { http::status::ok,
											req.version() };
										msg.set(http::field::server, BOOST_BEAST_VERSION_STRING);
										msg.set(http::field::content_type, "application/json");
										msg.keep_alive(req.keep_alive());
										msg.body() = response::toJSON(
											service
												->resolve(
													{ query, operationName, std::move(variables) })
												.get());
										msg.prepare_payload();
									}
								}
								catch (const std::runtime_error& ex)
								{
									msg = http::response<http::string_body> {
										http::status::bad_request,
										req.version()
									};
									msg.set(http::field::server, BOOST_BEAST_VERSION_STRING);
									msg.set(http::field::content_type, "text/plain");
									msg.keep_alive(req.keep_alive());
									msg.body() = "Error: "s + ex.what();
									msg.prepare_payload();
								}

								// Determine if we should close the connection
								keepAlive = msg.keep_alive();

								// Send the response
								co_await beast::async_write(stream,
									http::message_generator { std::move(msg) },
									net::use_awaitable);

							} while (keepAlive);
						}
						catch (boost::system::system_error& se)
						{
							if (se.code() != http::error::end_of_stream)
								throw;
						}

						// Send a TCP shutdown
						beast::error_code ec;
						stream.socket().shutdown(tcp::socket::shutdown_send, ec);

						// At this point the connection is closed gracefully
						// we ignore the error because the client might have
						// dropped the connection already.
					}(tcp_stream(co_await acceptor.async_accept()), service),
					[](std::exception_ptr exp) {
						if (exp)
						{
							try
							{
								std::rethrow_exception(exp);
							}
							catch (const std::exception& ex)
							{
								std::cerr << "Session error: " << ex.what() << std::endl;
							}
						}
					});
			}
		}(tcp::endpoint(address, c_port), std::move(service)),
		[](std::exception_ptr exp) {
			if (exp)
			{
				std::rethrow_exception(exp);
			}
		});

	try
	{
		ioc.run();
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
