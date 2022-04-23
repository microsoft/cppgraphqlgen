// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef INTROSPECTIONSCHEMA_H
#define INTROSPECTIONSCHEMA_H

#include "graphqlservice/internal/Schema.h"

// Check if the library version is compatible with schemagen 4.2.0
static_assert(graphql::internal::MajorVersion == 4, "regenerate with schemagen: major version mismatch");
static_assert(graphql::internal::MinorVersion == 2, "regenerate with schemagen: minor version mismatch");

#include <array>
#include <memory>
#include <string>
#include <string_view>

namespace graphql {
namespace introspection {

enum class TypeKind
{
	SCALAR,
	OBJECT,
	INTERFACE,
	UNION,
	ENUM,
	INPUT_OBJECT,
	LIST,
	NON_NULL
};

constexpr std::array<std::string_view, 8> getTypeKindNames() noexcept
{
	using namespace std::literals;

	return { {
		R"gql(SCALAR)gql"sv,
		R"gql(OBJECT)gql"sv,
		R"gql(INTERFACE)gql"sv,
		R"gql(UNION)gql"sv,
		R"gql(ENUM)gql"sv,
		R"gql(INPUT_OBJECT)gql"sv,
		R"gql(LIST)gql"sv,
		R"gql(NON_NULL)gql"sv
	} };
}

enum class DirectiveLocation
{
	QUERY,
	MUTATION,
	SUBSCRIPTION,
	FIELD,
	FRAGMENT_DEFINITION,
	FRAGMENT_SPREAD,
	INLINE_FRAGMENT,
	VARIABLE_DEFINITION,
	SCHEMA,
	SCALAR,
	OBJECT,
	FIELD_DEFINITION,
	ARGUMENT_DEFINITION,
	INTERFACE,
	UNION,
	ENUM,
	ENUM_VALUE,
	INPUT_OBJECT,
	INPUT_FIELD_DEFINITION
};

constexpr std::array<std::string_view, 19> getDirectiveLocationNames() noexcept
{
	using namespace std::literals;

	return { {
		R"gql(QUERY)gql"sv,
		R"gql(MUTATION)gql"sv,
		R"gql(SUBSCRIPTION)gql"sv,
		R"gql(FIELD)gql"sv,
		R"gql(FRAGMENT_DEFINITION)gql"sv,
		R"gql(FRAGMENT_SPREAD)gql"sv,
		R"gql(INLINE_FRAGMENT)gql"sv,
		R"gql(VARIABLE_DEFINITION)gql"sv,
		R"gql(SCHEMA)gql"sv,
		R"gql(SCALAR)gql"sv,
		R"gql(OBJECT)gql"sv,
		R"gql(FIELD_DEFINITION)gql"sv,
		R"gql(ARGUMENT_DEFINITION)gql"sv,
		R"gql(INTERFACE)gql"sv,
		R"gql(UNION)gql"sv,
		R"gql(ENUM)gql"sv,
		R"gql(ENUM_VALUE)gql"sv,
		R"gql(INPUT_OBJECT)gql"sv,
		R"gql(INPUT_FIELD_DEFINITION)gql"sv
	} };
}

class Schema;
class Type;
class Field;
class InputValue;
class EnumValue;
class Directive;

namespace object {

class Schema;
class Type;
class Field;
class InputValue;
class EnumValue;
class Directive;

} // namespace object

void AddSchemaDetails(const std::shared_ptr<schema::ObjectType>& typeSchema, const std::shared_ptr<schema::Schema>& schema);
void AddTypeDetails(const std::shared_ptr<schema::ObjectType>& typeType, const std::shared_ptr<schema::Schema>& schema);
void AddFieldDetails(const std::shared_ptr<schema::ObjectType>& typeField, const std::shared_ptr<schema::Schema>& schema);
void AddInputValueDetails(const std::shared_ptr<schema::ObjectType>& typeInputValue, const std::shared_ptr<schema::Schema>& schema);
void AddEnumValueDetails(const std::shared_ptr<schema::ObjectType>& typeEnumValue, const std::shared_ptr<schema::Schema>& schema);
void AddDirectiveDetails(const std::shared_ptr<schema::ObjectType>& typeDirective, const std::shared_ptr<schema::Schema>& schema);

GRAPHQLSERVICE_EXPORT void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema);

} // namespace introspection

namespace service {

#ifdef GRAPHQL_DLLEXPORTS
// Export all of the built-in converters
template <>
GRAPHQLSERVICE_EXPORT introspection::TypeKind ModifiedArgument<introspection::TypeKind>::convert(
	const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT AwaitableResolver ModifiedResult<introspection::TypeKind>::convert(
	AwaitableScalar<introspection::TypeKind> result, ResolverParams params);
template <>
GRAPHQLSERVICE_EXPORT void ModifiedResult<introspection::TypeKind>::validateScalar(
	const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT introspection::DirectiveLocation ModifiedArgument<introspection::DirectiveLocation>::convert(
	const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT AwaitableResolver ModifiedResult<introspection::DirectiveLocation>::convert(
	AwaitableScalar<introspection::DirectiveLocation> result, ResolverParams params);
template <>
GRAPHQLSERVICE_EXPORT void ModifiedResult<introspection::DirectiveLocation>::validateScalar(
	const response::Value& value);
#endif // GRAPHQL_DLLEXPORTS

} // namespace service
} // namespace graphql

#endif // INTROSPECTIONSCHEMA_H
