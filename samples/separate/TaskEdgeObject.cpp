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

TaskEdge::TaskEdge()
	: service::Object({
		"TaskEdge"
	}, {
		{ "node", [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } },
		{ "cursor", [this](service::ResolverParams&& params) { return resolveCursor(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<Task>> TaskEdge::getNode(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(TaskEdge::getNode is not implemented)ex");
}

std::future<response::Value> TaskEdge::resolveNode(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::Value> TaskEdge::getCursor(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(TaskEdge::getCursor is not implemented)ex");
}

std::future<response::Value> TaskEdge::resolveCursor(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> TaskEdge::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(TaskEdge)gql" }, std::move(params));
}

} /* namespace object */

void AddTaskEdgeDetails(std::shared_ptr<introspection::ObjectType> typeTaskEdge, const std::shared_ptr<introspection::Schema>& schema)
{
	typeTaskEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
}

} /* namespace graphql::today */
