// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodaySchema.h"

#include <graphqlservice/Introspection.h>

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <exception>

namespace facebook {
namespace graphql {
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

	if (value.type() != response::Type::EnumValue)
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
std::future<response::Value> service::ModifiedResult<today::TaskState>::convert(std::future<today::TaskState>&& value, ResolverParams&&)
{
	static const std::string s_names[] = {
		"New",
		"Started",
		"Complete",
		"Unassigned"
	};

	std::promise<response::Value> promise;

	promise.set_value(response::Value(std::string(s_names[static_cast<size_t>(value.get())])));

	return promise.get_future();
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
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } },
		{ "__schema", [this](service::ResolverParams&& params) { return resolve__schema(std::move(params)); } },
		{ "__type", [this](service::ResolverParams&& params) { return resolve__type(std::move(params)); } }
	})
	, _schema(std::make_shared<introspection::Schema>())
{
	introspection::AddTypesToSchema(_schema);
	today::AddTypesToSchema(_schema);
}

std::future<response::Value> Query::resolveNode(service::ResolverParams&& params)
{
	auto argId = service::ModifiedArgument<std::vector<uint8_t>>::require("id", params.arguments);
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argId));

	return service::ModifiedResult<service::Object>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
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

std::future<response::Value> Query::resolveTasks(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<response::IntType>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	auto result = getTasks(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));

	return service::ModifiedResult<TaskConnection>::convert(std::move(result), std::move(params));
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

std::future<response::Value> Query::resolveAppointmentsById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<uint8_t>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getAppointmentsById(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIds));

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Query::resolveTasksById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<uint8_t>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getTasksById(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIds));

	return service::ModifiedResult<Task>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Query::resolveUnreadCountsById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<uint8_t>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getUnreadCountsById(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argIds));

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Query::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("Query"));

	return promise.get_future();
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

std::future<response::Value> PageInfo::resolveHasNextPage(service::ResolverParams&& params)
{
	auto result = getHasNextPage(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> PageInfo::resolveHasPreviousPage(service::ResolverParams&& params)
{
	auto result = getHasPreviousPage(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> PageInfo::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("PageInfo"));

	return promise.get_future();
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

std::future<response::Value> AppointmentEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> AppointmentEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> AppointmentEdge::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("AppointmentEdge"));

	return promise.get_future();
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

std::future<response::Value> AppointmentConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

std::future<response::Value> AppointmentConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<AppointmentEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> AppointmentConnection::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("AppointmentConnection"));

	return promise.get_future();
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

std::future<response::Value> TaskEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> TaskEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> TaskEdge::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("TaskEdge"));

	return promise.get_future();
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

std::future<response::Value> TaskConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

std::future<response::Value> TaskConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<TaskEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> TaskConnection::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("TaskConnection"));

	return promise.get_future();
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

std::future<response::Value> FolderEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> FolderEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> FolderEdge::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("FolderEdge"));

	return promise.get_future();
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

std::future<response::Value> FolderConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

std::future<response::Value> FolderConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<FolderEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> FolderConnection::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("FolderConnection"));

	return promise.get_future();
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

std::future<response::Value> CompleteTaskPayload::resolveTask(service::ResolverParams&& params)
{
	auto result = getTask(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> CompleteTaskPayload::resolveClientMutationId(service::ResolverParams&& params)
{
	auto result = getClientMutationId(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> CompleteTaskPayload::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("CompleteTaskPayload"));

	return promise.get_future();
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

std::future<response::Value> Mutation::resolveCompleteTask(service::ResolverParams&& params)
{
	auto argInput = service::ModifiedArgument<CompleteTaskInput>::require("input", params.arguments);
	auto result = getCompleteTask(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argInput));

	return service::ModifiedResult<CompleteTaskPayload>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Mutation::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("Mutation"));

	return promise.get_future();
}

Subscription::Subscription()
	: service::Object({
		"Subscription"
	}, {
		{ "nextAppointmentChange", [this](service::ResolverParams&& params) { return resolveNextAppointmentChange(std::move(params)); } },
		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } }
	})
{
}

