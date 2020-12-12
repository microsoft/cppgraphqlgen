// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayObjects.h"

#include "graphqlservice/introspection/Introspection.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

namespace graphql::today {
namespace object {

Folder::Folder()
	: service::Object({
		"Node",
		"UnionType",
		"Folder"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(id)gql"sv, [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(unreadCount)gql"sv, [this](service::ResolverParams&& params) { return resolveUnreadCount(std::move(params)); } }
	})
{
}

service::FieldResult<response::IdType> Folder::getId(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Folder::getId is not implemented)ex");
}

std::future<response::Value> Folder::resolveId(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> Folder::getName(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Folder::getName is not implemented)ex");
}

std::future<response::Value> Folder::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::IntType> Folder::getUnreadCount(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Folder::getUnreadCount is not implemented)ex");
}

std::future<response::Value> Folder::resolveUnreadCount(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getUnreadCount(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Folder::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Folder)gql" }, std::move(params));
}

} /* namespace object */

void AddFolderDetails(std::shared_ptr<schema::ObjectType> typeFolder, const std::shared_ptr<schema::Schema>& schema)
{
	typeFolder->AddInterfaces({
		std::static_pointer_cast<schema::InterfaceType>(schema->LookupType(R"gql(Node)gql"sv))
	});
	typeFolder->AddFields({
		std::make_shared<schema::Field>(R"gql(id)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<schema::Field>(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("String")),
		std::make_shared<schema::Field>(R"gql(unreadCount)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int")))
	});
}

} /* namespace graphql::today */
