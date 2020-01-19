// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLTREE_H
#define GRAPHQLTREE_H

#define TAO_PEGTL_NAMESPACE tao::graphqlpeg

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include <string>
#include <variant>
#include <vector>

namespace graphql::peg {

using namespace tao::graphqlpeg;

struct ast_node : parse_tree::basic_node<ast_node>
{
	std::string unescaped;
};

struct ast_input
{
	std::variant<std::vector<char>, std::unique_ptr<file_input<>>, std::string_view> data;
};

} /* namespace graphql::peg */

#endif // GRAPHQLTREE_H
