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

struct Value;

using MapType = std::vector<std::pair<std::string, Value>>;
using ListType = std::vector<Value>;
using StringType = std::string;
using BooleanType = bool;
using IntType = int;
using FloatType = double;
using ScalarType = Value;

// Represent a discriminated union of GraphQL response value types.
struct Value
{
	Value(Type type = Type::Null);

	explicit Value(StringType&& value);
	explicit Value(BooleanType value);
	explicit Value(IntType value);
	explicit Value(FloatType value);

	Value(Value&& other) noexcept;
	explicit Value(const Value& other);

	Value& operator=(Value&& rhs) noexcept;
	Value& operator=(const Value& rhs) = delete;

	// Check the Type
	Type type() const;

	// Valid for Type::Map or Type::List
	void reserve(size_t count);
	size_t size() const;

	// Valid for Type::Map
	void emplace_back(std::string&& name, Value&& value);
	MapType::const_iterator find(const std::string& name) const;
	MapType::const_iterator begin() const;
	MapType::const_iterator end() const;
	const Value& operator[](const std::string& name) const;

	// Valid for Type::List
	void emplace_back(Value&& value);
	const Value& operator[](size_t index) const;

	// Specialized for all single-value Types.
	template <typename _Value>
	void set(_Value&& value);

	// Specialized for all Types.
	template <typename _Value>
	_Value get() const;

	// Specialized for all Types which allocate extra memory.
	template <typename _Value>
	_Value release();

private:
	const Type _type;

	// Type::Map
	std::unique_ptr<std::unordered_map<std::string, size_t>> _members;
	std::unique_ptr<MapType> _map;

	// Type::List
	std::unique_ptr<ListType> _list;

	// Type::String or Type::EnumValue
	std::unique_ptr<StringType> _string;

	// Type::Boolean
	BooleanType _boolean = false;

	// Type::Int
	IntType _int = 0;

	// Type::Float
	FloatType _float = 0.0;

	// Type::Scalar
	std::unique_ptr<ScalarType> _scalar;
};

} /* namespace response */
} /* namespace graphql */
} /* namespace facebook */