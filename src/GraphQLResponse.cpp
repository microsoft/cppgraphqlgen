// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLResponse.h"

#include "graphqlservice/internal/Base64.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace graphql::response {

IdType::IdType(IdType&& other /* = IdType { ByteData {} } */) noexcept
	: _data { std::move(other._data) }
{
}

IdType::IdType(const IdType& other)
	: _data { other._data }
{
}

IdType::~IdType()
{
	// The default destructor gets inlined and may use a different allocator to free IdType's member
	// variables than the graphqlresponse module used to allocate them. So even though this could be
	// omitted, declare it explicitly and define it in graphqlresponse.
}

IdType::IdType(std::size_t count, typename ByteData::value_type value /* = 0 */)
	: _data { ByteData(count, value) }
{
}

IdType::IdType(std::initializer_list<typename ByteData::value_type> values)
	: _data { ByteData { values } }
{
}

template <>
IdType::IdType(typename ByteData::const_iterator begin, typename ByteData::const_iterator end)
	: _data { ByteData { begin, end } }
{
}

template <>
IdType::IdType(typename ByteData::const_pointer begin, typename ByteData::const_pointer end)
	: _data { ByteData { begin, end } }
{
}

template <>
IdType::IdType(typename ByteData::pointer begin, typename ByteData::pointer end)
	: _data { ByteData { begin, end } }
{
}

IdType& IdType::operator=(IdType&& rhs) noexcept
{
	if (&rhs != this)
	{
		_data = std::move(rhs._data);
	}

	return *this;
}

IdType::IdType(ByteData&& data) noexcept
	: _data { std::move(data) }
{
}

IdType::IdType(OpaqueString&& opaque) noexcept
	: _data { std::move(opaque) }
{
}

bool IdType::operator==(const IdType& rhs) const noexcept
{
	if (_data.index() == rhs._data.index())
	{
		if (std::holds_alternative<ByteData>(_data))
		{
			return std::get<ByteData>(_data) == std::get<ByteData>(rhs._data);
		}
		else
		{
			return std::get<OpaqueString>(_data) == std::get<OpaqueString>(rhs._data);
		}
	}

	return (std::holds_alternative<ByteData>(_data)
				   ? internal::Base64::compareBase64(std::get<ByteData>(_data),
						 std::get<OpaqueString>(rhs._data))
				   : internal::Base64::compareBase64(std::get<ByteData>(rhs._data),
						 std::get<OpaqueString>(_data)))
		== internal::Base64::Comparison::EqualTo;
}

bool IdType::operator==(const ByteData& rhs) const noexcept
{
	if (std::holds_alternative<ByteData>(_data))
	{
		return std::get<ByteData>(_data) == rhs;
	}

	return internal::Base64::compareBase64(rhs, std::get<OpaqueString>(_data))
		== internal::Base64::Comparison::EqualTo;
}

bool IdType::operator==(const OpaqueString& rhs) const noexcept
{
	if (std::holds_alternative<OpaqueString>(_data))
	{
		return std::get<OpaqueString>(_data) == rhs;
	}

	return internal::Base64::compareBase64(std::get<ByteData>(_data), rhs)
		== internal::Base64::Comparison::EqualTo;
}

bool IdType::operator<(const IdType& rhs) const noexcept
{
	if (_data.index() == rhs._data.index())
	{
		if (std::holds_alternative<ByteData>(_data))
		{
			return std::get<ByteData>(_data) < std::get<ByteData>(rhs._data);
		}
		else
		{
			return std::get<OpaqueString>(_data) < std::get<OpaqueString>(rhs._data);
		}
	}

	return (std::holds_alternative<ByteData>(_data)
			? (internal::Base64::compareBase64(std::get<ByteData>(_data),
				   std::get<OpaqueString>(rhs._data))
				  < internal::Base64::Comparison::EqualTo)
			: (internal::Base64::compareBase64(std::get<ByteData>(rhs._data),
				  std::get<OpaqueString>(_data)))
				> internal::Base64::Comparison::EqualTo);
}

bool IdType::isBase64() const noexcept
{
	return !std::holds_alternative<OpaqueString>(_data)
		|| internal::Base64::validateBase64(std::get<OpaqueString>(_data));
}

