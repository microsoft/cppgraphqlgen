// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#define TAO_PEGTL_NAMESPACE tao::graphqlpeg

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include <vector>
#include <string_view>
#include <functional>

namespace facebook::graphql {
namespace peg {

using namespace tao::graphqlpeg;

struct ast_node
	: parse_tree::basic_node<ast_node>
{
	std::string unescaped;
};

template <typename Input>
struct ast
{
	ast() = default;
	ast(ast&& other) = default;
	~ast();

	ast& operator=(ast&& other) = default;

	Input input;
	std::unique_ptr<ast_node> root;
};

ast<std::vector<char>> parseString(std::string_view input);
ast<std::unique_ptr<file_input<>>> parseFile(std::string_view filename);

} /* namespace peg */

peg::ast<const char*> operator "" _graphql(const char* text, size_t size);

} /* namespace facebook::graphql */
