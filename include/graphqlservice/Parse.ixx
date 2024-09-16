// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "GraphQLParse.h"

export module GraphQL.Parse;

export namespace graphql {

namespace peg {

// clang-format off
using peg::ast_node;
using peg::ast_input;
using peg::ast;

namespace constants {

constexpr std::size_t c_defaultDepthLimit = peg::c_defaultDepthLimit;

} // namespace constants

using peg::parseSchemaString;
using peg::parseSchemaFile;
using peg::parseString;
using peg::parseFile;
// clang-format on

} // namespace peg

using graphql::operator"" _graphql;

} // namespace graphql
