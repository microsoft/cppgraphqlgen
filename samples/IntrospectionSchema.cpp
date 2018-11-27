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
introspection::__TypeKind ModifiedArgument<introspection::__TypeKind>::convert(const rapidjson::Value& value)
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

	auto itr = s_names.find(value.GetString());

	if (itr == s_names.cend())
	{
		throw service::schema_exception({ "not a valid __TypeKind value" });
	}

	return itr->second;
}

template <>
rapidjson::Document service::ModifiedResult<introspection::__TypeKind>::convert(introspection::__TypeKind&& value, ResolverParams&&)
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

	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef(s_names[static_cast<size_t>(value)].c_str()));

	return result;
}

template <>
introspection::__DirectiveLocation ModifiedArgument<introspection::__DirectiveLocation>::convert(const rapidjson::Value& value)
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

	auto itr = s_names.find(value.GetString());

	if (itr == s_names.cend())
	{
		throw service::schema_exception({ "not a valid __DirectiveLocation value" });
	}

	return itr->second;
}

template <>
rapidjson::Document service::ModifiedResult<introspection::__DirectiveLocation>::convert(introspection::__DirectiveLocation&& value, ResolverParams&&)
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

	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef(s_names[static_cast<size_t>(value)].c_str()));

	return result;
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

