// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphQLResponse.h"

#include <stdexcept>

namespace facebook {
namespace graphql {
namespace response {

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

		case Type::Boolean:
			_boolean = false;
			break;

		case Type::Int:
			_int = 0;
			break;

		case Type::Float:
			_float = 0;
			break;

		case Type::Scalar:
			_scalar.reset(new Value());
			break;

		default:
			break;
	}
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

Value::Value(Value&& other)
	: _type(other._type)
	, _members(std::move(other._members))
	, _map(std::move(other._map))
	, _list(std::move(other._list))
	, _string(std::move(other._string))
	, _scalar(std::move(other._scalar))
{
	switch (_type)
	{
		case Type::Boolean:
			_boolean = other._boolean;
			break;

		case Type::Int:
			_int = other._int;
			break;

		case Type::Float:
			_float = other._float;
			break;

		default:
			break;
	}
}

Value::Value(const Value& other)
	: _type(other._type)
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

		case Type::Boolean:
			_boolean = other._boolean;
			break;

		case Type::Int:
			_int = other._int;
			break;

		case Type::Float:
			_float = other._float;
			break;

		case Type::Scalar:
			_scalar.reset(new Value(*other._scalar));
			break;

		default:
			break;
	}
}

Value& Value::operator=(Value&& rhs)
{
	const_cast<Type&>(_type) = rhs._type;
	const_cast<Type&>(rhs._type) = Type::Null;

	_members = std::move(rhs._members);
	_map = std::move(rhs._map);
	_list = std::move(rhs._list);
	_string = std::move(rhs._string);
	_scalar = std::move(rhs._scalar);

	switch (_type)
	{
		case Type::Boolean:
			_boolean = rhs._boolean;
			break;

		case Type::Int:
			_int = rhs._int;
			break;

		case Type::Float:
			_float = rhs._float;
			break;

		default:
			break;
	}

	return *this;
}

Value::Type Value::type() const
{
	return _type;
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

Value::MapType::const_iterator Value::find(const std::string& name) const
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
void Value::set<Value::StringType>(StringType&& value)
{
	if (_type != Type::String
		&& _type != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::set for StringType");
	}

	*_string = std::move(value);
}

template <>
void Value::set<Value::BooleanType>(BooleanType&& value)
{
	if (_type != Type::Boolean)
	{
		throw std::logic_error("Invalid call to Value::set for BooleanType");
	}

	_boolean = value;
}

template <>
void Value::set<Value::IntType>(IntType&& value)
{
	if (_type != Type::Int)
	{
		throw std::logic_error("Invalid call to Value::set for IntType");
	}

	_int = value;
}

template <>
void Value::set<Value::FloatType>(FloatType&& value)
{
	if (_type != Type::Float)
	{
		throw std::logic_error("Invalid call to Value::set for FloatType");
	}

	_float = value;
}

template <>
void Value::set<Value::ScalarType>(ScalarType&& value)
{
	if (_type != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::set for ScalarType");
	}

	*_scalar = std::move(value);
}

template <>
const Value::MapType& Value::get<const Value::MapType&>() const
{
	if (_type != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::get for MapType");
	}

	return *_map;
}

template <>
const Value::ListType& Value::get<const Value::ListType&>() const
{
	if (_type != Type::List)
	{
		throw std::logic_error("Invalid call to Value::get for ListType");
	}

	return *_list;
}

template <>
const Value::StringType& Value::get<const Value::StringType&>() const
{
	if (_type != Type::String
		&& _type != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::get for StringType");
	}

	return *_string;
}

template <>
Value::BooleanType Value::get<Value::BooleanType>() const
{
	if (_type != Type::Boolean)
	{
		throw std::logic_error("Invalid call to Value::get for BooleanType");
	}

	return _boolean;
}

template <>
Value::IntType Value::get<Value::IntType>() const
{
	if (_type != Type::Int)
	{
		throw std::logic_error("Invalid call to Value::get for IntType");
	}

	return _int;
}

template <>
Value::FloatType Value::get<Value::FloatType>() const
{
	if (_type != Type::Float)
	{
		throw std::logic_error("Invalid call to Value::get for FloatType");
	}

	return _float;
}

template <>
const Value::ScalarType& Value::get<const Value::ScalarType&>() const
{
	if (_type != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::get for ScalarType");
	}

	return *_scalar;
}

template <>
Value::MapType Value::release<Value::MapType>()
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
Value::ListType Value::release<Value::ListType>()
{
	if (_type != Type::List)
	{
		throw std::logic_error("Invalid call to Value::release for ListType");
	}

	ListType result = std::move(*_list);

	return result;
}

template <>
Value::StringType Value::release<Value::StringType>()
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
Value::ScalarType Value::release<Value::ScalarType>()
{
	if (_type != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::release for ScalarType");
	}

	ScalarType result = std::move(*_scalar);

	return result;
}

} /* namespace response */
} /* namespace graphql */
} /* namespace facebook */