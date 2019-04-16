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

NestedType::NestedType()
	: service::Object({
		"NestedType"
	}, {
		{ "depth", [this](service::ResolverParams&& params) { return resolveDepth(std::move(params)); } },
		{ "nested", [this](service::ResolverParams&& params) { return resolveNested(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

std::future<response::IntType> NestedType::getDepth(service::FieldParams&&) const
{
	std::promise<response::IntType> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(NestedType::getDepth is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> NestedType::resolveDepth(service::ResolverParams&& params)
{
	auto result = getDepth(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

std::future<std::shared_ptr<NestedType>> NestedType::getNested(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<NestedType>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(NestedType::getNested is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> NestedType::resolveNested(service::ResolverParams&& params)
{
	auto result = getNested(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<NestedType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> NestedType::resolve_typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("NestedType");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

} /* namespace object */

void AddNestedTypeDetails(std::shared_ptr<introspection::ObjectType> typeNestedType, std::shared_ptr<introspection::Schema> schema)
{
	typeNestedType->AddFields({
		std::make_shared<introspection::Field>("depth", R"md(Depth of the nested element)md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int"))),
		std::make_shared<introspection::Field>("nested", R"md(Link to the next level)md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("NestedType")))
	});
}

} /* namespace facebook::graphql::today */
