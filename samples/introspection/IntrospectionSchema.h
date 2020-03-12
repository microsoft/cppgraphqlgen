// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef INTROSPECTIONSCHEMA_H
#define INTROSPECTIONSCHEMA_H

#include "graphqlservice/GraphQLService.h"

#include <memory>
#include <string>
#include <vector>

namespace graphql {
namespace introspection {

class Schema;

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
	std::future<response::Value> resolveTypes(service::ResolverParams&& params);
	std::future<response::Value> resolveQueryType(service::ResolverParams&& params);
	std::future<response::Value> resolveMutationType(service::ResolverParams&& params);
	std::future<response::Value> resolveSubscriptionType(service::ResolverParams&& params);
	std::future<response::Value> resolveDirectives(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
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
	std::future<response::Value> resolveKind(service::ResolverParams&& params);
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveDescription(service::ResolverParams&& params);
	std::future<response::Value> resolveFields(service::ResolverParams&& params);
	std::future<response::Value> resolveInterfaces(service::ResolverParams&& params);
	std::future<response::Value> resolvePossibleTypes(service::ResolverParams&& params);
	std::future<response::Value> resolveEnumValues(service::ResolverParams&& params);
	std::future<response::Value> resolveInputFields(service::ResolverParams&& params);
	std::future<response::Value> resolveOfType(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
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
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveDescription(service::ResolverParams&& params);
	std::future<response::Value> resolveArgs(service::ResolverParams&& params);
	std::future<response::Value> resolveType(service::ResolverParams&& params);
	std::future<response::Value> resolveIsDeprecated(service::ResolverParams&& params);
	std::future<response::Value> resolveDeprecationReason(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
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
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveDescription(service::ResolverParams&& params);
	std::future<response::Value> resolveType(service::ResolverParams&& params);
	std::future<response::Value> resolveDefaultValue(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
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
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveDescription(service::ResolverParams&& params);
	std::future<response::Value> resolveIsDeprecated(service::ResolverParams&& params);
	std::future<response::Value> resolveDeprecationReason(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
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
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveDescription(service::ResolverParams&& params);
	std::future<response::Value> resolveLocations(service::ResolverParams&& params);
	std::future<response::Value> resolveArgs(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace object */

GRAPHQLSERVICE_EXPORT void AddTypesToSchema(const std::shared_ptr<introspection::Schema>& schema);

} /* namespace introspection */
} /* namespace graphql */

#endif // INTROSPECTIONSCHEMA_H
