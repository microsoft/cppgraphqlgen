// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLSORTEDMAP_H
#define GRAPHQLSORTEDMAP_H

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace graphql::internal {

template <class K, class V>
struct [[nodiscard("unnecessary construction")]] sorted_map_key
{
	constexpr sorted_map_key(const std::pair<K, V>& entry) noexcept
		: key { entry.first }
	{
	}

	constexpr sorted_map_key(K key) noexcept
		: key { key }
	{
	}

	constexpr ~sorted_map_key() noexcept = default;

	K key;
};

template <class Compare, class Iterator, class K,
	class V = decltype(std::declval<Iterator>()->second)>
[[nodiscard("unnecessary call")]] constexpr std::pair<Iterator, Iterator> sorted_map_equal_range(
	Iterator itrBegin, Iterator itrEnd, K key) noexcept
{
	return std::equal_range(itrBegin,
		itrEnd,
		key,
		[](sorted_map_key<K, V> lhs, sorted_map_key<K, V> rhs) noexcept {
			return Compare {}(lhs.key, rhs.key);
		});
}

template <class Compare, class Container, class K>
[[nodiscard("unnecessary call")]] constexpr auto sorted_map_lookup(
	const Container& container, K key) noexcept
{
	const auto [itr, itrEnd] =
		sorted_map_equal_range<Compare>(container.begin(), container.end(), key);

	return itr == itrEnd ? std::nullopt : std::make_optional(itr->second);
}

template <class K, class V, class Compare = std::less<K>>
class [[nodiscard("unnecessary construction")]] sorted_map
{
public:
	using vector_type = std::vector<std::pair<K, V>>;
	using const_iterator = typename vector_type::const_iterator;
	using const_reverse_iterator = typename vector_type::const_reverse_iterator;
	using mapped_type = V;

	constexpr sorted_map() noexcept = default;
	constexpr sorted_map(const sorted_map& other) = default;
	sorted_map(sorted_map && other) noexcept = default;

	constexpr sorted_map(std::initializer_list<std::pair<K, V>> init)
		: _data { init }
	{
		std::sort(_data.begin(), _data.end(), [](const auto& lhs, const auto& rhs) noexcept {
			return Compare {}(lhs.first, rhs.first);
		});
	}

	constexpr ~sorted_map() noexcept = default;

	sorted_map& operator=(const sorted_map& rhs) = default;
	sorted_map& operator=(sorted_map&& rhs) noexcept = default;

	[[nodiscard("unnecessary call")]] constexpr bool operator==(const sorted_map& rhs)
		const noexcept
	{
		return _data == rhs._data;
	}

	void reserve(size_t size)
	{
		_data.reserve(size);
	}

	[[nodiscard("unnecessary call")]] constexpr size_t capacity() const noexcept
	{
		return _data.capacity();
	}

	void clear() noexcept
	{
		_data.clear();
	}

	[[nodiscard("unnecessary call")]] constexpr bool empty() const noexcept
	{
		return _data.empty();
	}

	[[nodiscard("unnecessary call")]] constexpr size_t size() const noexcept
	{
		return _data.size();
	}

	[[nodiscard("unnecessary call")]] constexpr const_iterator begin() const noexcept
	{
		return _data.begin();
	}

	[[nodiscard("unnecessary call")]] constexpr const_iterator end() const noexcept
	{
		return _data.end();
	}

	[[nodiscard("unnecessary call")]] constexpr const_reverse_iterator rbegin() const noexcept
	{
		return _data.rbegin();
	}

	[[nodiscard("unnecessary call")]] constexpr const_reverse_iterator rend() const noexcept
	{
		return _data.rend();
	}

	[[nodiscard("unnecessary call")]] constexpr const_iterator find(K key) const noexcept
	{
		const auto [itr, itrEnd] = sorted_map_equal_range<Compare>(_data.begin(), _data.end(), key);

		return itr == itrEnd ? _data.cend() : itr;
	}

	template <typename KeyArg>
	[[nodiscard("unnecessary call")]] constexpr const_iterator find(KeyArg && keyArg) const noexcept
	{
		const K key { std::forward<KeyArg>(keyArg) };

		return find(key);
	}

	template <typename KeyArg, typename... ValueArgs>
	std::pair<const_iterator, bool> emplace(KeyArg && keyArg, ValueArgs && ... args) noexcept
	{
		K key { std::forward<KeyArg>(keyArg) };
		const auto [itr, itrEnd] = sorted_map_equal_range<Compare>(_data.begin(), _data.end(), key);

		if (itr != itrEnd)
		{
			return { itr, false };
		}

		return { _data.emplace(itrEnd, std::move(key), V { std::forward<ValueArgs>(args)... }),
			true };
	}

	const_iterator erase(K key) noexcept
	{
		const auto [itr, itrEnd] = sorted_map_equal_range<Compare>(_data.begin(), _data.end(), key);

		if (itr == itrEnd)
		{
			return _data.end();
		}

		return _data.erase(itr);
	}

	template <typename KeyArg>
	const_iterator erase(KeyArg && keyArg) noexcept
	{
		const K key { std::forward<KeyArg>(keyArg) };

		return erase(key);
	}

	const_iterator erase(const_iterator itr) noexcept
	{
		return _data.erase(itr);
	}

	template <typename KeyArg>
	[[nodiscard("unnecessary call")]] V& operator[](KeyArg&& keyArg) noexcept
	{
		K key { std::forward<KeyArg>(keyArg) };
		const auto [itr, itrEnd] = sorted_map_equal_range<Compare>(_data.begin(), _data.end(), key);

		if (itr != itrEnd)
		{
			return itr->second;
		}

		return _data.emplace(itrEnd, std::move(key), V {})->second;
	}

	template <typename KeyArg>
	[[nodiscard("unnecessary call")]] V& at(KeyArg && keyArg)
	{
		const K key { std::forward<KeyArg>(keyArg) };
		const auto [itr, itrEnd] = sorted_map_equal_range<Compare>(_data.begin(), _data.end(), key);

		if (itr == itrEnd)
		{
			throw std::out_of_range("key not found");
		}

		return itr->second;
	}

private:
	vector_type _data;
};

template <class K, class Compare = std::less<K>>
class [[nodiscard("unnecessary construction")]] sorted_set
{
public:
	using vector_type = std::vector<K>;
	using const_iterator = typename vector_type::const_iterator;
	using const_reverse_iterator = typename vector_type::const_reverse_iterator;

	constexpr sorted_set() noexcept = default;
	constexpr sorted_set(const sorted_set& other) = default;
	sorted_set(sorted_set && other) noexcept = default;

	constexpr sorted_set(std::initializer_list<K> init)
		: _data { init }
	{
		std::sort(_data.begin(), _data.end(), Compare {});
	}

	constexpr ~sorted_set() noexcept = default;

	sorted_set& operator=(const sorted_set& rhs) = default;
	sorted_set& operator=(sorted_set&& rhs) noexcept = default;

	[[nodiscard("unnecessary call")]] constexpr bool operator==(const sorted_set& rhs)
		const noexcept
	{
		return _data == rhs._data;
	}

	void reserve(size_t size)
	{
		_data.reserve(size);
	}

	[[nodiscard("unnecessary call")]] constexpr size_t capacity() const noexcept
	{
		return _data.capacity();
	}

	void clear() noexcept
	{
		_data.clear();
	}

	[[nodiscard("unnecessary call")]] constexpr bool empty() const noexcept
	{
		return _data.empty();
	}

	[[nodiscard("unnecessary call")]] constexpr size_t size() const noexcept
	{
		return _data.size();
	}

	[[nodiscard("unnecessary call")]] constexpr const_iterator begin() const noexcept
	{
		return _data.begin();
	}

	[[nodiscard("unnecessary call")]] constexpr const_iterator end() const noexcept
	{
		return _data.end();
	}

	[[nodiscard("unnecessary call")]] constexpr const_reverse_iterator rbegin() const noexcept
	{
		return _data.rbegin();
	}

	[[nodiscard("unnecessary call")]] constexpr const_reverse_iterator rend() const noexcept
	{
		return _data.rend();
	}

	[[nodiscard("unnecessary call")]] constexpr const_iterator find(K key) const noexcept
	{
		const auto [itr, itrEnd] = std::equal_range(begin(), end(), key, [](K lhs, K rhs) noexcept {
			return Compare {}(lhs, rhs);
		});

		return itr == itrEnd ? _data.end() : itr;
	}

	template <typename Arg>
	[[nodiscard("unnecessary call")]] constexpr const_iterator find(Arg && arg) const noexcept
	{
		const K key { std::forward<Arg>(arg) };

		return find(key);
	}

	template <typename Arg>
	std::pair<const_iterator, bool> emplace(Arg && key) noexcept
	{
		const auto [itr, itrEnd] =
			std::equal_range(_data.begin(), _data.end(), key, [](K lhs, K rhs) noexcept {
				return Compare {}(lhs, rhs);
			});

		if (itr != itrEnd)
		{
			return { itr, false };
		}

		return { _data.emplace(itrEnd, std::forward<Arg>(key)), true };
	}

	const_iterator erase(K key) noexcept
	{
		const auto [itr, itrEnd] =
			std::equal_range(_data.begin(), _data.end(), key, [](K lhs, K rhs) noexcept {
				return Compare {}(lhs, rhs);
			});

		if (itr == itrEnd)
		{
			return _data.end();
		}

		return _data.erase(itr);
	}

	template <typename Arg>
	const_iterator erase(Arg && arg) noexcept
	{
		const K key { std::forward<Arg>(arg) };

		return erase(key);
	}

	const_iterator erase(const_iterator itr) noexcept
	{
		return _data.erase(itr);
	}

private:
	vector_type _data;
};

struct [[nodiscard("unnecessary construction")]] shorter_or_less {
	[[nodiscard("unnecessary call")]] constexpr bool operator()(
		std::string_view lhs, std::string_view rhs)
		const noexcept { return lhs.size() == rhs.size() ? lhs < rhs : lhs.size() < rhs.size();
} // namespace graphql::internal
}
;

template <class V>
using string_view_map = sorted_map<std::string_view, V, shorter_or_less>;
using string_view_set = sorted_set<std::string_view, shorter_or_less>;

} // namespace graphql::internal

#endif // GRAPHQLSORTEDMAP_H
