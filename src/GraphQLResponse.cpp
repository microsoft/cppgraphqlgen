// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLResponse.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <optional>
#include <stdexcept>
#include <variant>

namespace graphql::response {

bool Value::MapData::operator==(const MapData& rhs) const
{
	return map == rhs.map;
}

bool Value::StringData::operator==(const StringData& rhs) const
{
	return from_json == rhs.from_json && string == rhs.string;
}

bool Value::NullData::operator==(const NullData&) const
{
	return true;
}

bool Value::ScalarData::operator==(const ScalarData& rhs) const
{
	if (scalar && rhs.scalar)
	{
		return *scalar == *rhs.scalar;
	}

	return !scalar && !rhs.scalar;
}

template <>
void Value::set<StringType>(StringType&& value)
{
	if (std::holds_alternative<EnumData>(_data))
	{
		std::get<EnumData>(_data) = std::move(value);
	}
	else if (std::holds_alternative<StringData>(_data))
	{
		std::get<StringData>(_data).string = std::move(value);
	}
	else
	{
		throw std::logic_error("Invalid call to Value::set for StringType");
	}
}

template <>
void Value::set<BooleanType>(BooleanType value)
{
	if (!std::holds_alternative<BooleanType>(_data))
	{
		throw std::logic_error("Invalid call to Value::set for BooleanType");
	}

	_data = { value };
}

template <>
void Value::set<IntType>(IntType value)
{
	if (std::holds_alternative<FloatType>(_data))
	{
		// Coerce IntType to FloatType
		_data = { static_cast<FloatType>(value) };
		return;
	}

	if (!std::holds_alternative<IntType>(_data))
	{
		throw std::logic_error("Invalid call to Value::set for IntType");
	}

	_data = { value };
}

template <>
void Value::set<FloatType>(FloatType value)
{
	if (!std::holds_alternative<FloatType>(_data))
	{
		throw std::logic_error("Invalid call to Value::set for FloatType");
	}

	_data = { value };
}

template <>
void Value::set<ScalarType>(ScalarType&& value)
{
	if (!std::holds_alternative<ScalarData>(_data))
	{
		throw std::logic_error("Invalid call to Value::set for ScalarType");
	}

	_data = { ScalarData { std::make_unique<ScalarType>(std::move(value)) } };
}

template <>
const MapType& Value::get<MapType>() const
{
	if (!std::holds_alternative<MapData>(_data))
	{
		throw std::logic_error("Invalid call to Value::get for MapType");
	}

	return std::get<MapData>(_data).map;
}

template <>
const ListType& Value::get<ListType>() const
{
	if (!std::holds_alternative<ListType>(_data))
	{
		throw std::logic_error("Invalid call to Value::get for ListType");
	}

	return std::get<ListType>(_data);
}

template <>
const StringType& Value::get<StringType>() const
{
	if (std::holds_alternative<EnumData>(_data))
	{
		return std::get<EnumData>(_data);
	}
	else if (std::holds_alternative<StringData>(_data))
	{
		return std::get<StringData>(_data).string;
	}

	throw std::logic_error("Invalid call to Value::get for StringType");
}

template <>
BooleanType Value::get<BooleanType>() const
{
	if (!std::holds_alternative<BooleanType>(_data))
	{
		throw std::logic_error("Invalid call to Value::get for BooleanType");
	}

	return std::get<BooleanType>(_data);
}

template <>
IntType Value::get<IntType>() const
{
	if (!std::holds_alternative<IntType>(_data))
	{
		throw std::logic_error("Invalid call to Value::get for IntType");
	}

	return std::get<IntType>(_data);
}

template <>
FloatType Value::get<FloatType>() const
{
	if (std::holds_alternative<IntType>(_data))
	{
		// Coerce IntType to FloatType
		return static_cast<FloatType>(std::get<IntType>(_data));
	}

	if (!std::holds_alternative<FloatType>(_data))
	{
		throw std::logic_error("Invalid call to Value::get for FloatType");
	}

	return std::get<FloatType>(_data);
}

template <>
const ScalarType& Value::get<ScalarType>() const
{
	if (!std::holds_alternative<ScalarData>(_data))
	{
		throw std::logic_error("Invalid call to Value::get for ScalarType");
	}

	const auto& scalar = std::get<ScalarData>(_data).scalar;

	if (!scalar)
	{
		throw std::logic_error("Invalid call to Value::get for ScalarType");
	}

	return *scalar;
}

template <>
MapType Value::release<MapType>()
{
	if (!std::holds_alternative<MapData>(_data))
	{
		throw std::logic_error("Invalid call to Value::release for MapType");
	}

	auto& mapData = std::get<MapData>(_data);
	MapType result = std::move(mapData.map);

	mapData.members.clear();

	return result;
}

template <>
ListType Value::release<ListType>()
{
	if (!std::holds_alternative<ListType>(_data))
	{
		throw std::logic_error("Invalid call to Value::release for ListType");
	}

	ListType result = std::move(std::get<ListType>(_data));

	return result;
}

template <>
StringType Value::release<StringType>()
{
	StringType result;

	if (std::holds_alternative<EnumData>(_data))
	{
		result = std::move(std::get<EnumData>(_data));
	}
	else if (std::holds_alternative<StringData>(_data))
	{
		auto& stringData = std::get<StringData>(_data);

		result = std::move(stringData.string);
		stringData.from_json = false;
	}
	else
	{
		throw std::logic_error("Invalid call to Value::release for StringType");
	}

	return result;
}

template <>
ScalarType Value::release<ScalarType>()
{
	if (!std::holds_alternative<ScalarData>(_data))
	{
		throw std::logic_error("Invalid call to Value::release for ScalarType");
	}

	auto scalar = std::move(std::get<ScalarData>(_data).scalar);

	if (!scalar)
	{
		throw std::logic_error("Invalid call to Value::release for ScalarType");
	}

	ScalarType result = std::move(*scalar);

	return result;
}

Value::Value(Type type /*= Type::Null*/)
{
	switch (type)
	{
		case Type::Map:
			_data = { MapData {} };
			break;

		case Type::List:
			_data = { ListType {} };
			break;

		case Type::String:
			_data = { StringData {} };
			break;

		case Type::Null:
			_data = { NullData {} };
			break;

		case Type::Boolean:
			_data = { BooleanType { false } };
			break;

		case Type::Int:
			_data = { IntType { 0 } };
			break;

		case Type::Float:
			_data = { FloatType { 0.0 } };
			break;

		case Type::EnumValue:
			_data = { EnumData {} };
			break;

		case Type::Scalar:
			_data = { ScalarData {} };
			break;
	}
}

Value::~Value()
{
	// The default destructor gets inlined and may use a different allocator to free Value's member
	// variables than the graphqlresponse module used to allocate them. So even though this could be
	// omitted, declare it explicitly and define it in graphqlresponse.
}

Value::Value(const char* value)
	: _data(TypeData { StringData { StringType { value }, false } })
{
}

Value::Value(StringType&& value)
	: _data(TypeData { StringData { std::move(value), false } })
{
}

Value::Value(BooleanType value)
	: _data(TypeData { value })
{
}

Value::Value(IntType value)
	: _data(TypeData { value })
{
}

Value::Value(FloatType value)
	: _data(TypeData { value })
{
}

Value::Value(Value&& other) noexcept
	: _data(std::move(other._data))
{
}

Value::Value(const Value& other)
{
	switch (other.type())
	{
		case Type::Map:
		{
			MapData copy {};

			copy.map.reserve(other.size());
			for (const auto& entry : other)
			{
				copy.map.push_back({ entry.first, Value { entry.second } });
			}

			std::map<std::string_view, size_t> members;

			for (const auto& entry : copy.map)
			{
				members[entry.first] = members.size();
			}

			copy.members.reserve(members.size());
			std::transform(members.cbegin(),
				members.cend(),
				std::back_inserter(copy.members),
				[&copy](const auto& entry) noexcept {
					return entry.second;
				});

			_data = { std::move(copy) };
			break;
		}

		case Type::List:
		{
			ListType copy {};

			copy.reserve(other.size());
			for (size_t i = 0; i < other.size(); ++i)
			{
				copy.push_back(Value { other[i] });
			}

			_data = { std::move(copy) };
			break;
		}

		case Type::String:
			_data = { StringData { other.get<StringType>(), other.maybe_enum() } };
			break;

		case Type::Null:
			_data = { NullData {} };
			break;

		case Type::Boolean:
			_data = { other.get<BooleanType>() };
			break;

		case Type::Int:
			_data = { other.get<IntType>() };
			break;

		case Type::Float:
			_data = { other.get<FloatType>() };
			break;

		case Type::EnumValue:
			_data = { EnumData { other.get<StringType>() } };
			break;

		case Type::Scalar:
			_data = { ScalarData { std::make_unique<ScalarType>(other.get<ScalarType>()) } };
			break;
	}
}

Value& Value::operator=(Value&& rhs) noexcept
{
	if (&rhs != this)
	{
		_data = std::move(rhs._data);
	}

	return *this;
}

bool Value::operator==(const Value& rhs) const noexcept
{
	return _data == rhs._data;
}

bool Value::operator!=(const Value& rhs) const noexcept
{
	return !(*this == rhs);
}

Type Value::type() const noexcept
{
	// As long as the order of the variant alternatives matches the Type enum, we can cast the index
	// to the Type in one step.
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::Map), TypeData>,
			MapData>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::List), TypeData>,
			ListType>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::String), TypeData>,
			StringData>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::Boolean), TypeData>,
			BooleanType>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::Int), TypeData>,
			IntType>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::Float), TypeData>,
			FloatType>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::EnumValue), TypeData>,
			EnumData>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::Scalar), TypeData>,
			ScalarData>,
		"type mistmatch");

	return static_cast<Type>(_data.index());
}

