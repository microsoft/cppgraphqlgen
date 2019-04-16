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

AppointmentConnection::AppointmentConnection()
	: service::Object({
		"AppointmentConnection"
	}, {
		{ "pageInfo", [this](service::ResolverParams&& params) { return resolvePageInfo(std::move(params)); } },
		{ "edges", [this](service::ResolverParams&& params) { return resolveEdges(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<PageInfo>> AppointmentConnection::getPageInfo(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<PageInfo>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(AppointmentConnection::getPageInfo is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> AppointmentConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

std::future<std::optional<std::vector<std::shared_ptr<AppointmentEdge>>>> AppointmentConnection::getEdges(service::FieldParams&&) const
{
	std::promise<std::optional<std::vector<std::shared_ptr<AppointmentEdge>>>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(AppointmentConnection::getEdges is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> AppointmentConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<AppointmentEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> AppointmentConnection::resolve_typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("AppointmentConnection");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

} /* namespace object */

void AddAppointmentConnectionDetails(std::shared_ptr<introspection::ObjectType> typeAppointmentConnection, std::shared_ptr<introspection::Schema> schema)
{
	typeAppointmentConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("AppointmentEdge"))))
	});
}

} /* namespace facebook::graphql::today */
