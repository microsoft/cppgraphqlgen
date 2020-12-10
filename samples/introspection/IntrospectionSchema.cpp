// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/Introspection.h"
#include "graphqlservice/GraphQLValidation.h"

#include <algorithm>
#include <array>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

using namespace std::literals;

namespace graphql {
namespace service {

static const std::array<std::string_view, 8> s_namesTypeKind = {
	"SCALAR",
	"OBJECT",
	"INTERFACE",
	"UNION",
	"ENUM",
	"INPUT_OBJECT",
	"LIST",
	"NON_NULL"
};

template <>
introspection::TypeKind ModifiedArgument<introspection::TypeKind>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { "not a valid __TypeKind value" } };
	}

	auto itr = std::find(s_namesTypeKind.cbegin(), s_namesTypeKind.cend(), value.get<response::StringType>());

	if (itr == s_namesTypeKind.cend())
	{
		throw service::schema_exception { { "not a valid __TypeKind value" } };
	}

	return static_cast<introspection::TypeKind>(itr - s_namesTypeKind.cbegin());
}

template <>
std::future<response::Value> ModifiedResult<introspection::TypeKind>::convert(service::FieldResult<introspection::TypeKind>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[](introspection::TypeKind&& value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<response::StringType>(std::string(s_namesTypeKind[static_cast<size_t>(value)]));

			return result;
		});
}

static const std::array<std::string_view, 18> s_namesDirectiveLocation = {
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

template <>
introspection::DirectiveLocation ModifiedArgument<introspection::DirectiveLocation>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { "not a valid __DirectiveLocation value" } };
	}

	auto itr = std::find(s_namesDirectiveLocation.cbegin(), s_namesDirectiveLocation.cend(), value.get<response::StringType>());

	if (itr == s_namesDirectiveLocation.cend())
	{
		throw service::schema_exception { { "not a valid __DirectiveLocation value" } };
	}

	return static_cast<introspection::DirectiveLocation>(itr - s_namesDirectiveLocation.cbegin());
}

template <>
std::future<response::Value> ModifiedResult<introspection::DirectiveLocation>::convert(service::FieldResult<introspection::DirectiveLocation>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[](introspection::DirectiveLocation&& value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<response::StringType>(std::string(s_namesDirectiveLocation[static_cast<size_t>(value)]));

			return result;
		});
}

} /* namespace service */

