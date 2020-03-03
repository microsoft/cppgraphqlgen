// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef VALIDATIONSCHEMA_H
#define VALIDATIONSCHEMA_H

#include <graphqlservice/GraphQLService.h>

#include <memory>
#include <string>
#include <vector>

namespace graphql {
namespace introspection {

class Schema;

} /* namespace introspection */

namespace validation {

enum class DogCommand
{
	SIT,
	DOWN,
	HEEL
};

enum class CatCommand
{
	JUMP
};

struct MutateDogInput
{
	response::IdType id;
};

namespace object {

class Query;
class Mutation;
class Subscription;
class Message;
class Dog;
class Alien;
class Human;
class Cat;

} /* namespace object */

struct Sentient;
struct Pet;

struct Sentient
{
	virtual service::FieldResult<response::StringType> getName(service::FieldParams&& params) const = 0;
};

struct Pet
{
	virtual service::FieldResult<response::StringType> getName(service::FieldParams&& params) const = 0;
};

namespace object {

class Query
	: public service::Object
{
protected:
	explicit Query();

public:
	virtual service::FieldResult<std::shared_ptr<Dog>> getDog(service::FieldParams&& params) const;
	virtual service::FieldResult<std::shared_ptr<Human>> getHuman(service::FieldParams&& params) const;
	virtual service::FieldResult<std::shared_ptr<service::Object>> getPet(service::FieldParams&& params) const;
	virtual service::FieldResult<std::shared_ptr<service::Object>> getCatOrDog(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveDog(service::ResolverParams&& params);
	std::future<response::Value> resolveHuman(service::ResolverParams&& params);
	std::future<response::Value> resolvePet(service::ResolverParams&& params);
	std::future<response::Value> resolveCatOrDog(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
	std::future<response::Value> resolve_schema(service::ResolverParams&& params);
	std::future<response::Value> resolve_type(service::ResolverParams&& params);

	std::shared_ptr<introspection::Schema> _schema;
};

class Mutation
	: public service::Object
{
protected:
	explicit Mutation();

public:
	virtual service::FieldResult<std::shared_ptr<Dog>> applyMutateDog(service::FieldParams&& params, MutateDogInput&& inputArg) const;

private:
	std::future<response::Value> resolveMutateDog(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Subscription
	: public service::Object
{
protected:
	explicit Subscription();

public:
	virtual service::FieldResult<std::shared_ptr<Message>> getNewMessage(service::FieldParams&& params) const;
	virtual service::FieldResult<response::BooleanType> getDisallowedSecondRootField(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveNewMessage(service::ResolverParams&& params);
	std::future<response::Value> resolveDisallowedSecondRootField(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Message
	: public service::Object
{
protected:
	explicit Message();

public:
	virtual service::FieldResult<std::optional<response::StringType>> getBody(service::FieldParams&& params) const;
	virtual service::FieldResult<response::IdType> getSender(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveBody(service::ResolverParams&& params);
	std::future<response::Value> resolveSender(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Dog
	: public service::Object
	, public Pet
{
protected:
	explicit Dog();

public:
	virtual service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;
	virtual service::FieldResult<std::optional<response::StringType>> getNickname(service::FieldParams&& params) const;
	virtual service::FieldResult<std::optional<response::IntType>> getBarkVolume(service::FieldParams&& params) const;
	virtual service::FieldResult<response::BooleanType> getDoesKnowCommand(service::FieldParams&& params, DogCommand&& dogCommandArg) const;
	virtual service::FieldResult<response::BooleanType> getIsHousetrained(service::FieldParams&& params, std::optional<response::BooleanType>&& atOtherHomesArg) const;
	virtual service::FieldResult<std::shared_ptr<Human>> getOwner(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveNickname(service::ResolverParams&& params);
	std::future<response::Value> resolveBarkVolume(service::ResolverParams&& params);
	std::future<response::Value> resolveDoesKnowCommand(service::ResolverParams&& params);
	std::future<response::Value> resolveIsHousetrained(service::ResolverParams&& params);
	std::future<response::Value> resolveOwner(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Alien
	: public service::Object
	, public Sentient
{
protected:
	explicit Alien();

public:
	virtual service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;
	virtual service::FieldResult<std::optional<response::StringType>> getHomePlanet(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveHomePlanet(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Human
	: public service::Object
	, public Sentient
{
protected:
	explicit Human();

public:
	virtual service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;

private:
	std::future<response::Value> resolveName(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Cat
	: public service::Object
	, public Pet
{
protected:
	explicit Cat();

public:
	virtual service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;
	virtual service::FieldResult<std::optional<response::StringType>> getNickname(service::FieldParams&& params) const;
	virtual service::FieldResult<response::BooleanType> getDoesKnowCommand(service::FieldParams&& params, CatCommand&& catCommandArg) const;
	virtual service::FieldResult<std::optional<response::IntType>> getMeowVolume(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveNickname(service::ResolverParams&& params);
	std::future<response::Value> resolveDoesKnowCommand(service::ResolverParams&& params);
	std::future<response::Value> resolveMeowVolume(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace object */

class Operations
	: public service::Request
{
public:
	explicit Operations(std::shared_ptr<object::Query> query, std::shared_ptr<object::Mutation> mutation, std::shared_ptr<object::Subscription> subscription);

private:
	std::shared_ptr<object::Query> _query;
	std::shared_ptr<object::Mutation> _mutation;
	std::shared_ptr<object::Subscription> _subscription;
};

void AddTypesToSchema(const std::shared_ptr<introspection::Schema>& schema);

} /* namespace validation */
} /* namespace graphql */

#endif // VALIDATIONSCHEMA_H
