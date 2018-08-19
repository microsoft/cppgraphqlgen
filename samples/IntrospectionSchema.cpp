// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "IntrospectionSchema.h"
#include "Introspection.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <exception>

namespace facebook {
namespace graphql {
namespace service {

template <>
introspection::__DirectiveLocation ModifiedArgument<introspection::__DirectiveLocation>::convert(const web::json::value& value)
{
	static const std::unordered_map<std::string, introspection::__DirectiveLocation> s_names = {
		{ "QUERY", introspection::__DirectiveLocation::QUERY },
		{ "MUTATION", introspection::__DirectiveLocation::MUTATION },
		{ "SUBSCRIPTION", introspection::__DirectiveLocation::SUBSCRIPTION },
		{ "FIELD", introspection::__DirectiveLocation::FIELD },
		{ "FRAGMENT_DEFINITION", introspection::__DirectiveLocation::FRAGMENT_DEFINITION },
		{ "FRAGMENT_SPREAD", introspection::__DirectiveLocation::FRAGMENT_SPREAD },
		{ "INLINE_FRAGMENT", introspection::__DirectiveLocation::INLINE_FRAGMENT },
		{ "SCHEMA", introspection::__DirectiveLocation::SCHEMA },
		{ "SCALAR", introspection::__DirectiveLocation::SCALAR },
		{ "OBJECT", introspection::__DirectiveLocation::OBJECT },
		{ "FIELD_DEFINITION", introspection::__DirectiveLocation::FIELD_DEFINITION },
		{ "ARGUMENT_DEFINITION", introspection::__DirectiveLocation::ARGUMENT_DEFINITION },
		{ "INTERFACE", introspection::__DirectiveLocation::INTERFACE },
		{ "UNION", introspection::__DirectiveLocation::UNION },
		{ "ENUM", introspection::__DirectiveLocation::ENUM },
		{ "ENUM_VALUE", introspection::__DirectiveLocation::ENUM_VALUE },
		{ "INPUT_OBJECT", introspection::__DirectiveLocation::INPUT_OBJECT },
		{ "INPUT_FIELD_DEFINITION", introspection::__DirectiveLocation::INPUT_FIELD_DEFINITION }
	};

	auto itr = s_names.find(utility::conversions::to_utf8string(value.as_string()));

	if (itr == s_names.cend())
	{
		throw web::json::json_exception(_XPLATSTR("not a valid __DirectiveLocation value"));
	}

	return itr->second;
}

template <>
web::json::value service::ModifiedResult<introspection::__DirectiveLocation>::convert(const introspection::__DirectiveLocation& value, ResolverParams&&)
{
	static const std::string s_names[] = {
		"QUERY",
		"MUTATION",
		"SUBSCRIPTION",
		"FIELD",
		"FRAGMENT_DEFINITION",
		"FRAGMENT_SPREAD",
		"INLINE_FRAGMENT",
		"SCHEMA",
		"SCALAR",
		"OBJECT",
		"FIELD_DEFINITION",
		"ARGUMENT_DEFINITION",
		"INTERFACE",
		"UNION",
		"ENUM",
		"ENUM_VALUE",
		"INPUT_OBJECT",
		"INPUT_FIELD_DEFINITION"
	};

	return web::json::value::string(utility::conversions::to_string_t(s_names[static_cast<size_t>(value)]));
}

template <>
introspection::__TypeKind ModifiedArgument<introspection::__TypeKind>::convert(const web::json::value& value)
{
	static const std::unordered_map<std::string, introspection::__TypeKind> s_names = {
		{ "SCALAR", introspection::__TypeKind::SCALAR },
		{ "OBJECT", introspection::__TypeKind::OBJECT },
		{ "INTERFACE", introspection::__TypeKind::INTERFACE },
		{ "UNION", introspection::__TypeKind::UNION },
		{ "ENUM", introspection::__TypeKind::ENUM },
		{ "INPUT_OBJECT", introspection::__TypeKind::INPUT_OBJECT },
		{ "LIST", introspection::__TypeKind::LIST },
		{ "NON_NULL", introspection::__TypeKind::NON_NULL }
	};

	auto itr = s_names.find(utility::conversions::to_utf8string(value.as_string()));

	if (itr == s_names.cend())
	{
		throw web::json::json_exception(_XPLATSTR("not a valid __TypeKind value"));
	}

	return itr->second;
}

template <>
web::json::value service::ModifiedResult<introspection::__TypeKind>::convert(const introspection::__TypeKind& value, ResolverParams&&)
{
	static const std::string s_names[] = {
		"SCALAR",
		"OBJECT",
		"INTERFACE",
		"UNION",
		"ENUM",
		"INPUT_OBJECT",
		"LIST",
		"NON_NULL"
	};

	return web::json::value::string(utility::conversions::to_string_t(s_names[static_cast<size_t>(value)]));
}

} /* namespace service */

namespace introspection {
namespace object {

__Schema::__Schema()
	: service::Object({
		"__Schema"
	}, {
		{ "types", [this](service::ResolverParams&& params) { return resolveTypes(std::move(params)); } },
		{ "queryType", [this](service::ResolverParams&& params) { return resolveQueryType(std::move(params)); } },
		{ "mutationType", [this](service::ResolverParams&& params) { return resolveMutationType(std::move(params)); } },
		{ "subscriptionType", [this](service::ResolverParams&& params) { return resolveSubscriptionType(std::move(params)); } },
		{ "directives", [this](service::ResolverParams&& params) { return resolveDirectives(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

web::json::value __Schema::resolveTypes(service::ResolverParams&& params)
{
	auto result = getTypes();

	return service::ModifiedResult<__Type, service::TypeModifier::List>::convert(result, std::move(params));
}

web::json::value __Schema::resolveQueryType(service::ResolverParams&& params)
{
	auto result = getQueryType();

	return service::ModifiedResult<__Type>::convert(result, std::move(params));
}

web::json::value __Schema::resolveMutationType(service::ResolverParams&& params)
{
	auto result = getMutationType();

	return service::ModifiedResult<__Type, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __Schema::resolveSubscriptionType(service::ResolverParams&& params)
{
	auto result = getSubscriptionType();

	return service::ModifiedResult<__Type, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __Schema::resolveDirectives(service::ResolverParams&& params)
{
	auto result = getDirectives();

	return service::ModifiedResult<__Directive, service::TypeModifier::List>::convert(result, std::move(params));
}

web::json::value __Schema::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("__Schema"));
}

__Directive::__Directive()
	: service::Object({
		"__Directive"
	}, {
		{ "name", [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ "description", [this](service::ResolverParams&& params) { return resolveDescription(std::move(params)); } },
		{ "locations", [this](service::ResolverParams&& params) { return resolveLocations(std::move(params)); } },
		{ "args", [this](service::ResolverParams&& params) { return resolveArgs(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

web::json::value __Directive::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert(result, std::move(params));
}

web::json::value __Directive::resolveDescription(service::ResolverParams&& params)
{
	auto result = getDescription();

	return service::ModifiedResult<std::string, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __Directive::resolveLocations(service::ResolverParams&& params)
{
	auto result = getLocations();

	return service::ModifiedResult<__DirectiveLocation, service::TypeModifier::List>::convert(result, std::move(params));
}

web::json::value __Directive::resolveArgs(service::ResolverParams&& params)
{
	auto result = getArgs();

	return service::ModifiedResult<__InputValue, service::TypeModifier::List>::convert(result, std::move(params));
}

web::json::value __Directive::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("__Directive"));
}

__Type::__Type()
	: service::Object({
		"__Type"
	}, {
		{ "kind", [this](service::ResolverParams&& params) { return resolveKind(std::move(params)); } },
		{ "name", [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ "description", [this](service::ResolverParams&& params) { return resolveDescription(std::move(params)); } },
		{ "fields", [this](service::ResolverParams&& params) { return resolveFields(std::move(params)); } },
		{ "interfaces", [this](service::ResolverParams&& params) { return resolveInterfaces(std::move(params)); } },
		{ "possibleTypes", [this](service::ResolverParams&& params) { return resolvePossibleTypes(std::move(params)); } },
		{ "enumValues", [this](service::ResolverParams&& params) { return resolveEnumValues(std::move(params)); } },
		{ "inputFields", [this](service::ResolverParams&& params) { return resolveInputFields(std::move(params)); } },
		{ "ofType", [this](service::ResolverParams&& params) { return resolveOfType(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

web::json::value __Type::resolveKind(service::ResolverParams&& params)
{
	auto result = getKind();

	return service::ModifiedResult<__TypeKind>::convert(result, std::move(params));
}

web::json::value __Type::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __Type::resolveDescription(service::ResolverParams&& params)
{
	auto result = getDescription();

	return service::ModifiedResult<std::string, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __Type::resolveFields(service::ResolverParams&& params)
{
	static const auto defaultArguments = web::json::value::object({
		{ _XPLATSTR("includeDeprecated"), web::json::value::parse(_XPLATSTR(R"js(false)js")) }
	});

	auto pairIncludeDeprecated = service::ModifiedArgument<bool, service::TypeModifier::Nullable>::find("includeDeprecated", params.arguments);
	auto argIncludeDeprecated = (pairIncludeDeprecated.second
		? std::move(pairIncludeDeprecated.first)
		: service::ModifiedArgument<bool, service::TypeModifier::Nullable>::require("includeDeprecated", defaultArguments.as_object()));
	auto result = getFields(std::move(argIncludeDeprecated));

	return service::ModifiedResult<__Field, service::TypeModifier::Nullable, service::TypeModifier::List>::convert(result, std::move(params));
}

web::json::value __Type::resolveInterfaces(service::ResolverParams&& params)
{
	auto result = getInterfaces();

	return service::ModifiedResult<__Type, service::TypeModifier::Nullable, service::TypeModifier::List>::convert(result, std::move(params));
}

web::json::value __Type::resolvePossibleTypes(service::ResolverParams&& params)
{
	auto result = getPossibleTypes();

	return service::ModifiedResult<__Type, service::TypeModifier::Nullable, service::TypeModifier::List>::convert(result, std::move(params));
}

web::json::value __Type::resolveEnumValues(service::ResolverParams&& params)
{
	static const auto defaultArguments = web::json::value::object({
		{ _XPLATSTR("includeDeprecated"), web::json::value::parse(_XPLATSTR(R"js(false)js")) }
	});

	auto pairIncludeDeprecated = service::ModifiedArgument<bool, service::TypeModifier::Nullable>::find("includeDeprecated", params.arguments);
	auto argIncludeDeprecated = (pairIncludeDeprecated.second
		? std::move(pairIncludeDeprecated.first)
		: service::ModifiedArgument<bool, service::TypeModifier::Nullable>::require("includeDeprecated", defaultArguments.as_object()));
	auto result = getEnumValues(std::move(argIncludeDeprecated));

	return service::ModifiedResult<__EnumValue, service::TypeModifier::Nullable, service::TypeModifier::List>::convert(result, std::move(params));
}

web::json::value __Type::resolveInputFields(service::ResolverParams&& params)
{
	auto result = getInputFields();

	return service::ModifiedResult<__InputValue, service::TypeModifier::Nullable, service::TypeModifier::List>::convert(result, std::move(params));
}

web::json::value __Type::resolveOfType(service::ResolverParams&& params)
{
	auto result = getOfType();

	return service::ModifiedResult<__Type, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __Type::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("__Type"));
}

__Field::__Field()
	: service::Object({
		"__Field"
	}, {
		{ "name", [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ "description", [this](service::ResolverParams&& params) { return resolveDescription(std::move(params)); } },
		{ "args", [this](service::ResolverParams&& params) { return resolveArgs(std::move(params)); } },
		{ "type", [this](service::ResolverParams&& params) { return resolveType(std::move(params)); } },
		{ "isDeprecated", [this](service::ResolverParams&& params) { return resolveIsDeprecated(std::move(params)); } },
		{ "deprecationReason", [this](service::ResolverParams&& params) { return resolveDeprecationReason(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

web::json::value __Field::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert(result, std::move(params));
}

web::json::value __Field::resolveDescription(service::ResolverParams&& params)
{
	auto result = getDescription();

	return service::ModifiedResult<std::string, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __Field::resolveArgs(service::ResolverParams&& params)
{
	auto result = getArgs();

	return service::ModifiedResult<__InputValue, service::TypeModifier::List>::convert(result, std::move(params));
}

web::json::value __Field::resolveType(service::ResolverParams&& params)
{
	auto result = getType();

	return service::ModifiedResult<__Type>::convert(result, std::move(params));
}

web::json::value __Field::resolveIsDeprecated(service::ResolverParams&& params)
{
	auto result = getIsDeprecated();

	return service::ModifiedResult<bool>::convert(result, std::move(params));
}

web::json::value __Field::resolveDeprecationReason(service::ResolverParams&& params)
{
	auto result = getDeprecationReason();

	return service::ModifiedResult<std::string, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __Field::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("__Field"));
}

__InputValue::__InputValue()
	: service::Object({
		"__InputValue"
	}, {
		{ "name", [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ "description", [this](service::ResolverParams&& params) { return resolveDescription(std::move(params)); } },
		{ "type", [this](service::ResolverParams&& params) { return resolveType(std::move(params)); } },
		{ "defaultValue", [this](service::ResolverParams&& params) { return resolveDefaultValue(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

web::json::value __InputValue::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert(result, std::move(params));
}

web::json::value __InputValue::resolveDescription(service::ResolverParams&& params)
{
	auto result = getDescription();

	return service::ModifiedResult<std::string, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __InputValue::resolveType(service::ResolverParams&& params)
{
	auto result = getType();

	return service::ModifiedResult<__Type>::convert(result, std::move(params));
}

web::json::value __InputValue::resolveDefaultValue(service::ResolverParams&& params)
{
	auto result = getDefaultValue();

	return service::ModifiedResult<std::string, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __InputValue::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("__InputValue"));
}

__EnumValue::__EnumValue()
	: service::Object({
		"__EnumValue"
	}, {
		{ "name", [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ "description", [this](service::ResolverParams&& params) { return resolveDescription(std::move(params)); } },
		{ "isDeprecated", [this](service::ResolverParams&& params) { return resolveIsDeprecated(std::move(params)); } },
		{ "deprecationReason", [this](service::ResolverParams&& params) { return resolveDeprecationReason(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

web::json::value __EnumValue::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert(result, std::move(params));
}

web::json::value __EnumValue::resolveDescription(service::ResolverParams&& params)
{
	auto result = getDescription();

	return service::ModifiedResult<std::string, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __EnumValue::resolveIsDeprecated(service::ResolverParams&& params)
{
	auto result = getIsDeprecated();

	return service::ModifiedResult<bool>::convert(result, std::move(params));
}

web::json::value __EnumValue::resolveDeprecationReason(service::ResolverParams&& params)
{
	auto result = getDeprecationReason();

	return service::ModifiedResult<std::string, service::TypeModifier::Nullable>::convert(result, std::move(params));
}

web::json::value __EnumValue::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("__EnumValue"));
}

} /* namespace object */

void AddTypesToSchema(std::shared_ptr<introspection::Schema> schema)
{
	auto type__DirectiveLocation= std::make_shared<introspection::EnumType>("__DirectiveLocation");
	schema->AddType("__DirectiveLocation", type__DirectiveLocation);
	auto type__TypeKind= std::make_shared<introspection::EnumType>("__TypeKind");
	schema->AddType("__TypeKind", type__TypeKind);
	auto type__Schema= std::make_shared<introspection::ObjectType>("__Schema");
	schema->AddType("__Schema", type__Schema);
	auto type__Directive= std::make_shared<introspection::ObjectType>("__Directive");
	schema->AddType("__Directive", type__Directive);
	auto type__Type= std::make_shared<introspection::ObjectType>("__Type");
	schema->AddType("__Type", type__Type);
	auto type__Field= std::make_shared<introspection::ObjectType>("__Field");
	schema->AddType("__Field", type__Field);
	auto type__InputValue= std::make_shared<introspection::ObjectType>("__InputValue");
	schema->AddType("__InputValue", type__InputValue);
	auto type__EnumValue= std::make_shared<introspection::ObjectType>("__EnumValue");
	schema->AddType("__EnumValue", type__EnumValue);

	type__DirectiveLocation->AddEnumValues({
		"QUERY",
		"MUTATION",
		"SUBSCRIPTION",
		"FIELD",
		"FRAGMENT_DEFINITION",
		"FRAGMENT_SPREAD",
		"INLINE_FRAGMENT",
		"SCHEMA",
		"SCALAR",
		"OBJECT",
		"FIELD_DEFINITION",
		"ARGUMENT_DEFINITION",
		"INTERFACE",
		"UNION",
		"ENUM",
		"ENUM_VALUE",
		"INPUT_OBJECT",
		"INPUT_FIELD_DEFINITION"
	});
	type__TypeKind->AddEnumValues({
		"SCALAR",
		"OBJECT",
		"INTERFACE",
		"UNION",
		"ENUM",
		"INPUT_OBJECT",
		"LIST",
		"NON_NULL"
	});

	type__Schema->AddFields({
		std::make_shared<introspection::Field>("types", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))))),
		std::make_shared<introspection::Field>("queryType", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))),
		std::make_shared<introspection::Field>("mutationType", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("__Type")),
		std::make_shared<introspection::Field>("subscriptionType", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("__Type")),
		std::make_shared<introspection::Field>("directives", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Directive")))))
	});
	type__Directive->AddFields({
		std::make_shared<introspection::Field>("name", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("locations", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__DirectiveLocation"))))),
		std::make_shared<introspection::Field>("args", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__InputValue")))))
	});
	type__Type->AddFields({
		std::make_shared<introspection::Field>("kind", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__TypeKind"))),
		std::make_shared<introspection::Field>("name", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("description", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("fields", std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("includeDeprecated", schema->LookupType("Boolean"), web::json::value::parse(_XPLATSTR(R"js(false)js")))
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Field"))))),
		std::make_shared<introspection::Field>("interfaces", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))))),
		std::make_shared<introspection::Field>("possibleTypes", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))))),
		std::make_shared<introspection::Field>("enumValues", std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("includeDeprecated", schema->LookupType("Boolean"), web::json::value::parse(_XPLATSTR(R"js(false)js")))
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__EnumValue"))))),
		std::make_shared<introspection::Field>("inputFields", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__InputValue"))))),
		std::make_shared<introspection::Field>("ofType", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("__Type"))
	});
	type__Field->AddFields({
		std::make_shared<introspection::Field>("name", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("args", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__InputValue"))))),
		std::make_shared<introspection::Field>("type", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))),
		std::make_shared<introspection::Field>("isDeprecated", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("deprecationReason", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	type__InputValue->AddFields({
		std::make_shared<introspection::Field>("name", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("type", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))),
		std::make_shared<introspection::Field>("defaultValue", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	type__EnumValue->AddFields({
		std::make_shared<introspection::Field>("name", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isDeprecated", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("deprecationReason", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
}

} /* namespace introspection */
} /* namespace graphql */
} /* namespace facebook */