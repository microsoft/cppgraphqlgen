// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef JSONRESPONSE_H
#define JSONRESPONSE_H

// clang-format off
#ifdef GRAPHQL_DLLEXPORTS
	#ifdef IMPL_JSONRESPONSE_DLL
		#define JSONRESPONSE_EXPORT __declspec(dllexport)
	#else // !IMPL_JSONRESPONSE_DLL
		#define JSONRESPONSE_EXPORT __declspec(dllimport)
	#endif // !IMPL_JSONRESPONSE_DLL
#else // !GRAPHQL_DLLEXPORTS
	#define JSONRESPONSE_EXPORT
#endif // !GRAPHQL_DLLEXPORTS
// clang-format on

#include "graphqlservice/GraphQLResponse.h"

namespace graphql::response {

[[nodiscard("unnecessary conversion")]] JSONRESPONSE_EXPORT std::string toJSON(Value&& response);

[[nodiscard("unnecessary conversion")]] JSONRESPONSE_EXPORT Value parseJSON(
	const std::string& json);

} // namespace graphql::response

#endif // JSONRESPONSE_H
