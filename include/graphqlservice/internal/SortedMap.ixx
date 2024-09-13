// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "SortedMap.h"

export module GraphQL.Internal.SortedMap;

namespace included = graphql::internal;

export namespace graphql::internal {

namespace exported {

template <class K, class V>
using sorted_map_key = included::sorted_map_key<K, V>;

template <class Compare, class Iterator, class K,
	class V = decltype(std::declval<Iterator>()->second)>
[[nodiscard("unnecessary call")]] constexpr std::pair<Iterator, Iterator> sorted_map_equal_range(
	Iterator itrBegin, Iterator itrEnd, K&& key) noexcept
{
	return included::sorted_map_equal_range(itrBegin, itrEnd, std::forward<K>(key));
}

template <class Compare, class Container, class K>
[[nodiscard("unnecessary call")]] constexpr auto sorted_map_lookup(
	const Container& container, K&& key) noexcept
{
	return included::sorted_map_lookup(container, std::forward<K>(key));
}

template <class K, class V, class Compare = std::less<K>>
using sorted_map = included::sorted_map<K, V, Compare>;

template <class K, class Compare = std::less<K>>
using sorted_set = included::sorted_set<K, Compare>;

using shorter_or_less = included::shorter_or_less;

template <class V>
using string_view_map = included::string_view_map<V>;

using string_view_set = included::string_view_set;

} // namespace exported

using namespace exported;

} // namespace graphql::internal
