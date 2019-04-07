// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <graphqlservice/GraphQLResponse.h>

namespace facebook::graphql::response {

std::string toJSON(Value&& response);

Value parseJSON(const std::string& json);

} /* namespace facebook::graphql::response */
