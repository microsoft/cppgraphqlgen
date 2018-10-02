// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Today.h"

#include <graphqlparser/GraphQLParser.h>

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <iostream>

using namespace facebook::graphql;

int main(int argc, char** argv)
{
	std::vector<unsigned char> binAppointmentId;
	std::vector<unsigned char> binTaskId;
	std::vector<unsigned char> binFolderId;

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
		[&binAppointmentId]() -> std::vector<std::shared_ptr<today::Appointment>>
	{
		std::cout << "Called getAppointments..." << std::endl;
		return { std::make_shared<today::Appointment>(std::move(binAppointmentId), "tomorrow", "Lunch?", false) };
	}, [&binTaskId]() -> std::vector<std::shared_ptr<today::Task>>
	{
		std::cout << "Called getTasks..." << std::endl;
		return { std::make_shared<today::Task>(std::move(binTaskId), "Don't forget", true) };
	}, [&binFolderId]() -> std::vector<std::shared_ptr<today::Folder>>
	{
		std::cout << "Called getUnreadCounts..." << std::endl;
		return { std::make_shared<today::Folder>(std::move(binFolderId), "\"Fake\" Inbox", 3) };
	});
	auto mutation = std::make_shared<today::Mutation>(
		[](today::CompleteTaskInput&& input) -> std::shared_ptr<today::CompleteTaskPayload>
	{
		return std::make_shared<today::CompleteTaskPayload>(
			std::make_shared<today::Task>(std::move(input.id), "Mutated Task!", *(input.isComplete)),
			std::move(input.clientMutationId)
		);
	});
	auto subscription = std::make_shared<today::Subscription>();
	auto service = std::make_shared<today::Operations>(query, mutation, subscription);

	std::cout << "Created the service..." << std::endl;

	FILE* in = stdin;

	if (argc > 1)
	{
		in = std::fopen(argv[1], "r");

		if (nullptr == in)
		{
			std::cerr << "Could not open the file: " << argv[1] << std::endl;
			std::cerr << "Error: " << std::strerror(errno) << std::endl;
			return 1;
		}
	}

	const char* error = nullptr;
	auto ast = parseFile(in, &error);

	if (argc > 1)
	{
		std::fclose(in);
	}

	if (!ast)
	{
		if (nullptr == error)
		{
			std::cerr << "Unknown error!";
		}
		else
		{
			std::cerr << error;
			free(const_cast<char*>(error));
		}
		std::cerr << std::endl;
		return 1;
	}

	std::cout << "Executing query..." << std::endl;

	utility::ostringstream_t output;
	output << service->resolve(*ast, ((argc > 2) ? argv[2] : ""), web::json::value::object().as_object());
	std::cout << utility::conversions::to_utf8string(output.str()) << std::endl;

	return 0;
}
