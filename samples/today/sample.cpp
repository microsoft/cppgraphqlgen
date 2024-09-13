// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/JSONResponse.h"

#include <cstdio>
#include <iostream>
#include <iterator>
#include <stdexcept>

import GraphQL.Parse;

import GraphQL.Today.Mock;

using namespace graphql;

int main(int argc, char** argv)
{
	const auto mockService = today::mock_service();
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
