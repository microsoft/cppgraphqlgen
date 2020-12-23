// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayMock.h"

#include "graphqlservice/JSONResponse.h"

#include <cstdio>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

using namespace graphql;

using namespace std::literals;

int main(int argc, char** argv)
{
	response::IdType binAppointmentId;
	response::IdType binTaskId;
	response::IdType binFolderId;

	std::string fakeAppointmentId("fakeAppointmentId");
	binAppointmentId.resize(fakeAppointmentId.size());
	std::copy(fakeAppointmentId.cbegin(), fakeAppointmentId.cend(), binAppointmentId.begin());

	std::string fakeTaskId("fakeTaskId");
	binTaskId.resize(fakeTaskId.size());
	std::copy(fakeTaskId.cbegin(), fakeTaskId.cend(), binTaskId.begin());

	std::string fakeFolderId("fakeFolderId");
	binFolderId.resize(fakeFolderId.size());
	std::copy(fakeFolderId.cbegin(), fakeFolderId.cend(), binFolderId.begin());

	auto query = std::make_shared<today::Query>(
		[&binAppointmentId]() -> std::vector<std::shared_ptr<today::Appointment>> {
			std::cout << "Called getAppointments..." << std::endl;
			return { std::make_shared<today::Appointment>(std::move(binAppointmentId),
				"tomorrow",
				"Lunch?",
				false) };
		},
		[&binTaskId]() -> std::vector<std::shared_ptr<today::Task>> {
			std::cout << "Called getTasks..." << std::endl;
			return { std::make_shared<today::Task>(std::move(binTaskId), "Don't forget", true) };
		},
		[&binFolderId]() -> std::vector<std::shared_ptr<today::Folder>> {
			std::cout << "Called getUnreadCounts..." << std::endl;
			return { std::make_shared<today::Folder>(std::move(binFolderId), "\"Fake\" Inbox", 3) };
		});
	auto mutation = std::make_shared<today::Mutation>(
		[](today::CompleteTaskInput&& input) -> std::shared_ptr<today::CompleteTaskPayload> {
			return std::make_shared<today::CompleteTaskPayload>(
				std::make_shared<today::Task>(std::move(input.id),
					"Mutated Task!",
					*(input.isComplete)),
				std::move(input.clientMutationId));
		});
	auto subscription = std::make_shared<today::Subscription>();
	auto service = std::make_shared<today::Operations>(query, mutation, subscription);

	std::cout << "Created the service..." << std::endl;

	const size_t iterations = [](const char* arg) noexcept -> size_t {
		if (arg)
		{
			const int parsed = std::atoi(arg);

			if (parsed > 0)
			{
				return static_cast<size_t>(parsed);
			}
		}

		// Default to 100 iterations
		return 100;
	}((argc > 1) ? argv[1] : nullptr);

	for (size_t i = 0; i < iterations; ++i)
	{
		try
		{
			auto query = peg::parseString(R"gql(query {
				appointments {
					pageInfo { hasNextPage }
					edges {
						node {
							id
							when
							subject
							isNow
						}
					}
				}
			})gql"sv);

			std::cout << "Executing query..." << std::endl;

			std::cout << response::toJSON(
				service->resolve(nullptr, query, "", response::Value(response::Type::Map)).get())
					  << std::endl;
		}
		catch (const std::runtime_error& ex)
		{
			std::cerr << ex.what() << std::endl;
			return 1;
		}
	}

	return 0;
}
