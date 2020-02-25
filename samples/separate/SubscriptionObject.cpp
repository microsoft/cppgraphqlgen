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

Subscription::Subscription()
	: service::Object({
		"Subscription"
	}, {
		{ "nextAppointmentChange", [this](service::ResolverParams&& params) { return resolveNextAppointmentChange(std::move(params)); } },
		{ "nodeChange", [this](service::ResolverParams&& params) { return resolveNodeChange(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<Appointment>> Subscription::getNextAppointmentChange(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Subscription::getNextAppointmentChange is not implemented)ex");
}

std::future<response::Value> Subscription::resolveNextAppointmentChange(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNextAppointmentChange(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<service::Object>> Subscription::getNodeChange(service::FieldParams&&, response::IdType&&) const
{
	throw std::runtime_error(R"ex(Subscription::getNodeChange is not implemented)ex");
}

std::future<response::Value> Subscription::resolveNodeChange(service::ResolverParams&& params)
{
	auto argId = service::ModifiedArgument<response::IdType>::require("id", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNodeChange(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argId));
	resolverLock.unlock();

	return service::ModifiedResult<service::Object>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Subscription::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Subscription)gql" }, std::move(params));
}

} /* namespace object */

void AddSubscriptionDetails(std::shared_ptr<introspection::ObjectType> typeSubscription, const std::shared_ptr<introspection::Schema>& schema)
{
	typeSubscription->AddFields({
		std::make_shared<introspection::Field>("nextAppointmentChange", R"md()md", std::make_optional<response::StringType>(R"md(Need to deprecate a [field](https://facebook.github.io/graphql/June2018/#sec-Deprecation))md"), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment")),
		std::make_shared<introspection::Field>("nodeChange", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("id", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Node")))
	});
}

} /* namespace graphql::today */