bool IdType::empty() const noexcept
{
	return std::visit(
		[](const auto& data) noexcept {
			return data.empty();
		},
		_data);
}

std::size_t IdType::size() const noexcept
{
	return std::visit(
		[](const auto& data) noexcept {
			return data.size();
		},
		_data);
}

std::size_t IdType::max_size() const noexcept
{
	return std::visit(
		[](const auto& data) noexcept {
			return data.max_size();
		},
		_data);
}

void IdType::reserve(std::size_t new_cap)
{
	std::visit(
		[new_cap](auto& data) {
			data.reserve(new_cap);
		},
		_data);
}

std::size_t IdType::capacity() const noexcept
{
	return std::visit(
		[](const auto& data) noexcept {
			return data.capacity();
		},
		_data);
}

void IdType::shrink_to_fit()
{
	std::visit(
		[](auto& data) {
			data.shrink_to_fit();
		},
		_data);
}

void IdType::clear() noexcept
{
	std::visit(
		[](auto& data) noexcept {
			data.clear();
		},
		_data);
}

const std::uint8_t& IdType::at(std::size_t pos) const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::at for ByteData");
	}

	return std::get<ByteData>(_data).at(pos);
}

std::uint8_t& IdType::at(std::size_t pos)
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::at for ByteData");
	}

	return std::get<ByteData>(_data).at(pos);
}

const std::uint8_t& IdType::operator[](std::size_t pos) const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::operator[] for ByteData");
	}

	return std::get<ByteData>(_data)[pos];
}

std::uint8_t& IdType::operator[](std::size_t pos)
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::operator[] for ByteData");
	}

	return std::get<ByteData>(_data)[pos];
}

const std::uint8_t& IdType::front() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::front for ByteData");
	}

	return std::get<ByteData>(_data).front();
}

std::uint8_t& IdType::front()
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::front for ByteData");
	}

	return std::get<ByteData>(_data).front();
}

const std::uint8_t& IdType::back() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::back for ByteData");
	}

	return std::get<ByteData>(_data).back();
}

std::uint8_t& IdType::back()
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::back for ByteData");
	}

	return std::get<ByteData>(_data).back();
}

const std::uint8_t* IdType::data() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::data for ByteData");
	}

	return std::get<ByteData>(_data).data();
}

std::uint8_t* IdType::data()
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::data for ByteData");
	}

	return std::get<ByteData>(_data).data();
}

IdType::ByteData::const_iterator IdType::begin() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::begin for ByteData");
	}

	return std::get<ByteData>(_data).begin();
}

IdType::ByteData::iterator IdType::begin()
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::begin for ByteData");
	}

	return std::get<ByteData>(_data).begin();
}

IdType::ByteData::const_iterator IdType::cbegin() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::cbegin for ByteData");
	}

	return std::get<ByteData>(_data).cbegin();
}

IdType::ByteData::const_iterator IdType::end() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::end for ByteData");
	}

	return std::get<ByteData>(_data).end();
}

IdType::ByteData::iterator IdType::end()
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::end for ByteData");
	}

	return std::get<ByteData>(_data).end();
}

IdType::ByteData::const_iterator IdType::cend() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::cend for ByteData");
	}

	return std::get<ByteData>(_data).cend();
}

IdType::ByteData::const_reverse_iterator IdType::rbegin() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::rbegin for ByteData");
	}

	return std::get<ByteData>(_data).rbegin();
}

IdType::ByteData::reverse_iterator IdType::rbegin()
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::rbegin for ByteData");
	}

	return std::get<ByteData>(_data).rbegin();
}

IdType::ByteData::const_reverse_iterator IdType::crbegin() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::crbegin for ByteData");
	}

	return std::get<ByteData>(_data).crbegin();
}

IdType::ByteData::const_reverse_iterator IdType::rend() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::rend for ByteData");
	}

	return std::get<ByteData>(_data).rend();
}

IdType::ByteData::reverse_iterator IdType::rend()
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::rend for ByteData");
	}

	return std::get<ByteData>(_data).rend();
}

IdType::ByteData::const_reverse_iterator IdType::crend() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::crend for ByteData");
	}

	return std::get<ByteData>(_data).crend();
}

