// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef JSONRESPONSE_H
#define JSONRESPONSE_H

#include <graphqlservice/GraphQLResponse.h>

namespace graphql::response {

std::string toJSON(Value&& response);

Value parseJSON(const std::string& json);

} /* namespace graphql::response */

#endif // JSONRESPONSE_H