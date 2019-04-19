// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "SeparateToday.h"

#include <graphqlservice/JSONResponse.h>

#include <iostream>
#include <stdexcept>
#include <cstdio>

using namespace facebook::graphql;

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

	try
	{
		const peg::ast_node* ast = nullptr;
		peg::ast<std::vector<char>> ast_input;
		peg::ast<std::unique_ptr<peg::file_input<>>> ast_file;

		if (argc > 1)
		{
			ast_file = peg::parseFile(argv[1]);
			ast = ast_file.root.get();
		}
		else
		{
			std::string input;
			std::string line;

			while (std::getline(std::cin, line))
			{
				input.append(line);
			}

			ast_input = peg::parseString(std::move(input));
			ast = ast_input.root.get();
		}

		if (!ast)
		{
			std::cerr << "Unknown error!" << std::endl;
			std::cerr << std::endl;
			return 1;
		}

		std::cout << "Executing query..." << std::endl;

		std::cout << response::toJSON(service->resolve(nullptr, *ast, ((argc > 2) ? argv[2] : ""), response::Value(response::Type::Map)).get()) << std::endl;
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
