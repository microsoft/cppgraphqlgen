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

AppointmentEdge::AppointmentEdge()
	: service::Object({
		"AppointmentEdge"
	}, {
		{ "node", [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } },
		{ "cursor", [this](service::ResolverParams&& params) { return resolveCursor(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<Appointment>> AppointmentEdge::getNode(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<Appointment>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(AppointmentEdge::getNode is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> AppointmentEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> AppointmentEdge::getCursor(service::FieldParams&&) const
{
	std::promise<response::Value> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(AppointmentEdge::getCursor is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> AppointmentEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> AppointmentEdge::resolve_typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("AppointmentEdge");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

} /* namespace object */

void AddAppointmentEdgeDetails(std::shared_ptr<introspection::ObjectType> typeAppointmentEdge, std::shared_ptr<introspection::Schema> schema)
{
	typeAppointmentEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
}

} /* namespace facebook::graphql::today */
