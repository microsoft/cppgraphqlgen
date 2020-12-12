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

FolderEdge::FolderEdge()
	: service::Object({
		"FolderEdge"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(cursor)gql"sv, [this](service::ResolverParams&& params) { return resolveCursor(std::move(params)); } },
		{ R"gql(node)gql"sv, [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<Folder>> FolderEdge::getNode(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(FolderEdge::getNode is not implemented)ex");
}

std::future<response::Value> FolderEdge::resolveNode(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::Value> FolderEdge::getCursor(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(FolderEdge::getCursor is not implemented)ex");
}

std::future<response::Value> FolderEdge::resolveCursor(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> FolderEdge::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(FolderEdge)gql" }, std::move(params));
}

} /* namespace object */

void AddFolderEdgeDetails(std::shared_ptr<schema::ObjectType> typeFolderEdge, const std::shared_ptr<schema::Schema>& schema)
{
	typeFolderEdge->AddFields({
		std::make_shared<schema::Field>(R"gql(node)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("Folder")),
		std::make_shared<schema::Field>(R"gql(cursor)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
}

} /* namespace graphql::today */