rapidjson::Document __Schema::resolveTypes(service::ResolverParams&& params)
{
	auto result = getTypes();

	return service::ModifiedResult<__Type>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

rapidjson::Document __Schema::resolveQueryType(service::ResolverParams&& params)
{
	auto result = getQueryType();

	return service::ModifiedResult<__Type>::convert(std::move(result), std::move(params));
}

rapidjson::Document __Schema::resolveMutationType(service::ResolverParams&& params)
{
	auto result = getMutationType();

	return service::ModifiedResult<__Type>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __Schema::resolveSubscriptionType(service::ResolverParams&& params)
{
	auto result = getSubscriptionType();

	return service::ModifiedResult<__Type>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __Schema::resolveDirectives(service::ResolverParams&& params)
{
	auto result = getDirectives();

	return service::ModifiedResult<__Directive>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

rapidjson::Document __Schema::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("__Schema"));

	return result;
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

rapidjson::Document __Type::resolveKind(service::ResolverParams&& params)
{
	auto result = getKind();

	return service::ModifiedResult<__TypeKind>::convert(std::move(result), std::move(params));
}

rapidjson::Document __Type::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __Type::resolveDescription(service::ResolverParams&& params)
{
	auto result = getDescription();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __Type::resolveFields(service::ResolverParams&& params)
{
	const auto defaultArguments = []()
	{
		rapidjson::Document values(rapidjson::Type::kObjectType);
		auto& allocator = values.GetAllocator();
		rapidjson::Document parsed;
		rapidjson::Value entry;

		parsed.Parse(R"js(false)js");
		entry.CopyFrom(parsed, allocator);
		values.AddMember(rapidjson::StringRef("includeDeprecated"), entry, allocator);

		return values;
	}();

	auto pairIncludeDeprecated = service::ModifiedArgument<bool>::find<service::TypeModifier::Nullable>("includeDeprecated", params.arguments);
	auto argIncludeDeprecated = (pairIncludeDeprecated.second
		? std::move(pairIncludeDeprecated.first)
		: service::ModifiedArgument<bool>::require<service::TypeModifier::Nullable>("includeDeprecated", defaultArguments.GetObject()));
	auto result = getFields(std::move(argIncludeDeprecated));

	return service::ModifiedResult<__Field>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

rapidjson::Document __Type::resolveInterfaces(service::ResolverParams&& params)
{
	auto result = getInterfaces();

	return service::ModifiedResult<__Type>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

rapidjson::Document __Type::resolvePossibleTypes(service::ResolverParams&& params)
{
	auto result = getPossibleTypes();

	return service::ModifiedResult<__Type>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

rapidjson::Document __Type::resolveEnumValues(service::ResolverParams&& params)
{
	const auto defaultArguments = []()
	{
		rapidjson::Document values(rapidjson::Type::kObjectType);
		auto& allocator = values.GetAllocator();
		rapidjson::Document parsed;
		rapidjson::Value entry;

		parsed.Parse(R"js(false)js");
		entry.CopyFrom(parsed, allocator);
		values.AddMember(rapidjson::StringRef("includeDeprecated"), entry, allocator);

		return values;
	}();

	auto pairIncludeDeprecated = service::ModifiedArgument<bool>::find<service::TypeModifier::Nullable>("includeDeprecated", params.arguments);
	auto argIncludeDeprecated = (pairIncludeDeprecated.second
		? std::move(pairIncludeDeprecated.first)
		: service::ModifiedArgument<bool>::require<service::TypeModifier::Nullable>("includeDeprecated", defaultArguments.GetObject()));
	auto result = getEnumValues(std::move(argIncludeDeprecated));

	return service::ModifiedResult<__EnumValue>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

rapidjson::Document __Type::resolveInputFields(service::ResolverParams&& params)
{
	auto result = getInputFields();

	return service::ModifiedResult<__InputValue>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

rapidjson::Document __Type::resolveOfType(service::ResolverParams&& params)
{
	auto result = getOfType();

	return service::ModifiedResult<__Type>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __Type::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("__Type"));

	return result;
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

rapidjson::Document __Field::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert(std::move(result), std::move(params));
}

rapidjson::Document __Field::resolveDescription(service::ResolverParams&& params)
{
	auto result = getDescription();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __Field::resolveArgs(service::ResolverParams&& params)
{
	auto result = getArgs();

	return service::ModifiedResult<__InputValue>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

rapidjson::Document __Field::resolveType(service::ResolverParams&& params)
{
	auto result = getType();

	return service::ModifiedResult<__Type>::convert(std::move(result), std::move(params));
}

rapidjson::Document __Field::resolveIsDeprecated(service::ResolverParams&& params)
{
	auto result = getIsDeprecated();

	return service::ModifiedResult<bool>::convert(std::move(result), std::move(params));
}

rapidjson::Document __Field::resolveDeprecationReason(service::ResolverParams&& params)
{
	auto result = getDeprecationReason();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __Field::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("__Field"));

	return result;
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

rapidjson::Document __InputValue::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert(std::move(result), std::move(params));
}

rapidjson::Document __InputValue::resolveDescription(service::ResolverParams&& params)
{
	auto result = getDescription();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __InputValue::resolveType(service::ResolverParams&& params)
{
	auto result = getType();

	return service::ModifiedResult<__Type>::convert(std::move(result), std::move(params));
}

rapidjson::Document __InputValue::resolveDefaultValue(service::ResolverParams&& params)
{
	auto result = getDefaultValue();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __InputValue::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("__InputValue"));

	return result;
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

rapidjson::Document __EnumValue::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert(std::move(result), std::move(params));
}

rapidjson::Document __EnumValue::resolveDescription(service::ResolverParams&& params)
{
	auto result = getDescription();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __EnumValue::resolveIsDeprecated(service::ResolverParams&& params)
{
	auto result = getIsDeprecated();

	return service::ModifiedResult<bool>::convert(std::move(result), std::move(params));
}

rapidjson::Document __EnumValue::resolveDeprecationReason(service::ResolverParams&& params)
{
	auto result = getDeprecationReason();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __EnumValue::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("__EnumValue"));

	return result;
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

rapidjson::Document __Directive::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert(std::move(result), std::move(params));
}

rapidjson::Document __Directive::resolveDescription(service::ResolverParams&& params)
{
	auto result = getDescription();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document __Directive::resolveLocations(service::ResolverParams&& params)
{
	auto result = getLocations();

	return service::ModifiedResult<__DirectiveLocation>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

rapidjson::Document __Directive::resolveArgs(service::ResolverParams&& params)
{
	auto result = getArgs();

	return service::ModifiedResult<__InputValue>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

rapidjson::Document __Directive::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("__Directive"));

	return result;
}

} /* namespace object */

void AddTypesToSchema(std::shared_ptr<introspection::Schema> schema)
{
	auto type__TypeKind= std::make_shared<introspection::EnumType>("__TypeKind", R"md()md");
	schema->AddType("__TypeKind", type__TypeKind);
	auto type__DirectiveLocation= std::make_shared<introspection::EnumType>("__DirectiveLocation", R"md()md");
	schema->AddType("__DirectiveLocation", type__DirectiveLocation);
	auto type__Schema= std::make_shared<introspection::ObjectType>("__Schema", R"md()md");
	schema->AddType("__Schema", type__Schema);
	auto type__Type= std::make_shared<introspection::ObjectType>("__Type", R"md()md");
	schema->AddType("__Type", type__Type);
	auto type__Field= std::make_shared<introspection::ObjectType>("__Field", R"md()md");
	schema->AddType("__Field", type__Field);
	auto type__InputValue= std::make_shared<introspection::ObjectType>("__InputValue", R"md()md");
	schema->AddType("__InputValue", type__InputValue);
	auto type__EnumValue= std::make_shared<introspection::ObjectType>("__EnumValue", R"md()md");
	schema->AddType("__EnumValue", type__EnumValue);
	auto type__Directive= std::make_shared<introspection::ObjectType>("__Directive", R"md()md");
	schema->AddType("__Directive", type__Directive);

	type__TypeKind->AddEnumValues({
		{ "SCALAR", R"md()md", nullptr },
		{ "OBJECT", R"md()md", nullptr },
		{ "INTERFACE", R"md()md", nullptr },
		{ "UNION", R"md()md", nullptr },
		{ "ENUM", R"md()md", nullptr },
		{ "INPUT_OBJECT", R"md()md", nullptr },
		{ "LIST", R"md()md", nullptr },
		{ "NON_NULL", R"md()md", nullptr }
	});
	type__DirectiveLocation->AddEnumValues({
		{ "QUERY", R"md()md", nullptr },
		{ "MUTATION", R"md()md", nullptr },
		{ "SUBSCRIPTION", R"md()md", nullptr },
		{ "FIELD", R"md()md", nullptr },
		{ "FRAGMENT_DEFINITION", R"md()md", nullptr },
		{ "FRAGMENT_SPREAD", R"md()md", nullptr },
		{ "INLINE_FRAGMENT", R"md()md", nullptr },
		{ "SCHEMA", R"md()md", nullptr },
		{ "SCALAR", R"md()md", nullptr },
		{ "OBJECT", R"md()md", nullptr },
		{ "FIELD_DEFINITION", R"md()md", nullptr },
		{ "ARGUMENT_DEFINITION", R"md()md", nullptr },
		{ "INTERFACE", R"md()md", nullptr },
		{ "UNION", R"md()md", nullptr },
		{ "ENUM", R"md()md", nullptr },
		{ "ENUM_VALUE", R"md()md", nullptr },
		{ "INPUT_OBJECT", R"md()md", nullptr },
		{ "INPUT_FIELD_DEFINITION", R"md()md", nullptr }
	});

	type__Schema->AddFields({
		std::make_shared<introspection::Field>("types", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))))),
		std::make_shared<introspection::Field>("queryType", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))),
		std::make_shared<introspection::Field>("mutationType", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("__Type")),
		std::make_shared<introspection::Field>("subscriptionType", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("__Type")),
		std::make_shared<introspection::Field>("directives", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Directive")))))
	});
	rapidjson::Document default__TypefieldsincludeDeprecated;
	default__TypefieldsincludeDeprecated.Parse(R"js(false)js");
	rapidjson::Document default__TypeenumValuesincludeDeprecated;
	default__TypeenumValuesincludeDeprecated.Parse(R"js(false)js");
	type__Type->AddFields({
		std::make_shared<introspection::Field>("kind", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__TypeKind"))),
		std::make_shared<introspection::Field>("name", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("description", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("fields", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("includeDeprecated", R"md()md", schema->LookupType("Boolean"), default__TypefieldsincludeDeprecated)
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Field"))))),
		std::make_shared<introspection::Field>("interfaces", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))))),
		std::make_shared<introspection::Field>("possibleTypes", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))))),
		std::make_shared<introspection::Field>("enumValues", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("includeDeprecated", R"md()md", schema->LookupType("Boolean"), default__TypeenumValuesincludeDeprecated)
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__EnumValue"))))),
		std::make_shared<introspection::Field>("inputFields", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__InputValue"))))),
		std::make_shared<introspection::Field>("ofType", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("__Type"))
	});
	type__Field->AddFields({
		std::make_shared<introspection::Field>("name", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("args", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__InputValue"))))),
		std::make_shared<introspection::Field>("type", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))),
		std::make_shared<introspection::Field>("isDeprecated", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("deprecationReason", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	type__InputValue->AddFields({
		std::make_shared<introspection::Field>("name", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("type", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__Type"))),
		std::make_shared<introspection::Field>("defaultValue", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	type__EnumValue->AddFields({
		std::make_shared<introspection::Field>("name", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isDeprecated", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("deprecationReason", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	type__Directive->AddFields({
		std::make_shared<introspection::Field>("name", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("locations", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__DirectiveLocation"))))),
		std::make_shared<introspection::Field>("args", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("__InputValue")))))
	});
}

} /* namespace introspection */
} /* namespace graphql */
} /* namespace facebook */