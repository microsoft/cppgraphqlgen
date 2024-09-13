// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "GraphQLClient.h"

export module GraphQL.Client;

export namespace graphql::client {

// clang-format off
using client::ErrorLocation;
using client::ErrorPathSegment;
using client::Error;
using client::ServiceResponse;
using client::parseServiceResponse;
using client::TypeModifier;
using client::Variable;

using client::ModifiedVariable;
using client::IntVariable;
using client::FloatVariable;
using client::StringVariable;
using client::BooleanVariable;
using client::IdVariable;
using client::ScalarVariable;

using client::Response;

using client::ModifiedResponse;
using client::IntResponse;
using client::FloatResponse;
using client::StringResponse;
using client::BooleanResponse;
using client::IdResponse;
using client::ScalarResponse;
// clang-format on

} // namespace graphql::client
