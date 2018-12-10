// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace facebook {
namespace graphql {
namespace response {

// Represent a discriminated union of GraphQL response value types.
struct Value
{
	// GraphQL responses are not technically JSON-specific, although that is probably the most common
	// way of representing them. These are the primitive types that may be represented in GraphQL, as
	// of the [June 2018 spec](https://facebook.github.io/graphql/June2018/#sec-Serialization-Format).
	enum class Type : uint8_t
	{
		Map,		// JSON Object
		List,		// JSON Array
		String,		// JSON String
		Null,		// JSON null
		Boolean,	// JSON true or false
		Int,		// JSON Number
		Float,		// JSON Number
		EnumValue,	// JSON String
		Scalar,		// JSON any type
	};

	using MapType = std::vector<std::pair<std::string, Value>>;
	using ListType = std::vector<Value>;
	using StringType = std::string;
	using BooleanType = bool;
	using IntType = int;
	using FloatType = double;
	using ScalarType = Value;

	Value(Type type = Type::Null);

	explicit Value(StringType&& value);
	explicit Value(BooleanType value);
	explicit Value(IntType value);
	explicit Value(FloatType value);

	Value(Value&& other);
	explicit Value(const Value& other);

	Value& operator=(Value&& rhs);
	Value& operator=(const Value& rhs) = delete;

	// Check the Type
	Type type() const;

	// Valid for Type::Map or Type::List
	void reserve(size_t count);
	size_t size() const;

	// Valid for Type::Map
	void emplace_back(std::string&& name, Value&& value);
	MapType::const_iterator find(const std::string& name) const;
	const Value& operator[](const std::string& name) const;

	// Valid for Type::List
	void emplace_back(Value&& value);
	const Value& operator[](size_t index) const;

	template <typename _Value>
	void set(_Value&& value)
	{
		// Specialized for all single-value Types.
		static_assert(false, "Use emplace_back to add elements to a collection.");
	}

	template <> void set<StringType>(StringType&& value);
	template <> void set<BooleanType>(BooleanType&& value);
	template <> void set<IntType>(IntType&& value);
	template <> void set<FloatType>(FloatType&& value);
	template <> void set<ScalarType>(ScalarType&& value);

	template <typename _Value>
	_Value get() const
	{
		// Specialized for all Types.
		static_assert(false, "Invalid type.");
	}

	template <> const MapType& get<const MapType&>() const;
	template <> const ListType& get<const ListType&>() const;
	template <> const StringType& get<const StringType&>() const;
	template <> BooleanType get<BooleanType>() const;
	template <> IntType get<IntType>() const;
	template <> FloatType get<FloatType>() const;
	template <> const ScalarType& get<const ScalarType&>() const;

	template <typename _Value>
	_Value release()
	{
		// Specialized for all Types which allocate extra memory.
		static_assert(false, "Use get<> to access this type.");
	}

	template <> MapType release<MapType>();
	template <> ListType release<ListType>();
	template <> StringType release<StringType>();
	template <> ScalarType release<ScalarType>();

private:
	const Type _type;

	// Type::Map
	std::unique_ptr<std::unordered_map<std::string, size_t>> _members;
	std::unique_ptr<MapType> _map;

	// Type::List
	std::unique_ptr<ListType> _list;

	// Type::String or Type::EnumValue
	std::unique_ptr<StringType> _string;

	union
	{
		// Type::Boolean
		BooleanType _boolean;

		// Type::Int
		IntType _int;

		// Type::Float
		FloatType _float;
	};

	// Type::Scalar
	std::unique_ptr<ScalarType> _scalar;
};

template <>
void Value::set<Value::StringType>(StringType&& value)
{
	if (_type != Type::String
		&& _type != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::set<StringType>");
	}

	*_string = std::move(value);
}

template <>
void Value::set<Value::BooleanType>(BooleanType&& value)
{
	if (_type != Type::Boolean)
	{
		throw std::logic_error("Invalid call to Value::set<BooleanType>");
	}

	_boolean = value;
}

template <>
void Value::set<Value::IntType>(IntType&& value)
{
	if (_type != Type::Int)
	{
		throw std::logic_error("Invalid call to Value::set<IntType>");
	}

	_int = value;
}

template <>
void Value::set<Value::FloatType>(FloatType&& value)
{
	if (_type != Type::Float)
	{
		throw std::logic_error("Invalid call to Value::set<FloatType>");
	}

	_float = value;
}

template <>
void Value::set<Value::ScalarType>(ScalarType&& value)
{
	if (_type != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::set<ScalarType>");
	}

	*_scalar = std::move(value);
}

template <>
const Value::MapType& Value::get<const Value::MapType&>() const
{
	if (_type != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::get<MapType>");
	}

	return *_map;
}

template <>
const Value::ListType& Value::get<const Value::ListType&>() const
{
	if (_type != Type::List)
	{
		throw std::logic_error("Invalid call to Value::get<ListType>");
	}

	return *_list;
}

template <>
const Value::StringType& Value::get<const Value::StringType&>() const
{
	if (_type != Type::String
		&& _type != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::get<StringType>");
	}

	return *_string;
}

template <>
Value::BooleanType Value::get<Value::BooleanType>() const
{
	if (_type != Type::Boolean)
	{
		throw std::logic_error("Invalid call to Value::get<BooleanType>");
	}

	return _boolean;

}
template <>
Value::IntType Value::get<Value::IntType>() const
{
	if (_type != Type::Int)
	{
		throw std::logic_error("Invalid call to Value::get<IntType>");
	}

	return _int;
}

template <>
Value::FloatType Value::get<Value::FloatType>() const
{
	if (_type != Type::Float)
	{
		throw std::logic_error("Invalid call to Value::get<FloatType>");
	}

	return _float;
}

template <>
const Value::ScalarType& Value::get<const Value::ScalarType&>() const
{
	if (_type != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::get<ScalarType>");
	}

	return *_scalar;
}

template <>
Value::MapType Value::release<Value::MapType>()
{
	if (_type != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::release<MapType>");
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
		throw std::logic_error("Invalid call to Value::release<ListType>");
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
		throw std::logic_error("Invalid call to Value::release<StringType>");
	}

	StringType result = std::move(*_string);

	return result;
}

template <>
Value::ScalarType Value::release<Value::ScalarType>()
{
	if (_type != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::release<ScalarType>");
	}

	ScalarType result = std::move(*_scalar);

	return result;
}

} /* namespace response */
} /* namespace graphql */
} /* namespace facebook */