const char* IdType::c_str() const
{
	if (!std::holds_alternative<OpaqueString>(_data))
	{
		throw std::logic_error("Invalid call to IdType::crend for OpaqueString");
	}

	return std::get<OpaqueString>(_data).c_str();
}

template <>
const IdType::ByteData& IdType::get<IdType::ByteData>() const
{
	if (!std::holds_alternative<ByteData>(_data))
	{
		throw std::logic_error("Invalid call to IdType::get for ByteData");
	}

	return std::get<ByteData>(_data);
}

template <>
const IdType::OpaqueString& IdType::get<IdType::OpaqueString>() const
{
	if (!std::holds_alternative<OpaqueString>(_data))
	{
		throw std::logic_error("Invalid call to IdType::get for OpaqueString");
	}

	return std::get<OpaqueString>(_data);
}

template <>
IdType::ByteData IdType::release<IdType::ByteData>()
{
	if (std::holds_alternative<ByteData>(_data))
	{
		ByteData data { std::move(std::get<ByteData>(_data)) };

		return data;
	}
	else
	{
		OpaqueString opaque { std::move(std::get<OpaqueString>(_data)) };

		return internal::Base64::fromBase64(opaque);
	}
}

template <>
IdType::OpaqueString IdType::release<IdType::OpaqueString>()
{
	if (std::holds_alternative<OpaqueString>(_data))
	{
		OpaqueString opaque { std::move(std::get<OpaqueString>(_data)) };

		return opaque;
	}
	else
	{
		ByteData data { std::move(std::get<ByteData>(_data)) };

		return internal::Base64::toBase64(data);
	}
}

bool Value::MapData::operator==(const MapData& rhs) const
{
	return map == rhs.map;
}

