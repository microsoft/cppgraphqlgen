// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <graphqlservice/GraphQLResponse.h>

#include <stdexcept>

namespace facebook::graphql::response {

Value::Value(Type type /*= Type::Null*/)
	: _type(type)
{
	switch (type)
	{
		case Type::Map:
			_members.reset(new std::unordered_map<std::string, size_t>());
			_map.reset(new MapType());
			break;

		case Type::List:
			_list.reset(new ListType());
			break;

		case Type::String:
		case Type::EnumValue:
			_string.reset(new StringType());
			break;

		case Type::Scalar:
			_scalar.reset(new Value());
			break;

		default:
			break;
	}
}

Value::~Value()
{
	// The default destructor gets inlined and may use a different allocator to free Value's member
	// variables than the graphqlservice module used to allocate them. So even though this could be
	// omitted, declare it explicitly and define it in graphqlservice.
}

Value::Value(const char* value)
	: _type(Type::String)
	, _string(new StringType(value))
{
}

Value::Value(StringType&& value)
	: _type(Type::String)
	, _string(new StringType(std::move(value)))
{
}

Value::Value(BooleanType value)
	: _type(Type::Boolean)
	, _boolean(value)
{
}

Value::Value(IntType value)
	: _type(Type::Int)
	, _int(value)
{
}

Value::Value(FloatType value)
	: _type(Type::Float)
	, _float(value)
{
}

Value::Value(Value&& other) noexcept
	: _type(other._type)
	, _members(std::move(other._members))
	, _map(std::move(other._map))
	, _list(std::move(other._list))
	, _string(std::move(other._string))
	, _from_json(other._from_json)
	, _scalar(std::move(other._scalar))
	, _boolean(other._boolean)
	, _int(other._int)
	, _float(other._float)
{
	const_cast<Type&>(other._type) = Type::Null;
	other._from_json = false;
	other._boolean = false;
	other._int = 0;
	other._float = 0.0;
}

Value::Value(const Value& other)
	: _type(other._type)
	, _boolean(other._boolean)
	, _int(other._int)
	, _float(other._float)
{
	switch (_type)
	{
		case Type::Map:
			_members.reset(new std::unordered_map<std::string, size_t>(*other._members));
			_map.reset(new MapType(*other._map));
			break;

		case Type::List:
			_list.reset(new ListType(*other._list));
			break;

		case Type::String:
		case Type::EnumValue:
			_string.reset(new StringType(*other._string));
			break;

		case Type::Scalar:
			_scalar.reset(new Value(*other._scalar));
			break;

		default:
			break;
	}
}

Value& Value::operator=(Value&& rhs) noexcept
{
	const_cast<Type&>(_type) = rhs._type;
	const_cast<Type&>(rhs._type) = Type::Null;

	_members = std::move(rhs._members);
	_map = std::move(rhs._map);
	_list = std::move(rhs._list);
	_string = std::move(rhs._string);
	_from_json = rhs._from_json;
	rhs._from_json = false;
	_scalar = std::move(rhs._scalar);
	_boolean = rhs._boolean;
	rhs._boolean = false;
	_int = rhs._int;
	rhs._int = 0;
	_float = rhs._float;
	rhs._float = 0.0;

	return *this;
}

bool Value::operator==(const Value& rhs) const noexcept
{
	if (rhs.type() != _type)
	{
		return false;
	}

	switch (_type)
	{
		case Type::Map:
			return *_map == *rhs._map;

		case Type::List:
			return *_list == *rhs._list;

		case Type::String:
		case Type::EnumValue:
			return *_string == *rhs._string;

		case Type::Null:
			return true;

		case Type::Boolean:
			return _boolean == rhs._boolean;

		case Type::Int:
			return _int == rhs._int;

		case Type::Float:
			return _float == rhs._float;

		case Type::Scalar:
			return *_scalar == *rhs._scalar;

		default:
			return false;
	}
}

bool Value::operator!=(const Value& rhs) const noexcept
{
	return !(*this == rhs);
}

Type Value::type() const  noexcept
{
	return _type;
}

Value&& Value::from_json() noexcept
{
	_from_json = true;

	return std::move(*this);
}

bool Value::maybe_enum() const noexcept
{
	return _type == Type::EnumValue
		|| (_from_json && _type == Type::String);
}

void Value::reserve(size_t count)
{
	switch (_type)
	{
		case Type::Map:
			_members->reserve(count);
			_map->reserve(count);
			break;

		case Type::List:
			_list->reserve(count);
			break;

		default:
			throw std::logic_error("Invalid call to Value::reserve");
	}
}

size_t Value::size() const
{
	switch (_type)
	{
		case Type::Map:
			return _map->size();

		case Type::List:
			return _list->size();

		default:
			throw std::logic_error("Invalid call to Value::size");
	}
}

void Value::emplace_back(std::string&& name, Value&& value)
{
	if (_type != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::emplace_back for MapType");
	}

	if (_members->find(name) != _members->cend())
	{
		throw std::runtime_error("Duplicate Map member");
	}

	_members->insert({ name, _map->size() });
	_map->emplace_back(std::make_pair(std::move(name), std::move(value)));
}

MapType::const_iterator Value::find(const std::string& name) const
{
	if (_type != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::find for MapType");
	}

	const auto itr = _members->find(name);

	if (itr == _members->cend())
	{
		return _map->cend();
	}

	return _map->cbegin() + itr->second;
}

MapType::const_iterator Value::begin() const
{
	if (_type != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::end for MapType");
	}

	return _map->cbegin();
}

MapType::const_iterator Value::end() const
{
	if (_type != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::end for MapType");
	}

	return _map->cend();
}

const Value& Value::operator[](const std::string& name) const
{
	const auto itr = find(name);

	if (itr == _map->cend())
	{
		throw std::runtime_error("Missing Map member");
	}

	return itr->second;
}

void Value::emplace_back(Value&& value)
{
	if (_type != Type::List)
	{
		throw std::logic_error("Invalid call to Value::emplace_back for ListType");
	}

	_list->emplace_back(std::move(value));
}

const Value& Value::operator[](size_t index) const
{
	if (_type != Type::List)
	{
		throw std::logic_error("Invalid call to Value::emplace_back for ListType");
	}

	return _list->at(index);
}

template <>
void Value::set<StringType>(StringType&& value)
{
	if (_type != Type::String
		&& _type != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::set for StringType");
	}

	*_string = std::move(value);
}

template <>
void Value::set<BooleanType>(BooleanType&& value)
{
	if (_type != Type::Boolean)
	{
		throw std::logic_error("Invalid call to Value::set for BooleanType");
	}

	_boolean = value;
}

template <>
void Value::set<IntType>(IntType&& value)
{
	if (_type != Type::Int)
	{
		throw std::logic_error("Invalid call to Value::set for IntType");
	}

	_int = value;
}

template <>
void Value::set<FloatType>(FloatType&& value)
{
	if (_type != Type::Float)
	{
		throw std::logic_error("Invalid call to Value::set for FloatType");
	}

	_float = value;
}

template <>
void Value::set<ScalarType>(ScalarType&& value)
{
	if (_type != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::set for ScalarType");
	}

	*_scalar = std::move(value);
}

template <>
const MapType& Value::get<const MapType&>() const
{
	if (_type != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::get for MapType");
	}

	return *_map;
}

template <>
const ListType& Value::get<const ListType&>() const
{
	if (_type != Type::List)
	{
		throw std::logic_error("Invalid call to Value::get for ListType");
	}

	return *_list;
}

template <>
const StringType& Value::get<const StringType&>() const
{
	if (_type != Type::String
		&& _type != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::get for StringType");
	}

	return *_string;
}

template <>
BooleanType Value::get<BooleanType>() const
{
	if (_type != Type::Boolean)
	{
		throw std::logic_error("Invalid call to Value::get for BooleanType");
	}

	return _boolean;
}

template <>
IntType Value::get<IntType>() const
{
	if (_type != Type::Int)
	{
		throw std::logic_error("Invalid call to Value::get for IntType");
	}

	return _int;
}

template <>
FloatType Value::get<FloatType>() const
{
	if (_type != Type::Float)
	{
		throw std::logic_error("Invalid call to Value::get for FloatType");
	}

	return _float;
}

template <>
const ScalarType& Value::get<const ScalarType&>() const
{
	if (_type != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::get for ScalarType");
	}

	return *_scalar;
}

template <>
MapType Value::release<MapType>()
{
	if (_type != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::release for MapType");
	}

	MapType result = std::move(*_map);

	_members->clear();

	return result;
}

template <>
ListType Value::release<ListType>()
{
	if (_type != Type::List)
	{
		throw std::logic_error("Invalid call to Value::release for ListType");
	}

	ListType result = std::move(*_list);

	return result;
}

template <>
StringType Value::release<StringType>()
{
	if (_type != Type::String
		&& _type != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::release for StringType");
	}

	StringType result = std::move(*_string);

	return result;
}

template <>
ScalarType Value::release<ScalarType>()
{
	if (_type != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::release for ScalarType");
	}

	ScalarType result = std::move(*_scalar);

	return result;
}

} /* namespace facebook::graphql::response */
