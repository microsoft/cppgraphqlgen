// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "QueryObject.h"
#include "MutationObject.h"
#include "SubscriptionObject.h"

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

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

static const auto s_namesDogCommand = validation::getDogCommandNames();
static const auto s_valuesDogCommand = validation::getDogCommandValues();

template <>
validation::DogCommand ModifiedArgument<validation::DogCommand>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid DogCommand value)ex" } };
	}

	const auto itr = s_valuesDogCommand.find(value.get<std::string>());

	if (itr == s_valuesDogCommand.end())
	{
		throw service::schema_exception { { R"ex(not a valid DogCommand value)ex" } };
	}

	return itr->second;
}

template <>
service::AwaitableResolver ModifiedResult<validation::DogCommand>::convert(service::AwaitableScalar<validation::DogCommand> result, ResolverParams params)
{
	return resolve(std::move(result), std::move(params),
		[](validation::DogCommand value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<std::string>(std::string { s_namesDogCommand[static_cast<size_t>(value)] });

			return result;
		});
}

template <>
void ModifiedResult<validation::DogCommand>::validateScalar(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid DogCommand value)ex" } };
	}

	const auto itr = s_valuesDogCommand.find(value.get<std::string>());

	if (itr == s_valuesDogCommand.end())
	{
		throw service::schema_exception { { R"ex(not a valid DogCommand value)ex" } };
	}
}

static const auto s_namesCatCommand = validation::getCatCommandNames();
static const auto s_valuesCatCommand = validation::getCatCommandValues();

template <>
validation::CatCommand ModifiedArgument<validation::CatCommand>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid CatCommand value)ex" } };
	}

	const auto itr = s_valuesCatCommand.find(value.get<std::string>());

	if (itr == s_valuesCatCommand.end())
	{
		throw service::schema_exception { { R"ex(not a valid CatCommand value)ex" } };
	}

	return itr->second;
}

template <>
service::AwaitableResolver ModifiedResult<validation::CatCommand>::convert(service::AwaitableScalar<validation::CatCommand> result, ResolverParams params)
{
	return resolve(std::move(result), std::move(params),
		[](validation::CatCommand value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<std::string>(std::string { s_namesCatCommand[static_cast<size_t>(value)] });

			return result;
		});
}

template <>
void ModifiedResult<validation::CatCommand>::validateScalar(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid CatCommand value)ex" } };
	}

	const auto itr = s_valuesCatCommand.find(value.get<std::string>());

	if (itr == s_valuesCatCommand.end())
	{
		throw service::schema_exception { { R"ex(not a valid CatCommand value)ex" } };
	}
}

template <>
validation::ComplexInput ModifiedArgument<validation::ComplexInput>::convert(const response::Value& value)
{
	auto valueName = service::ModifiedArgument<std::string>::require<service::TypeModifier::Nullable>("name", value);
	auto valueOwner = service::ModifiedArgument<std::string>::require<service::TypeModifier::Nullable>("owner", value);

	return {
		std::move(valueName),
		std::move(valueOwner)
	};
}

} // namespace service

