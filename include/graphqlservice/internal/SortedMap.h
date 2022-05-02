// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLSORTEDMAP_H
#define GRAPHQLSORTEDMAP_H

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace graphql::internal {

template <class K, class V, class Compare = std::less<K>>
class sorted_map
{
public:
	using vector_type = std::vector<std::pair<K, V>>;
	using const_iterator = typename vector_type::const_iterator;
	using const_reverse_iterator = typename vector_type::const_reverse_iterator;
	using mapped_type = V;

	constexpr sorted_map() = default;
	constexpr sorted_map(const sorted_map& other) = default;
	sorted_map(sorted_map&& other) noexcept = default;

	constexpr sorted_map(std::initializer_list<std::pair<K, V>> init)
		: _data { init }
	{
		std::sort(_data.begin(), _data.end(), [](const auto& lhs, const auto& rhs) noexcept {
			return Compare {}(lhs.first, rhs.first);
		});
	}

	constexpr ~sorted_map() = default;

	sorted_map& operator=(const sorted_map& rhs) = default;
	sorted_map& operator=(sorted_map&& rhs) noexcept = default;

	constexpr bool operator==(const sorted_map& rhs) const noexcept
	{
		return _data == rhs._data;
	}

	void reserve(size_t size)
	{
		_data.reserve(size);
	}

	constexpr size_t capacity() const noexcept
	{
		return _data.capacity();
	}

	void clear() noexcept
	{
		_data.clear();
	}

	constexpr bool empty() const noexcept
	{
		return _data.empty();
	}

	constexpr size_t size() const noexcept
	{
		return _data.size();
	}

	constexpr const_iterator begin() const noexcept
	{
		return _data.begin();
	}

	constexpr const_iterator end() const noexcept
	{
		return _data.end();
	}

	constexpr const_reverse_iterator rbegin() const noexcept
	{
		return _data.rbegin();
	}

	constexpr const_reverse_iterator rend() const noexcept
	{
		return _data.rend();
	}

	constexpr const_iterator find(const K& key) const noexcept
	{
		const auto [itr, itrEnd] = std::equal_range(begin(),
			end(),
			key,
			[](sorted_map_key lhs, sorted_map_key rhs) noexcept {
				return Compare {}(lhs.key, rhs.key);
			});

		return itr == itrEnd ? _data.end() : itr;
	}

	template <typename KeyArg>
	constexpr const_iterator find(KeyArg&& keyArg) const noexcept
	{
		const K key { std::forward<KeyArg>(keyArg) };

		return find(key);
	}

	template <typename KeyArg, typename... ValueArgs>
	std::pair<const_iterator, bool> emplace(KeyArg&& keyArg, ValueArgs&&... args) noexcept
	{
		K key { std::forward<KeyArg>(keyArg) };
		const auto [itr, itrEnd] = std::equal_range(_data.begin(),
			_data.end(),
			key,
			[](sorted_map_key lhs, sorted_map_key rhs) noexcept {
				return Compare {}(lhs.key, rhs.key);
			});

		if (itr != itrEnd)
		{
			return { itr, false };
		}

		return { _data.emplace(itrEnd, std::move(key), V { std::forward<ValueArgs>(args)... }),
			true };
	}

	const_iterator erase(const K& key) noexcept
	{
		const auto [itr, itrEnd] = std::equal_range(_data.begin(),
			_data.end(),
			key,
			[](sorted_map_key lhs, sorted_map_key rhs) noexcept {
				return Compare {}(lhs.key, rhs.key);
			});

		if (itr == itrEnd)
		{
			return _data.end();
		}

		return _data.erase(itr);
	}

	template <typename KeyArg>
	const_iterator erase(KeyArg&& keyArg) noexcept
	{
		const K key { std::forward<KeyArg>(keyArg) };

		return erase(key);
	}

	const_iterator erase(const_iterator itr) noexcept
	{
		return _data.erase(itr);
	}

	template <typename KeyArg>
	V& operator[](KeyArg&& keyArg) noexcept
	{
		K key { std::forward<KeyArg>(keyArg) };
		const auto [itr, itrEnd] = std::equal_range(_data.begin(),
			_data.end(),
			key,
			[](sorted_map_key lhs, sorted_map_key rhs) noexcept {
				return Compare {}(lhs.key, rhs.key);
			});

		if (itr != itrEnd)
		{
			return itr->second;
		}

		return _data.emplace(itrEnd, std::move(key), V {})->second;
	}

	template <typename KeyArg>
	V& at(KeyArg&& keyArg)
	{
		const K key { std::forward<KeyArg>(keyArg) };
		const auto [itr, itrEnd] = std::equal_range(_data.begin(),
			_data.end(),
			key,
			[](sorted_map_key lhs, sorted_map_key rhs) noexcept {
				return Compare {}(lhs.key, rhs.key);
			});

		if (itr == itrEnd)
		{
			throw std::out_of_range("key not found");
		}

		return itr->second;
	}

private:
	struct sorted_map_key
	{
		constexpr sorted_map_key(const std::pair<K, V>& entry)
			: key { entry.first }
		{
		}

		constexpr sorted_map_key(const K& key)
			: key { key }
		{
		}

		constexpr ~sorted_map_key() = default;

		const K& key;
	};

	vector_type _data;
};

template <class K, class Compare = std::less<K>>
class sorted_set
{
public:
	using vector_type = std::vector<K>;
	using const_iterator = typename vector_type::const_iterator;
	using const_reverse_iterator = typename vector_type::const_reverse_iterator;

	constexpr sorted_set() = default;
	constexpr sorted_set(const sorted_set& other) = default;
	sorted_set(sorted_set&& other) noexcept = default;

	constexpr sorted_set(std::initializer_list<K> init)
		: _data { init }
	{
		std::sort(_data.begin(), _data.end(), Compare {});
	}

	constexpr ~sorted_set() = default;

	sorted_set& operator=(const sorted_set& rhs) = default;
	sorted_set& operator=(sorted_set&& rhs) noexcept = default;

	constexpr bool operator==(const sorted_set& rhs) const noexcept
	{
		return _data == rhs._data;
	}

	void reserve(size_t size)
	{
		_data.reserve(size);
	}

	constexpr size_t capacity() const noexcept
	{
		return _data.capacity();
	}

	void clear() noexcept
	{
		_data.clear();
	}

	constexpr bool empty() const noexcept
	{
		return _data.empty();
	}

	constexpr size_t size() const noexcept
	{
		return _data.size();
	}

	constexpr const_iterator begin() const noexcept
	{
		return _data.begin();
	}

	constexpr const_iterator end() const noexcept
	{
		return _data.end();
	}

	constexpr const_reverse_iterator rbegin() const noexcept
	{
		return _data.rbegin();
	}

	constexpr const_reverse_iterator rend() const noexcept
	{
		return _data.rend();
	}

	constexpr const_iterator find(const K& key) const noexcept
	{
		const auto [itr, itrEnd] =
			std::equal_range(begin(), end(), key, [](const K& lhs, const K& rhs) noexcept {
				return Compare {}(lhs, rhs);
			});

		return itr == itrEnd ? _data.end() : itr;
	}

	template <typename Arg>
	constexpr const_iterator find(Arg&& arg) const noexcept
	{
		const K key { std::forward<Arg>(arg) };

		return find(key);
	}

	template <typename Arg>
	std::pair<const_iterator, bool> emplace(Arg&& key) noexcept
	{
		const auto [itr, itrEnd] = std::equal_range(_data.begin(),
			_data.end(),
			key,
			[](const auto& lhs, const auto& rhs) noexcept {
				return Compare {}(lhs, rhs);
			});

		if (itr != itrEnd)
		{
			return { itr, false };
		}

		return { _data.emplace(itrEnd, std::forward<Arg>(key)), true };
	}

	const_iterator erase(const K& key) noexcept
	{
		const auto [itr, itrEnd] = std::equal_range(_data.begin(),
			_data.end(),
			key,
			[](const K& lhs, const K& rhs) noexcept {
				return Compare {}(lhs, rhs);
			});

		if (itr == itrEnd)
		{
			return _data.end();
		}

		return _data.erase(itr);
	}

	template <typename Arg>
	const_iterator erase(Arg&& arg) noexcept
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

struct shorter_or_less
{
	constexpr bool operator()(const std::string_view& lhs, const std::string_view& rhs) const
	{
		return lhs.size() == rhs.size() ? lhs < rhs : lhs.size() < rhs.size();
	}
};

template <class V>
using string_view_map = sorted_map<std::string_view, V, shorter_or_less>;
using string_view_set = sorted_set<std::string_view, shorter_or_less>;

} // namespace graphql::internal

#endif // GRAPHQLSORTEDMAP_H
