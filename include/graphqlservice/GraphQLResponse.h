// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLRESPONSE_H
#define GRAPHQLRESPONSE_H

// clang-format off
#ifdef GRAPHQL_DLLEXPORTS
	#ifdef IMPL_GRAPHQLRESPONSE_DLL
		#define GRAPHQLRESPONSE_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLRESPONSE_DLL
		#define GRAPHQLRESPONSE_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLRESPONSE_DLL
#else // !GRAPHQL_DLLEXPORTS
	#define GRAPHQLRESPONSE_EXPORT
#endif // !GRAPHQL_DLLEXPORTS
// clang-format on

#include "graphqlservice/GraphQLError.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace graphql::response {

// GraphQL responses are not technically JSON-specific, although that is probably the most common
// way of representing them. These are the primitive types that may be represented in GraphQL, as
// of the [June 2018 spec](https://facebook.github.io/graphql/June2018/#sec-Serialization-Format).
enum class Type : uint8_t
{
	Map,	   // JSON Object
	List,	   // JSON Array
	String,	   // JSON String
	Null,	   // JSON null
	Boolean,   // JSON true or false
	Int,	   // JSON Number
	Float,	   // JSON Number
	EnumValue, // JSON String
	Scalar,	   // JSON any type
	Result,	   // pair of data=Value, errors=vector<schema_error>
};

struct Value;

using MapType = std::vector<std::pair<std::string, Value>>;
using ListType = std::vector<Value>;
using StringType = std::string;
using BooleanType = bool;
using IntType = int;
using FloatType = double;
using ScalarType = Value;
using IdType = std::vector<uint8_t>;
struct ResultType;

template <typename ValueType>
struct ValueTypeTraits
{
	// Set by r-value reference, get by const reference, and release by value. The only types
	// that actually support all 3 methods are StringType and ScalarType, everything else
	// overrides some subset of these types with a template specialization.
	using set_type = ValueType&&;
	using get_type = const ValueType&;
	using release_type = ValueType;
};

template <>
struct ValueTypeTraits<MapType>
{
	// Get by const reference and release by value.
	using get_type = const MapType&;
	using release_type = MapType;
};

template <>
struct ValueTypeTraits<ListType>
{
	// Get by const reference and release by value.
	using get_type = const ListType&;
	using release_type = ListType;
};

template <>
struct ValueTypeTraits<BooleanType>
{
	// Set and get by value.
	using set_type = BooleanType;
	using get_type = BooleanType;
};

template <>
struct ValueTypeTraits<IntType>
{
	// Set and get by value.
	using set_type = IntType;
	using get_type = IntType;
};

template <>
struct ValueTypeTraits<FloatType>
{
	// Set and get by value.
	using set_type = FloatType;
	using get_type = FloatType;
};

template <>
struct ValueTypeTraits<ResultType>
{
	// Get by const reference and release by value.
	using get_type = const ResultType&;
	using release_type = ResultType;
};

template <>
struct ValueTypeTraits<IdType>
{
	// ID values are represented as a String, there's no separate handling of this type.
};

struct TypedData;

// Represent a discriminated union of GraphQL response value types.
struct Value
{
	GRAPHQLRESPONSE_EXPORT Value(Type type = Type::Null);
	GRAPHQLRESPONSE_EXPORT ~Value();

	GRAPHQLRESPONSE_EXPORT explicit Value(const char* value);
	GRAPHQLRESPONSE_EXPORT explicit Value(StringType&& value);
	GRAPHQLRESPONSE_EXPORT explicit Value(BooleanType value);
	GRAPHQLRESPONSE_EXPORT explicit Value(IntType value);
	GRAPHQLRESPONSE_EXPORT explicit Value(FloatType value);
	GRAPHQLRESPONSE_EXPORT explicit Value(ResultType&& value);

	GRAPHQLRESPONSE_EXPORT Value(Value&& other) noexcept;
	GRAPHQLRESPONSE_EXPORT explicit Value(const Value& other);

	GRAPHQLRESPONSE_EXPORT Value& operator=(Value&& rhs) noexcept;
	Value& operator=(const Value& rhs) = delete;

	// Comparison
	GRAPHQLRESPONSE_EXPORT bool operator==(const Value& rhs) const noexcept;
	GRAPHQLRESPONSE_EXPORT bool operator!=(const Value& rhs) const noexcept;

	// Check the Type
	GRAPHQLRESPONSE_EXPORT Type type() const noexcept;

	// JSON doesn't distinguish between Type::String and Type::EnumValue, so if this value comes
	// from JSON and it's a string we need to track the fact that it can be interpreted as either.
	GRAPHQLRESPONSE_EXPORT Value&& from_json() noexcept;
	GRAPHQLRESPONSE_EXPORT bool maybe_enum() const noexcept;

	// Valid for Type::Map or Type::List
	GRAPHQLRESPONSE_EXPORT void reserve(size_t count);
	GRAPHQLRESPONSE_EXPORT size_t size() const;

	// Valid for Type::Map
	GRAPHQLRESPONSE_EXPORT void emplace_back(std::string&& name, Value&& value);
	GRAPHQLRESPONSE_EXPORT MapType::const_iterator find(const std::string& name) const;
	GRAPHQLRESPONSE_EXPORT MapType::const_iterator begin() const;
	GRAPHQLRESPONSE_EXPORT MapType::const_iterator end() const;
	GRAPHQLRESPONSE_EXPORT const Value& operator[](const std::string& name) const;

