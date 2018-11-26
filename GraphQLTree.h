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

std::unique_ptr<ast_node> parseString(const char* text);
std::unique_ptr<ast_node> parseFile(file_input<>&& in);

} /* namespace peg */
} /* namespace graphql */
} /* namespace facebook */