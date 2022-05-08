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
struct [[nodiscard]] ErrorLocation
{
	int line {};
	int column {};
};

// Errors may specify a path to the field which triggered the error. The path consists of
// field names and the indices of elements in a list.
using ErrorPathSegment = std::variant<std::string, int>;

// Error returned from the service.
struct [[nodiscard]] Error
{
	std::string message;
	std::vector<ErrorLocation> locations;
	std::vector<ErrorPathSegment> path;
};

// Complete response from the service, split into the unparsed graphql::response::Value in
// data and the (typically empty) collection of Errors in errors.
struct [[nodiscard]] ServiceResponse
{
	response::Value data;
	std::vector<Error> errors;
};

// Split a service response into separate ServiceResponse data and errors members.
GRAPHQLCLIENT_EXPORT [[nodiscard]] ServiceResponse parseServiceResponse(response::Value response);

// GraphQL types are nullable by default, but they may be wrapped with non-null or list types.
// Since nullability is a more special case in C++, we invert the default and apply that modifier
// instead when the non-null wrapper is not present in that part of the wrapper chain.
enum class [[nodiscard]] TypeModifier {
	None,
	Nullable,
	List,
};

// Specialized to return true for all INPUT_OBJECT types.
template <typename Type>
[[nodiscard]] constexpr bool isInputType() noexcept
{
	return false;
}

// Special-case an innermost nullable INPUT_OBJECT type.
template <TypeModifier... Other>
[[nodiscard]] constexpr bool onlyNoneModifiers() noexcept
{
	return (... && (Other == TypeModifier::None));
}

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
			typename std::conditional_t<isInputType<U>() && onlyNoneModifiers<Other...>(),
				std::unique_ptr<U>, std::optional<typename VariableTraits<U, Other...>::type>>,
			typename std::conditional_t<TypeModifier::List == Modifier,
				std::vector<typename VariableTraits<U, Other...>::type>, U>>;
	};

	template <typename U>
	struct VariableTraits<U, TypeModifier::None>
	{
		using type = U;
	};

	// Serialize a single value to the variables document.
	[[nodiscard]] static response::Value serialize(Type&& value);

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	[[nodiscard]] static
		typename std::enable_if_t<TypeModifier::None == Modifier && sizeof...(Other) == 0,
			response::Value>
		serialize(Type&& value)
	{
		// Just call through to the non-template method without the modifiers.
		return serialize(std::move(value));
	}

	// Peel off nullable modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	[[nodiscard]] static
		typename std::enable_if_t<TypeModifier::Nullable == Modifier, response::Value>
		serialize(typename VariableTraits<Type, Modifier, Other...>::type&& nullableValue)
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
	[[nodiscard]] static typename std::enable_if_t<TypeModifier::List == Modifier, response::Value>
	serialize(typename VariableTraits<Type, Modifier, Other...>::type&& listValue)
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
	[[nodiscard]] static
		typename std::enable_if_t<TypeModifier::None == Modifier && sizeof...(Other) == 0, Type>
		duplicate(const Type& value)
	{
		// Just copy the value.
		return Type { value };
	}

	// Peel off nullable modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	[[nodiscard]] static typename std::enable_if_t<TypeModifier::Nullable == Modifier,
		typename VariableTraits<Type, Modifier, Other...>::type>
	duplicate(const typename VariableTraits<Type, Modifier, Other...>::type& nullableValue)
	{
		typename VariableTraits<Type, Modifier, Other...>::type result {};

		if (nullableValue)
		{
			if constexpr (isInputType<Type>() && onlyNoneModifiers<Other...>())
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
	[[nodiscard]] static typename std::enable_if_t<TypeModifier::List == Modifier,
		typename VariableTraits<Type, Modifier, Other...>::type>
	duplicate(const typename VariableTraits<Type, Modifier, Other...>::type& listValue)
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

#ifdef GRAPHQL_DLLEXPORTS
// Export all of the built-in converters
template <>
GRAPHQLCLIENT_EXPORT response::Value ModifiedVariable<int>::serialize(int&& value);
template <>
GRAPHQLCLIENT_EXPORT response::Value ModifiedVariable<double>::serialize(double&& value);
template <>
GRAPHQLCLIENT_EXPORT response::Value ModifiedVariable<std::string>::serialize(std::string&& value);
template <>
GRAPHQLCLIENT_EXPORT response::Value ModifiedVariable<bool>::serialize(bool&& value);
template <>
GRAPHQLCLIENT_EXPORT response::Value ModifiedVariable<response::IdType>::serialize(
	response::IdType&& value);
template <>
GRAPHQLCLIENT_EXPORT response::Value ModifiedVariable<response::Value>::serialize(
	response::Value&& value);
#endif // GRAPHQL_DLLEXPORTS

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

	// Parse a single value of the response document.
	[[nodiscard]] static Type parse(response::Value&& response);

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	[[nodiscard]] static
		typename std::enable_if_t<TypeModifier::None == Modifier && sizeof...(Other) == 0, Type>
		parse(response::Value&& response)
	{
		return parse(std::move(response));
	}

	// Peel off nullable modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	[[nodiscard]] static typename std::enable_if_t<TypeModifier::Nullable == Modifier,
		std::optional<typename ResponseTraits<Type, Other...>::type>>
	parse(response::Value&& response)
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
	[[nodiscard]] static typename std::enable_if_t<TypeModifier::List == Modifier,
		std::vector<typename ResponseTraits<Type, Other...>::type>>
	parse(response::Value&& response)
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

#ifdef GRAPHQL_DLLEXPORTS
// Export all of the built-in converters
template <>
GRAPHQLCLIENT_EXPORT int ModifiedResponse<int>::parse(response::Value&& response);
template <>
GRAPHQLCLIENT_EXPORT double ModifiedResponse<double>::parse(response::Value&& response);
template <>
GRAPHQLCLIENT_EXPORT std::string ModifiedResponse<std::string>::parse(response::Value&& response);
template <>
GRAPHQLCLIENT_EXPORT bool ModifiedResponse<bool>::parse(response::Value&& response);
template <>
GRAPHQLCLIENT_EXPORT response::IdType ModifiedResponse<response::IdType>::parse(
	response::Value&& response);
template <>
GRAPHQLCLIENT_EXPORT response::Value ModifiedResponse<response::Value>::parse(
	response::Value&& response);
#endif // GRAPHQL_DLLEXPORTS

} // namespace graphql::client

#endif // GRAPHQLCLIENT_H
