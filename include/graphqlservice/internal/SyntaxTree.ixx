// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "SyntaxTree.h"

export module GraphQL.Internal.SyntaxTree;

export namespace graphql::peg {

namespace exported {

// clang-format off
using namespace tao::graphqlpeg;
namespace peginternal = tao::graphqlpeg::internal;

using graphql::peg::ast_node;
// clang-format on

} // namespace exported

using namespace exported;

} // namespace graphql::peg
