// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayObjects.h"

#include "graphqlservice/Introspection.h"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <unordered_map>

namespace graphql::today {
namespace object {

Task::Task()
	: service::Object({
		"Node",
		"UnionType",
		"Task"
	}, {
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ "id", [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ "isComplete", [this](service::ResolverParams&& params) { return resolveIsComplete(std::move(params)); } },
		{ "title", [this](service::ResolverParams&& params) { return resolveTitle(std::move(params)); } }
	})
{
}

service::FieldResult<response::IdType> Task::getId(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Task::getId is not implemented)ex");
}

std::future<response::Value> Task::resolveId(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> Task::getTitle(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Task::getTitle is not implemented)ex");
}

std::future<response::Value> Task::resolveTitle(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getTitle(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> Task::getIsComplete(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Task::getIsComplete is not implemented)ex");
}

std::future<response::Value> Task::resolveIsComplete(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getIsComplete(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Task::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Task)gql" }, std::move(params));
}

} /* namespace object */

void AddTaskDetails(std::shared_ptr<introspection::ObjectType> typeTask, const std::shared_ptr<introspection::Schema>& schema)
{
	typeTask->AddInterfaces({
		std::static_pointer_cast<introspection::InterfaceType>(schema->LookupType("Node"))
	});
	typeTask->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("title", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isComplete", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
}

} /* namespace graphql::today */
