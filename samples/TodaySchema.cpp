// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodaySchema.h"

#include <graphqlservice/Introspection.h>

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <exception>

namespace facebook::graphql {
namespace service {

template <>
today::TaskState ModifiedArgument<today::TaskState>::convert(const response::Value& value)
{
	static const std::unordered_map<std::string, today::TaskState> s_names = {
		{ "New", today::TaskState::New },
		{ "Started", today::TaskState::Started },
		{ "Complete", today::TaskState::Complete },
		{ "Unassigned", today::TaskState::Unassigned }
	};

	if (!value.maybe_enum())
	{
		throw service::schema_exception({ "not a valid TaskState value" });
	}

	auto itr = s_names.find(value.get<const response::StringType&>());

	if (itr == s_names.cend())
	{
		throw service::schema_exception({ "not a valid TaskState value" });
	}

	return itr->second;
}

template <>
std::future<response::Value> ModifiedResult<today::TaskState>::convert(std::future<today::TaskState>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[](today::TaskState && value, const ResolverParams&)
		{
			static const std::string s_names[] = {
				"New",
				"Started",
				"Complete",
				"Unassigned"
			};

			response::Value result(response::Type::EnumValue);

			result.set<response::StringType>(std::string(s_names[static_cast<size_t>(value)]));

			return result;
		});
}

template <>
today::CompleteTaskInput ModifiedArgument<today::CompleteTaskInput>::convert(const response::Value& value)
{
	const auto defaultValue = []()
	{
		response::Value values(response::Type::Map);
		response::Value entry;

		entry = response::Value(true);
		values.emplace_back("isComplete", std::move(entry));

		return values;
	}();

	auto valueId = service::ModifiedArgument<std::vector<uint8_t>>::require("id", value);
	auto pairIsComplete = service::ModifiedArgument<response::BooleanType>::find<service::TypeModifier::Nullable>("isComplete", value);
	auto valueIsComplete = (pairIsComplete.second
		? std::move(pairIsComplete.first)
		: service::ModifiedArgument<response::BooleanType>::require<service::TypeModifier::Nullable>("isComplete", defaultValue));
	auto valueClientMutationId = service::ModifiedArgument<response::StringType>::require<service::TypeModifier::Nullable>("clientMutationId", value);

	return {
		std::move(valueId),
		std::move(valueIsComplete),
		std::move(valueClientMutationId)
	};
}

} /* namespace service */

namespace today {
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
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } },
		{ "__schema", [this](service::ResolverParams&& params) { return resolve__schema(std::move(params)); } },
		{ "__type", [this](service::ResolverParams&& params) { return resolve__type(std::move(params)); } }
	})
	, _schema(std::make_shared<introspection::Schema>())
{
	introspection::AddTypesToSchema(_schema);
	today::AddTypesToSchema(_schema);
}

std::future<std::shared_ptr<service::Object>> Query::getNode(service::FieldParams&&, std::vector<uint8_t>&&) const
{
	std::promise<std::shared_ptr<service::Object>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Query::getNode is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Query::resolveNode(service::ResolverParams&& params)
{
	auto argId = service::ModifiedArgument<std::vector<uint8_t>>::require("id", params.arguments);
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argId));

	return service::ModifiedResult<service::Object>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<std::shared_ptr<AppointmentConnection>> Query::getAppointments(service::FieldParams&&, std::optional<response::IntType>&&, std::optional<response::Value>&&, std::optional<response::IntType>&&, std::optional<response::Value>&&) const
{
	std::promise<std::shared_ptr<AppointmentConnection>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Query::getAppointments is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Query::resolveAppointments(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	auto result = getAppointments(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));

	return service::ModifiedResult<AppointmentConnection>::convert(std::move(result), std::move(params));
}