Value&& Value::from_json() noexcept
{
	if (std::holds_alternative<StringData>(_data))
	{
		std::get<StringData>(_data).from_json = true;
	}

	return std::move(*this);
}

bool Value::maybe_enum() const noexcept
{
	return std::holds_alternative<EnumData>(_data)
		|| (std::holds_alternative<StringData>(_data) && std::get<StringData>(_data).from_json);
}

void Value::reserve(size_t count)
{
	switch (type())
	{
		case Type::Map:
		{
			auto& mapData = std::get<MapData>(_data);

			mapData.members.reserve(count);
			mapData.map.reserve(count);
			break;
		}

		case Type::List:
		{
			std::get<ListType>(_data).reserve(count);
			break;
		}

		default:
			throw std::logic_error("Invalid call to Value::reserve");
	}
}

size_t Value::size() const
{
	switch (type())
	{
		case Type::Map:
		{
			return std::get<MapData>(_data).map.size();
		}

		case Type::List:
		{
			return std::get<ListType>(_data).size();
		}

		default:
			throw std::logic_error("Invalid call to Value::size");
	}
}

bool Value::emplace_back(std::string&& name, Value&& value)
{
	if (!std::holds_alternative<MapData>(_data))
	{
		throw std::logic_error("Invalid call to Value::emplace_back for MapType");
	}

	auto& mapData = std::get<MapData>(_data);
	const auto [itr, itrEnd] = std::equal_range(mapData.members.cbegin(),
		mapData.members.cend(),
		std::nullopt,
		[&mapData, &name](std::optional<size_t> lhs, std::optional<size_t> rhs) noexcept {
			std::string_view lhsName { lhs == std::nullopt ? name : mapData.map[*lhs].first };
			std::string_view rhsName { rhs == std::nullopt ? name : mapData.map[*rhs].first };
			return lhsName < rhsName;
		});

	if (itr != itrEnd)
	{
		return false;
	}

	mapData.map.emplace_back(std::make_pair(std::move(name), std::move(value)));
	mapData.members.insert(itr, mapData.members.size());

	return true;
}

