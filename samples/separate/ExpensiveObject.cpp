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

Expensive::Expensive()
	: service::Object({
		"Expensive"
	}, {
		{ "order", [this](service::ResolverParams&& params) { return resolveOrder(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

service::FieldResult<response::IntType> Expensive::getOrder(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Expensive::getOrder is not implemented)ex");
}

std::future<response::Value> Expensive::resolveOrder(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getOrder(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Expensive::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Expensive)gql" }, std::move(params));
}

} /* namespace object */

void AddExpensiveDetails(std::shared_ptr<introspection::ObjectType> typeExpensive, const std::shared_ptr<introspection::Schema>& schema)
{
	typeExpensive->AddFields({
		std::make_shared<introspection::Field>("order", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int")))
	});
}

} /* namespace graphql::today */
