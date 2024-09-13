// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "SyntaxTree.h"

export module GraphQL.Internal:SyntaxTree;

export namespace graphql::peg {

namespace exported {

using namespace tao::graphqlpeg;
namespace peginternal = tao::graphqlpeg::internal;

using ast_node = graphql::peg::ast_node;

} // namespace exported

using namespace exported;

} // namespace graphql::peg
