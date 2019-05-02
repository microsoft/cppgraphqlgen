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

FolderConnection::FolderConnection()
	: service::Object({
		"FolderConnection"
	}, {
		{ "pageInfo", [this](service::ResolverParams&& params) { return resolvePageInfo(std::move(params)); } },
		{ "edges", [this](service::ResolverParams&& params) { return resolveEdges(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<PageInfo>> FolderConnection::getPageInfo(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(FolderConnection::getPageInfo is not implemented)ex");
}

std::future<response::Value> FolderConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<FolderEdge>>>> FolderConnection::getEdges(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(FolderConnection::getEdges is not implemented)ex");
}

std::future<response::Value> FolderConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<FolderEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> FolderConnection::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(FolderConnection)gql" }, std::move(params));
}

} /* namespace object */

void AddFolderConnectionDetails(std::shared_ptr<introspection::ObjectType> typeFolderConnection, std::shared_ptr<introspection::Schema> schema)
{
	typeFolderConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("FolderEdge"))))
	});
}

} /* namespace graphql::today */
