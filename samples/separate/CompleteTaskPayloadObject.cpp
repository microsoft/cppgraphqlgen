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

CompleteTaskPayload::CompleteTaskPayload()
	: service::Object({
		"CompleteTaskPayload"
	}, {
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ "clientMutationId", [this](service::ResolverParams&& params) { return resolveClientMutationId(std::move(params)); } },
		{ "task", [this](service::ResolverParams&& params) { return resolveTask(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<Task>> CompleteTaskPayload::getTask(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(CompleteTaskPayload::getTask is not implemented)ex");
}

std::future<response::Value> CompleteTaskPayload::resolveTask(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getTask(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> CompleteTaskPayload::getClientMutationId(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(CompleteTaskPayload::getClientMutationId is not implemented)ex");
}

std::future<response::Value> CompleteTaskPayload::resolveClientMutationId(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getClientMutationId(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> CompleteTaskPayload::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(CompleteTaskPayload)gql" }, std::move(params));
}

} /* namespace object */

void AddCompleteTaskPayloadDetails(std::shared_ptr<introspection::ObjectType> typeCompleteTaskPayload, const std::shared_ptr<introspection::Schema>& schema)
{
	typeCompleteTaskPayload->AddFields({
		std::make_shared<introspection::Field>("task", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("clientMutationId", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
}

} /* namespace graphql::today */
