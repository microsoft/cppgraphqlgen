// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#define TAO_PEGTL_NAMESPACE graphqlpeg

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include <string>
#include <functional>

namespace facebook {
namespace graphql {
namespace peg {

using namespace tao::graphqlpeg;

struct ast_node
	: parse_tree::basic_node<ast_node>
{
	std::string unescaped;
};

template <typename _Input>
struct ast
{
	_Input input;
	std::unique_ptr<ast_node> root;
};

std::unique_ptr<ast<std::string>> parseString(std::string&& input);
std::unique_ptr<ast<std::unique_ptr<file_input<>>>> parseFile(const char* filename);

} /* namespace peg */

std::unique_ptr<peg::ast<const char*>> operator "" _graphql(const char* text, size_t size);

} /* namespace graphql */
} /* namespace facebook */