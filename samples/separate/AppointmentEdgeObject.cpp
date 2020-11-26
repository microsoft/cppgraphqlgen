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

AppointmentEdge::AppointmentEdge()
	: service::Object({
		"AppointmentEdge"
	}, {
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ "cursor", [this](service::ResolverParams&& params) { return resolveCursor(std::move(params)); } },
		{ "node", [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<Appointment>> AppointmentEdge::getNode(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(AppointmentEdge::getNode is not implemented)ex");
}

std::future<response::Value> AppointmentEdge::resolveNode(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::Value> AppointmentEdge::getCursor(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(AppointmentEdge::getCursor is not implemented)ex");
}

std::future<response::Value> AppointmentEdge::resolveCursor(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> AppointmentEdge::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(AppointmentEdge)gql" }, std::move(params));
}

} /* namespace object */

void AddAppointmentEdgeDetails(std::shared_ptr<introspection::ObjectType> typeAppointmentEdge, const std::shared_ptr<introspection::Schema>& schema)
{
	typeAppointmentEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
}

} /* namespace graphql::today */
