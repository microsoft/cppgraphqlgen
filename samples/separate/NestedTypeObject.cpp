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

service::FieldResult<response::IntType> NestedType::getDepth(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(NestedType::getDepth is not implemented)ex");
}

std::future<response::Value> NestedType::resolveDepth(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDepth(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<NestedType>> NestedType::getNested(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(NestedType::getNested is not implemented)ex");
}

std::future<response::Value> NestedType::resolveNested(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNested(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<NestedType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> NestedType::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(NestedType)gql" }, std::move(params));
}

} /* namespace object */

void AddNestedTypeDetails(std::shared_ptr<introspection::ObjectType> typeNestedType, const std::shared_ptr<introspection::Schema>& schema)
{
	typeNestedType->AddFields({
		std::make_shared<introspection::Field>("depth", R"md(Depth of the nested element)md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int"))),
		std::make_shared<introspection::Field>("nested", R"md(Link to the next level)md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("NestedType")))
	});
}

} /* namespace graphql::today */
