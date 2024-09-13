// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "SortedMap.h"

export module GraphQL.Internal.SortedMap;

namespace included = graphql::internal;

export namespace graphql::internal {

namespace exported {

// clang-format off
using included::sorted_map_key;
using included::sorted_map_equal_range;
using included::sorted_map_lookup;
using included::sorted_map;
using included::sorted_set;
using included::shorter_or_less;
using included::string_view_map;
using included::string_view_set;
// clang-format on

} // namespace exported

using namespace exported;

} // namespace graphql::internal
