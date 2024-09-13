// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "GraphQLResponse.h"

export module GraphQL.Response;

namespace included = graphql::response;

export namespace graphql::response {

namespace exported {

// clang-format off
using included::Type;

using included::MapType;
using included::ListType;
using included::StringType;
using included::BooleanType;
using included::IntType;
using included::FloatType;
using included::ScalarType;

using included::IdType;

using included::ValueTypeTraits;
using included::Value;
using included::AwaitableValue;

using included::Writer;
// clang-format on

} // namespace exported

using namespace exported;

} // namespace graphql::response
