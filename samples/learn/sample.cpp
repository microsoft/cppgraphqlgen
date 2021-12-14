// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "StarWarsData.h"

#include "graphqlservice/JSONResponse.h"

#include <cstdio>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace graphql;

int main(int argc, char** argv)
{
	auto service = star_wars::GetService();

	std::cout << "Created the service..." << std::endl;

	try
	{
		peg::ast query;

		if (argc > 1)
		{
			query = peg::parseFile(argv[1]);
		}
		else
		{
			std::istream_iterator<char> start { std::cin >> std::noskipws }, end {};
			std::string input { start, end };

			query = peg::parseString(std::move(input));
		}

		if (!query.root)
		{
			std::cerr << "Unknown error!" << std::endl;
			std::cerr << std::endl;
			return 1;
		}

		std::cout << "Executing query..." << std::endl;

		std::cout << response::toJSON(
			service->resolve({ query, ((argc > 2) ? argv[2] : "") }).get())
				  << std::endl;
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
