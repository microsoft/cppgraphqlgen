// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "GraphQLClient.h"

export module GraphQL.Client;

namespace included = graphql::client;

export namespace graphql::client {

namespace exported {

// clang-format off
using included::ErrorLocation;
using included::ErrorPathSegment;
using included::Error;
using included::ServiceResponse;
using included::parseServiceResponse;
using included::TypeModifier;
using included::Variable;

using included::ModifiedVariable;
using included::IntVariable;
using included::FloatVariable;
using included::StringVariable;
using included::BooleanVariable;
using included::IdVariable;
using included::ScalarVariable;

using included::Response;

using included::ModifiedResponse;
using included::IntResponse;
using included::FloatResponse;
using included::StringResponse;
using included::BooleanResponse;
using included::IdResponse;
using included::ScalarResponse;
// clang-format on

} // namespace exported

using namespace exported;

} // namespace graphql::included
