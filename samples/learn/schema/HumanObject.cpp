// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "HumanObject.h"
#include "CharacterObject.h"

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

namespace graphql::learn {
namespace object {

Human::Human(std::unique_ptr<const Concept> pimpl) noexcept
	: service::Object{ getTypeNames(), getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}

service::TypeNames Human::getTypeNames() const noexcept
{
	return {
		R"gql(Character)gql"sv,
		R"gql(Human)gql"sv
	};
}

service::ResolverMap Human::getResolvers() const noexcept
{
	return {
		{ R"gql(id)gql"sv, [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(friends)gql"sv, [this](service::ResolverParams&& params) { return resolveFriends(std::move(params)); } },
		{ R"gql(appearsIn)gql"sv, [this](service::ResolverParams&& params) { return resolveAppearsIn(std::move(params)); } },
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(homePlanet)gql"sv, [this](service::ResolverParams&& params) { return resolveHomePlanet(std::move(params)); } }
	};
}

void Human::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void Human::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}

service::AwaitableResolver Human::resolveId(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getId(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Human::resolveName(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getName(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Human::resolveFriends(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getFriends(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<Character>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Human::resolveAppearsIn(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getAppearsIn(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<Episode>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Human::resolveHomePlanet(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getHomePlanet(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Human::resolve_typename(service::ResolverParams&& params) const
{
	return service::Result<std::string>::convert(std::string{ R"gql(Human)gql" }, std::move(params));
}

} // namespace object

void AddHumanDetails(const std::shared_ptr<schema::ObjectType>& typeHuman, const std::shared_ptr<schema::Schema>& schema)
{
	typeHuman->AddInterfaces({
		std::static_pointer_cast<const schema::InterfaceType>(schema->LookupType(R"gql(Character)gql"sv))
	});
	typeHuman->AddFields({
		schema::Field::Make(R"gql(id)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv))),
		schema::Field::Make(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType(R"gql(String)gql"sv)),
		schema::Field::Make(R"gql(friends)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType(R"gql(Character)gql"sv))),
		schema::Field::Make(R"gql(appearsIn)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType(R"gql(Episode)gql"sv))),
		schema::Field::Make(R"gql(homePlanet)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType(R"gql(String)gql"sv))
	});
}

} // namespace graphql::learn
