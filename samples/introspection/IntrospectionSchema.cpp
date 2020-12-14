// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/introspection/Introspection.h"

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
std::future<service::ResolverResult> ModifiedResult<introspection::TypeKind>::convert(service::FieldResult<introspection::TypeKind>&& result, ResolverParams&& params)
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
std::future<service::ResolverResult> ModifiedResult<introspection::DirectiveLocation>::convert(service::FieldResult<introspection::DirectiveLocation>&& result, ResolverParams&& params)
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

std::future<service::ResolverResult> Schema::resolveTypes(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getTypes(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Schema::resolveQueryType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getQueryType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Schema::resolveMutationType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getMutationType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Schema::resolveSubscriptionType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getSubscriptionType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Schema::resolveDirectives(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDirectives(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Directive>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Schema::resolve_typename(service::ResolverParams&& params)
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

std::future<service::ResolverResult> Type::resolveKind(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getKind(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<TypeKind>::convert(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Type::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Type::resolveDescription(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDescription(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Type::resolveFields(service::ResolverParams&& params)
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

std::future<service::ResolverResult> Type::resolveInterfaces(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getInterfaces(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Type::resolvePossibleTypes(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getPossibleTypes(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Type::resolveEnumValues(service::ResolverParams&& params)
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

std::future<service::ResolverResult> Type::resolveInputFields(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getInputFields(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<InputValue>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Type::resolveOfType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getOfType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Type::resolve_typename(service::ResolverParams&& params)
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

std::future<service::ResolverResult> Field::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Field::resolveDescription(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDescription(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Field::resolveArgs(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getArgs(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<InputValue>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Field::resolveType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Field::resolveIsDeprecated(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getIsDeprecated(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Field::resolveDeprecationReason(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDeprecationReason(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Field::resolve_typename(service::ResolverParams&& params)
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

std::future<service::ResolverResult> InputValue::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

std::future<service::ResolverResult> InputValue::resolveDescription(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDescription(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> InputValue::resolveType(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getType(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Type>::convert(std::move(result), std::move(params));
}

std::future<service::ResolverResult> InputValue::resolveDefaultValue(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDefaultValue(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> InputValue::resolve_typename(service::ResolverParams&& params)
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

std::future<service::ResolverResult> EnumValue::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

std::future<service::ResolverResult> EnumValue::resolveDescription(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDescription(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> EnumValue::resolveIsDeprecated(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getIsDeprecated(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<service::ResolverResult> EnumValue::resolveDeprecationReason(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDeprecationReason(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> EnumValue::resolve_typename(service::ResolverParams&& params)
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

std::future<service::ResolverResult> Directive::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Directive::resolveDescription(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDescription(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Directive::resolveLocations(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getLocations(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<DirectiveLocation>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Directive::resolveArgs(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getArgs(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<InputValue>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<service::ResolverResult> Directive::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(__Directive)gql" }, std::move(params));
}

} /* namespace object */

void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema)
{
	schema->AddType(R"gql(Boolean)gql"sv, schema::ScalarType::Make(R"gql(Boolean)gql"sv, R"md(Built-in type)md"));
	schema->AddType(R"gql(Float)gql"sv, schema::ScalarType::Make(R"gql(Float)gql"sv, R"md(Built-in type)md"));
	schema->AddType(R"gql(ID)gql"sv, schema::ScalarType::Make(R"gql(ID)gql"sv, R"md(Built-in type)md"));
	schema->AddType(R"gql(Int)gql"sv, schema::ScalarType::Make(R"gql(Int)gql"sv, R"md(Built-in type)md"));
	schema->AddType(R"gql(String)gql"sv, schema::ScalarType::Make(R"gql(String)gql"sv, R"md(Built-in type)md"));
	auto typeTypeKind = schema::EnumType::Make(R"gql(__TypeKind)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(__TypeKind)gql"sv, typeTypeKind);
	auto typeDirectiveLocation = schema::EnumType::Make(R"gql(__DirectiveLocation)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(__DirectiveLocation)gql"sv, typeDirectiveLocation);
	auto typeSchema = schema::ObjectType::Make(R"gql(__Schema)gql"sv, R"md()md");
	schema->AddType(R"gql(__Schema)gql"sv, typeSchema);
	auto typeType = schema::ObjectType::Make(R"gql(__Type)gql"sv, R"md()md");
	schema->AddType(R"gql(__Type)gql"sv, typeType);
	auto typeField = schema::ObjectType::Make(R"gql(__Field)gql"sv, R"md()md");
	schema->AddType(R"gql(__Field)gql"sv, typeField);
	auto typeInputValue = schema::ObjectType::Make(R"gql(__InputValue)gql"sv, R"md()md");
	schema->AddType(R"gql(__InputValue)gql"sv, typeInputValue);
	auto typeEnumValue = schema::ObjectType::Make(R"gql(__EnumValue)gql"sv, R"md()md");
	schema->AddType(R"gql(__EnumValue)gql"sv, typeEnumValue);
	auto typeDirective = schema::ObjectType::Make(R"gql(__Directive)gql"sv, R"md()md");
	schema->AddType(R"gql(__Directive)gql"sv, typeDirective);

	typeTypeKind->AddEnumValues({
		{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::SCALAR)], R"md()md"sv, std::nullopt },
		{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::OBJECT)], R"md()md"sv, std::nullopt },
		{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::INTERFACE)], R"md()md"sv, std::nullopt },
		{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::UNION)], R"md()md"sv, std::nullopt },
		{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::ENUM)], R"md()md"sv, std::nullopt },
		{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::INPUT_OBJECT)], R"md()md"sv, std::nullopt },
		{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::LIST)], R"md()md"sv, std::nullopt },
		{ service::s_namesTypeKind[static_cast<size_t>(introspection::TypeKind::NON_NULL)], R"md()md"sv, std::nullopt }
	});
	typeDirectiveLocation->AddEnumValues({
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::QUERY)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::MUTATION)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::SUBSCRIPTION)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::FIELD)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::FRAGMENT_DEFINITION)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::FRAGMENT_SPREAD)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::INLINE_FRAGMENT)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::SCHEMA)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::SCALAR)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::OBJECT)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::FIELD_DEFINITION)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::ARGUMENT_DEFINITION)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::INTERFACE)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::UNION)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::ENUM)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::ENUM_VALUE)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::INPUT_OBJECT)], R"md()md"sv, std::nullopt },
		{ service::s_namesDirectiveLocation[static_cast<size_t>(introspection::DirectiveLocation::INPUT_FIELD_DEFINITION)], R"md()md"sv, std::nullopt }
	});

	typeSchema->AddFields({
		schema::Field::Make(R"gql(types)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type"))))),
		schema::Field::Make(R"gql(queryType)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type"))),
		schema::Field::Make(R"gql(mutationType)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("__Type")),
		schema::Field::Make(R"gql(subscriptionType)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("__Type")),
		schema::Field::Make(R"gql(directives)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Directive")))))
	});
	typeType->AddFields({
		schema::Field::Make(R"gql(kind)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__TypeKind"))),
		schema::Field::Make(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("String")),
		schema::Field::Make(R"gql(description)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("String")),
		schema::Field::Make(R"gql(fields)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Field"))), {
			schema::InputValue::Make(R"gql(includeDeprecated)gql"sv, R"md()md"sv, schema->LookupType("Boolean"), R"gql(false)gql"sv)
		}),
		schema::Field::Make(R"gql(interfaces)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type")))),
		schema::Field::Make(R"gql(possibleTypes)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type")))),
		schema::Field::Make(R"gql(enumValues)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__EnumValue"))), {
			schema::InputValue::Make(R"gql(includeDeprecated)gql"sv, R"md()md"sv, schema->LookupType("Boolean"), R"gql(false)gql"sv)
		}),
		schema::Field::Make(R"gql(inputFields)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__InputValue")))),
		schema::Field::Make(R"gql(ofType)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("__Type"))
	});
	typeField->AddFields({
		schema::Field::Make(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		schema::Field::Make(R"gql(description)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("String")),
		schema::Field::Make(R"gql(args)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__InputValue"))))),
		schema::Field::Make(R"gql(type)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type"))),
		schema::Field::Make(R"gql(isDeprecated)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		schema::Field::Make(R"gql(deprecationReason)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("String"))
	});
	typeInputValue->AddFields({
		schema::Field::Make(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		schema::Field::Make(R"gql(description)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("String")),
		schema::Field::Make(R"gql(type)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__Type"))),
		schema::Field::Make(R"gql(defaultValue)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("String"))
	});
	typeEnumValue->AddFields({
		schema::Field::Make(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		schema::Field::Make(R"gql(description)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("String")),
		schema::Field::Make(R"gql(isDeprecated)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		schema::Field::Make(R"gql(deprecationReason)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("String"))
	});
	typeDirective->AddFields({
		schema::Field::Make(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		schema::Field::Make(R"gql(description)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType("String")),
		schema::Field::Make(R"gql(locations)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__DirectiveLocation"))))),
		schema::Field::Make(R"gql(args)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("__InputValue")))))
	});

	schema->AddDirective(schema::Directive::Make(R"gql(skip)gql"sv, R"md()md"sv, {
		introspection::DirectiveLocation::FIELD,
		introspection::DirectiveLocation::FRAGMENT_SPREAD,
		introspection::DirectiveLocation::INLINE_FRAGMENT
	}, {
		schema::InputValue::Make(R"gql(if)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")), R"gql()gql"sv)
	}));
	schema->AddDirective(schema::Directive::Make(R"gql(include)gql"sv, R"md()md"sv, {
		introspection::DirectiveLocation::FIELD,
		introspection::DirectiveLocation::FRAGMENT_SPREAD,
		introspection::DirectiveLocation::INLINE_FRAGMENT
	}, {
		schema::InputValue::Make(R"gql(if)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")), R"gql()gql"sv)
	}));
	schema->AddDirective(schema::Directive::Make(R"gql(deprecated)gql"sv, R"md()md"sv, {
		introspection::DirectiveLocation::FIELD_DEFINITION,
		introspection::DirectiveLocation::ENUM_VALUE
	}, {
		schema::InputValue::Make(R"gql(reason)gql"sv, R"md()md"sv, schema->LookupType("String"), R"gql("No longer supported")gql"sv)
	}));
}

} /* namespace introspection */
} /* namespace graphql */
