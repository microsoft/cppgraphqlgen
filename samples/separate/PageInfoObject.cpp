// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayObjects.h"

#include "graphqlservice/Introspection.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

namespace graphql::today {
namespace object {

PageInfo::PageInfo()
	: service::Object({
		"PageInfo"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(hasNextPage)gql"sv, [this](service::ResolverParams&& params) { return resolveHasNextPage(std::move(params)); } },
		{ R"gql(hasPreviousPage)gql"sv, [this](service::ResolverParams&& params) { return resolveHasPreviousPage(std::move(params)); } }
	})
{
}

service::FieldResult<response::BooleanType> PageInfo::getHasNextPage(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(PageInfo::getHasNextPage is not implemented)ex");
}

std::future<response::Value> PageInfo::resolveHasNextPage(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getHasNextPage(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> PageInfo::getHasPreviousPage(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(PageInfo::getHasPreviousPage is not implemented)ex");
}

std::future<response::Value> PageInfo::resolveHasPreviousPage(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getHasPreviousPage(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> PageInfo::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(PageInfo)gql" }, std::move(params));
}

} /* namespace object */

void AddPageInfoDetails(std::shared_ptr<schema::ObjectType> typePageInfo, const std::shared_ptr<schema::Schema>& schema)
{
	typePageInfo->AddFields({
		std::make_shared<schema::Field>(R"gql(hasNextPage)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<schema::Field>(R"gql(hasPreviousPage)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
}

} /* namespace graphql::today */