std::future<std::shared_ptr<TaskConnection>> Query::getTasks(service::FieldParams&&, std::optional<response::IntType>&&, std::optional<response::Value>&&, std::optional<response::IntType>&&, std::optional<response::Value>&&) const
{
	std::promise<std::shared_ptr<TaskConnection>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Query::getTasks is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Query::resolveTasks(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	auto result = getTasks(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));

	return service::ModifiedResult<TaskConnection>::convert(std::move(result), std::move(params));
}

std::future<std::shared_ptr<FolderConnection>> Query::getUnreadCounts(service::FieldParams&&, std::optional<response::IntType>&&, std::optional<response::Value>&&, std::optional<response::IntType>&&, std::optional<response::Value>&&) const
{
	std::promise<std::shared_ptr<FolderConnection>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Query::getUnreadCounts is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Query::resolveUnreadCounts(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	auto result = getUnreadCounts(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));

	return service::ModifiedResult<FolderConnection>::convert(std::move(result), std::move(params));
}

std::future<std::vector<std::shared_ptr<Appointment>>> Query::getAppointmentsById(service::FieldParams&&, std::vector<std::vector<uint8_t>>&&) const
{
	std::promise<std::vector<std::shared_ptr<Appointment>>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Query::getAppointmentsById is not implemented)ex")));

	return promise.get_future();
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

	auto pairIds = service::ModifiedArgument<std::vector<uint8_t>>::find<service::TypeModifier::List>("ids", params.arguments);
	auto argIds = (pairIds.second
		? std::move(pairIds.first)
		: service::ModifiedArgument<std::vector<uint8_t>>::require<service::TypeModifier::List>("ids", defaultArguments));
	auto result = getAppointmentsById(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIds));

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<std::vector<std::shared_ptr<Task>>> Query::getTasksById(service::FieldParams&&, std::vector<std::vector<uint8_t>>&&) const
{
	std::promise<std::vector<std::shared_ptr<Task>>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Query::getTasksById is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Query::resolveTasksById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<uint8_t>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getTasksById(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIds));

	return service::ModifiedResult<Task>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<std::vector<std::shared_ptr<Folder>>> Query::getUnreadCountsById(service::FieldParams&&, std::vector<std::vector<uint8_t>>&&) const
{
	std::promise<std::vector<std::shared_ptr<Folder>>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Query::getUnreadCountsById is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Query::resolveUnreadCountsById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<uint8_t>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getUnreadCountsById(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIds));

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<std::shared_ptr<NestedType>> Query::getNested(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<NestedType>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Query::getNested is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Query::resolveNested(service::ResolverParams&& params)
{
	auto result = getNested(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<NestedType>::convert(std::move(result), std::move(params));
}

std::future<response::StringType> Query::getUnimplemented(service::FieldParams&&) const
{
	std::promise<response::StringType> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Query::getUnimplemented is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Query::resolveUnimplemented(service::ResolverParams&& params)
{
	auto result = getUnimplemented(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Query::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("Query");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

std::future<response::Value> Query::resolve__schema(service::ResolverParams&& params)
{
	std::promise<std::shared_ptr<service::Object>> promise;

	promise.set_value(std::static_pointer_cast<service::Object>(_schema));

	return service::ModifiedResult<service::Object>::convert(promise.get_future(), std::move(params));
}

std::future<response::Value> Query::resolve__type(service::ResolverParams&& params)
{
	auto argName = service::ModifiedArgument<std::string>::require("name", params.arguments);
	std::promise<std::shared_ptr<introspection::object::__Type>> promise;

	promise.set_value(_schema->LookupType(argName));

	return service::ModifiedResult<introspection::object::__Type>::convert<service::TypeModifier::Nullable>(promise.get_future(), std::move(params));
}

PageInfo::PageInfo()
	: service::Object({
		"PageInfo"
	}, {
		{ "hasNextPage", [this](service::ResolverParams&& params) { return resolveHasNextPage(std::move(params)); } },
		{ "hasPreviousPage", [this](service::ResolverParams&& params) { return resolveHasPreviousPage(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<response::BooleanType> PageInfo::getHasNextPage(service::FieldParams&&) const
{
	std::promise<response::BooleanType> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(PageInfo::getHasNextPage is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> PageInfo::resolveHasNextPage(service::ResolverParams&& params)
{
	auto result = getHasNextPage(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::BooleanType> PageInfo::getHasPreviousPage(service::FieldParams&&) const
{
	std::promise<response::BooleanType> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(PageInfo::getHasPreviousPage is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> PageInfo::resolveHasPreviousPage(service::ResolverParams&& params)
{
	auto result = getHasPreviousPage(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> PageInfo::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("PageInfo");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

AppointmentEdge::AppointmentEdge()
	: service::Object({
		"AppointmentEdge"
	}, {
		{ "node", [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } },
		{ "cursor", [this](service::ResolverParams&& params) { return resolveCursor(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
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

std::future<response::Value> AppointmentEdge::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("AppointmentEdge");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

AppointmentConnection::AppointmentConnection()
	: service::Object({
		"AppointmentConnection"
	}, {
		{ "pageInfo", [this](service::ResolverParams&& params) { return resolvePageInfo(std::move(params)); } },
		{ "edges", [this](service::ResolverParams&& params) { return resolveEdges(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
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

std::future<response::Value> AppointmentConnection::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("AppointmentConnection");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

TaskEdge::TaskEdge()
	: service::Object({
		"TaskEdge"
	}, {
		{ "node", [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } },
		{ "cursor", [this](service::ResolverParams&& params) { return resolveCursor(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<Task>> TaskEdge::getNode(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<Task>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(TaskEdge::getNode is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> TaskEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> TaskEdge::getCursor(service::FieldParams&&) const
{
	std::promise<response::Value> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(TaskEdge::getCursor is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> TaskEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> TaskEdge::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("TaskEdge");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

TaskConnection::TaskConnection()
	: service::Object({
		"TaskConnection"
	}, {
		{ "pageInfo", [this](service::ResolverParams&& params) { return resolvePageInfo(std::move(params)); } },
		{ "edges", [this](service::ResolverParams&& params) { return resolveEdges(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<PageInfo>> TaskConnection::getPageInfo(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<PageInfo>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(TaskConnection::getPageInfo is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> TaskConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

std::future<std::optional<std::vector<std::shared_ptr<TaskEdge>>>> TaskConnection::getEdges(service::FieldParams&&) const
{
	std::promise<std::optional<std::vector<std::shared_ptr<TaskEdge>>>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(TaskConnection::getEdges is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> TaskConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<TaskEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> TaskConnection::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("TaskConnection");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

FolderEdge::FolderEdge()
	: service::Object({
		"FolderEdge"
	}, {
		{ "node", [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } },
		{ "cursor", [this](service::ResolverParams&& params) { return resolveCursor(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<Folder>> FolderEdge::getNode(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<Folder>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(FolderEdge::getNode is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> FolderEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> FolderEdge::getCursor(service::FieldParams&&) const
{
	std::promise<response::Value> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(FolderEdge::getCursor is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> FolderEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> FolderEdge::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("FolderEdge");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

FolderConnection::FolderConnection()
	: service::Object({
		"FolderConnection"
	}, {
		{ "pageInfo", [this](service::ResolverParams&& params) { return resolvePageInfo(std::move(params)); } },
		{ "edges", [this](service::ResolverParams&& params) { return resolveEdges(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<PageInfo>> FolderConnection::getPageInfo(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<PageInfo>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(FolderConnection::getPageInfo is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> FolderConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

std::future<std::optional<std::vector<std::shared_ptr<FolderEdge>>>> FolderConnection::getEdges(service::FieldParams&&) const
{
	std::promise<std::optional<std::vector<std::shared_ptr<FolderEdge>>>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(FolderConnection::getEdges is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> FolderConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<FolderEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> FolderConnection::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("FolderConnection");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

CompleteTaskPayload::CompleteTaskPayload()
	: service::Object({
		"CompleteTaskPayload"
	}, {
		{ "task", [this](service::ResolverParams&& params) { return resolveTask(std::move(params)); } },
		{ "clientMutationId", [this](service::ResolverParams&& params) { return resolveClientMutationId(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<Task>> CompleteTaskPayload::getTask(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<Task>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(CompleteTaskPayload::getTask is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> CompleteTaskPayload::resolveTask(service::ResolverParams&& params)
{
	auto result = getTask(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<std::optional<response::StringType>> CompleteTaskPayload::getClientMutationId(service::FieldParams&&) const
{
	std::promise<std::optional<response::StringType>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(CompleteTaskPayload::getClientMutationId is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> CompleteTaskPayload::resolveClientMutationId(service::ResolverParams&& params)
{
	auto result = getClientMutationId(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> CompleteTaskPayload::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("CompleteTaskPayload");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

Mutation::Mutation()
	: service::Object({
		"Mutation"
	}, {
		{ "completeTask", [this](service::ResolverParams&& params) { return resolveCompleteTask(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<CompleteTaskPayload>> Mutation::getCompleteTask(service::FieldParams&&, CompleteTaskInput&&) const
{
	std::promise<std::shared_ptr<CompleteTaskPayload>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Mutation::getCompleteTask is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Mutation::resolveCompleteTask(service::ResolverParams&& params)
{
	auto argInput = service::ModifiedArgument<CompleteTaskInput>::require("input", params.arguments);
	auto result = getCompleteTask(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argInput));

	return service::ModifiedResult<CompleteTaskPayload>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Mutation::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("Mutation");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

Subscription::Subscription()
	: service::Object({
		"Subscription"
	}, {
		{ "nextAppointmentChange", [this](service::ResolverParams&& params) { return resolveNextAppointmentChange(std::move(params)); } },
		{ "nodeChange", [this](service::ResolverParams&& params) { return resolveNodeChange(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<std::shared_ptr<Appointment>> Subscription::getNextAppointmentChange(service::FieldParams&&) const
{
	std::promise<std::shared_ptr<Appointment>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Subscription::getNextAppointmentChange is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Subscription::resolveNextAppointmentChange(service::ResolverParams&& params)
{
	auto result = getNextAppointmentChange(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<std::shared_ptr<service::Object>> Subscription::getNodeChange(service::FieldParams&&, std::vector<uint8_t>&&) const
{
	std::promise<std::shared_ptr<service::Object>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Subscription::getNodeChange is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Subscription::resolveNodeChange(service::ResolverParams&& params)
{
	auto argId = service::ModifiedArgument<std::vector<uint8_t>>::require("id", params.arguments);
	auto result = getNodeChange(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argId));

	return service::ModifiedResult<service::Object>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Subscription::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("Subscription");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

Appointment::Appointment()
	: service::Object({
		"Node",
		"Appointment"
	}, {
		{ "id", [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ "when", [this](service::ResolverParams&& params) { return resolveWhen(std::move(params)); } },
		{ "subject", [this](service::ResolverParams&& params) { return resolveSubject(std::move(params)); } },
		{ "isNow", [this](service::ResolverParams&& params) { return resolveIsNow(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<std::vector<uint8_t>> Appointment::getId(service::FieldParams&&) const
{
	std::promise<std::vector<uint8_t>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Appointment::getId is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Appointment::resolveId(service::ResolverParams&& params)
{
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<std::vector<uint8_t>>::convert(std::move(result), std::move(params));
}

std::future<std::optional<response::Value>> Appointment::getWhen(service::FieldParams&&) const
{
	std::promise<std::optional<response::Value>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Appointment::getWhen is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Appointment::resolveWhen(service::ResolverParams&& params)
{
	auto result = getWhen(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::Value>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<std::optional<response::StringType>> Appointment::getSubject(service::FieldParams&&) const
{
	std::promise<std::optional<response::StringType>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Appointment::getSubject is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Appointment::resolveSubject(service::ResolverParams&& params)
{
	auto result = getSubject(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::BooleanType> Appointment::getIsNow(service::FieldParams&&) const
{
	std::promise<response::BooleanType> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Appointment::getIsNow is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Appointment::resolveIsNow(service::ResolverParams&& params)
{
	auto result = getIsNow(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Appointment::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("Appointment");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

Task::Task()
	: service::Object({
		"Node",
		"Task"
	}, {
		{ "id", [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ "title", [this](service::ResolverParams&& params) { return resolveTitle(std::move(params)); } },
		{ "isComplete", [this](service::ResolverParams&& params) { return resolveIsComplete(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<std::vector<uint8_t>> Task::getId(service::FieldParams&&) const
{
	std::promise<std::vector<uint8_t>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Task::getId is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Task::resolveId(service::ResolverParams&& params)
{
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<std::vector<uint8_t>>::convert(std::move(result), std::move(params));
}

std::future<std::optional<response::StringType>> Task::getTitle(service::FieldParams&&) const
{
	std::promise<std::optional<response::StringType>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Task::getTitle is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Task::resolveTitle(service::ResolverParams&& params)
{
	auto result = getTitle(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::BooleanType> Task::getIsComplete(service::FieldParams&&) const
{
	std::promise<response::BooleanType> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Task::getIsComplete is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Task::resolveIsComplete(service::ResolverParams&& params)
{
	auto result = getIsComplete(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Task::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("Task");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

Folder::Folder()
	: service::Object({
		"Node",
		"Folder"
	}, {
		{ "id", [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ "name", [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ "unreadCount", [this](service::ResolverParams&& params) { return resolveUnreadCount(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<std::vector<uint8_t>> Folder::getId(service::FieldParams&&) const
{
	std::promise<std::vector<uint8_t>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Folder::getId is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Folder::resolveId(service::ResolverParams&& params)
{
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<std::vector<uint8_t>>::convert(std::move(result), std::move(params));
}

std::future<std::optional<response::StringType>> Folder::getName(service::FieldParams&&) const
{
	std::promise<std::optional<response::StringType>> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Folder::getName is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Folder::resolveName(service::ResolverParams&& params)
{
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::IntType> Folder::getUnreadCount(service::FieldParams&&) const
{
	std::promise<response::IntType> promise;

	promise.set_exception(std::make_exception_ptr(std::runtime_error(R"ex(Folder::getUnreadCount is not implemented)ex")));

	return promise.get_future();
}

std::future<response::Value> Folder::resolveUnreadCount(service::ResolverParams&& params)
{
	auto result = getUnreadCount(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Folder::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("Folder");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

NestedType::NestedType()
	: service::Object({
		"NestedType"
	}, {
		{ "depth", [this](service::ResolverParams&& params) { return resolveDepth(std::move(params)); } },
		{ "nested", [this](service::ResolverParams&& params) { return resolveNested(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
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

std::future<response::Value> NestedType::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value("NestedType");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}

} /* namespace object */

Operations::Operations(std::shared_ptr<object::Query> query, std::shared_ptr<object::Mutation> mutation, std::shared_ptr<object::Subscription> subscription)
	: service::Request({
		{ "query", query },
		{ "mutation", mutation },
		{ "subscription", subscription }
	})
	, _query(std::move(query))
	, _mutation(std::move(mutation))
	, _subscription(std::move(subscription))
{
}

void AddTypesToSchema(std::shared_ptr<introspection::Schema> schema)
{
	schema->AddType("ItemCursor", std::make_shared<introspection::ScalarType>("ItemCursor", R"md()md"));
	schema->AddType("DateTime", std::make_shared<introspection::ScalarType>("DateTime", R"md()md"));
	auto typeTaskState= std::make_shared<introspection::EnumType>("TaskState", R"md()md");
	schema->AddType("TaskState", typeTaskState);
	auto typeCompleteTaskInput= std::make_shared<introspection::InputObjectType>("CompleteTaskInput", R"md()md");
	schema->AddType("CompleteTaskInput", typeCompleteTaskInput);
	auto typeUnionType= std::make_shared<introspection::UnionType>("UnionType", R"md()md");
	schema->AddType("UnionType", typeUnionType);
	auto typeNode= std::make_shared<introspection::InterfaceType>("Node", R"md(Node interface for Relay support)md");
	schema->AddType("Node", typeNode);
	auto typeQuery= std::make_shared<introspection::ObjectType>("Query", R"md(Root Query type)md");
	schema->AddType("Query", typeQuery);
	auto typePageInfo= std::make_shared<introspection::ObjectType>("PageInfo", R"md()md");
	schema->AddType("PageInfo", typePageInfo);
	auto typeAppointmentEdge= std::make_shared<introspection::ObjectType>("AppointmentEdge", R"md()md");
	schema->AddType("AppointmentEdge", typeAppointmentEdge);
	auto typeAppointmentConnection= std::make_shared<introspection::ObjectType>("AppointmentConnection", R"md()md");
	schema->AddType("AppointmentConnection", typeAppointmentConnection);
	auto typeTaskEdge= std::make_shared<introspection::ObjectType>("TaskEdge", R"md()md");
	schema->AddType("TaskEdge", typeTaskEdge);
	auto typeTaskConnection= std::make_shared<introspection::ObjectType>("TaskConnection", R"md()md");
	schema->AddType("TaskConnection", typeTaskConnection);
	auto typeFolderEdge= std::make_shared<introspection::ObjectType>("FolderEdge", R"md()md");
	schema->AddType("FolderEdge", typeFolderEdge);
	auto typeFolderConnection= std::make_shared<introspection::ObjectType>("FolderConnection", R"md()md");
	schema->AddType("FolderConnection", typeFolderConnection);
	auto typeCompleteTaskPayload= std::make_shared<introspection::ObjectType>("CompleteTaskPayload", R"md()md");
	schema->AddType("CompleteTaskPayload", typeCompleteTaskPayload);
	auto typeMutation= std::make_shared<introspection::ObjectType>("Mutation", R"md()md");
	schema->AddType("Mutation", typeMutation);
	auto typeSubscription= std::make_shared<introspection::ObjectType>("Subscription", R"md()md");
	schema->AddType("Subscription", typeSubscription);
	auto typeAppointment= std::make_shared<introspection::ObjectType>("Appointment", R"md()md");
	schema->AddType("Appointment", typeAppointment);
	auto typeTask= std::make_shared<introspection::ObjectType>("Task", R"md()md");
	schema->AddType("Task", typeTask);
	auto typeFolder= std::make_shared<introspection::ObjectType>("Folder", R"md()md");
	schema->AddType("Folder", typeFolder);
	auto typeNestedType= std::make_shared<introspection::ObjectType>("NestedType", R"md(Infinitely nestable type which can be used with nested fragments to test directive handling)md");
	schema->AddType("NestedType", typeNestedType);

	typeTaskState->AddEnumValues({
		{ "New", R"md()md", nullptr },
		{ "Started", R"md()md", nullptr },
		{ "Complete", R"md()md", nullptr },
		{ "Unassigned", R"md()md", R"md(Need to deprecate an [enum value](https://facebook.github.io/graphql/June2018/#sec-Deprecation))md" }
	});

	typeCompleteTaskInput->AddInputValues({
		std::make_shared<introspection::InputValue>("id", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")), R"gql()gql"),
		std::make_shared<introspection::InputValue>("isComplete", R"md()md", schema->LookupType("Boolean"), R"gql(true)gql"),
		std::make_shared<introspection::InputValue>("clientMutationId", R"md()md", schema->LookupType("String"), R"gql()gql")
	});

	typeUnionType->AddPossibleTypes({
		schema->LookupType("Appointment"),
		schema->LookupType("Task"),
		schema->LookupType("Folder")
	});

	typeNode->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))
	});

	typeQuery->AddFields({
		std::make_shared<introspection::Field>("node", R"md([Object Identification](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#object-identification))md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("id", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")), R"gql()gql")
		}), schema->LookupType("Node")),
		std::make_shared<introspection::Field>("appointments", R"md(Appointments [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("AppointmentConnection"))),
		std::make_shared<introspection::Field>("tasks", R"md(Tasks [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("TaskConnection"))),
		std::make_shared<introspection::Field>("unreadCounts", R"md(Folder unread counts [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("FolderConnection"))),
		std::make_shared<introspection::Field>("appointmentsById", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), R"gql(["ZmFrZUFwcG9pbnRtZW50SWQ="])gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("Appointment")))),
		std::make_shared<introspection::Field>("tasksById", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("Task")))),
		std::make_shared<introspection::Field>("unreadCountsById", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("Folder")))),
		std::make_shared<introspection::Field>("nested", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("NestedType"))),
		std::make_shared<introspection::Field>("unimplemented", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("String")))
	});
	typePageInfo->AddFields({
		std::make_shared<introspection::Field>("hasNextPage", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("hasPreviousPage", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeAppointmentEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeAppointmentConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("AppointmentEdge"))))
	});
	typeTaskEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeTaskConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("TaskEdge"))))
	});
	typeFolderEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Folder")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeFolderConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("FolderEdge"))))
	});
	typeCompleteTaskPayload->AddFields({
		std::make_shared<introspection::Field>("task", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("clientMutationId", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	typeMutation->AddFields({
		std::make_shared<introspection::Field>("completeTask", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("input", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("CompleteTaskInput")), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("CompleteTaskPayload")))
	});
	typeSubscription->AddFields({
		std::make_shared<introspection::Field>("nextAppointmentChange", R"md()md", std::optional<std::string>{ std::in_place, R"md(Need to deprecate a [field](https://facebook.github.io/graphql/June2018/#sec-Deprecation))md" }, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment")),
		std::make_shared<introspection::Field>("nodeChange", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("id", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Node")))
	});
	typeAppointment->AddInterfaces({
		typeNode
	});
	typeAppointment->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("when", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("DateTime")),
		std::make_shared<introspection::Field>("subject", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isNow", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeTask->AddInterfaces({
		typeNode
	});
	typeTask->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("title", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isComplete", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeFolder->AddInterfaces({
		typeNode
	});
	typeFolder->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("name", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("unreadCount", R"md()md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Int")))
	});
	typeNestedType->AddFields({
		std::make_shared<introspection::Field>("depth", R"md(Depth of the nested element)md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Int"))),
		std::make_shared<introspection::Field>("nested", R"md(Link to the next level)md", std::optional<std::string>{}, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("NestedType")))
	});

	schema->AddDirective(std::make_shared<introspection::Directive>("subscriptionTag", R"md()md", std::vector<response::StringType>({
		R"gql(SUBSCRIPTION)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("field", R"md()md", schema->LookupType("String"), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("queryTag", R"md()md", std::vector<response::StringType>({
		R"gql(QUERY)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("query", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("fieldTag", R"md()md", std::vector<response::StringType>({
		R"gql(FIELD)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("field", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("fragmentDefinitionTag", R"md()md", std::vector<response::StringType>({
		R"gql(FRAGMENT_DEFINITION)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("fragmentDefinition", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("fragmentSpreadTag", R"md()md", std::vector<response::StringType>({
		R"gql(FRAGMENT_SPREAD)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("fragmentSpread", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("inlineFragmentTag", R"md()md", std::vector<response::StringType>({
		R"gql(INLINE_FRAGMENT)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("inlineFragment", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));

	schema->AddQueryType(typeQuery);
	schema->AddMutationType(typeMutation);
	schema->AddSubscriptionType(typeSubscription);
}

} /* namespace today */
} /* namespace facebook::graphql */