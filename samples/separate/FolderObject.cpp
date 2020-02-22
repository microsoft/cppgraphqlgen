// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayObjects.h"

#include <graphqlservice/Introspection.h>

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <exception>

namespace graphql::today {
namespace object {

Folder::Folder()
	: service::Object({
		"Node",
		"Folder"
	}, {
		{ "id", [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ "name", [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ "unreadCount", [this](service::ResolverParams&& params) { return resolveUnreadCount(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

service::FieldResult<response::IdType> Folder::getId(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Folder::getId is not implemented)ex");
}

std::future<response::Value> Folder::resolveId(service::ResolverParams&& params)
{
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> Folder::getName(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Folder::getName is not implemented)ex");
}

std::future<response::Value> Folder::resolveName(service::ResolverParams&& params)
{
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::IntType> Folder::getUnreadCount(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Folder::getUnreadCount is not implemented)ex");
}

std::future<response::Value> Folder::resolveUnreadCount(service::ResolverParams&& params)
{
	auto result = getUnreadCount(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Folder::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Folder)gql" }, std::move(params));
}

} /* namespace object */

void AddFolderDetails(std::shared_ptr<introspection::ObjectType> typeFolder, const std::shared_ptr<introspection::Schema>& schema)
{
	typeFolder->AddInterfaces({
		std::static_pointer_cast<introspection::InterfaceType>(schema->LookupType("Node"))
	});
	typeFolder->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("name", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("unreadCount", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int")))
	});
}

} /* namespace graphql::today */
