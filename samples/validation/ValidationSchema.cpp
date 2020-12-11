// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ValidationSchema.h"

#include "graphqlservice/Introspection.h"

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

static const std::array<std::string_view, 3> s_namesDogCommand = {
	"SIT",
	"DOWN",
	"HEEL"
};

template <>
validation::DogCommand ModifiedArgument<validation::DogCommand>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { "not a valid DogCommand value" } };
	}

	auto itr = std::find(s_namesDogCommand.cbegin(), s_namesDogCommand.cend(), value.get<response::StringType>());

	if (itr == s_namesDogCommand.cend())
	{
		throw service::schema_exception { { "not a valid DogCommand value" } };
	}

	return static_cast<validation::DogCommand>(itr - s_namesDogCommand.cbegin());
}

template <>
std::future<response::Value> ModifiedResult<validation::DogCommand>::convert(service::FieldResult<validation::DogCommand>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[](validation::DogCommand&& value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<response::StringType>(std::string(s_namesDogCommand[static_cast<size_t>(value)]));

			return result;
		});
}

static const std::array<std::string_view, 1> s_namesCatCommand = {
	"JUMP"
};

template <>
validation::CatCommand ModifiedArgument<validation::CatCommand>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { "not a valid CatCommand value" } };
	}

	auto itr = std::find(s_namesCatCommand.cbegin(), s_namesCatCommand.cend(), value.get<response::StringType>());

	if (itr == s_namesCatCommand.cend())
	{
		throw service::schema_exception { { "not a valid CatCommand value" } };
	}

	return static_cast<validation::CatCommand>(itr - s_namesCatCommand.cbegin());
}

template <>
std::future<response::Value> ModifiedResult<validation::CatCommand>::convert(service::FieldResult<validation::CatCommand>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[](validation::CatCommand&& value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<response::StringType>(std::string(s_namesCatCommand[static_cast<size_t>(value)]));

			return result;
		});
}

template <>
validation::ComplexInput ModifiedArgument<validation::ComplexInput>::convert(const response::Value& value)
{
	auto valueName = service::ModifiedArgument<response::StringType>::require<service::TypeModifier::Nullable>("name", value);
	auto valueOwner = service::ModifiedArgument<response::StringType>::require<service::TypeModifier::Nullable>("owner", value);

	return {
		std::move(valueName),
		std::move(valueOwner)
	};
}

} /* namespace service */

