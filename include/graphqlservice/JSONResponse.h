// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef JSONRESPONSE_H
#define JSONRESPONSE_H

#include "GraphQLResponse.h"

#include "internal/DllExports.h"

namespace graphql::response {

[[nodiscard("unnecessary conversion")]] JSONRESPONSE_EXPORT std::string toJSON(Value&& response);

[[nodiscard("unnecessary conversion")]] JSONRESPONSE_EXPORT Value parseJSON(
	const std::string& json);

} // namespace graphql::response

#endif // JSONRESPONSE_H
