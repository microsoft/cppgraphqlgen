// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLPARSE_H
#define GRAPHQLPARSE_H

#include <memory>
#include <string_view>

namespace graphql {
namespace peg {

struct ast_node;
struct ast_input;

struct ast
{
	std::shared_ptr<ast_input> input;
	std::shared_ptr<ast_node> root;
	bool validated = false;
};

ast parseString(std::string_view input);
ast parseFile(std::string_view filename);

} /* namespace peg */

peg::ast operator "" _graphql(const char* text, size_t size);

} /* namespace graphql */

#endif // GRAPHQLPARSE_H
