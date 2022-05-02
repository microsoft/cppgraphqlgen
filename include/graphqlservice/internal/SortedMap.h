// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLSORTEDMAP_H
#define GRAPHQLSORTEDMAP_H

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

namespace graphql::internal {

template <class K, class V, class Compare = std::less<K>>
class sorted_map
{
public:
	struct value_type
	{
		constexpr value_type(K first, V second) noexcept
			: first { std::move(first) }
			, second { std::move(second) }
		{
		}

		constexpr value_type(const value_type& other)
			: first { other.first }
			, second { other.second }
		{
		}

		value_type(value_type&& other) noexcept
			: first { std::move(other.first) }
			, second { std::move(other.second) }
		{
		}

		constexpr ~value_type() noexcept
		{
		}

		value_type& operator=(const value_type& rhs)
		{
			first = rhs.first;
			second = rhs.second;
			return *this;
		}

		value_type& operator=(value_type&& rhs) noexcept
		{
			first = std::move(rhs.first);
			second = std::move(rhs.second);
			return *this;
		}

		constexpr bool operator==(const value_type& rhs) const noexcept
		{
			return first == rhs.first && second == rhs.second;
		}

		K first;
		V second;
	};

	using vector_type = std::vector<value_type>;
	using const_iterator = typename vector_type::const_iterator;
	using const_reverse_iterator = typename vector_type::const_reverse_iterator;
	using mapped_type = V;

	constexpr sorted_map() noexcept
		: _data {}
	{
	}

	constexpr sorted_map(const sorted_map& other)
		: _data { other._data }
	{
	}

	sorted_map(sorted_map&& other) noexcept
		: _data { std::move(other._data) }
	{
	}

	constexpr sorted_map(std::initializer_list<std::pair<K, V>> init)
		: _data {}
	{
		_data.reserve(init.size());
		std::transform(init.begin(),
			init.end(),
			std::back_inserter(_data),
			[](auto& entry) noexcept {
				return value_type { std::move(entry.first), std::move(entry.second) };
			});
		std::sort(_data.begin(), _data.end(), [](const auto& lhs, const auto& rhs) noexcept {
			return Compare {}(lhs.first, rhs.first);
		});
	}

	constexpr ~sorted_map() noexcept
	{
	}

	sorted_map& operator=(const sorted_map& rhs)
	{
		_data = rhs._data;
		return *this;
	}

	sorted_map& operator=(sorted_map&& rhs) noexcept
	{
		_data = std::move(rhs._data);
		return *this;
	}

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
		constexpr sorted_map_key(const value_type& entry) noexcept
			: key { entry.first }
		{
		}

		constexpr sorted_map_key(const K& key) noexcept
			: key { key }
		{
		}

		constexpr ~sorted_map_key() noexcept
		{
		}

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

	constexpr sorted_set() noexcept
		: _data {}
	{
	}

	constexpr sorted_set(const sorted_set& other)
		: _data { other._data }
	{
	}

	sorted_set(sorted_set&& other) noexcept
		: _data { std::move(other._data) }
	{
	}

	constexpr sorted_set(std::initializer_list<K> init)
		: _data { init }
	{
		std::sort(_data.begin(), _data.end(), Compare {});
	}

	constexpr ~sorted_set() noexcept
	{
	}

	sorted_set& operator=(const sorted_set& rhs)
	{
		_data = rhs._data;
		return *this;
	}

	sorted_set& operator=(sorted_set&& rhs) noexcept
	{
		_data = std::move(rhs._data);
		return *this;
	}

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
	constexpr shorter_or_less() noexcept
	{
	}

	constexpr ~shorter_or_less() noexcept
	{
	}

	constexpr bool operator()(
		const std::string_view& lhs, const std::string_view& rhs) const noexcept
	{
		return lhs.size() == rhs.size() ? lhs < rhs : lhs.size() < rhs.size();
	}
};

template <class V>
using string_view_map = sorted_map<std::string_view, V, shorter_or_less>;
using string_view_set = sorted_set<std::string_view, shorter_or_less>;

} // namespace graphql::internal

#endif // GRAPHQLSORTEDMAP_H
