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

Appointment::Appointment()
	: service::Object({
		"Node",
		"UnionType",
		"Appointment"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(id)gql"sv, [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ R"gql(isNow)gql"sv, [this](service::ResolverParams&& params) { return resolveIsNow(std::move(params)); } },
		{ R"gql(subject)gql"sv, [this](service::ResolverParams&& params) { return resolveSubject(std::move(params)); } },
		{ R"gql(when)gql"sv, [this](service::ResolverParams&& params) { return resolveWhen(std::move(params)); } }
	})
{
}

service::FieldResult<response::IdType> Appointment::getId(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Appointment::getId is not implemented)ex");
}

std::future<response::Value> Appointment::resolveId(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::Value>> Appointment::getWhen(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Appointment::getWhen is not implemented)ex");
}

std::future<response::Value> Appointment::resolveWhen(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getWhen(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::Value>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> Appointment::getSubject(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Appointment::getSubject is not implemented)ex");
}

std::future<response::Value> Appointment::resolveSubject(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getSubject(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> Appointment::getIsNow(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Appointment::getIsNow is not implemented)ex");
}

std::future<response::Value> Appointment::resolveIsNow(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getIsNow(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Appointment::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Appointment)gql" }, std::move(params));
}

} /* namespace object */

void AddAppointmentDetails(std::shared_ptr<schema::ObjectType> typeAppointment, const std::shared_ptr<schema::Schema>& schema)
{
	typeAppointment->AddInterfaces({
		std::static_pointer_cast<schema::InterfaceType>(schema->LookupType(R"gql(Node)gql"sv))
	});
	typeAppointment->AddFields({
		std::make_shared<schema::Field>(R"gql(id)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<schema::Field>(R"gql(when)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("DateTime")),
		std::make_shared<schema::Field>(R"gql(subject)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->LookupType("String")),
		std::make_shared<schema::Field>(R"gql(isNow)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
}

} /* namespace graphql::today */
