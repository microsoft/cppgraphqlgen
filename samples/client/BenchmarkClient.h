// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef BENCHMARKCLIENT_H
#define BENCHMARKCLIENT_H

#include "graphqlservice/GraphQLClient.h"
#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLResponse.h"

#include "graphqlservice/internal/Version.h"

// Check if the library version is compatible with clientgen 4.0.0
static_assert(graphql::internal::MajorVersion == 4, "regenerate with clientgen: major version mismatch");
static_assert(graphql::internal::MinorVersion == 0, "regenerate with clientgen: minor version mismatch");

#include <optional>
#include <string>
#include <vector>

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
namespace graphql::client::query::Query {

// Return the original text of the request document.
const std::string& GetRequestText() noexcept;

// Return a pre-parsed, pre-validated request object.
const peg::ast& GetRequestObject() noexcept;

struct Response
{
	struct appointments_AppointmentConnection
	{
		struct pageInfo_PageInfo
		{
			response::BooleanType hasNextPage {};
		};

		struct edges_AppointmentEdge
		{
			struct node_Appointment
			{
				response::IdType id {};
				std::optional<response::Value> when {};
				std::optional<response::StringType> subject {};
				response::BooleanType isNow {};
			};

			std::optional<node_Appointment> node {};
		};

		pageInfo_PageInfo pageInfo {};
		std::optional<std::vector<std::optional<edges_AppointmentEdge>>> edges {};
	};

	appointments_AppointmentConnection appointments {};
};

Response parseResponse(response::Value response);

} // namespace graphql::client::query::Query

#endif // BENCHMARKCLIENT_H
