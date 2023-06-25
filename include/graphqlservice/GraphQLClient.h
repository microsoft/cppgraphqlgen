// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLCLIENT_H
#define GRAPHQLCLIENT_H

// clang-format off
#ifdef GRAPHQL_DLLEXPORTS
	#ifdef IMPL_GRAPHQLCLIENT_DLL
		#define GRAPHQLCLIENT_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLCLIENT_DLL
		#define GRAPHQLCLIENT_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLCLIENT_DLL
#else // !GRAPHQL_DLLEXPORTS
	#define GRAPHQLCLIENT_EXPORT
#endif // !GRAPHQL_DLLEXPORTS
// clang-format on

#include "graphqlservice/GraphQLResponse.h"

#include "graphqlservice/internal/Version.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace graphql::client {

// Errors may specify the line number and column number where the error occurred.
struct [[nodiscard("unnecessary construction")]] ErrorLocation
{
	int line {};
	int column {};
};

// Errors may specify a path to the field which triggered the error. The path consists of
// field names and the indices of elements in a list.
using ErrorPathSegment = std::variant<std::string, int>;

// Error returned from the service.
struct [[nodiscard("unnecessary construction")]] Error
{
	std::string message;
	std::vector<ErrorLocation> locations;
	std::vector<ErrorPathSegment> path;
};

// Complete response from the service, split into the unparsed graphql::response::Value in
// data and the (typically empty) collection of Errors in errors.
struct [[nodiscard("unnecessary construction")]] ServiceResponse
{
	response::Value data;
	std::vector<Error> errors;
};

// Split a service response into separate ServiceResponse data and errors members.
[[nodiscard("unnecessary conversion")]] GRAPHQLCLIENT_EXPORT ServiceResponse parseServiceResponse(
	response::Value response);

// GraphQL types are nullable by default, but they may be wrapped with non-null or list types.
// Since nullability is a more special case in C++, we invert the default and apply that modifier
// instead when the non-null wrapper is not present in that part of the wrapper chain.
enum class [[nodiscard("unnecessary conversion")]] TypeModifier {
	None,
	Nullable,
	List,
};

// Serialize a single variable input value.
template <typename Type>
struct Variable
{
	// Serialize a single value to the variables document.
	[[nodiscard("unnecessary conversion")]] static response::Value serialize(Type&& value);
};

#ifdef GRAPHQL_DLLEXPORTS
// Export all of the built-in converters
template <>
GRAPHQLCLIENT_EXPORT response::Value Variable<int>::serialize(int&& value);
template <>
GRAPHQLCLIENT_EXPORT response::Value Variable<double>::serialize(double&& value);
template <>
GRAPHQLCLIENT_EXPORT response::Value Variable<std::string>::serialize(std::string&& value);
template <>
GRAPHQLCLIENT_EXPORT response::Value Variable<bool>::serialize(bool&& value);
template <>
GRAPHQLCLIENT_EXPORT response::Value Variable<response::IdType>::serialize(
	response::IdType&& value);
template <>
GRAPHQLCLIENT_EXPORT response::Value Variable<response::Value>::serialize(response::Value&& value);
#endif // GRAPHQL_DLLEXPORTS

