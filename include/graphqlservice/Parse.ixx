// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "GraphQLParse.h"

export module GraphQL.Parse;

namespace included = graphql::peg;

export namespace graphql {

namespace peg {

namespace exported {

// clang-format off
using included::ast_node;
using included::ast_input;
using included::ast;

constexpr std::size_t c_defaultDepthLimit = included::c_defaultDepthLimit;

using included::parseSchemaString;
using included::parseSchemaFile;
using included::parseString;
using included::parseFile;
// clang-format on

} // namespace exported

using namespace exported;

} // namespace peg

using graphql::operator"" _graphql;

} // namespace graphql
