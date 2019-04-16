// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayObjects.h"

#include <graphqlservice/Introspection.h>

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <exception>

namespace facebook::graphql::today {
namespace object {

CompleteTaskPayload::CompleteTaskPayload()
	: service::Object({
		"CompleteTaskPayload"
	}, {
		{ "task", [this](service::ResolverParams&& params) { return resolveTask(std::move(params)); } },
		{ "clientMutationId", [this](service::ResolverParams&& params) { return resolveClientMutationId(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<Task>> CompleteTaskPayload::getTask(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<Task>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(CompleteTaskPayload::getTask is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> CompleteTaskPayload::resolveTask(service::ResolverParams&& params)
{
	auto result = getTask(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<std::optional<response::StringType>> CompleteTaskPayload::getClientMutationId(service::FieldParams&&) const
{
	std::promise<std::optional<response::StringType>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(CompleteTaskPayload::getClientMutationId is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> CompleteTaskPayload::resolveClientMutationId(service::ResolverParams&& params)
{
	auto result = getClientMutationId(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> CompleteTaskPayload::resolve_typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("CompleteTaskPayload");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

} /* namespace object */

void AddCompleteTaskPayloadDetails(std::shared_ptr<introspection::ObjectType> typeCompleteTaskPayload, std::shared_ptr<introspection::Schema> schema)
{
	typeCompleteTaskPayload->AddFields({
		std::make_shared<introspection::Field>("task", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("clientMutationId", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
}

} /* namespace facebook::graphql::today */