namespace {

// These types are used as scalar variables even though they are represented with a class.
template <typename Type>
concept ScalarVariableClass = std::is_same_v<Type, std::string>
	|| std::is_same_v<Type, response::IdType> || std::is_same_v<Type, response::Value>;

// Any non-scalar class used in a variable is a generated INPUT_OBJECT type.
template <typename Type>
concept InputVariableClass = std::is_class_v<Type> && !ScalarVariableClass<Type>;

// Test if there are any non-None modifiers left.
template <TypeModifier... Other>
concept OnlyNoneModifiers = (... && (Other == TypeModifier::None));

// Test if the next modifier is Nullable.
template <TypeModifier Modifier>
concept NullableModifier = Modifier == TypeModifier::Nullable;

// Test if the next modifier is List.
template <TypeModifier Modifier>
concept ListModifier = Modifier == TypeModifier::List;

// Special-case an innermost nullable INPUT_OBJECT type.
template <typename Type, TypeModifier... Other>
concept InputVariableUniquePtr = InputVariableClass<Type> && OnlyNoneModifiers<Other...>;

// Serialize variable input values with chained type modifiers which add nullable or list wrappers.
template <typename Type>
struct ModifiedVariable
{
	// Peel off modifiers until we get to the underlying type.
	template <typename U, TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	struct VariableTraits
	{
		// Peel off modifiers until we get to the underlying type.
		using type = typename std::conditional_t<TypeModifier::Nullable == Modifier,
			typename std::conditional_t<InputVariableUniquePtr<U, Other...>, std::unique_ptr<U>,
				std::optional<typename VariableTraits<U, Other...>::type>>,
			typename std::conditional_t<TypeModifier::List == Modifier,
				std::vector<typename VariableTraits<U, Other...>::type>, U>>;
	};

	template <typename U>
	struct VariableTraits<U, TypeModifier::None>
	{
		using type = U;
	};

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	[[nodiscard("unnecessary conversion")]] static response::Value serialize(Type&& value)
		requires OnlyNoneModifiers<Modifier, Other...>
	{
		// Just call through to the non-template method without the modifiers.
		return Variable<Type>::serialize(std::move(value));
	}

	// Peel off nullable modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	[[nodiscard("unnecessary conversion")]] static response::Value serialize(
		typename VariableTraits<Type, Modifier, Other...>::type&& nullableValue)
		requires NullableModifier<Modifier>
	{
		response::Value result;

		if (nullableValue)
		{
			result = serialize<Other...>(std::move(*nullableValue));
			nullableValue.reset();
		}

		return result;
	}

	// Peel off list modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	[[nodiscard("unnecessary conversion")]] static response::Value serialize(
		typename VariableTraits<Type, Modifier, Other...>::type&& listValue)
		requires ListModifier<Modifier>
	{
		response::Value result { response::Type::List };

		result.reserve(listValue.size());
		std::for_each(listValue.begin(), listValue.end(), [&result](auto& value) {
			result.emplace_back(serialize<Other...>(std::move(value)));
		});
		listValue.clear();

		return result;
	}

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	[[nodiscard("unnecessary memory copy")]] static Type duplicate(const Type& value)
		requires OnlyNoneModifiers<Modifier, Other...>
	{
		// Just copy the value.
		return Type { value };
	}

	// Peel off nullable modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	[[nodiscard("unnecessary memory copy")]] static
		typename VariableTraits<Type, Modifier, Other...>::type
		duplicate(const typename VariableTraits<Type, Modifier, Other...>::type& nullableValue)
		requires NullableModifier<Modifier>
	{
		typename VariableTraits<Type, Modifier, Other...>::type result {};

		if (nullableValue)
		{
			if constexpr (InputVariableUniquePtr<Type, Other...>)
			{
				// Special case duplicating the std::unique_ptr.
				result = std::make_unique<Type>(Type { *nullableValue });
			}
			else
			{
				result = duplicate<Other...>(*nullableValue);
			}
		}

		return result;
	}

	// Peel off list modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	[[nodiscard("unnecessary memory copy")]] static
		typename VariableTraits<Type, Modifier, Other...>::type
		duplicate(const typename VariableTraits<Type, Modifier, Other...>::type& listValue)
		requires ListModifier<Modifier>
	{
		typename VariableTraits<Type, Modifier, Other...>::type result(listValue.size());

		std::transform(listValue.cbegin(), listValue.cend(), result.begin(), duplicate<Other...>);

		return result;
	}
};

