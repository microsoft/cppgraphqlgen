// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef BENCHMARKCLIENT_H
#define BENCHMARKCLIENT_H

#include "graphqlservice/GraphQLClient.h"
#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLResponse.h"

#include <optional>
#include <string>
#include <vector>

import Internal.Version;

// Check if the library version is compatible with clientgen 4.5.0
static_assert(graphql::internal::MajorVersion == 4, "regenerate with clientgen: major version mismatch");
static_assert(graphql::internal::MinorVersion == 5, "regenerate with clientgen: minor version mismatch");

namespace graphql::client {

/// <summary>
/// Operation: query (unnamed)
/// </summary>
/// <code class="language-graphql">
/// # Copyright (c) Microsoft Corporation. All rights reserved.
/// # Licensed under the MIT License.
/// 
/// query {
///   appointments {
///     pageInfo {
///       hasNextPage
///     }
///     edges {
///       node {
///         id
///         when
///         subject
///         isNow
///       }
///     }
///   }
/// }
/// </code>
namespace benchmark {

// Return the original text of the request document.
[[nodiscard("unnecessary call")]] const std::string& GetRequestText() noexcept;

// Return a pre-parsed, pre-validated request object.
[[nodiscard("unnecessary call")]] const peg::ast& GetRequestObject() noexcept;

} // namespace benchmark

namespace query::Query {

using benchmark::GetRequestText;
using benchmark::GetRequestObject;

// Return the name of this operation in the shared request document.
[[nodiscard("unnecessary call")]] const std::string& GetOperationName() noexcept;

struct [[nodiscard("unnecessary construction")]] Response
{
	struct [[nodiscard("unnecessary construction")]] appointments_AppointmentConnection
	{
		struct [[nodiscard("unnecessary construction")]] pageInfo_PageInfo
		{
			bool hasNextPage {};
		};

		struct [[nodiscard("unnecessary construction")]] edges_AppointmentEdge
		{
			struct [[nodiscard("unnecessary construction")]] node_Appointment
			{
				response::IdType id {};
				std::optional<response::Value> when {};
				std::optional<std::string> subject {};
				bool isNow {};
			};

			std::optional<node_Appointment> node {};
		};

		pageInfo_PageInfo pageInfo {};
		std::optional<std::vector<std::optional<edges_AppointmentEdge>>> edges {};
	};

	appointments_AppointmentConnection appointments {};
};

[[nodiscard("unnecessary conversion")]] Response parseResponse(response::Value&& response);

struct Traits
{
	[[nodiscard("unnecessary call")]] static const std::string& GetRequestText() noexcept;
	[[nodiscard("unnecessary call")]] static const peg::ast& GetRequestObject() noexcept;
	[[nodiscard("unnecessary call")]] static const std::string& GetOperationName() noexcept;

	using Response = Query::Response;

	[[nodiscard("unnecessary conversion")]] static Response parseResponse(response::Value&& response);
};

} // namespace query::Query
} // namespace graphql::client

#endif // BENCHMARKCLIENT_H
