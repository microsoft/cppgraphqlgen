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

Query::Query()
	: service::Object({
		"Query"
	}, {
		{ "node", [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } },
		{ "appointments", [this](service::ResolverParams&& params) { return resolveAppointments(std::move(params)); } },
		{ "tasks", [this](service::ResolverParams&& params) { return resolveTasks(std::move(params)); } },
		{ "unreadCounts", [this](service::ResolverParams&& params) { return resolveUnreadCounts(std::move(params)); } },
		{ "appointmentsById", [this](service::ResolverParams&& params) { return resolveAppointmentsById(std::move(params)); } },
		{ "tasksById", [this](service::ResolverParams&& params) { return resolveTasksById(std::move(params)); } },
		{ "unreadCountsById", [this](service::ResolverParams&& params) { return resolveUnreadCountsById(std::move(params)); } },
		{ "nested", [this](service::ResolverParams&& params) { return resolveNested(std::move(params)); } },
		{ "unimplemented", [this](service::ResolverParams&& params) { return resolveUnimplemented(std::move(params)); } },
		{ "expensive", [this](service::ResolverParams&& params) { return resolveExpensive(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ "__schema", [this](service::ResolverParams&& params) { return resolve_schema(std::move(params)); } },
		{ "__type", [this](service::ResolverParams&& params) { return resolve_type(std::move(params)); } }
	})
	, _schema(std::make_shared<introspection::Schema>())
{
	introspection::AddTypesToSchema(_schema);
	today::AddTypesToSchema(_schema);
}

service::FieldResult<std::shared_ptr<service::Object>> Query::getNode(service::FieldParams&&, response::IdType&&) const
{
	throw std::runtime_error(R"ex(Query::getNode is not implemented)ex");
}

std::future<response::Value> Query::resolveNode(service::ResolverParams&& params)
{
	auto argId = service::ModifiedArgument<response::IdType>::require("id", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argId));
	resolverLock.unlock();

	return service::ModifiedResult<service::Object>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<AppointmentConnection>> Query::getAppointments(service::FieldParams&&, std::optional<response::IntType>&&, std::optional<response::Value>&&, std::optional<response::IntType>&&, std::optional<response::Value>&&) const
{
	throw std::runtime_error(R"ex(Query::getAppointments is not implemented)ex");
}

std::future<response::Value> Query::resolveAppointments(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getAppointments(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));
	resolverLock.unlock();

	return service::ModifiedResult<AppointmentConnection>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<TaskConnection>> Query::getTasks(service::FieldParams&&, std::optional<response::IntType>&&, std::optional<response::Value>&&, std::optional<response::IntType>&&, std::optional<response::Value>&&) const
{
	throw std::runtime_error(R"ex(Query::getTasks is not implemented)ex");
}

std::future<response::Value> Query::resolveTasks(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getTasks(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));
	resolverLock.unlock();

	return service::ModifiedResult<TaskConnection>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<FolderConnection>> Query::getUnreadCounts(service::FieldParams&&, std::optional<response::IntType>&&, std::optional<response::Value>&&, std::optional<response::IntType>&&, std::optional<response::Value>&&) const
{
	throw std::runtime_error(R"ex(Query::getUnreadCounts is not implemented)ex");
}

std::future<response::Value> Query::resolveUnreadCounts(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getUnreadCounts(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));
	resolverLock.unlock();

	return service::ModifiedResult<FolderConnection>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::vector<std::shared_ptr<Appointment>>> Query::getAppointmentsById(service::FieldParams&&, std::vector<response::IdType>&&) const
{
	throw std::runtime_error(R"ex(Query::getAppointmentsById is not implemented)ex");
}

std::future<response::Value> Query::resolveAppointmentsById(service::ResolverParams&& params)
{
	const auto defaultArguments = []()
	{
		response::Value values(response::Type::Map);
		response::Value entry;

		entry = []()
		{
			response::Value elements(response::Type::List);
			response::Value entry;

			entry = response::Value(std::string(R"gql(ZmFrZUFwcG9pbnRtZW50SWQ=)gql"));
			elements.emplace_back(std::move(entry));
			return elements;
		}();
		values.emplace_back("ids", std::move(entry));

		return values;
	}();

	auto pairIds = service::ModifiedArgument<response::IdType>::find<service::TypeModifier::List>("ids", params.arguments);
	auto argIds = (pairIds.second
		? std::move(pairIds.first)
		: service::ModifiedArgument<response::IdType>::require<service::TypeModifier::List>("ids", defaultArguments));
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getAppointmentsById(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIds));
	resolverLock.unlock();

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::vector<std::shared_ptr<Task>>> Query::getTasksById(service::FieldParams&&, std::vector<response::IdType>&&) const
{
	throw std::runtime_error(R"ex(Query::getTasksById is not implemented)ex");
}

std::future<response::Value> Query::resolveTasksById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<response::IdType>::require<service::TypeModifier::List>("ids", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getTasksById(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIds));
	resolverLock.unlock();

	return service::ModifiedResult<Task>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::vector<std::shared_ptr<Folder>>> Query::getUnreadCountsById(service::FieldParams&&, std::vector<response::IdType>&&) const
{
	throw std::runtime_error(R"ex(Query::getUnreadCountsById is not implemented)ex");
}

std::future<response::Value> Query::resolveUnreadCountsById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<response::IdType>::require<service::TypeModifier::List>("ids", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getUnreadCountsById(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIds));
	resolverLock.unlock();

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<NestedType>> Query::getNested(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Query::getNested is not implemented)ex");
}

std::future<response::Value> Query::resolveNested(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNested(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<NestedType>::convert(std::move(result), std::move(params));
}

service::FieldResult<response::StringType> Query::getUnimplemented(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Query::getUnimplemented is not implemented)ex");
}

std::future<response::Value> Query::resolveUnimplemented(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getUnimplemented(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::vector<std::shared_ptr<Expensive>>> Query::getExpensive(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Query::getExpensive is not implemented)ex");
}

std::future<response::Value> Query::resolveExpensive(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getExpensive(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Expensive>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

std::future<response::Value> Query::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Query)gql" }, std::move(params));
}

std::future<response::Value> Query::resolve_schema(service::ResolverParams&& params)
{
	return service::ModifiedResult<service::Object>::convert(std::static_pointer_cast<service::Object>(_schema), std::move(params));
}

std::future<response::Value> Query::resolve_type(service::ResolverParams&& params)
{
	auto argName = service::ModifiedArgument<response::StringType>::require("name", params.arguments);

	return service::ModifiedResult<introspection::object::Type>::convert<service::TypeModifier::Nullable>(_schema->LookupType(argName), std::move(params));
}

} /* namespace object */

void AddQueryDetails(std::shared_ptr<introspection::ObjectType> typeQuery, const std::shared_ptr<introspection::Schema>& schema)
{
	typeQuery->AddFields({
		std::make_shared<introspection::Field>("node", R"md([Object Identification](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#object-identification))md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("id", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")), R"gql()gql")
		}), schema->LookupType("Node")),
		std::make_shared<introspection::Field>("appointments", R"md(Appointments [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("AppointmentConnection"))),
		std::make_shared<introspection::Field>("tasks", R"md(Tasks [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("TaskConnection"))),
		std::make_shared<introspection::Field>("unreadCounts", R"md(Folder unread counts [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("FolderConnection"))),
		std::make_shared<introspection::Field>("appointmentsById", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")))), R"gql(["ZmFrZUFwcG9pbnRtZW50SWQ="])gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("Appointment")))),
		std::make_shared<introspection::Field>("tasksById", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")))), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("Task")))),
		std::make_shared<introspection::Field>("unreadCountsById", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")))), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("Folder")))),
		std::make_shared<introspection::Field>("nested", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("NestedType"))),
		std::make_shared<introspection::Field>("unimplemented", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String"))),
		std::make_shared<introspection::Field>("expensive", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Expensive")))))
	});
}

} /* namespace graphql::today */
