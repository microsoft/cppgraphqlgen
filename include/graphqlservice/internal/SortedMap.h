// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef SORTEDMAP_H
#define SORTEDMAP_H

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace graphql::internal {

template <class K, class V>
class sorted_map
{
public:
	using vector_type = std::vector<std::pair<K, V>>;
	using const_iterator = typename vector_type::const_iterator;
	using const_reverse_iterator = typename vector_type::const_reverse_iterator;
	using mapped_type = V;

	sorted_map() = default;

	sorted_map(std::initializer_list<std::pair<K, V>> init)
		: _data { init }
	{
		std::sort(_data.begin(), _data.end(), [](const auto& lhs, const auto& rhs) noexcept {
			return lhs.first < rhs.first;
		});
	}

	bool operator==(const sorted_map& rhs) const noexcept
	{
		return _data == rhs._data;
	}

	void clear() noexcept
	{
		_data.clear();
	}

	bool empty() const noexcept
	{
		return _data.empty();
	}

	size_t size() const noexcept
	{
		return _data.size();
	}

	const_iterator begin() const noexcept
	{
		return _data.begin();
	}

	const_iterator end() const noexcept
	{
		return _data.end();
	}

	const_reverse_iterator rbegin() const noexcept
	{
		return _data.rbegin();
	}

	const_reverse_iterator rend() const noexcept
	{
		return _data.rend();
	}

	const_iterator find(const K& key) const noexcept
	{
		const auto [itr, itrEnd] = std::equal_range(begin(),
			end(),
			key,
			[](sorted_map_key lhs, sorted_map_key rhs) noexcept {
				return lhs.key < rhs.key;
			});

		return itr == itrEnd ? _data.end() : itr;
	}

	template <typename KeyArg>
	const_iterator find(KeyArg&& keyArg) const noexcept
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
				return lhs.key < rhs.key;
			});

		if (itr != itrEnd)
		{
			return { itr, false };
		}

		return { _data.emplace(itrEnd,
					 std::move(key),
					 V { std::forward<ValueArgs>(args)... }),
			true };
	}

	const_iterator erase(const K& key) noexcept
	{
		const auto [itr, itrEnd] = std::equal_range(_data.begin(),
			_data.end(),
			key,
			[](sorted_map_key lhs, sorted_map_key rhs) noexcept {
				return lhs.key < rhs.key;
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
				return lhs.key < rhs.key;
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
				return lhs.key < rhs.key;
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
		sorted_map_key(const std::pair<K, V>& entry)
			: key { entry.first }
		{
		}

		sorted_map_key(const K& key)
			: key { key }
		{
		}

		const K& key;
	};

	vector_type _data;
};

template <class K>
class sorted_set
{
public:
	using vector_type = std::vector<K>;
	using const_iterator = typename vector_type::const_iterator;
	using const_reverse_iterator = typename vector_type::const_reverse_iterator;

	sorted_set() = default;

	sorted_set(std::initializer_list<K> init)
		: _data { init }
	{
		std::sort(_data.begin(), _data.end(), [](const auto& lhs, const auto& rhs) noexcept {
			return lhs < rhs;
		});
	}

	bool operator==(const sorted_set& rhs) const noexcept
	{
		return _data == rhs._data;
	}

	void clear() noexcept
	{
		_data.clear();
	}

	bool empty() const noexcept
	{
		return _data.empty();
	}

	size_t size() const noexcept
	{
		return _data.size();
	}

	const_iterator begin() const noexcept
	{
		return _data.begin();
	}

	const_iterator end() const noexcept
	{
		return _data.end();
	}

	const_reverse_iterator rbegin() const noexcept
	{
		return _data.rbegin();
	}

	const_reverse_iterator rend() const noexcept
	{
		return _data.rend();
	}

	const_iterator find(const K& key) const noexcept
	{
		const auto [itr, itrEnd] =
			std::equal_range(begin(), end(), key, [](const auto& lhs, const auto& rhs) noexcept {
				return lhs < rhs;
			});

		return itr == itrEnd ? _data.end() : itr;
	}

	template <typename Arg>
	const_iterator find(Arg&& arg) const noexcept
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
				return lhs < rhs;
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
			[](const auto& lhs, const auto& rhs) noexcept {
				return lhs < rhs;
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

} // namespace graphql::internal

#endif // SORTEDMAP_H
