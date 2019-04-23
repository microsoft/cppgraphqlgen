// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <memory>
#include <string_view>

namespace facebook::graphql {
namespace peg {

struct ast_node;
struct ast_input;

struct ast
{
	std::shared_ptr<ast_input> input;
	std::shared_ptr<ast_node> root;
};

ast parseString(std::string_view input);
ast parseFile(std::string_view filename);

} /* namespace peg */

peg::ast operator "" _graphql(const char* text, size_t size);

} /* namespace facebook::graphql */
