// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "SortedMap.h"

export module GraphQL.Internal.SortedMap;

export namespace graphql::internal {

// clang-format off
using internal::sorted_map_key;
using internal::sorted_map_equal_range;
using internal::sorted_map_lookup;
using internal::sorted_map;
using internal::sorted_set;
using internal::shorter_or_less;
using internal::string_view_map;
using internal::string_view_set;
// clang-format on

} // namespace graphql::internal