bool Value::StringData::operator==(const StringData& rhs) const
{
	return (from_json || from_input) == (rhs.from_json || rhs.from_input) && string == rhs.string;
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
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

	if (std::holds_alternative<EnumData>(_data))
	{
		std::get<EnumData>(_data) = std::move(value);
	}
	else if (std::holds_alternative<IdType>(_data))
	{
		std::get<IdType>(_data) = std::move(value);
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
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

	if (!std::holds_alternative<BooleanType>(_data))
	{
		throw std::logic_error("Invalid call to Value::set for BooleanType");
	}

	_data = { value };
}

template <>
void Value::set<IntType>(IntType value)
{
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

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
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

	if (!std::holds_alternative<FloatType>(_data))
	{
		throw std::logic_error("Invalid call to Value::set for FloatType");
	}

	_data = { value };
}

template <>
void Value::set<ScalarType>(ScalarType&& value)
{
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

	if (!std::holds_alternative<ScalarData>(_data))
	{
		throw std::logic_error("Invalid call to Value::set for ScalarType");
	}

	_data = { ScalarData { std::make_unique<ScalarType>(std::move(value)) } };
}

template <>
void Value::set<IdType>(IdType&& value)
{
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

	if (!std::holds_alternative<IdType>(_data))
	{
		throw std::logic_error("Invalid call to Value::set for IdType");
	}

	std::get<IdType>(_data) = std::move(value);
}

template <>
const MapType& Value::get<MapType>() const
{
	const auto& typeData = data();

	if (!std::holds_alternative<MapData>(typeData))
	{
		throw std::logic_error("Invalid call to Value::get for MapType");
	}

	return std::get<MapData>(typeData).map;
}

template <>
const ListType& Value::get<ListType>() const
{
	const auto& typeData = data();

	if (!std::holds_alternative<ListType>(typeData))
	{
		throw std::logic_error("Invalid call to Value::get for ListType");
	}

	return std::get<ListType>(typeData);
}

template <>
const StringType& Value::get<StringType>() const
{
	const auto& typeData = data();

	if (std::holds_alternative<EnumData>(typeData))
	{
		return std::get<EnumData>(typeData);
	}
	else if (std::holds_alternative<IdType>(typeData))
	{
		const auto& idType = std::get<IdType>(typeData);

		return idType.get<IdType::OpaqueString>();
	}
	else if (std::holds_alternative<StringData>(typeData))
	{
		return std::get<StringData>(typeData).string;
	}

	throw std::logic_error("Invalid call to Value::get for StringType");
}

template <>
BooleanType Value::get<BooleanType>() const
{
	const auto& typeData = data();

	if (!std::holds_alternative<BooleanType>(typeData))
	{
		throw std::logic_error("Invalid call to Value::get for BooleanType");
	}

	return std::get<BooleanType>(typeData);
}

template <>
IntType Value::get<IntType>() const
{
	const auto& typeData = data();

	if (!std::holds_alternative<IntType>(typeData))
	{
		throw std::logic_error("Invalid call to Value::get for IntType");
	}

	return std::get<IntType>(typeData);
}

template <>
FloatType Value::get<FloatType>() const
{
	const auto& typeData = data();

	if (std::holds_alternative<IntType>(typeData))
	{
		// Coerce IntType to FloatType
		return static_cast<FloatType>(std::get<IntType>(typeData));
	}

	if (!std::holds_alternative<FloatType>(typeData))
	{
		throw std::logic_error("Invalid call to Value::get for FloatType");
	}

	return std::get<FloatType>(typeData);
}

template <>
const ScalarType& Value::get<ScalarType>() const
{
	const auto& typeData = data();

	if (!std::holds_alternative<ScalarData>(typeData))
	{
		throw std::logic_error("Invalid call to Value::get for ScalarType");
	}

	const auto& scalar = std::get<ScalarData>(typeData).scalar;

	if (!scalar)
	{
		throw std::logic_error("Invalid call to Value::get for ScalarType");
	}

	return *scalar;
}

template <>
const IdType& Value::get<IdType>() const
{
	const auto& typeData = data();

	if (std::holds_alternative<IdType>(typeData))
	{
		return std::get<IdType>(typeData);
	}

	throw std::logic_error("Invalid call to Value::get for IdType");
}

template <>
MapType Value::release<MapType>()
{
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

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
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

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
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

	StringType result;

	if (std::holds_alternative<EnumData>(_data))
	{
		result = std::move(std::get<EnumData>(_data));
	}
	else if (std::holds_alternative<IdType>(_data))
	{
		auto& idType = std::get<IdType>(_data);

		result = idType.release<IdType::OpaqueString>();
	}
	else if (std::holds_alternative<StringData>(_data))
	{
		auto& stringData = std::get<StringData>(_data);

		result = std::move(stringData.string);
		stringData.from_json = false;
		stringData.from_input = false;
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
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

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

template <>
IdType Value::release<IdType>()
{
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

	if (std::holds_alternative<StringData>(_data))
	{
		auto& stringData = std::get<StringData>(_data);

		if (stringData.from_json || stringData.from_input)
		{
			auto stringValue = std::move(stringData.string);

			return IdType { std::move(stringValue) };
		}
	}
	else if (std::holds_alternative<IdType>(_data))
	{
		auto idValue = std::move(std::get<IdType>(_data));

		return idValue;
	}

	throw std::logic_error("Invalid call to Value::release for IdType");
}

Value::Value(Type type /* = Type::Null */)
	: _data { NullData {} }
{
	switch (type)
	{
		case Type::Map:
			_data = MapData {};
			break;

		case Type::List:
			_data = ListType {};
			break;

		case Type::String:
			_data = StringData {};
			break;

		case Type::Boolean:
			_data = BooleanType { false };
			break;

		case Type::Int:
			_data = IntType { 0 };
			break;

		case Type::Float:
			_data = FloatType { 0.0 };
			break;

		case Type::EnumValue:
			_data = EnumData {};
			break;

		case Type::ID:
			_data = IdType {};
			break;

		case Type::Scalar:
			_data = ScalarData {};
			break;

		default:
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

Value::Value(IdType&& value)
	: _data(TypeData { IdType { std::move(value) } })
{
}

Value::Value(Value&& other) noexcept
	: _data(std::move(other._data))
{
}

Value::Value(const Value& other)
{
	if (std::holds_alternative<SharedData>(other._data))
	{
		_data = std::get<SharedData>(other._data);
		return;
	}

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

			std::map<std::string_view, std::size_t> members;

			for (const auto& entry : copy.map)
			{
				members[entry.first] = members.size();
			}

			copy.members.reserve(members.size());
			std::transform(members.cbegin(),
				members.cend(),
				std::back_inserter(copy.members),
				[](const auto& entry) noexcept {
					return entry.second;
				});

			_data = { std::move(copy) };
			break;
		}

		case Type::List:
		{
			ListType copy {};

			copy.reserve(other.size());
			for (std::size_t i = 0; i < other.size(); ++i)
			{
				copy.push_back(Value { other[i] });
			}

			_data = { std::move(copy) };
			break;
		}

		case Type::String:
			_data = { StringData { std::get<StringData>(other._data) } };
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

		case Type::ID:
			_data = { IdType { std::get<IdType>(other._data) } };
			break;

		case Type::Scalar:
			_data = { ScalarData { std::make_unique<ScalarType>(other.get<ScalarType>()) } };
			break;
	}
}

Value::Value(std::shared_ptr<const Value> value) noexcept
	: _data(TypeData { value })
{
}

const Value::TypeData& Value::data() const noexcept
{
	return std::holds_alternative<SharedData>(_data) ? std::get<SharedData>(_data)->data() : _data;
}

Type Value::typeOf(const TypeData& data) noexcept
{
	if (std::holds_alternative<SharedData>(data))
	{
		return typeOf(std::get<SharedData>(data)->_data);
	}

	// As long as the order of the variant alternatives matches the Type enum, we can cast the index
	// to the Type in one step.
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(Type::Map), TypeData>,
			MapData>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(Type::List), TypeData>,
			ListType>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(Type::String), TypeData>,
			StringData>,
		"type mistmatch");
	static_assert(std::is_same_v<
					  std::variant_alternative_t<static_cast<std::size_t>(Type::Boolean), TypeData>,
					  BooleanType>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(Type::Int), TypeData>,
			IntType>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(Type::Float), TypeData>,
			FloatType>,
		"type mistmatch");
	static_assert(
		std::is_same_v<
			std::variant_alternative_t<static_cast<std::size_t>(Type::EnumValue), TypeData>,
			EnumData>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(Type::ID), TypeData>,
			IdType>,
		"type mistmatch");
	static_assert(
		std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(Type::Scalar), TypeData>,
			ScalarData>,
		"type mistmatch");

	return static_cast<Type>(data.index());
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
	const auto& lhsData = data();
	const auto& rhsData = rhs.data();
	const auto lhsType = typeOf(lhsData);
	const auto rhsType = typeOf(rhsData);

	if (lhsType != rhsType)
	{
		// There are a limited set of overlapping string types which we can compare.
		switch (lhsType)
		{
			case Type::String:
			{
				const auto& stringData = std::get<StringData>(lhsData);

				if (std::holds_alternative<EnumData>(rhsData))
				{
					if (stringData.from_json)
					{
						const auto& enumData = std::get<EnumData>(rhsData);

						return stringData.string == enumData;
					}
				}
				else if (std::holds_alternative<IdType>(rhsData))
				{
					const auto& idType = std::get<IdType>(rhsData);

					if (stringData.from_json || stringData.from_input)
					{
						return stringData.string == idType;
					}
				}

				return false;
			}

			case Type::EnumValue:
			{
				const auto& enumData = std::get<EnumData>(lhsData);

				if (std::holds_alternative<StringData>(rhsData))
				{
					const auto& stringData = std::get<StringData>(rhsData);

					if (stringData.from_json)
					{
						return enumData == stringData.string;
					}
				}

				return false;
			}

			case Type::ID:
			{
				const auto& idType = std::get<IdType>(rhsData);

				if (std::holds_alternative<StringData>(rhsData))
				{
					const auto& stringData = std::get<StringData>(rhsData);

					if (stringData.from_json || stringData.from_input)
					{
						return idType == stringData.string;
					}
				}

				return false;
			}

			default:
				return false;
		}
	}

	return lhsData == rhsData;
}

Type Value::type() const noexcept
{
	return typeOf(_data);
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
	const auto& typeData = data();

	return std::holds_alternative<EnumData>(typeData)
		|| (std::holds_alternative<StringData>(typeData)
			&& std::get<StringData>(typeData).from_json);
}

Value&& Value::from_input() noexcept
{
	if (std::holds_alternative<StringData>(_data))
	{
		std::get<StringData>(_data).from_input = true;
	}

	return std::move(*this);
}

bool Value::maybe_id() const noexcept
{
	const auto& typeData = data();

	if (std::holds_alternative<IdType>(typeData))
	{
		return true;
	}
	else if (std::holds_alternative<StringData>(typeData))
	{
		const auto& stringData = std::get<StringData>(typeData);

		return stringData.from_json || stringData.from_input;
	}

	return false;
}

void Value::reserve(std::size_t count)
{
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

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

std::size_t Value::size() const
{
	switch (type())
	{
		case Type::Map:
		{
			return std::get<MapData>(data()).map.size();
		}

		case Type::List:
		{
			return std::get<ListType>(data()).size();
		}

		default:
			throw std::logic_error("Invalid call to Value::size");
	}
}

bool Value::emplace_back(std::string&& name, Value&& value)
{
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

	if (!std::holds_alternative<MapData>(_data))
	{
		throw std::logic_error("Invalid call to Value::emplace_back for MapType");
	}

	auto& mapData = std::get<MapData>(_data);
	const auto [itr, itrEnd] = std::equal_range(mapData.members.cbegin(),
		mapData.members.cend(),
		std::nullopt,
		[&mapData, &name](std::optional<std::size_t> lhs, std::optional<std::size_t> rhs) noexcept {
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
	const auto& typeData = data();

	if (!std::holds_alternative<MapData>(typeData))
	{
		throw std::logic_error("Invalid call to Value::find for MapType");
	}

	const auto& mapData = std::get<MapData>(typeData);
	const auto [itr, itrEnd] = std::equal_range(mapData.members.cbegin(),
		mapData.members.cend(),
		std::nullopt,
		[&mapData, name](std::optional<std::size_t> lhs, std::optional<std::size_t> rhs) noexcept {
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
	const auto& typeData = data();

	if (!std::holds_alternative<MapData>(typeData))
	{
		throw std::logic_error("Invalid call to Value::begin for MapType");
	}

	return std::get<MapData>(typeData).map.cbegin();
}

MapType::const_iterator Value::end() const
{
	const auto& typeData = data();

	if (!std::holds_alternative<MapData>(typeData))
	{
		throw std::logic_error("Invalid call to Value::end for MapType");
	}

	return std::get<MapData>(typeData).map.cend();
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
	if (std::holds_alternative<SharedData>(_data))
	{
		*this = Value { *std::get<SharedData>(_data) };
	}

	if (!std::holds_alternative<ListType>(_data))
	{
		throw std::logic_error("Invalid call to Value::emplace_back for ListType");
	}

	std::get<ListType>(_data).emplace_back(std::move(value));
}

const Value& Value::operator[](std::size_t index) const
{
	const auto& typeData = data();

	if (!std::holds_alternative<ListType>(typeData))
	{
		throw std::logic_error("Invalid call to Value::operator[] for ListType");
	}

	return std::get<ListType>(typeData).at(index);
}

void Writer::write(Value response) const
{
	switch (response.type())
	{
		case Type::Map:
		{
			auto members = response.release<MapType>();

			_concept->start_object();

			for (auto& entry : members)
			{
				_concept->add_member(entry.first);
				write(std::move(entry.second));
			}

			_concept->end_object();
			break;
		}

		case Type::List:
		{
			auto elements = response.release<ListType>();

			_concept->start_array();

			for (auto& entry : elements)
			{
				write(std::move(entry));
			}

			_concept->end_arrary();
			break;
		}

		case Type::String:
		case Type::EnumValue:
		case Type::ID:
		{
			auto value = response.release<StringType>();

			_concept->write_string(value);
			break;
		}

		case Type::Null:
		{
			_concept->write_null();
			break;
		}

		case Type::Boolean:
		{
			_concept->write_bool(response.get<BooleanType>());
			break;
		}

		case Type::Int:
		{
			_concept->write_int(response.get<IntType>());
			break;
		}

		case Type::Float:
		{
			_concept->write_float(response.get<FloatType>());
			break;
		}

		case Type::Scalar:
		{
			write(response.release<ScalarType>());
			break;
		}

		default:
		{
			_concept->write_null();
			break;
		}
	}
}

} // namespace graphql::response
