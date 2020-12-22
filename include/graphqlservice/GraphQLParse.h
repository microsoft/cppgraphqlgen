// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLPARSE_H
#define GRAPHQLPARSE_H

// clang-format off
#ifdef GRAPHQL_DLLEXPORTS
	#ifdef IMPL_GRAPHQLPEG_DLL
		#define GRAPHQLPEG_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLPEG_DLL
		#define GRAPHQLPEG_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLPEG_DLL
#else // !GRAPHQL_DLLEXPORTS
	#define GRAPHQLPEG_EXPORT
#endif // !GRAPHQL_DLLEXPORTS
// clang-format on

#include <memory>
#include <string_view>

namespace graphql {
namespace peg {

class ast_node;
struct ast_input;

struct ast
{
	std::shared_ptr<ast_input> input;
	std::shared_ptr<ast_node> root;
	bool validated = false;
};

GRAPHQLPEG_EXPORT ast parseString(std::string_view input);
GRAPHQLPEG_EXPORT ast parseFile(std::string_view filename);

} /* namespace peg */

GRAPHQLPEG_EXPORT peg::ast operator"" _graphql(const char* text, size_t size);

} /* namespace graphql */

#endif // GRAPHQLPARSE_H