std::future<response::Value> Subscription::resolveNextAppointmentChange(service::ResolverParams&& params)
{
	auto result = getNextAppointmentChange(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Subscription::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("Subscription"));

	return promise.get_future();
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

std::future<response::Value> Appointment::resolveId(service::ResolverParams&& params)
{
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<std::vector<uint8_t>>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Appointment::resolveWhen(service::ResolverParams&& params)
{
	auto result = getWhen(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::Value>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Appointment::resolveSubject(service::ResolverParams&& params)
{
	auto result = getSubject(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Appointment::resolveIsNow(service::ResolverParams&& params)
{
	auto result = getIsNow(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Appointment::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("Appointment"));

	return promise.get_future();
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

std::future<response::Value> Task::resolveId(service::ResolverParams&& params)
{
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<std::vector<uint8_t>>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Task::resolveTitle(service::ResolverParams&& params)
{
	auto result = getTitle(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Task::resolveIsComplete(service::ResolverParams&& params)
{
	auto result = getIsComplete(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Task::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("Task"));

	return promise.get_future();
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

std::future<response::Value> Folder::resolveId(service::ResolverParams&& params)
{
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<std::vector<uint8_t>>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Folder::resolveName(service::ResolverParams&& params)
{
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> Folder::resolveUnreadCount(service::ResolverParams&& params)
{
	auto result = getUnreadCount(service::FieldParams(params, std::move(params.fieldDirectives)));

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Folder::resolve__typename(service::ResolverParams&&)
{
	std::promise<response::Value> promise;

	promise.set_value(response::Value("Folder"));

	return promise.get_future();
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

	typeNode->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))
	});

	typeQuery->AddFields({
		std::make_shared<introspection::Field>("node", R"md([Object Identification](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#object-identification))md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("id", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")), R"gql()gql")
		}), schema->LookupType("Node")),
		std::make_shared<introspection::Field>("appointments", R"md(Appointments [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("AppointmentConnection"))),
		std::make_shared<introspection::Field>("tasks", R"md(Tasks [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("TaskConnection"))),
		std::make_shared<introspection::Field>("unreadCounts", R"md(Folder unread counts [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), R"gql()gql"),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("FolderConnection"))),
		std::make_shared<introspection::Field>("appointmentsById", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("Appointment")))),
		std::make_shared<introspection::Field>("tasksById", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("Task")))),
		std::make_shared<introspection::Field>("unreadCountsById", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("Folder"))))
	});
	typePageInfo->AddFields({
		std::make_shared<introspection::Field>("hasNextPage", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("hasPreviousPage", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeAppointmentEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeAppointmentConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("AppointmentEdge"))))
	});
	typeTaskEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeTaskConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("TaskEdge"))))
	});
	typeFolderEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Folder")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeFolderConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->WrapType(introspection::__TypeKind::LIST, schema->LookupType("FolderEdge"))))
	});
	typeCompleteTaskPayload->AddFields({
		std::make_shared<introspection::Field>("task", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("clientMutationId", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	typeMutation->AddFields({
		std::make_shared<introspection::Field>("completeTask", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("input", R"md()md", schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("CompleteTaskInput")), R"gql()gql")
		}), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("CompleteTaskPayload")))
	});
	typeSubscription->AddFields({
		std::make_shared<introspection::Field>("nextAppointmentChange", R"md()md", std::unique_ptr<std::string>(new std::string(R"md(Need to deprecate a [field](https://facebook.github.io/graphql/June2018/#sec-Deprecation))md")), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment"))
	});
	typeAppointment->AddInterfaces({
		typeNode
	});
	typeAppointment->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("when", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("DateTime")),
		std::make_shared<introspection::Field>("subject", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isNow", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeTask->AddInterfaces({
		typeNode
	});
	typeTask->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("title", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isComplete", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeFolder->AddInterfaces({
		typeNode
	});
	typeFolder->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("name", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("unreadCount", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::__TypeKind::NON_NULL, schema->LookupType("Int")))
	});

	schema->AddDirective(std::make_shared<introspection::Directive>("subscriptionTag", R"md()md", std::vector<response::StringType>({
		R"gql(SUBSCRIPTION)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("field", R"md()md", schema->LookupType("String"), R"gql()gql")
	})));

	schema->AddQueryType(typeQuery);
	schema->AddMutationType(typeMutation);
	schema->AddSubscriptionType(typeSubscription);
}

} /* namespace today */
} /* namespace graphql */
} /* namespace facebook */