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

using modified_variable::ModifiedVariable;
using modified_variable::IntVariable;
using modified_variable::FloatVariable;
using modified_variable::StringVariable;
using modified_variable::BooleanVariable;
using modified_variable::IdVariable;
using modified_variable::ScalarVariable;

using client::Response;

using modified_response::ModifiedResponse;
using modified_response::IntResponse;
using modified_response::FloatResponse;
using modified_response::StringResponse;
using modified_response::BooleanResponse;
using modified_response::IdResponse;
using modified_response::ScalarResponse;
// clang-format on

} // namespace graphql::client
