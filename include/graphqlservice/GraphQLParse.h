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

struct [[nodiscard("unnecessary parse")]] ast
{
	std::shared_ptr<ast_input> input;
	std::shared_ptr<ast_node> root;
	bool validated = false;
};

// By default, we want to limit the depth of nested nodes. You can override this with
// another value for the depthLimit parameter in these parse functions.
constexpr size_t c_defaultDepthLimit = 25;

[[nodiscard("unnecessary parse")]] GRAPHQLPEG_EXPORT ast parseSchemaString(
	std::string_view input, size_t depthLimit = c_defaultDepthLimit);
[[nodiscard("unnecessary parse")]] GRAPHQLPEG_EXPORT ast parseSchemaFile(
	std::string_view filename, size_t depthLimit = c_defaultDepthLimit);

[[nodiscard("unnecessary parse")]] GRAPHQLPEG_EXPORT ast parseString(
	std::string_view input, size_t depthLimit = c_defaultDepthLimit);
[[nodiscard("unnecessary parse")]] GRAPHQLPEG_EXPORT ast parseFile(
	std::string_view filename, size_t depthLimit = c_defaultDepthLimit);

} // namespace peg

[[nodiscard("unnecessary parse")]] GRAPHQLPEG_EXPORT peg::ast operator"" _graphql(
	const char* text, size_t size);

} // namespace graphql

#endif // GRAPHQLPARSE_H