MapType::const_iterator Value::find(std::string_view name) const
{
	if (!std::holds_alternative<MapData>(_data))
	{
		throw std::logic_error("Invalid call to Value::find for MapType");
	}

	const auto& mapData = std::get<MapData>(_data);
	const auto [itr, itrEnd] = std::equal_range(mapData.members.cbegin(),
		mapData.members.cend(),
		std::nullopt,
		[&mapData, name](std::optional<size_t> lhs, std::optional<size_t> rhs) noexcept {
			std::string_view lhsName { lhs == std::nullopt ? name : mapData.map[*lhs].first };
			std::string_view rhsName { rhs == std::nullopt ? name : mapData.map[*rhs].first };
			return lhsName < rhsName;
		});

	if (itr == itrEnd)
	{
		return mapData.map.cend();
	}

	return mapData.map.cbegin() + *itr;
}

MapType::const_iterator Value::begin() const
{
	if (!std::holds_alternative<MapData>(_data))
	{
		throw std::logic_error("Invalid call to Value::begin for MapType");
	}

	return std::get<MapData>(_data).map.cbegin();
}

MapType::const_iterator Value::end() const
{
	if (!std::holds_alternative<MapData>(_data))
	{
		throw std::logic_error("Invalid call to Value::end for MapType");
	}

	return std::get<MapData>(_data).map.cend();
}

const Value& Value::operator[](std::string_view name) const
{
	const auto itr = find(name);

	if (itr == end())
	{
		throw std::runtime_error("Missing Map member");
	}

	return itr->second;
}

void Value::emplace_back(Value&& value)
{
	if (!std::holds_alternative<ListType>(_data))
	{
		throw std::logic_error("Invalid call to Value::emplace_back for ListType");
	}

	std::get<ListType>(_data).emplace_back(std::move(value));
}

const Value& Value::operator[](size_t index) const
{
	if (!std::holds_alternative<ListType>(_data))
	{
		throw std::logic_error("Invalid call to Value::operator[] for ListType");
	}

	return std::get<ListType>(_data).at(index);
}

} /* namespace graphql::response */
