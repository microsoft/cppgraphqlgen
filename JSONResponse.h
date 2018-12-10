// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "GraphQLResponse.h"

#define RAPIDJSON_NAMESPACE facebook::graphql::rapidjson
#define RAPIDJSON_NAMESPACE_BEGIN namespace facebook { namespace graphql { namespace rapidjson {
#define RAPIDJSON_NAMESPACE_END } /* namespace rapidjson */ } /* namespace graphql */ } /* namespace facebook */
#include <rapidjson/document.h>

namespace facebook {
namespace graphql {
namespace rapidjson {

Document convertResponse(response::Value&& response);

response::Value convertResponse(const Document& document);

} /* namespace rapidjson */
} /* namespace graphql */
} /* namespace facebook */
