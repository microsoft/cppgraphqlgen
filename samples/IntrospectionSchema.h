// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "GraphQLService.h"

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
	virtual std::vector<std::shared_ptr<__Type>> getTypes() const = 0;
	virtual std::shared_ptr<__Type> getQueryType() const = 0;
	virtual std::shared_ptr<__Type> getMutationType() const = 0;
	virtual std::shared_ptr<__Type> getSubscriptionType() const = 0;
	virtual std::vector<std::shared_ptr<__Directive>> getDirectives() const = 0;

private:
	rapidjson::Document resolveTypes(service::ResolverParams&& params);
	rapidjson::Document resolveQueryType(service::ResolverParams&& params);
	rapidjson::Document resolveMutationType(service::ResolverParams&& params);
	rapidjson::Document resolveSubscriptionType(service::ResolverParams&& params);
	rapidjson::Document resolveDirectives(service::ResolverParams&& params);

	rapidjson::Document resolve__typename(service::ResolverParams&& params);
};

class __Type
	: public service::Object
{
protected:
	__Type();

public:
	virtual __TypeKind getKind() const = 0;
	virtual std::unique_ptr<std::string> getName() const = 0;
	virtual std::unique_ptr<std::string> getDescription() const = 0;
	virtual std::unique_ptr<std::vector<std::shared_ptr<__Field>>> getFields(std::unique_ptr<bool>&& includeDeprecated) const = 0;
	virtual std::unique_ptr<std::vector<std::shared_ptr<__Type>>> getInterfaces() const = 0;
	virtual std::unique_ptr<std::vector<std::shared_ptr<__Type>>> getPossibleTypes() const = 0;
	virtual std::unique_ptr<std::vector<std::shared_ptr<__EnumValue>>> getEnumValues(std::unique_ptr<bool>&& includeDeprecated) const = 0;
	virtual std::unique_ptr<std::vector<std::shared_ptr<__InputValue>>> getInputFields() const = 0;
	virtual std::shared_ptr<__Type> getOfType() const = 0;

private:
	rapidjson::Document resolveKind(service::ResolverParams&& params);
	rapidjson::Document resolveName(service::ResolverParams&& params);
	rapidjson::Document resolveDescription(service::ResolverParams&& params);
	rapidjson::Document resolveFields(service::ResolverParams&& params);
	rapidjson::Document resolveInterfaces(service::ResolverParams&& params);
	rapidjson::Document resolvePossibleTypes(service::ResolverParams&& params);
	rapidjson::Document resolveEnumValues(service::ResolverParams&& params);
	rapidjson::Document resolveInputFields(service::ResolverParams&& params);
	rapidjson::Document resolveOfType(service::ResolverParams&& params);

	rapidjson::Document resolve__typename(service::ResolverParams&& params);
};

class __Field
	: public service::Object
{
protected:
	__Field();

public:
	virtual std::string getName() const = 0;
	virtual std::unique_ptr<std::string> getDescription() const = 0;
	virtual std::vector<std::shared_ptr<__InputValue>> getArgs() const = 0;
	virtual std::shared_ptr<__Type> getType() const = 0;
	virtual bool getIsDeprecated() const = 0;
	virtual std::unique_ptr<std::string> getDeprecationReason() const = 0;

private:
	rapidjson::Document resolveName(service::ResolverParams&& params);
	rapidjson::Document resolveDescription(service::ResolverParams&& params);
	rapidjson::Document resolveArgs(service::ResolverParams&& params);
	rapidjson::Document resolveType(service::ResolverParams&& params);
	rapidjson::Document resolveIsDeprecated(service::ResolverParams&& params);
	rapidjson::Document resolveDeprecationReason(service::ResolverParams&& params);

	rapidjson::Document resolve__typename(service::ResolverParams&& params);
};

class __InputValue
	: public service::Object
{
protected:
	__InputValue();

public:
	virtual std::string getName() const = 0;
	virtual std::unique_ptr<std::string> getDescription() const = 0;
	virtual std::shared_ptr<__Type> getType() const = 0;
	virtual std::unique_ptr<std::string> getDefaultValue() const = 0;

private:
	rapidjson::Document resolveName(service::ResolverParams&& params);
	rapidjson::Document resolveDescription(service::ResolverParams&& params);
	rapidjson::Document resolveType(service::ResolverParams&& params);
	rapidjson::Document resolveDefaultValue(service::ResolverParams&& params);

	rapidjson::Document resolve__typename(service::ResolverParams&& params);
};

class __EnumValue
	: public service::Object
{
protected:
	__EnumValue();

public:
	virtual std::string getName() const = 0;
	virtual std::unique_ptr<std::string> getDescription() const = 0;
	virtual bool getIsDeprecated() const = 0;
	virtual std::unique_ptr<std::string> getDeprecationReason() const = 0;

private:
	rapidjson::Document resolveName(service::ResolverParams&& params);
	rapidjson::Document resolveDescription(service::ResolverParams&& params);
	rapidjson::Document resolveIsDeprecated(service::ResolverParams&& params);
	rapidjson::Document resolveDeprecationReason(service::ResolverParams&& params);

	rapidjson::Document resolve__typename(service::ResolverParams&& params);
};

class __Directive
	: public service::Object
{
protected:
	__Directive();

public:
	virtual std::string getName() const = 0;
	virtual std::unique_ptr<std::string> getDescription() const = 0;
	virtual std::vector<__DirectiveLocation> getLocations() const = 0;
	virtual std::vector<std::shared_ptr<__InputValue>> getArgs() const = 0;

private:
	rapidjson::Document resolveName(service::ResolverParams&& params);
	rapidjson::Document resolveDescription(service::ResolverParams&& params);
	rapidjson::Document resolveLocations(service::ResolverParams&& params);
	rapidjson::Document resolveArgs(service::ResolverParams&& params);

	rapidjson::Document resolve__typename(service::ResolverParams&& params);
};

} /* namespace object */

void AddTypesToSchema(std::shared_ptr<introspection::Schema> schema);

} /* namespace introspection */
} /* namespace graphql */
} /* namespace facebook */