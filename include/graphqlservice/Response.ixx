// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "GraphQLResponse.h"

export module GraphQL.Response;

export import GraphQL.Internal.Awaitable;

export namespace graphql::response {

// clang-format off
using response::Type;

using response::MapType;
using response::ListType;
using response::StringType;
using response::BooleanType;
using response::IntType;
using response::FloatType;
using response::ScalarType;

using response::IdType;

using response::ValueTypeTraits;
using response::Value;
using response::AwaitableValue;

using response::ValueVisitor;
using response::ValueToken;
using response::ValueTokenStream;
// clang-format on

} // namespace graphql::response
