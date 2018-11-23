// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "GraphQLGrammar.h"

#include <tao/pegtl/contrib/parse_tree.hpp>

#include <string>
#include <functional>

namespace facebook {
namespace graphql {
namespace peg {

using namespace tao::pegtl;

struct ast_node
	: parse_tree::basic_node<ast_node>
{
	std::string unescaped;
};

template <typename _Rule>
void for_each_child(const ast_node& n, std::function<bool(const ast_node&)>&& func)
{
	for (const auto& child : n.children)
	{
		if (child->is<_Rule>()
			&& !func(*child))
		{
			return;
		}
	}
}

std::unique_ptr<ast_node> parseString(const char* text);
std::unique_ptr<ast_node> parseFile(file_input<>&& in);

} /* namespace peg */
} /* namespace graphql */
} /* namespace facebook */