namespace validation {
namespace object {

Query::Query()
	: service::Object({
		"Query"
	}, {
		{ R"gql(__schema)gql"sv, [this](service::ResolverParams&& params) { return resolve_schema(std::move(params)); } },
		{ R"gql(__type)gql"sv, [this](service::ResolverParams&& params) { return resolve_type(std::move(params)); } },
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(arguments)gql"sv, [this](service::ResolverParams&& params) { return resolveArguments(std::move(params)); } },
		{ R"gql(booleanList)gql"sv, [this](service::ResolverParams&& params) { return resolveBooleanList(std::move(params)); } },
		{ R"gql(catOrDog)gql"sv, [this](service::ResolverParams&& params) { return resolveCatOrDog(std::move(params)); } },
		{ R"gql(dog)gql"sv, [this](service::ResolverParams&& params) { return resolveDog(std::move(params)); } },
		{ R"gql(findDog)gql"sv, [this](service::ResolverParams&& params) { return resolveFindDog(std::move(params)); } },
		{ R"gql(human)gql"sv, [this](service::ResolverParams&& params) { return resolveHuman(std::move(params)); } },
		{ R"gql(pet)gql"sv, [this](service::ResolverParams&& params) { return resolvePet(std::move(params)); } }
	})
	, _schema(GetSchema())
{
}

service::FieldResult<std::shared_ptr<Dog>> Query::getDog(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Query::getDog is not implemented)ex");
}

std::future<response::Value> Query::resolveDog(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDog(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Dog>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<Human>> Query::getHuman(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Query::getHuman is not implemented)ex");
}

std::future<response::Value> Query::resolveHuman(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getHuman(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Human>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<service::Object>> Query::getPet(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Query::getPet is not implemented)ex");
}

std::future<response::Value> Query::resolvePet(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getPet(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<service::Object>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<service::Object>> Query::getCatOrDog(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Query::getCatOrDog is not implemented)ex");
}

std::future<response::Value> Query::resolveCatOrDog(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getCatOrDog(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<service::Object>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<Arguments>> Query::getArguments(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Query::getArguments is not implemented)ex");
}

std::future<response::Value> Query::resolveArguments(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getArguments(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Arguments>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<Dog>> Query::getFindDog(service::FieldParams&&, std::optional<ComplexInput>&&) const
{
	throw std::runtime_error(R"ex(Query::getFindDog is not implemented)ex");
}

std::future<response::Value> Query::resolveFindDog(service::ResolverParams&& params)
{
	auto argComplex = service::ModifiedArgument<validation::ComplexInput>::require<service::TypeModifier::Nullable>("complex", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getFindDog(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argComplex));
	resolverLock.unlock();

	return service::ModifiedResult<Dog>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::BooleanType>> Query::getBooleanList(service::FieldParams&&, std::optional<std::vector<response::BooleanType>>&&) const
{
	throw std::runtime_error(R"ex(Query::getBooleanList is not implemented)ex");
}

std::future<response::Value> Query::resolveBooleanList(service::ResolverParams&& params)
{
	auto argBooleanListArg = service::ModifiedArgument<response::BooleanType>::require<service::TypeModifier::Nullable, service::TypeModifier::List>("booleanListArg", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getBooleanList(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argBooleanListArg));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Query::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Query)gql" }, std::move(params));
}

std::future<response::Value> Query::resolve_schema(service::ResolverParams&& params)
{
	return service::ModifiedResult<service::Object>::convert(std::static_pointer_cast<service::Object>(std::make_shared<introspection::Schema>(_schema)), std::move(params));
}

std::future<response::Value> Query::resolve_type(service::ResolverParams&& params)
{
	auto argName = service::ModifiedArgument<response::StringType>::require("name", params.arguments);
	const auto& baseType = _schema->LookupType(argName);
	std::shared_ptr<introspection::object::Type> result { baseType ? std::make_shared<introspection::Type>(baseType) : nullptr };

	return service::ModifiedResult<introspection::object::Type>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

Dog::Dog()
	: service::Object({
		"Pet",
		"CatOrDog",
		"DogOrHuman",
		"Dog"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(barkVolume)gql"sv, [this](service::ResolverParams&& params) { return resolveBarkVolume(std::move(params)); } },
		{ R"gql(doesKnowCommand)gql"sv, [this](service::ResolverParams&& params) { return resolveDoesKnowCommand(std::move(params)); } },
		{ R"gql(isHousetrained)gql"sv, [this](service::ResolverParams&& params) { return resolveIsHousetrained(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(nickname)gql"sv, [this](service::ResolverParams&& params) { return resolveNickname(std::move(params)); } },
		{ R"gql(owner)gql"sv, [this](service::ResolverParams&& params) { return resolveOwner(std::move(params)); } }
	})
{
}

service::FieldResult<response::StringType> Dog::getName(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Dog::getName is not implemented)ex");
}

std::future<response::Value> Dog::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> Dog::getNickname(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Dog::getNickname is not implemented)ex");
}

std::future<response::Value> Dog::resolveNickname(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNickname(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::IntType>> Dog::getBarkVolume(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Dog::getBarkVolume is not implemented)ex");
}

std::future<response::Value> Dog::resolveBarkVolume(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getBarkVolume(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IntType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> Dog::getDoesKnowCommand(service::FieldParams&&, DogCommand&&) const
{
	throw std::runtime_error(R"ex(Dog::getDoesKnowCommand is not implemented)ex");
}

std::future<response::Value> Dog::resolveDoesKnowCommand(service::ResolverParams&& params)
{
	auto argDogCommand = service::ModifiedArgument<DogCommand>::require("dogCommand", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDoesKnowCommand(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argDogCommand));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> Dog::getIsHousetrained(service::FieldParams&&, std::optional<response::BooleanType>&&) const
{
	throw std::runtime_error(R"ex(Dog::getIsHousetrained is not implemented)ex");
}

std::future<response::Value> Dog::resolveIsHousetrained(service::ResolverParams&& params)
{
	auto argAtOtherHomes = service::ModifiedArgument<response::BooleanType>::require<service::TypeModifier::Nullable>("atOtherHomes", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getIsHousetrained(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argAtOtherHomes));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<Human>> Dog::getOwner(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Dog::getOwner is not implemented)ex");
}

std::future<response::Value> Dog::resolveOwner(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getOwner(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Human>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Dog::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Dog)gql" }, std::move(params));
}

Alien::Alien()
	: service::Object({
		"Sentient",
		"HumanOrAlien",
		"Alien"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(homePlanet)gql"sv, [this](service::ResolverParams&& params) { return resolveHomePlanet(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } }
	})
{
}

service::FieldResult<response::StringType> Alien::getName(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Alien::getName is not implemented)ex");
}

std::future<response::Value> Alien::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> Alien::getHomePlanet(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Alien::getHomePlanet is not implemented)ex");
}

std::future<response::Value> Alien::resolveHomePlanet(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getHomePlanet(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Alien::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Alien)gql" }, std::move(params));
}

Human::Human()
	: service::Object({
		"Sentient",
		"DogOrHuman",
		"HumanOrAlien",
		"Human"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(pets)gql"sv, [this](service::ResolverParams&& params) { return resolvePets(std::move(params)); } }
	})
{
}

service::FieldResult<response::StringType> Human::getName(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Human::getName is not implemented)ex");
}

std::future<response::Value> Human::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> Human::getPets(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Human::getPets is not implemented)ex");
}

std::future<response::Value> Human::resolvePets(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getPets(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<service::Object>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Human::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Human)gql" }, std::move(params));
}

Cat::Cat()
	: service::Object({
		"Pet",
		"CatOrDog",
		"Cat"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(doesKnowCommand)gql"sv, [this](service::ResolverParams&& params) { return resolveDoesKnowCommand(std::move(params)); } },
		{ R"gql(meowVolume)gql"sv, [this](service::ResolverParams&& params) { return resolveMeowVolume(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(nickname)gql"sv, [this](service::ResolverParams&& params) { return resolveNickname(std::move(params)); } }
	})
{
}

service::FieldResult<response::StringType> Cat::getName(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Cat::getName is not implemented)ex");
}

std::future<response::Value> Cat::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> Cat::getNickname(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Cat::getNickname is not implemented)ex");
}

std::future<response::Value> Cat::resolveNickname(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNickname(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> Cat::getDoesKnowCommand(service::FieldParams&&, CatCommand&&) const
{
	throw std::runtime_error(R"ex(Cat::getDoesKnowCommand is not implemented)ex");
}

std::future<response::Value> Cat::resolveDoesKnowCommand(service::ResolverParams&& params)
{
	auto argCatCommand = service::ModifiedArgument<CatCommand>::require("catCommand", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDoesKnowCommand(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argCatCommand));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::IntType>> Cat::getMeowVolume(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Cat::getMeowVolume is not implemented)ex");
}

std::future<response::Value> Cat::resolveMeowVolume(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getMeowVolume(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IntType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Cat::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Cat)gql" }, std::move(params));
}

Mutation::Mutation()
	: service::Object({
		"Mutation"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(mutateDog)gql"sv, [this](service::ResolverParams&& params) { return resolveMutateDog(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<MutateDogResult>> Mutation::applyMutateDog(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Mutation::applyMutateDog is not implemented)ex");
}

std::future<response::Value> Mutation::resolveMutateDog(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = applyMutateDog(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<MutateDogResult>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Mutation::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Mutation)gql" }, std::move(params));
}

MutateDogResult::MutateDogResult()
	: service::Object({
		"MutateDogResult"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(id)gql"sv, [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } }
	})
{
}

service::FieldResult<response::IdType> MutateDogResult::getId(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(MutateDogResult::getId is not implemented)ex");
}

std::future<response::Value> MutateDogResult::resolveId(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> MutateDogResult::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(MutateDogResult)gql" }, std::move(params));
}

Subscription::Subscription()
	: service::Object({
		"Subscription"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(disallowedSecondRootField)gql"sv, [this](service::ResolverParams&& params) { return resolveDisallowedSecondRootField(std::move(params)); } },
		{ R"gql(newMessage)gql"sv, [this](service::ResolverParams&& params) { return resolveNewMessage(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<Message>> Subscription::getNewMessage(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Subscription::getNewMessage is not implemented)ex");
}

std::future<response::Value> Subscription::resolveNewMessage(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNewMessage(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Message>::convert(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> Subscription::getDisallowedSecondRootField(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Subscription::getDisallowedSecondRootField is not implemented)ex");
}

std::future<response::Value> Subscription::resolveDisallowedSecondRootField(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDisallowedSecondRootField(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Subscription::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Subscription)gql" }, std::move(params));
}

Message::Message()
	: service::Object({
		"Message"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(body)gql"sv, [this](service::ResolverParams&& params) { return resolveBody(std::move(params)); } },
		{ R"gql(sender)gql"sv, [this](service::ResolverParams&& params) { return resolveSender(std::move(params)); } }
	})
{
}

service::FieldResult<std::optional<response::StringType>> Message::getBody(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Message::getBody is not implemented)ex");
}

std::future<response::Value> Message::resolveBody(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getBody(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::IdType> Message::getSender(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Message::getSender is not implemented)ex");
}

std::future<response::Value> Message::resolveSender(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getSender(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Message::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Message)gql" }, std::move(params));
}

Arguments::Arguments()
	: service::Object({
		"Arguments"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(booleanArgField)gql"sv, [this](service::ResolverParams&& params) { return resolveBooleanArgField(std::move(params)); } },
		{ R"gql(booleanListArgField)gql"sv, [this](service::ResolverParams&& params) { return resolveBooleanListArgField(std::move(params)); } },
		{ R"gql(floatArgField)gql"sv, [this](service::ResolverParams&& params) { return resolveFloatArgField(std::move(params)); } },
		{ R"gql(intArgField)gql"sv, [this](service::ResolverParams&& params) { return resolveIntArgField(std::move(params)); } },
		{ R"gql(multipleReqs)gql"sv, [this](service::ResolverParams&& params) { return resolveMultipleReqs(std::move(params)); } },
		{ R"gql(nonNullBooleanArgField)gql"sv, [this](service::ResolverParams&& params) { return resolveNonNullBooleanArgField(std::move(params)); } },
		{ R"gql(nonNullBooleanListField)gql"sv, [this](service::ResolverParams&& params) { return resolveNonNullBooleanListField(std::move(params)); } },
		{ R"gql(optionalNonNullBooleanArgField)gql"sv, [this](service::ResolverParams&& params) { return resolveOptionalNonNullBooleanArgField(std::move(params)); } }
	})
{
}

service::FieldResult<response::IntType> Arguments::getMultipleReqs(service::FieldParams&&, response::IntType&&, response::IntType&&) const
{
	throw std::runtime_error(R"ex(Arguments::getMultipleReqs is not implemented)ex");
}

std::future<response::Value> Arguments::resolveMultipleReqs(service::ResolverParams&& params)
{
	auto argX = service::ModifiedArgument<response::IntType>::require("x", params.arguments);
	auto argY = service::ModifiedArgument<response::IntType>::require("y", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getMultipleReqs(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argX), std::move(argY));
	resolverLock.unlock();

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::BooleanType>> Arguments::getBooleanArgField(service::FieldParams&&, std::optional<response::BooleanType>&&) const
{
	throw std::runtime_error(R"ex(Arguments::getBooleanArgField is not implemented)ex");
}

std::future<response::Value> Arguments::resolveBooleanArgField(service::ResolverParams&& params)
{
	auto argBooleanArg = service::ModifiedArgument<response::BooleanType>::require<service::TypeModifier::Nullable>("booleanArg", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getBooleanArgField(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argBooleanArg));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::FloatType>> Arguments::getFloatArgField(service::FieldParams&&, std::optional<response::FloatType>&&) const
{
	throw std::runtime_error(R"ex(Arguments::getFloatArgField is not implemented)ex");
}

std::future<response::Value> Arguments::resolveFloatArgField(service::ResolverParams&& params)
{
	auto argFloatArg = service::ModifiedArgument<response::FloatType>::require<service::TypeModifier::Nullable>("floatArg", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getFloatArgField(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argFloatArg));
	resolverLock.unlock();

	return service::ModifiedResult<response::FloatType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::IntType>> Arguments::getIntArgField(service::FieldParams&&, std::optional<response::IntType>&&) const
{
	throw std::runtime_error(R"ex(Arguments::getIntArgField is not implemented)ex");
}

std::future<response::Value> Arguments::resolveIntArgField(service::ResolverParams&& params)
{
	auto argIntArg = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("intArg", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getIntArgField(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIntArg));
	resolverLock.unlock();

	return service::ModifiedResult<response::IntType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> Arguments::getNonNullBooleanArgField(service::FieldParams&&, response::BooleanType&&) const
{
	throw std::runtime_error(R"ex(Arguments::getNonNullBooleanArgField is not implemented)ex");
}

std::future<response::Value> Arguments::resolveNonNullBooleanArgField(service::ResolverParams&& params)
{
	auto argNonNullBooleanArg = service::ModifiedArgument<response::BooleanType>::require("nonNullBooleanArg", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNonNullBooleanArgField(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argNonNullBooleanArg));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<std::vector<response::BooleanType>>> Arguments::getNonNullBooleanListField(service::FieldParams&&, std::optional<std::vector<response::BooleanType>>&&) const
{
	throw std::runtime_error(R"ex(Arguments::getNonNullBooleanListField is not implemented)ex");
}

std::future<response::Value> Arguments::resolveNonNullBooleanListField(service::ResolverParams&& params)
{
	auto argNonNullBooleanListArg = service::ModifiedArgument<response::BooleanType>::require<service::TypeModifier::Nullable, service::TypeModifier::List>("nonNullBooleanListArg", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNonNullBooleanListField(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argNonNullBooleanListArg));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert<service::TypeModifier::Nullable, service::TypeModifier::List>(std::move(result), std::move(params));
}

service::FieldResult<std::optional<std::vector<std::optional<response::BooleanType>>>> Arguments::getBooleanListArgField(service::FieldParams&&, std::vector<std::optional<response::BooleanType>>&&) const
{
	throw std::runtime_error(R"ex(Arguments::getBooleanListArgField is not implemented)ex");
}

std::future<response::Value> Arguments::resolveBooleanListArgField(service::ResolverParams&& params)
{
	auto argBooleanListArg = service::ModifiedArgument<response::BooleanType>::require<service::TypeModifier::List, service::TypeModifier::Nullable>("booleanListArg", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getBooleanListArgField(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argBooleanListArg));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> Arguments::getOptionalNonNullBooleanArgField(service::FieldParams&&, response::BooleanType&&) const
{
	throw std::runtime_error(R"ex(Arguments::getOptionalNonNullBooleanArgField is not implemented)ex");
}

std::future<response::Value> Arguments::resolveOptionalNonNullBooleanArgField(service::ResolverParams&& params)
{
	const auto defaultArguments = []()
	{
		response::Value values(response::Type::Map);
		response::Value entry;

		entry = response::Value(false);
		values.emplace_back("optionalBooleanArg", std::move(entry));

		return values;
	}();

	auto pairOptionalBooleanArg = service::ModifiedArgument<response::BooleanType>::find("optionalBooleanArg", params.arguments);
	auto argOptionalBooleanArg = (pairOptionalBooleanArg.second
		? std::move(pairOptionalBooleanArg.first)
		: service::ModifiedArgument<response::BooleanType>::require("optionalBooleanArg", defaultArguments));
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getOptionalNonNullBooleanArgField(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argOptionalBooleanArg));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Arguments::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Arguments)gql" }, std::move(params));
}

} /* namespace object */

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
	auto typeDogCommand = std::make_shared<schema::EnumType>(R"gql(DogCommand)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(DogCommand)gql"sv, typeDogCommand);
	auto typeCatCommand = std::make_shared<schema::EnumType>(R"gql(CatCommand)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(CatCommand)gql"sv, typeCatCommand);
	auto typeComplexInput = std::make_shared<schema::InputObjectType>(R"gql(ComplexInput)gql"sv, R"md([Example 155](http://spec.graphql.org/June2018/#example-f3185))md"sv);
	schema->AddType(R"gql(ComplexInput)gql"sv, typeComplexInput);
	auto typeCatOrDog = std::make_shared<schema::UnionType>(R"gql(CatOrDog)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(CatOrDog)gql"sv, typeCatOrDog);
	auto typeDogOrHuman = std::make_shared<schema::UnionType>(R"gql(DogOrHuman)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(DogOrHuman)gql"sv, typeDogOrHuman);
	auto typeHumanOrAlien = std::make_shared<schema::UnionType>(R"gql(HumanOrAlien)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(HumanOrAlien)gql"sv, typeHumanOrAlien);
	auto typeSentient = std::make_shared<schema::InterfaceType>(R"gql(Sentient)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Sentient)gql"sv, typeSentient);
	auto typePet = std::make_shared<schema::InterfaceType>(R"gql(Pet)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Pet)gql"sv, typePet);
	auto typeQuery = std::make_shared<schema::ObjectType>(R"gql(Query)gql"sv, R"md(GraphQL validation [sample](http://spec.graphql.org/June2018/#example-26a9d))md");
	schema->AddType(R"gql(Query)gql"sv, typeQuery);
	auto typeDog = std::make_shared<schema::ObjectType>(R"gql(Dog)gql"sv, R"md()md");
	schema->AddType(R"gql(Dog)gql"sv, typeDog);
	auto typeAlien = std::make_shared<schema::ObjectType>(R"gql(Alien)gql"sv, R"md()md");
	schema->AddType(R"gql(Alien)gql"sv, typeAlien);
	auto typeHuman = std::make_shared<schema::ObjectType>(R"gql(Human)gql"sv, R"md()md");
	schema->AddType(R"gql(Human)gql"sv, typeHuman);
	auto typeCat = std::make_shared<schema::ObjectType>(R"gql(Cat)gql"sv, R"md()md");
	schema->AddType(R"gql(Cat)gql"sv, typeCat);
	auto typeMutation = std::make_shared<schema::ObjectType>(R"gql(Mutation)gql"sv, R"md(Support for [Counter Example 94](http://spec.graphql.org/June2018/#example-77c2e))md");
	schema->AddType(R"gql(Mutation)gql"sv, typeMutation);
	auto typeMutateDogResult = std::make_shared<schema::ObjectType>(R"gql(MutateDogResult)gql"sv, R"md(Support for [Counter Example 94](http://spec.graphql.org/June2018/#example-77c2e))md");
	schema->AddType(R"gql(MutateDogResult)gql"sv, typeMutateDogResult);
	auto typeSubscription = std::make_shared<schema::ObjectType>(R"gql(Subscription)gql"sv, R"md(Support for [Example 97](http://spec.graphql.org/June2018/#example-5bbc3) - [Counter Example 101](http://spec.graphql.org/June2018/#example-2353b))md");
	schema->AddType(R"gql(Subscription)gql"sv, typeSubscription);
	auto typeMessage = std::make_shared<schema::ObjectType>(R"gql(Message)gql"sv, R"md(Support for [Example 97](http://spec.graphql.org/June2018/#example-5bbc3) - [Counter Example 101](http://spec.graphql.org/June2018/#example-2353b))md");
	schema->AddType(R"gql(Message)gql"sv, typeMessage);
	auto typeArguments = std::make_shared<schema::ObjectType>(R"gql(Arguments)gql"sv, R"md(Support for [Example 120](http://spec.graphql.org/June2018/#example-1891c))md");
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
		std::make_shared<schema::InputValue>(R"gql(name)gql"sv, R"md()md"sv, schema->LookupType("String"), R"gql()gql"sv),
		std::make_shared<schema::InputValue>(R"gql(owner)gql"sv, R"md()md"sv, schema->LookupType("String"), R"gql()gql"sv)
	});

	typeCatOrDog->AddPossibleTypes({
		schema->LookupType(R"gql(Cat)gql"sv),
		schema->LookupType(R"gql(Dog)gql"sv)
	});
	typeDogOrHuman->AddPossibleTypes({
		schema->LookupType(R"gql(Dog)gql"sv),
		schema->LookupType(R"gql(Human)gql"sv)
	});
	typeHumanOrAlien->AddPossibleTypes({
		schema->LookupType(R"gql(Human)gql"sv),
		schema->LookupType(R"gql(Alien)gql"sv)
	});

	typeSentient->AddFields({
		std::make_shared<schema::Field>(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")))
	});
	typePet->AddFields({
		std::make_shared<schema::Field>(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")))
	});

	typeQuery->AddFields({
		std::make_shared<schema::Field>(R"gql(dog)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("Dog")),
		std::make_shared<schema::Field>(R"gql(human)gql"sv, R"md(Support for [Counter Example 116](http://spec.graphql.org/June2018/#example-77c2e))md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("Human")),
		std::make_shared<schema::Field>(R"gql(pet)gql"sv, R"md(Support for [Counter Example 116](http://spec.graphql.org/June2018/#example-77c2e))md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("Pet")),
		std::make_shared<schema::Field>(R"gql(catOrDog)gql"sv, R"md(Support for [Counter Example 116](http://spec.graphql.org/June2018/#example-77c2e))md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("CatOrDog")),
		std::make_shared<schema::Field>(R"gql(arguments)gql"sv, R"md(Support for [Example 120](http://spec.graphql.org/June2018/#example-1891c))md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("Arguments")),
		std::make_shared<schema::Field>(R"gql(findDog)gql"sv, R"md([Example 155](http://spec.graphql.org/June2018/#example-f3185))md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(complex)gql"sv, R"md()md"sv, schema->LookupType("ComplexInput"), R"gql()gql"sv)
		}), schema->LookupType("Dog")),
		std::make_shared<schema::Field>(R"gql(booleanList)gql"sv, R"md([Example 155](http://spec.graphql.org/June2018/#example-f3185))md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(booleanListArg)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))), R"gql()gql"sv)
		}), schema->LookupType("Boolean"))
	});
	typeDog->AddInterfaces({
		typePet
	});
	typeDog->AddFields({
		std::make_shared<schema::Field>(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<schema::Field>(R"gql(nickname)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("String")),
		std::make_shared<schema::Field>(R"gql(barkVolume)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("Int")),
		std::make_shared<schema::Field>(R"gql(doesKnowCommand)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(dogCommand)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("DogCommand")), R"gql()gql"sv)
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<schema::Field>(R"gql(isHousetrained)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(atOtherHomes)gql"sv, R"md()md"sv, schema->LookupType("Boolean"), R"gql()gql"sv)
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<schema::Field>(R"gql(owner)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("Human"))
	});
	typeAlien->AddInterfaces({
		typeSentient
	});
	typeAlien->AddFields({
		std::make_shared<schema::Field>(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<schema::Field>(R"gql(homePlanet)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("String"))
	});
	typeHuman->AddInterfaces({
		typeSentient
	});
	typeHuman->AddFields({
		std::make_shared<schema::Field>(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<schema::Field>(R"gql(pets)gql"sv, R"md(Support for [Counter Example 136](http://spec.graphql.org/June2018/#example-6bbad))md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Pet")))))
	});
	typeCat->AddInterfaces({
		typePet
	});
	typeCat->AddFields({
		std::make_shared<schema::Field>(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<schema::Field>(R"gql(nickname)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("String")),
		std::make_shared<schema::Field>(R"gql(doesKnowCommand)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(catCommand)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("CatCommand")), R"gql()gql"sv)
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<schema::Field>(R"gql(meowVolume)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("Int"))
	});
	typeMutation->AddFields({
		std::make_shared<schema::Field>(R"gql(mutateDog)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("MutateDogResult"))
	});
	typeMutateDogResult->AddFields({
		std::make_shared<schema::Field>(R"gql(id)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")))
	});
	typeSubscription->AddFields({
		std::make_shared<schema::Field>(R"gql(newMessage)gql"sv, R"md(Support for [Example 97](http://spec.graphql.org/June2018/#example-5bbc3) - [Counter Example 101](http://spec.graphql.org/June2018/#example-2353b))md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Message"))),
		std::make_shared<schema::Field>(R"gql(disallowedSecondRootField)gql"sv, R"md(Support for [Counter Example 99](http://spec.graphql.org/June2018/#example-3997d) - [Counter Example 100](http://spec.graphql.org/June2018/#example-18466))md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeMessage->AddFields({
		std::make_shared<schema::Field>(R"gql(body)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("String")),
		std::make_shared<schema::Field>(R"gql(sender)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")))
	});
	typeArguments->AddFields({
		std::make_shared<schema::Field>(R"gql(multipleReqs)gql"sv, R"md(Support for [Example 121](http://spec.graphql.org/June2018/#example-18fab))md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(x)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int")), R"gql()gql"sv),
			std::make_shared<schema::InputValue>(R"gql(y)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int")), R"gql()gql"sv)
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int"))),
		std::make_shared<schema::Field>(R"gql(booleanArgField)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(booleanArg)gql"sv, R"md()md"sv, schema->LookupType("Boolean"), R"gql()gql"sv)
		}), schema->LookupType("Boolean")),
		std::make_shared<schema::Field>(R"gql(floatArgField)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(floatArg)gql"sv, R"md()md"sv, schema->LookupType("Float"), R"gql()gql"sv)
		}), schema->LookupType("Float")),
		std::make_shared<schema::Field>(R"gql(intArgField)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(intArg)gql"sv, R"md()md"sv, schema->LookupType("Int"), R"gql()gql"sv)
		}), schema->LookupType("Int")),
		std::make_shared<schema::Field>(R"gql(nonNullBooleanArgField)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(nonNullBooleanArg)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")), R"gql()gql"sv)
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<schema::Field>(R"gql(nonNullBooleanListField)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(nonNullBooleanListArg)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))), R"gql()gql"sv)
		}), schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")))),
		std::make_shared<schema::Field>(R"gql(booleanListArgField)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(booleanListArg)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("Boolean"))), R"gql()gql"sv)
		}), schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("Boolean"))),
		std::make_shared<schema::Field>(R"gql(optionalNonNullBooleanArgField)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>({
			std::make_shared<schema::InputValue>(R"gql(optionalBooleanArg)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")), R"gql(false)gql"sv)
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});

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
		schema = std::make_shared<schema::Schema>(false);
		introspection::AddTypesToSchema(schema);
		AddTypesToSchema(schema);
		s_wpSchema = schema;
	}

	return schema;
}

} /* namespace validation */
} /* namespace graphql */