namespace introspection {
namespace object {

Schema::Schema()
	: service::Object({
		"__Schema"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(directives)gql"sv, [this](service::ResolverParams&& params) { return resolveDirectives(std::move(params)); } },
		{ R"gql(mutationType)gql"sv, [this](service::ResolverParams&& params) { return resolveMutationType(std::move(params)); } },
		{ R"gql(queryType)gql"sv, [this](service::ResolverParams&& params) { return resolveQueryType(std::move(params)); } },
		{ R"gql(subscriptionType)gql"sv, [this](service::ResolverParams&& params) { return resolveSubscriptionType(std::move(params)); } },
		{ R"gql(types)gql"sv, [this](service::ResolverParams&& params) { return resolveTypes(std::move(params)); } }
	})
{
}

std::future<response::Value> Schema::resolveTypes(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getTypes(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Schema::resolveQueryType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getQueryType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Schema::resolveMutationType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getMutationType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Schema::resolveSubscriptionType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getSubscriptionType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Schema::resolveDirectives(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDirectives(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Directive>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Schema::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(__Schema)gql" }, std::move(params));
}

Type::Type()
	: service::Object({
		"__Type"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(description)gql"sv, [this](service::ResolverParams&& params) { return resolveDescription(std::move(params)); } },
		{ R"gql(enumValues)gql"sv, [this](service::ResolverParams&& params) { return resolveEnumValues(std::move(params)); } },
		{ R"gql(fields)gql"sv, [this](service::ResolverParams&& params) { return resolveFields(std::move(params)); } },
		{ R"gql(inputFields)gql"sv, [this](service::ResolverParams&& params) { return resolveInputFields(std::move(params)); } },
		{ R"gql(interfaces)gql"sv, [this](service::ResolverParams&& params) { return resolveInterfaces(std::move(params)); } },
		{ R"gql(kind)gql"sv, [this](service::ResolverParams&& params) { return resolveKind(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(ofType)gql"sv, [this](service::ResolverParams&& params) { return resolveOfType(std::move(params)); } },
		{ R"gql(possibleTypes)gql"sv, [this](service::ResolverParams&& params) { return resolvePossibleTypes(std::move(params)); } }
	})
{
}

std::future<response::Value> Type::resolveKind(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getKind(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<TypeKind>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Type::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Type::resolveDescription(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDescription(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Type::resolveFields(service::ResolverParams&& params)
{
	const auto defaultArguments = []()
	{
		response::Value values(response::Type::Map);
		response::Value entry;

		entry = response::Value(false);
		values.emplace_back("includeDeprecated", std::move(entry));

		return values;
	}();

	auto pairIncludeDeprecated = service::ModifiedArgument<response::BooleanType>::find<service::TypeModifier::Nullable>("includeDeprecated", params.arguments);
	auto argIncludeDeprecated = (pairIncludeDeprecated.second
		? std::move(pairIncludeDeprecated.first)
		: service::ModifiedArgument<response::BooleanType>::require<service::TypeModifier::Nullable>("includeDeprecated", defaultArguments));
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getFields(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIncludeDeprecated));
	resolverLock.unlock();

	return service::ModifiedResult<Field>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Type::resolveInterfaces(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getInterfaces(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Type::resolvePossibleTypes(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getPossibleTypes(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Type::resolveEnumValues(service::ResolverParams&& params)
{
	const auto defaultArguments = []()
	{
		response::Value values(response::Type::Map);
		response::Value entry;

		entry = response::Value(false);
		values.emplace_back("includeDeprecated", std::move(entry));

		return values;
	}();

	auto pairIncludeDeprecated = service::ModifiedArgument<response::BooleanType>::find<service::TypeModifier::Nullable>("includeDeprecated", params.arguments);
	auto argIncludeDeprecated = (pairIncludeDeprecated.second
		? std::move(pairIncludeDeprecated.first)
		: service::ModifiedArgument<response::BooleanType>::require<service::TypeModifier::Nullable>("includeDeprecated", defaultArguments));
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getEnumValues(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIncludeDeprecated));
	resolverLock.unlock();

	return service::ModifiedResult<EnumValue>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Type::resolveInputFields(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getInputFields(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<InputValue>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Type::resolveOfType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getOfType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Type::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(__Type)gql" }, std::move(params));
}

Field::Field()
	: service::Object({
		"__Field"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(args)gql"sv, [this](service::ResolverParams&& params) { return resolveArgs(std::move(params)); } },
		{ R"gql(deprecationReason)gql"sv, [this](service::ResolverParams&& params) { return resolveDeprecationReason(std::move(params)); } },
		{ R"gql(description)gql"sv, [this](service::ResolverParams&& params) { return resolveDescription(std::move(params)); } },
		{ R"gql(isDeprecated)gql"sv, [this](service::ResolverParams&& params) { return resolveIsDeprecated(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(type)gql"sv, [this](service::ResolverParams&& params) { return resolveType(std::move(params)); } }
	})
{
}

std::future<response::Value> Field::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Field::resolveDescription(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDescription(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Field::resolveArgs(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getArgs(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<InputValue>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Field::resolveType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Field::resolveIsDeprecated(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getIsDeprecated(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Field::resolveDeprecationReason(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDeprecationReason(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Field::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(__Field)gql" }, std::move(params));
}

InputValue::InputValue()
	: service::Object({
		"__InputValue"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(defaultValue)gql"sv, [this](service::ResolverParams&& params) { return resolveDefaultValue(std::move(params)); } },
		{ R"gql(description)gql"sv, [this](service::ResolverParams&& params) { return resolveDescription(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(type)gql"sv, [this](service::ResolverParams&& params) { return resolveType(std::move(params)); } }
	})
{
}

std::future<response::Value> InputValue::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> InputValue::resolveDescription(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDescription(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> InputValue::resolveType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert(std::move(result), std::move(params));
}

std::future<response::Value> InputValue::resolveDefaultValue(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDefaultValue(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> InputValue::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(__InputValue)gql" }, std::move(params));
}

EnumValue::EnumValue()
	: service::Object({
		"__EnumValue"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(deprecationReason)gql"sv, [this](service::ResolverParams&& params) { return resolveDeprecationReason(std::move(params)); } },
		{ R"gql(description)gql"sv, [this](service::ResolverParams&& params) { return resolveDescription(std::move(params)); } },
		{ R"gql(isDeprecated)gql"sv, [this](service::ResolverParams&& params) { return resolveIsDeprecated(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } }
	})
{
}

std::future<response::Value> EnumValue::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> EnumValue::resolveDescription(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDescription(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> EnumValue::resolveIsDeprecated(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getIsDeprecated(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> EnumValue::resolveDeprecationReason(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDeprecationReason(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> EnumValue::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(__EnumValue)gql" }, std::move(params));
}

Directive::Directive()
	: service::Object({
		"__Directive"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(args)gql"sv, [this](service::ResolverParams&& params) { return resolveArgs(std::move(params)); } },
		{ R"gql(description)gql"sv, [this](service::ResolverParams&& params) { return resolveDescription(std::move(params)); } },
		{ R"gql(locations)gql"sv, [this](service::ResolverParams&& params) { return resolveLocations(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } }
	})
{
}

std::future<response::Value> Directive::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Directive::resolveDescription(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDescription(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Directive::resolveLocations(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getLocations(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<DirectiveLocation>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Directive::resolveArgs(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getArgs(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<InputValue>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Directive::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(__Directive)gql" }, std::move(params));
}

} /* namespace object */

void AddTypesToSchema(const std::shared_ptr<introspection::Schema>& schema)
{
	schema->AddType("Boolean", std::make_shared<introspection::ScalarType>("Boolean", R"md(Built-in type)md"));
	schema->AddType("Float", std::make_shared<introspection::ScalarType>("Float", R"md(Built-in type)md"));
	schema->AddType("ID", std::make_shared<introspection::ScalarType>("ID", R"md(Built-in type)md"));
	schema->AddType("Int", std::make_shared<introspection::ScalarType>("Int", R"md(Built-in type)md"));
	schema->AddType("String", std::make_shared<introspection::ScalarType>("String", R"md(Built-in type)md"));
	auto typeTypeKind = std::make_shared<introspection::EnumType>("__TypeKind", R"md()md");
	schema->AddType("__TypeKind", typeTypeKind);
	auto typeDirectiveLocation = std::make_shared<introspection::EnumType>("__DirectiveLocation", R"md()md");
	schema->AddType("__DirectiveLocation", typeDirectiveLocation);
	auto typeSchema = std::make_shared<introspection::ObjectType>("__Schema", R"md()md");
	schema->AddType("__Schema", typeSchema);
	auto typeType = std::make_shared<introspection::ObjectType>("__Type", R"md()md");
	schema->AddType("__Type", typeType);
	auto typeField = std::make_shared<introspection::ObjectType>("__Field", R"md()md");
	schema->AddType("__Field", typeField);
	auto typeInputValue = std::make_shared<introspection::ObjectType>("__InputValue", R"md()md");
	schema->AddType("__InputValue", typeInputValue);
	auto typeEnumValue = std::make_shared<introspection::ObjectType>("__EnumValue", R"md()md");
	schema->AddType("__EnumValue", typeEnumValue);
	auto typeDirective = std::make_shared<introspection::ObjectType>("__Directive", R"md()md");
	schema->AddType("__Directive", typeDirective);

	typeTypeKind->AddEnumValues({
		{ std::string{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::SCALAR)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::OBJECT)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::INTERFACE)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::UNION)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::ENUM)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::INPUT_OBJECT)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::LIST)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::NON_NULL)] }, R"md()md", std::nullopt }
	});
	typeDirectiveLocation->AddEnumValues({
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::QUERY)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::MUTATION)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::SUBSCRIPTION)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::FIELD)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::FRAGMENT_DEFINITION)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::FRAGMENT_SPREAD)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::INLINE_FRAGMENT)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::SCHEMA)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::SCALAR)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::OBJECT)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::FIELD_DEFINITION)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::ARGUMENT_DEFINITION)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::INTERFACE)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::UNION)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::ENUM)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::ENUM_VALUE)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::INPUT_OBJECT)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::INPUT_FIELD_DEFINITION)] }, R"md()md", std::nullopt }
	});

	typeSchema->AddFields({
		std::make_shared<introspection::Field>("types", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type"))))),
		std::make_shared<introspection::Field>("queryType", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type"))),
		std::make_shared<introspection::Field>("mutationType", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("__Type")),
		std::make_shared<introspection::Field>("subscriptionType", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("__Type")),
		std::make_shared<introspection::Field>("directives", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Directive")))))
	});
	typeType->AddFields({
		std::make_shared<introspection::Field>("kind", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__TypeKind"))),
		std::make_shared<introspection::Field>("name", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("description", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("fields", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("includeDeprecated", R"md()md", schema->LookupType("Boolean"), R"gql(false)gql")
		}), schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Field")))),
		std::make_shared<introspection::Field>("interfaces", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type")))),
		std::make_shared<introspection::Field>("possibleTypes", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type")))),
		std::make_shared<introspection::Field>("enumValues", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("includeDeprecated", R"md()md", schema->LookupType("Boolean"), R"gql(false)gql")
		}), schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__EnumValue")))),
		std::make_shared<introspection::Field>("inputFields", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__InputValue")))),
		std::make_shared<introspection::Field>("ofType", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("__Type"))
	});
	typeField->AddFields({
		std::make_shared<introspection::Field>("name", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("args", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__InputValue"))))),
		std::make_shared<introspection::Field>("type", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type"))),
		std::make_shared<introspection::Field>("isDeprecated", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("deprecationReason", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	typeInputValue->AddFields({
		std::make_shared<introspection::Field>("name", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("type", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type"))),
		std::make_shared<introspection::Field>("defaultValue", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	typeEnumValue->AddFields({
		std::make_shared<introspection::Field>("name", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isDeprecated", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("deprecationReason", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	typeDirective->AddFields({
		std::make_shared<introspection::Field>("name", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("description", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("locations", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__DirectiveLocation"))))),
		std::make_shared<introspection::Field>("args", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__InputValue")))))
	});

	schema->AddDirective(std::make_shared<introspection::Directive>("skip", R"md()md", std::vector<response::StringType>({
		R"gql(FIELD)gql",
		R"gql(FRAGMENT_SPREAD)gql",
		R"gql(INLINE_FRAGMENT)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("if", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("include", R"md()md", std::vector<response::StringType>({
		R"gql(FIELD)gql",
		R"gql(FRAGMENT_SPREAD)gql",
		R"gql(INLINE_FRAGMENT)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("if", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("deprecated", R"md()md", std::vector<response::StringType>({
		R"gql(FIELD_DEFINITION)gql",
		R"gql(ENUM_VALUE)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("reason", R"md()md", schema->LookupType("String"), R"gql("No longer supported")gql")
	})));
}

} /* namespace introspection */
} /* namespace graphql */
