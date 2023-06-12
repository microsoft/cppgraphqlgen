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

#include "graphqlservice/internal/Awaitable.h"

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace graphql::response {

// GraphQL responses are not technically JSON-specific, although that is probably the most common
// way of representing them. These are the primitive types that may be represented in GraphQL, as
// of the [October 2021 spec](https://spec.graphql.org/October2021/#sec-Serialization-Format).
enum class [[nodiscard("unnecessary conversion")]] Type : std::uint8_t {
	Map,	   // JSON Object
	List,	   // JSON Array
	String,	   // JSON String
	Null,	   // JSON null
	Boolean,   // JSON true or false
	Int,	   // JSON Number
	Float,	   // JSON Number
	EnumValue, // JSON String
	ID,		   // JSON String
	Scalar,	   // JSON any type
};

struct Value;

using MapType = std::vector<std::pair<std::string, Value>>;
using ListType = std::vector<Value>;
using StringType = std::string;
using BooleanType = bool;
using IntType = int;
using FloatType = double;
using ScalarType = Value;

struct [[nodiscard("unnecessary conversion")]] IdType
{
	using ByteData = std::vector<std::uint8_t>;
	using OpaqueString = std::string;

	GRAPHQLRESPONSE_EXPORT IdType(IdType&& other = IdType { ByteData {} }) noexcept;
	GRAPHQLRESPONSE_EXPORT IdType(const IdType& other);
	GRAPHQLRESPONSE_EXPORT ~IdType();

	// Implicit ByteData constructors
	GRAPHQLRESPONSE_EXPORT IdType(size_t count, typename ByteData::value_type value = 0);
	GRAPHQLRESPONSE_EXPORT IdType(std::initializer_list<typename ByteData::value_type> values);
	template <typename InputIt>
	IdType(InputIt begin, InputIt end);

	// Assignment
	GRAPHQLRESPONSE_EXPORT IdType& operator=(IdType&& rhs) noexcept;
	IdType& operator=(const IdType& rhs) = delete;

	// Conversion
	GRAPHQLRESPONSE_EXPORT IdType(ByteData&& data) noexcept;
	GRAPHQLRESPONSE_EXPORT IdType(OpaqueString&& opaque) noexcept;

	template <typename ValueType>
	[[nodiscard("unnecessary call")]] const ValueType& get() const;

	template <typename ValueType>
	[[nodiscard("unnecessary call")]] ValueType release();

	// Comparison
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT bool operator==(
		const IdType& rhs) const noexcept;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT bool operator==(
		const ByteData& rhs) const noexcept;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT bool operator==(
		const OpaqueString& rhs) const noexcept;

	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT bool operator<(
		const IdType& rhs) const noexcept;

	// Check the type
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT bool isBase64() const noexcept;

	// Shared accessors
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT bool empty() const noexcept;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT size_t size() const noexcept;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT size_t max_size() const noexcept;
	GRAPHQLRESPONSE_EXPORT void reserve(size_t new_cap);
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT size_t capacity() const noexcept;
	GRAPHQLRESPONSE_EXPORT void shrink_to_fit();
	GRAPHQLRESPONSE_EXPORT void clear() noexcept;

	// ByteData accessors
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT const std::uint8_t& at(
		size_t pos) const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT std::uint8_t& at(size_t pos);
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT const std::uint8_t& operator[](
		size_t pos) const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT std::uint8_t& operator[](size_t pos);
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT const std::uint8_t& front() const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT std::uint8_t& front();
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT const std::uint8_t& back() const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT std::uint8_t& back();
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT const std::uint8_t* data() const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT std::uint8_t* data();

	// ByteData iterators
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::const_iterator begin() const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::iterator begin();
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::const_iterator cbegin()
		const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::const_iterator end() const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::iterator end();
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::const_iterator cend() const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::const_reverse_iterator
	rbegin() const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::reverse_iterator rbegin();
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::const_reverse_iterator
	crbegin() const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::const_reverse_iterator rend()
		const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::reverse_iterator rend();
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT ByteData::const_reverse_iterator
	crend() const;

	// OpaqueString accessors
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT const char* c_str() const;

private:
	std::variant<ByteData, OpaqueString> _data;
};

#ifdef GRAPHQL_DLLEXPORTS
// Export all of the specialized template methods
template <>
GRAPHQLRESPONSE_EXPORT IdType::IdType(
	typename ByteData::const_iterator begin, typename ByteData::const_iterator end);
template <>
GRAPHQLRESPONSE_EXPORT IdType::IdType(
	typename ByteData::const_pointer begin, typename ByteData::const_pointer end);
template <>
GRAPHQLRESPONSE_EXPORT IdType::IdType(
	typename ByteData::pointer begin, typename ByteData::pointer end);
template <>
GRAPHQLRESPONSE_EXPORT const IdType::ByteData& IdType::get<IdType::ByteData>() const;
template <>
GRAPHQLRESPONSE_EXPORT const IdType::OpaqueString& IdType::get<IdType::OpaqueString>() const;
template <>
GRAPHQLRESPONSE_EXPORT IdType::ByteData IdType::release<IdType::ByteData>();
template <>
GRAPHQLRESPONSE_EXPORT IdType::OpaqueString IdType::release<IdType::OpaqueString>();
#endif // GRAPHQL_DLLEXPORTS

template <typename ValueType>
struct ValueTypeTraits
{
	// Set by r-value reference, get by const reference, and release by value. The only types that
	// actually support all 3 methods are StringType, IdType, and ScalarType, everything else
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

// Represent a discriminated union of GraphQL response value types.
struct [[nodiscard("unnecessary conversion")]] Value
{
	GRAPHQLRESPONSE_EXPORT Value(Type type = Type::Null);
	GRAPHQLRESPONSE_EXPORT ~Value();

	GRAPHQLRESPONSE_EXPORT explicit Value(const char* value);
	GRAPHQLRESPONSE_EXPORT explicit Value(StringType&& value);
	GRAPHQLRESPONSE_EXPORT explicit Value(BooleanType value);
	GRAPHQLRESPONSE_EXPORT explicit Value(IntType value);
	GRAPHQLRESPONSE_EXPORT explicit Value(FloatType value);
	GRAPHQLRESPONSE_EXPORT explicit Value(IdType&& value);

	GRAPHQLRESPONSE_EXPORT Value(Value&& other) noexcept;
	GRAPHQLRESPONSE_EXPORT explicit Value(const Value& other);

	GRAPHQLRESPONSE_EXPORT Value(std::shared_ptr<const Value> other) noexcept;

	GRAPHQLRESPONSE_EXPORT Value& operator=(Value&& rhs) noexcept;
	Value& operator=(const Value& rhs) = delete;

	// Comparison
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT bool operator==(
		const Value& rhs) const noexcept;

	// Check the Type
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT Type type() const noexcept;

	// JSON doesn't distinguish between Type::String, Type::EnumValue, and Type::ID, so if this
	// value comes from JSON and it's a string we need to track the fact that it can be interpreted
	// as any of those types.
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT Value&& from_json() noexcept;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT bool maybe_enum() const noexcept;

	// Input values don't distinguish between Type::String and Type::ID, so if this value comes from
	// a string literal input value we need to track that fact that it can be interpreted as either
	// of those types.
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT Value&& from_input() noexcept;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT bool maybe_id() const noexcept;

	// Valid for Type::Map or Type::List
	GRAPHQLRESPONSE_EXPORT void reserve(size_t count);
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT size_t size() const;

	// Valid for Type::Map
	GRAPHQLRESPONSE_EXPORT bool emplace_back(std::string&& name, Value&& value);
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT MapType::const_iterator find(
		std::string_view name) const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT MapType::const_iterator begin() const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT MapType::const_iterator end() const;
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT const Value& operator[](
		std::string_view name) const;

	// Valid for Type::List
	GRAPHQLRESPONSE_EXPORT void emplace_back(Value&& value);
	[[nodiscard("unnecessary call")]] GRAPHQLRESPONSE_EXPORT const Value& operator[](
		size_t index) const;

	// Specialized for all single-value Types.
	template <typename ValueType>
	void set(typename ValueTypeTraits<ValueType>::set_type value);

	// Specialized for all Types.
	template <typename ValueType>
	[[nodiscard("unnecessary call")]] typename ValueTypeTraits<ValueType>::get_type get() const;

	// Specialized for all Types which allocate extra memory.
	template <typename ValueType>
	[[nodiscard("unnecessary call")]] typename ValueTypeTraits<ValueType>::release_type release();

private:
	// Type::Map
	struct [[nodiscard("unnecessary construction")]] MapData
	{
		[[nodiscard("unnecessary call")]] bool operator==(const MapData& rhs) const;

		MapType map;
		std::vector<size_t> members;
	};

	// Type::String
	struct [[nodiscard("unnecessary construction")]] StringData
	{
		[[nodiscard("unnecessary call")]] bool operator==(const StringData& rhs) const;

		StringType string;
		bool from_json = false;
		bool from_input = false;
	};

	// Type::Null
	struct [[nodiscard("unnecessary construction")]] NullData
	{
		[[nodiscard("unnecessary call")]] bool operator==(const NullData& rhs) const;
	};

	// Type::EnumValue
	using EnumData = StringType;

	// Type::Scalar
	struct [[nodiscard("unnecessary construction")]] ScalarData
	{
		[[nodiscard("unnecessary call")]] bool operator==(const ScalarData& rhs) const;

		std::unique_ptr<ScalarType> scalar;
	};

	using SharedData = std::shared_ptr<const Value>;

	using TypeData = std::variant<MapData, ListType, StringData, NullData, BooleanType, IntType,
		FloatType, EnumData, IdType, ScalarData, SharedData>;

	[[nodiscard("unnecessary call")]] const TypeData& data() const noexcept;

	[[nodiscard("unnecessary call")]] static Type typeOf(const TypeData& data) noexcept;

	TypeData _data;
};

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
GRAPHQLRESPONSE_EXPORT void Value::set<IdType>(IdType&& value);
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
GRAPHQLRESPONSE_EXPORT const IdType& Value::get<IdType>() const;
template <>
GRAPHQLRESPONSE_EXPORT MapType Value::release<MapType>();
template <>
GRAPHQLRESPONSE_EXPORT ListType Value::release<ListType>();
template <>
GRAPHQLRESPONSE_EXPORT StringType Value::release<StringType>();
template <>
GRAPHQLRESPONSE_EXPORT ScalarType Value::release<ScalarType>();
template <>
GRAPHQLRESPONSE_EXPORT IdType Value::release<IdType>();
#endif // GRAPHQL_DLLEXPORTS

using AwaitableValue = internal::Awaitable<Value>;

class [[nodiscard("unnecessary construction")]] Writer final
{
private:
	struct Concept
	{
		virtual ~Concept() = default;

		virtual void start_object() const = 0;
		virtual void add_member(const std::string& key) const = 0;
		virtual void end_object() const = 0;

		virtual void start_array() const = 0;
		virtual void end_arrary() const = 0;

		virtual void write_null() const = 0;
		virtual void write_string(const std::string& value) const = 0;
		virtual void write_bool(bool value) const = 0;
		virtual void write_int(int value) const = 0;
		virtual void write_float(double value) const = 0;
	};

	template <class T>
	struct Model : Concept
	{
		explicit Model(std::unique_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		void start_object() const final
		{
			_pimpl->start_object();
		}

		void add_member(const std::string& key) const final
		{
			_pimpl->add_member(key);
		}

		void end_object() const final
		{
			_pimpl->end_object();
		}

		void start_array() const final
		{
			_pimpl->start_array();
		}

		void end_arrary() const final
		{
			_pimpl->end_arrary();
		}

		void write_null() const final
		{
			_pimpl->write_null();
		}

		void write_string(const std::string& value) const final
		{
			_pimpl->write_string(value);
		}

		void write_bool(bool value) const final
		{
			_pimpl->write_bool(value);
		}

		void write_int(int value) const final
		{
			_pimpl->write_int(value);
		}

		void write_float(double value) const final
		{
			_pimpl->write_float(value);
		}

	private:
		std::unique_ptr<T> _pimpl;
	};

	const std::shared_ptr<const Concept> _concept;

public:
	template <class T>
	Writer(std::unique_ptr<T> writer) noexcept
		: _concept { std::static_pointer_cast<const Concept>(
			std::make_shared<Model<T>>(std::move(writer))) }
	{
	}

	GRAPHQLRESPONSE_EXPORT void write(Value value) const;
};

} // namespace graphql::response

#endif // GRAPHQLRESPONSE_H
