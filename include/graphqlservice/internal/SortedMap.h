// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef SORTEDMAP_H
#define SORTEDMAP_H

#include <algorithm>
#include <initializer_list>
#include <tuple>
#include <vector>

namespace graphql::internal {

template <class K, class V>
class sorted_map
{
public:
	using vector_type = std::vector<std::pair<K, V>>;
	using iterator = typename vector_type::iterator;
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

	iterator find(const K& key) noexcept
	{
		const auto [itr, itrEnd] = std::equal_range(_data.begin(),
			_data.end(),
			key,
			[](sorted_map_key lhs, sorted_map_key rhs) noexcept {
				return lhs.key < rhs.key;
			});

		return itr == itrEnd ? _data.end() : itr;
	}

	template <typename KeyArg, typename... ValueArgs>
	std::pair<iterator, bool> emplace(KeyArg&& key, ValueArgs&&... args) noexcept
	{
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
					 std::forward<KeyArg>(key),
					 V { std::forward<ValueArgs>(args)... }),
			true };
	}

	iterator erase(const K& key) noexcept
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

	iterator erase(typename vector_type::const_iterator itr) noexcept
	{
		return _data.erase(itr);
	}

	V& operator[](const K& key) noexcept
	{
		return emplace(key).first->second;
	}

	V& operator[](K&& key) noexcept
	{
		return emplace(std::move(key)).first->second;
	}

	V& at(const K& key)
	{
		return find(key)->second;
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

	const_iterator erase(typename vector_type::const_iterator itr) noexcept
	{
		return _data.erase(itr);
	}

private:
	vector_type _data;
};

} // namespace graphql::internal

#endif // SORTEDMAP_H
