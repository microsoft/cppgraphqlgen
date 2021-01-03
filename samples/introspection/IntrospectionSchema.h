// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef INTROSPECTIONSCHEMA_H
#define INTROSPECTIONSCHEMA_H

#include "graphqlservice/GraphQLSchema.h"
#include "graphqlservice/GraphQLService.h"

// Check if the library version is compatible with schemagen 3.4.1
static_assert(graphql::internal::MajorVersion == 3, "regenerate with schemagen: major version mismatch");
static_assert(graphql::internal::MinorVersion == 4, "regenerate with schemagen: minor version mismatch");

// clang-format off
#ifdef GRAPHQL_DLLEXPORTS
	#ifdef IMPL_GRAPHQLINTROSPECTION_DLL
		#define GRAPHQLINTROSPECTION_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLINTROSPECTION_DLL
		#define GRAPHQLINTROSPECTION_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLINTROSPECTION_DLL
#else // !GRAPHQL_DLLEXPORTS
	#define GRAPHQLINTROSPECTION_EXPORT
#endif // !GRAPHQL_DLLEXPORTS
// clang-format on

#include <memory>
#include <string>
#include <vector>

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

enum class DirectiveLocation
{
	QUERY,
	MUTATION,
	SUBSCRIPTION,
	FIELD,
	FRAGMENT_DEFINITION,
	FRAGMENT_SPREAD,
	INLINE_FRAGMENT,
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

namespace object {

class Schema;
class Type;
class Field;
class InputValue;
class EnumValue;
class Directive;

class Schema
	: public service::Object
{
protected:
	explicit Schema();

public:
	virtual service::FieldResult<std::vector<std::shared_ptr<Type>>> getTypes(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::shared_ptr<Type>> getQueryType(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::shared_ptr<Type>> getMutationType(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::shared_ptr<Type>> getSubscriptionType(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::vector<std::shared_ptr<Directive>>> getDirectives(service::FieldParams&& params) const = 0;

private:
	std::future<service::ResolverResult> resolveTypes(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveQueryType(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveMutationType(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveSubscriptionType(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveDirectives(service::ResolverParams&& params);

	std::future<service::ResolverResult> resolve_typename(service::ResolverParams&& params);
};

class Type
	: public service::Object
{
protected:
	explicit Type();

public:
	virtual service::FieldResult<TypeKind> getKind(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<std::vector<std::shared_ptr<Field>>>> getFields(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const = 0;
	virtual service::FieldResult<std::optional<std::vector<std::shared_ptr<Type>>>> getInterfaces(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<std::vector<std::shared_ptr<Type>>>> getPossibleTypes(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<std::vector<std::shared_ptr<EnumValue>>>> getEnumValues(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const = 0;
	virtual service::FieldResult<std::optional<std::vector<std::shared_ptr<InputValue>>>> getInputFields(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::shared_ptr<Type>> getOfType(service::FieldParams&& params) const = 0;

private:
	std::future<service::ResolverResult> resolveKind(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveName(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveDescription(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveFields(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveInterfaces(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolvePossibleTypes(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveEnumValues(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveInputFields(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveOfType(service::ResolverParams&& params);

	std::future<service::ResolverResult> resolve_typename(service::ResolverParams&& params);
};

class Field
	: public service::Object
{
protected:
	explicit Field();

public:
	virtual service::FieldResult<response::StringType> getName(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::vector<std::shared_ptr<InputValue>>> getArgs(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::shared_ptr<Type>> getType(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<response::BooleanType> getIsDeprecated(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<response::StringType>> getDeprecationReason(service::FieldParams&& params) const = 0;

private:
	std::future<service::ResolverResult> resolveName(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveDescription(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveArgs(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveType(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveIsDeprecated(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveDeprecationReason(service::ResolverParams&& params);

	std::future<service::ResolverResult> resolve_typename(service::ResolverParams&& params);
};

class InputValue
	: public service::Object
{
protected:
	explicit InputValue();

public:
	virtual service::FieldResult<response::StringType> getName(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::shared_ptr<Type>> getType(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<response::StringType>> getDefaultValue(service::FieldParams&& params) const = 0;

private:
	std::future<service::ResolverResult> resolveName(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveDescription(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveType(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveDefaultValue(service::ResolverParams&& params);

	std::future<service::ResolverResult> resolve_typename(service::ResolverParams&& params);
};

class EnumValue
	: public service::Object
{
protected:
	explicit EnumValue();

public:
	virtual service::FieldResult<response::StringType> getName(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<response::BooleanType> getIsDeprecated(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<response::StringType>> getDeprecationReason(service::FieldParams&& params) const = 0;

private:
	std::future<service::ResolverResult> resolveName(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveDescription(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveIsDeprecated(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveDeprecationReason(service::ResolverParams&& params);

	std::future<service::ResolverResult> resolve_typename(service::ResolverParams&& params);
};

class Directive
	: public service::Object
{
protected:
	explicit Directive();

public:
	virtual service::FieldResult<response::StringType> getName(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::vector<DirectiveLocation>> getLocations(service::FieldParams&& params) const = 0;
	virtual service::FieldResult<std::vector<std::shared_ptr<InputValue>>> getArgs(service::FieldParams&& params) const = 0;

private:
	std::future<service::ResolverResult> resolveName(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveDescription(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveLocations(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveArgs(service::ResolverParams&& params);

	std::future<service::ResolverResult> resolve_typename(service::ResolverParams&& params);
};

} /* namespace object */

GRAPHQLINTROSPECTION_EXPORT void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema);

} /* namespace introspection */
} /* namespace graphql */

#endif // INTROSPECTIONSCHEMA_H
