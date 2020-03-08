// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayObjects.h"

#include "graphqlservice/Introspection.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <exception>

namespace graphql::today {
namespace object {

TaskConnection::TaskConnection()
	: service::Object({
		"TaskConnection"
	}, {
		{ "pageInfo", [this](service::ResolverParams&& params) { return resolvePageInfo(std::move(params)); } },
		{ "edges", [this](service::ResolverParams&& params) { return resolveEdges(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<PageInfo>> TaskConnection::getPageInfo(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(TaskConnection::getPageInfo is not implemented)ex");
}

std::future<response::Value> TaskConnection::resolvePageInfo(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<TaskEdge>>>> TaskConnection::getEdges(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(TaskConnection::getEdges is not implemented)ex");
}

std::future<response::Value> TaskConnection::resolveEdges(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<TaskEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> TaskConnection::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(TaskConnection)gql" }, std::move(params));
}

} /* namespace object */

void AddTaskConnectionDetails(std::shared_ptr<introspection::ObjectType> typeTaskConnection, const std::shared_ptr<introspection::Schema>& schema)
{
	typeTaskConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("TaskEdge")))
	});
}

} /* namespace graphql::today */
