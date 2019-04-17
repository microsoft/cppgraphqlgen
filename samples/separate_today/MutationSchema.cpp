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

Mutation::Mutation()
	: service::Object({
		"Mutation"
	}, {
		{ "completeTask", [this](service::ResolverParams&& params) { return resolveCompleteTask(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<CompleteTaskPayload>> Mutation::getCompleteTask(service::FieldParams&&, CompleteTaskInput&&) const
{
	std::promise<std::shared_ptr<CompleteTaskPayload>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Mutation::getCompleteTask is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Mutation::resolveCompleteTask(service::ResolverParams&& params)
{
	auto argInput = service::ModifiedArgument<CompleteTaskInput>::require("input", params.arguments);
	auto result = getCompleteTask(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argInput));

	return service::ModifiedResult<CompleteTaskPayload>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Mutation::resolve_typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("Mutation");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

} /* namespace object */

void AddMutationDetails(std::shared_ptr<introspection::ObjectType> typeMutation, std::shared_ptr<introspection::Schema> schema)
{
	typeMutation->AddFields({
		std::make_shared<introspection::Field>("completeTask", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("input", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("CompleteTaskInput")), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("CompleteTaskPayload")))
	});
}

} /* namespace facebook::graphql::today */