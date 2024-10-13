// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "graphqlservice/GraphQLService.h"

#include "IntrospectionSharedTypes.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace graphql {
namespace service {

static const auto s_namesTypeKind = introspection::getTypeKindNames();
static const auto s_valuesTypeKind = introspection::getTypeKindValues();

template <>
introspection::TypeKind Argument<introspection::TypeKind>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid __TypeKind value)ex" } };
	}

	const auto result = internal::sorted_map_lookup<internal::shorter_or_less>(
		s_valuesTypeKind,
		std::string_view { value.get<std::string>() });

	if (!result)
	{
		throw service::schema_exception { { R"ex(not a valid __TypeKind value)ex" } };
	}

	return *result;
}

template <>
service::AwaitableResolver Result<introspection::TypeKind>::convert(service::AwaitableScalar<introspection::TypeKind> result, ResolverParams&& params)
{
	return ModifiedResult<introspection::TypeKind>::resolve(std::move(result), std::move(params),
		[](introspection::TypeKind value, const ResolverParams&)
		{
			return ResolverResult { { ResultToken { ResultToken::EnumValue { std::string { s_namesTypeKind[static_cast<std::size_t>(value)] } } } } };
		});
}

template <>
void Result<introspection::TypeKind>::validateScalar(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid __TypeKind value)ex" } };
	}

	const auto [itr, itrEnd] = internal::sorted_map_equal_range<internal::shorter_or_less>(
		s_valuesTypeKind.begin(),
		s_valuesTypeKind.end(),
		std::string_view { value.get<std::string>() });

	if (itr == itrEnd)
	{
		throw service::schema_exception { { R"ex(not a valid __TypeKind value)ex" } };
	}
}

static const auto s_namesDirectiveLocation = introspection::getDirectiveLocationNames();
static const auto s_valuesDirectiveLocation = introspection::getDirectiveLocationValues();

template <>
introspection::DirectiveLocation Argument<introspection::DirectiveLocation>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid __DirectiveLocation value)ex" } };
	}

	const auto result = internal::sorted_map_lookup<internal::shorter_or_less>(
		s_valuesDirectiveLocation,
		std::string_view { value.get<std::string>() });

	if (!result)
	{
		throw service::schema_exception { { R"ex(not a valid __DirectiveLocation value)ex" } };
	}

	return *result;
}

template <>
service::AwaitableResolver Result<introspection::DirectiveLocation>::convert(service::AwaitableScalar<introspection::DirectiveLocation> result, ResolverParams&& params)
{
	return ModifiedResult<introspection::DirectiveLocation>::resolve(std::move(result), std::move(params),
		[](introspection::DirectiveLocation value, const ResolverParams&)
		{
			return ResolverResult { { ResultToken { ResultToken::EnumValue { std::string { s_namesDirectiveLocation[static_cast<std::size_t>(value)] } } } } };
		});
}

template <>
void Result<introspection::DirectiveLocation>::validateScalar(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid __DirectiveLocation value)ex" } };
	}

	const auto [itr, itrEnd] = internal::sorted_map_equal_range<internal::shorter_or_less>(
		s_valuesDirectiveLocation.begin(),
		s_valuesDirectiveLocation.end(),
		std::string_view { value.get<std::string>() });

	if (itr == itrEnd)
	{
		throw service::schema_exception { { R"ex(not a valid __DirectiveLocation value)ex" } };
	}
}

} // namespace service
} // namespace graphql