namespace validation {

Operations::Operations(std::shared_ptr<object::Query> query, std::shared_ptr<object::Mutation> mutation, std::shared_ptr<object::Subscription> subscription)
	: service::Request({
		{ "query", query },
		{ "mutation", mutation },
		{ "subscription", subscription }
	}, GetSchema())
	, _query(std::move(query))
	, _mutation(std::move(mutation))
	, _subscription(std::move(subscription))
{
}

void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema)
{
	auto typeDogCommand = schema::EnumType::Make(R"gql(DogCommand)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(DogCommand)gql"sv, typeDogCommand);
	auto typeCatCommand = schema::EnumType::Make(R"gql(CatCommand)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(CatCommand)gql"sv, typeCatCommand);
	auto typeComplexInput = schema::InputObjectType::Make(R"gql(ComplexInput)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(ComplexInput)gql"sv, typeComplexInput);
	auto typeSentient = schema::InterfaceType::Make(R"gql(Sentient)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Sentient)gql"sv, typeSentient);
	auto typePet = schema::InterfaceType::Make(R"gql(Pet)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Pet)gql"sv, typePet);
	auto typeNode = schema::InterfaceType::Make(R"gql(Node)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Node)gql"sv, typeNode);
	auto typeResource = schema::InterfaceType::Make(R"gql(Resource)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Resource)gql"sv, typeResource);
	auto typeCatOrDog = schema::UnionType::Make(R"gql(CatOrDog)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(CatOrDog)gql"sv, typeCatOrDog);
	auto typeDogOrHuman = schema::UnionType::Make(R"gql(DogOrHuman)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(DogOrHuman)gql"sv, typeDogOrHuman);
	auto typeHumanOrAlien = schema::UnionType::Make(R"gql(HumanOrAlien)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(HumanOrAlien)gql"sv, typeHumanOrAlien);
	auto typeQuery = schema::ObjectType::Make(R"gql(Query)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Query)gql"sv, typeQuery);
	auto typeDog = schema::ObjectType::Make(R"gql(Dog)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Dog)gql"sv, typeDog);
	auto typeAlien = schema::ObjectType::Make(R"gql(Alien)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Alien)gql"sv, typeAlien);
	auto typeHuman = schema::ObjectType::Make(R"gql(Human)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Human)gql"sv, typeHuman);
	auto typeCat = schema::ObjectType::Make(R"gql(Cat)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Cat)gql"sv, typeCat);
	auto typeMutation = schema::ObjectType::Make(R"gql(Mutation)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Mutation)gql"sv, typeMutation);
	auto typeMutateDogResult = schema::ObjectType::Make(R"gql(MutateDogResult)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(MutateDogResult)gql"sv, typeMutateDogResult);
	auto typeSubscription = schema::ObjectType::Make(R"gql(Subscription)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Subscription)gql"sv, typeSubscription);
	auto typeMessage = schema::ObjectType::Make(R"gql(Message)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Message)gql"sv, typeMessage);
	auto typeArguments = schema::ObjectType::Make(R"gql(Arguments)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Arguments)gql"sv, typeArguments);

	typeDogCommand->AddEnumValues({
		{ service::s_namesDogCommand[static_cast<size_t>(validation::DogCommand::SIT)], R"md()md"sv, std::nullopt },
		{ service::s_namesDogCommand[static_cast<size_t>(validation::DogCommand::DOWN)], R"md()md"sv, std::nullopt },
		{ service::s_namesDogCommand[static_cast<size_t>(validation::DogCommand::HEEL)], R"md()md"sv, std::nullopt }
	});
	typeCatCommand->AddEnumValues({
		{ service::s_namesCatCommand[static_cast<size_t>(validation::CatCommand::JUMP)], R"md()md"sv, std::nullopt }
	});

	typeComplexInput->AddInputValues({
		schema::InputValue::Make(R"gql(name)gql"sv, R"md()md"sv, schema->LookupType(R"gql(String)gql"sv), R"gql()gql"sv),
		schema::InputValue::Make(R"gql(owner)gql"sv, R"md()md"sv, schema->LookupType(R"gql(String)gql"sv), R"gql()gql"sv)
	});

	AddSentientDetails(typeSentient, schema);
	AddPetDetails(typePet, schema);
	AddNodeDetails(typeNode, schema);
	AddResourceDetails(typeResource, schema);

	AddCatOrDogDetails(typeCatOrDog, schema);
	AddDogOrHumanDetails(typeDogOrHuman, schema);
	AddHumanOrAlienDetails(typeHumanOrAlien, schema);

	AddQueryDetails(typeQuery, schema);
	AddDogDetails(typeDog, schema);
	AddAlienDetails(typeAlien, schema);
	AddHumanDetails(typeHuman, schema);
	AddCatDetails(typeCat, schema);
	AddMutationDetails(typeMutation, schema);
	AddMutateDogResultDetails(typeMutateDogResult, schema);
	AddSubscriptionDetails(typeSubscription, schema);
	AddMessageDetails(typeMessage, schema);
	AddArgumentsDetails(typeArguments, schema);

	schema->AddQueryType(typeQuery);
	schema->AddMutationType(typeMutation);
	schema->AddSubscriptionType(typeSubscription);
}

std::shared_ptr<schema::Schema> GetSchema()
{
	static std::weak_ptr<schema::Schema> s_wpSchema;
	auto schema = s_wpSchema.lock();

	if (!schema)
	{
		schema = std::make_shared<schema::Schema>(true, R"md()md"sv);
		introspection::AddTypesToSchema(schema);
		AddTypesToSchema(schema);
		s_wpSchema = schema;
	}

	return schema;
}

} // namespace validation
} // namespace graphql