	// Valid for Type::List
	GRAPHQLRESPONSE_EXPORT void emplace_back(Value&& value);
	GRAPHQLRESPONSE_EXPORT const Value& operator[](size_t index) const;

	// Valid for Type::Result
	GRAPHQLRESPONSE_EXPORT Value toMap();

	// Specialized for all single-value Types.
	template <typename ValueType>
	void set(typename std::enable_if_t<std::is_same_v<std::decay_t<ValueType>, ValueType>,
		typename ValueTypeTraits<ValueType>::set_type>
			value);

	// Specialized for all Types.
	template <typename ValueType>
	typename std::enable_if_t<std::is_same_v<std::decay_t<ValueType>, ValueType>,
		typename ValueTypeTraits<ValueType>::get_type>
	get() const;

	// Specialized for all Types which allocate extra memory.
	template <typename ValueType>
	typename ValueTypeTraits<ValueType>::release_type release();

	// Compatibility wrappers
	template <typename ReferenceType>
	[[deprecated("Use the unqualified Value::set<> specialization instead of specializing on the "
				 "r-value reference.")]] void
	set(typename std::enable_if_t<std::is_rvalue_reference_v<ReferenceType>, ReferenceType> value)
	{
		set<std::decay_t<ReferenceType>>(std::move(value));
	}

	template <typename ReferenceType>
	[[deprecated("Use the unqualified Value::get<> specialization instead of specializing on the "
				 "const reference.")]]
	typename std::enable_if_t<
		std::is_lvalue_reference_v<
			ReferenceType> && std::is_const_v<typename std::remove_reference_t<ReferenceType>>,
		ReferenceType>
	get() const
	{
		return get<std::decay_t<ReferenceType>>();
	}

private:
	std::unique_ptr<TypedData> _data;
};

struct ResultType
{
	Value data = Value(Type::Null);
	std::vector<graphql::error::schema_error> errors = {};

	ResultType& operator=(const ResultType& other) = delete;

	GRAPHQLRESPONSE_EXPORT bool operator==(const ResultType& rhs) const noexcept
	{
		return data == rhs.data && errors == rhs.errors;
	}

	GRAPHQLRESPONSE_EXPORT size_t size() const;
};

namespace {

using namespace std::literals;

constexpr std::string_view strData { "data"sv };
constexpr std::string_view strErrors { "errors"sv };
constexpr std::string_view strMessage { "message"sv };
constexpr std::string_view strLocations { "locations"sv };
constexpr std::string_view strLine { "line"sv };
constexpr std::string_view strColumn { "column"sv };
constexpr std::string_view strPath { "path"sv };

} // namespace

// Errors should have a message string, and optional locations and a path.
GRAPHQLRESPONSE_EXPORT void addErrorMessage(std::string&& message, Value& error);

GRAPHQLRESPONSE_EXPORT void addErrorLocation(
	const graphql::error::schema_location& location, Value& error);

GRAPHQLRESPONSE_EXPORT void addErrorPath(graphql::error::field_path&& path, Value& error);

GRAPHQLRESPONSE_EXPORT Value buildErrorValues(
	const std::vector<graphql::error::schema_error>& structuredErrors);

#ifdef GRAPHQL_DLLEXPORTS
// Export all of the specialized template methods
template <>
GRAPHQLRESPONSE_EXPORT void Value::set<StringType>(StringType&& value);
template <>
GRAPHQLRESPONSE_EXPORT void Value::set<BooleanType>(BooleanType value);
template <>
GRAPHQLRESPONSE_EXPORT void Value::set<IntType>(IntType value);
template <>
GRAPHQLRESPONSE_EXPORT void Value::set<FloatType>(FloatType value);
template <>
GRAPHQLRESPONSE_EXPORT void Value::set<ScalarType>(ScalarType&& value);
template <>
GRAPHQLRESPONSE_EXPORT const MapType& Value::get<MapType>() const;
template <>
GRAPHQLRESPONSE_EXPORT const ListType& Value::get<ListType>() const;
template <>
GRAPHQLRESPONSE_EXPORT const StringType& Value::get<StringType>() const;
template <>
GRAPHQLRESPONSE_EXPORT BooleanType Value::get<BooleanType>() const;
template <>
GRAPHQLRESPONSE_EXPORT IntType Value::get<IntType>() const;
template <>
GRAPHQLRESPONSE_EXPORT FloatType Value::get<FloatType>() const;
template <>
GRAPHQLRESPONSE_EXPORT const ScalarType& Value::get<ScalarType>() const;
template <>
GRAPHQLRESPONSE_EXPORT const ResultType& Value::get<ResultType>() const;
template <>
GRAPHQLRESPONSE_EXPORT MapType Value::release<MapType>();
template <>
GRAPHQLRESPONSE_EXPORT ListType Value::release<ListType>();
template <>
GRAPHQLRESPONSE_EXPORT StringType Value::release<StringType>();
template <>
GRAPHQLRESPONSE_EXPORT ScalarType Value::release<ScalarType>();
template <>
GRAPHQLRESPONSE_EXPORT ResultType Value::release<ResultType>();
#endif // GRAPHQL_DLLEXPORTS

} /* namespace graphql::response */

#endif // GRAPHQLRESPONSE_H
