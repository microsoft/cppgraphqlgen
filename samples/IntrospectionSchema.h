// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <graphqlservice/GraphQLService.h>

#include <memory>
#include <string>
#include <vector>

namespace facebook {
namespace graphql {
namespace introspection {

class Schema;

enum class __TypeKind
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

enum class __DirectiveLocation
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

class __Schema;
class __Type;
class __Field;
class __InputValue;
class __EnumValue;
class __Directive;

class __Schema
	: public service::Object
{
protected:
	__Schema();

public:
	virtual std::future<std::vector<std::shared_ptr<__Type>>> getTypes(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::shared_ptr<__Type>> getQueryType(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::shared_ptr<__Type>> getMutationType(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::shared_ptr<__Type>> getSubscriptionType(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::vector<std::shared_ptr<__Directive>>> getDirectives(const std::shared_ptr<service::RequestState>& state) const = 0;

private:
	std::future<response::Value> resolveTypes(service::ResolverParams&& params);
	std::future<response::Value> resolveQueryType(service::ResolverParams&& params);
	std::future<response::Value> resolveMutationType(service::ResolverParams&& params);
	std::future<response::Value> resolveSubscriptionType(service::ResolverParams&& params);
	std::future<response::Value> resolveDirectives(service::ResolverParams&& params);

	std::future<response::Value> resolve__typename(service::ResolverParams&& params);
};

class __Type
	: public service::Object
{
protected:
	__Type();

public:
	virtual std::future<__TypeKind> getKind(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<response::StringType>> getName(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<response::StringType>> getDescription(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<std::vector<std::shared_ptr<__Field>>>> getFields(const std::shared_ptr<service::RequestState>& state, std::unique_ptr<response::BooleanType>&& includeDeprecated) const = 0;
	virtual std::future<std::unique_ptr<std::vector<std::shared_ptr<__Type>>>> getInterfaces(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<std::vector<std::shared_ptr<__Type>>>> getPossibleTypes(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<std::vector<std::shared_ptr<__EnumValue>>>> getEnumValues(const std::shared_ptr<service::RequestState>& state, std::unique_ptr<response::BooleanType>&& includeDeprecated) const = 0;
	virtual std::future<std::unique_ptr<std::vector<std::shared_ptr<__InputValue>>>> getInputFields(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::shared_ptr<__Type>> getOfType(const std::shared_ptr<service::RequestState>& state) const = 0;

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

	std::future<response::Value> resolve__typename(service::ResolverParams&& params);
};

class __Field
	: public service::Object
{
protected:
	__Field();

public:
	virtual std::future<response::StringType> getName(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<response::StringType>> getDescription(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::vector<std::shared_ptr<__InputValue>>> getArgs(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::shared_ptr<__Type>> getType(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<response::BooleanType> getIsDeprecated(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<response::StringType>> getDeprecationReason(const std::shared_ptr<service::RequestState>& state) const = 0;

private:
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveDescription(service::ResolverParams&& params);
	std::future<response::Value> resolveArgs(service::ResolverParams&& params);
	std::future<response::Value> resolveType(service::ResolverParams&& params);
	std::future<response::Value> resolveIsDeprecated(service::ResolverParams&& params);
	std::future<response::Value> resolveDeprecationReason(service::ResolverParams&& params);

	std::future<response::Value> resolve__typename(service::ResolverParams&& params);
};

class __InputValue
	: public service::Object
{
protected:
	__InputValue();

public:
	virtual std::future<response::StringType> getName(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<response::StringType>> getDescription(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::shared_ptr<__Type>> getType(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<response::StringType>> getDefaultValue(const std::shared_ptr<service::RequestState>& state) const = 0;

private:
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveDescription(service::ResolverParams&& params);
	std::future<response::Value> resolveType(service::ResolverParams&& params);
	std::future<response::Value> resolveDefaultValue(service::ResolverParams&& params);

	std::future<response::Value> resolve__typename(service::ResolverParams&& params);
};

class __EnumValue
	: public service::Object
{
protected:
	__EnumValue();

public:
	virtual std::future<response::StringType> getName(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<response::StringType>> getDescription(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<response::BooleanType> getIsDeprecated(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<response::StringType>> getDeprecationReason(const std::shared_ptr<service::RequestState>& state) const = 0;

private:
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveDescription(service::ResolverParams&& params);
	std::future<response::Value> resolveIsDeprecated(service::ResolverParams&& params);
	std::future<response::Value> resolveDeprecationReason(service::ResolverParams&& params);

	std::future<response::Value> resolve__typename(service::ResolverParams&& params);
};

class __Directive
	: public service::Object
{
protected:
	__Directive();

public:
	virtual std::future<response::StringType> getName(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::unique_ptr<response::StringType>> getDescription(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::vector<__DirectiveLocation>> getLocations(const std::shared_ptr<service::RequestState>& state) const = 0;
	virtual std::future<std::vector<std::shared_ptr<__InputValue>>> getArgs(const std::shared_ptr<service::RequestState>& state) const = 0;

private:
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveDescription(service::ResolverParams&& params);
	std::future<response::Value> resolveLocations(service::ResolverParams&& params);
	std::future<response::Value> resolveArgs(service::ResolverParams&& params);

	std::future<response::Value> resolve__typename(service::ResolverParams&& params);
};

} /* namespace object */

void AddTypesToSchema(std::shared_ptr<introspection::Schema> schema);

} /* namespace introspection */
} /* namespace graphql */
} /* namespace facebook */