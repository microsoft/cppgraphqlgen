// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef QUERYCLIENT_H
#define QUERYCLIENT_H

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLResponse.h"

// Check if the library version is compatible with clientgen 3.6.0
static_assert(graphql::internal::MajorVersion == 3, "regenerate with clientgen: major version mismatch");
static_assert(graphql::internal::MinorVersion == 6, "regenerate with clientgen: minor version mismatch");

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace graphql::query::Query {

/** Operation: query (default)
 **
 ** # Copyright (c) Microsoft Corporation. All rights reserved.
 ** # Licensed under the MIT License.
 ** 
 ** query {
 **   appointments {
 **     edges {
 **       node {
 **         id
 **         subject
 **         when
 **         isNow
 **         __typename
 **       }
 **     }
 **   }
 **   tasks {
 **     edges {
 **       node {
 **         id
 **         title
 **         isComplete
 **         __typename
 **       }
 **     }
 **   }
 **   unreadCounts {
 **     edges {
 **       node {
 **         id
 **         name
 **         unreadCount
 **         __typename
 **       }
 **     }
 **   }
 ** }
 **
 **/

// Return the original text of the request document.
const std::string& GetRequestText() noexcept;

// Return a pre-parsed, pre-validated request object.
const peg::ast& GetRequestObject() noexcept;

} /* namespace graphql::query::Query */

#endif // QUERYCLIENT_H
