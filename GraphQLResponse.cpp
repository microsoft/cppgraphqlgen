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

} /* namespace response */
} /* namespace graphql */
} /* namespace facebook */