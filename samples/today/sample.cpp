// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <iostream>
#include <iterator>
#include <stdexcept>

import GraphQL.Parse;
import GraphQL.Response;
import GraphQL.JSONResponse;
import GraphQL.Service;

import GraphQL.Today.TodaySchema;

import GraphQL.Today.QueryObject;
import GraphQL.Today.MutationObject;
import GraphQL.Today.SubscriptionObject;

import GraphQL.Today.AppointmentEdgeObject;
import GraphQL.Today.AppointmentObject;
import GraphQL.Today.AppointmentConnectionObject;
import GraphQL.Today.CompleteTaskPayloadObject;
import GraphQL.Today.ExpensiveObject;
import GraphQL.Today.FolderEdgeObject;
import GraphQL.Today.FolderObject;
import GraphQL.Today.FolderConnectionObject;
import GraphQL.Today.NestedTypeObject;
import GraphQL.Today.NodeObject;
import GraphQL.Today.PageInfoObject;
import GraphQL.Today.TaskConnectionObject;
import GraphQL.Today.TaskEdgeObject;
import GraphQL.Today.TaskObject;
import GraphQL.Today.UnionTypeObject;

import GraphQL.Today.Mock;

using namespace graphql;

int main(int argc, char** argv)
{
	const auto mockService = graphql::today::mock_service();
	const auto& service = mockService->service;

	std::cout << "Created the service..." << std::endl;

	try
	{
		peg::ast ast;

		if (argc > 1)
		{
			ast = peg::parseFile(argv[1]);
		}
		else
		{
			std::istream_iterator<char> start { std::cin >> std::noskipws }, end {};
			std::string input { start, end };

			ast = peg::parseString(std::move(input));
		}

		if (!ast.root)
		{
			std::cerr << "Unknown error!" << std::endl;
			std::cerr << std::endl;
			return 1;
		}

		std::cout << "Executing query..." << std::endl;

		std::cout << response::toJSON(service->resolve({ ast, ((argc > 2) ? argv[2] : "") }).get())
				  << std::endl;
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