// Convenient type aliases for testing, generated code won't actually use these. These are also
// the specializations which are implemented in the GraphQLClient library, other specializations
// for input types should be generated in schemagen.
using IntVariable = ModifiedVariable<int>;
using FloatVariable = ModifiedVariable<double>;
using StringVariable = ModifiedVariable<std::string>;
using BooleanVariable = ModifiedVariable<bool>;
using IdVariable = ModifiedVariable<response::IdType>;
using ScalarVariable = ModifiedVariable<response::Value>;

} // namespace

// Parse a single response output value. This is the inverse of Variable for output types instead of
// input types.
template <typename Type>
struct Response
{
	// Parse a single value of the response document.
	[[nodiscard("unnecessary conversion")]] static Type parse(response::Value&& response);
};

#ifdef GRAPHQL_DLLEXPORTS
// Export all of the built-in converters
template <>
GRAPHQLCLIENT_EXPORT int Response<int>::parse(response::Value&& response);
template <>
GRAPHQLCLIENT_EXPORT double Response<double>::parse(response::Value&& response);
template <>
GRAPHQLCLIENT_EXPORT std::string Response<std::string>::parse(response::Value&& response);
template <>
GRAPHQLCLIENT_EXPORT bool Response<bool>::parse(response::Value&& response);
template <>
GRAPHQLCLIENT_EXPORT response::IdType Response<response::IdType>::parse(response::Value&& response);
template <>
GRAPHQLCLIENT_EXPORT response::Value Response<response::Value>::parse(response::Value&& response);
#endif // GRAPHQL_DLLEXPORTS

namespace {

// Parse response output values with chained type modifiers that add nullable or list wrappers.
// This is the inverse of ModifiedVariable for output types instead of input types.
template <typename Type>
struct ModifiedResponse
{
	// Peel off modifiers until we get to the underlying type.
	template <typename U, TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	struct ResponseTraits
	{
		using type = typename std::conditional_t<TypeModifier::Nullable == Modifier,
			std::optional<typename ResponseTraits<U, Other...>::type>,
			typename std::conditional_t<TypeModifier::List == Modifier,
				std::vector<typename ResponseTraits<U, Other...>::type>, U>>;
	};

	template <typename U>
	struct ResponseTraits<U, TypeModifier::None>
	{
		using type = U;
	};

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	[[nodiscard("unnecessary conversion")]] static Type parse(response::Value&& response)
		requires OnlyNoneModifiers<Modifier, Other...>
	{
		return Response<Type>::parse(std::move(response));
	}

	// Peel off nullable modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	[[nodiscard("unnecessary conversion")]] static std::optional<
		typename ResponseTraits<Type, Other...>::type>
	parse(response::Value&& response)
		requires NullableModifier<Modifier>
	{
		if (response.type() == response::Type::Null)
		{
			return std::nullopt;
		}

		return std::make_optional<typename ResponseTraits<Type, Other...>::type>(
			parse<Other...>(std::move(response)));
	}

	// Peel off list modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	[[nodiscard("unnecessary conversion")]] static std::vector<
		typename ResponseTraits<Type, Other...>::type>
	parse(response::Value&& response)
		requires ListModifier<Modifier>
	{
		std::vector<typename ResponseTraits<Type, Other...>::type> result;

		if (response.type() == response::Type::List)
		{
			auto listValue = response.release<response::ListType>();

			result.reserve(listValue.size());
			std::transform(listValue.begin(),
				listValue.end(),
				std::back_inserter(result),
				[](response::Value& value) {
					return parse<Other...>(std::move(value));
				});
		}

		return result;
	}
};

// Convenient type aliases for testing, generated code won't actually use these. These are also
// the specializations which are implemented in the GraphQLClient library, other specializations
// for output types should be generated in schemagen.
using IntResponse = ModifiedResponse<int>;
using FloatResponse = ModifiedResponse<double>;
using StringResponse = ModifiedResponse<std::string>;
using BooleanResponse = ModifiedResponse<bool>;
using IdResponse = ModifiedResponse<response::IdType>;
using ScalarResponse = ModifiedResponse<response::Value>;

} // namespace
} // namespace graphql::client

#endif // GRAPHQLCLIENT_H
