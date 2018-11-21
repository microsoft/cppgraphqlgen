// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodaySchema.h"
#include "Introspection.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <exception>

namespace facebook {
namespace graphql {
namespace service {

template <>
today::TaskState ModifiedArgument<today::TaskState>::convert(const web::json::value& value)
{
	static const std::unordered_map<std::string, today::TaskState> s_names = {
		{ "New", today::TaskState::New },
		{ "Started", today::TaskState::Started },
		{ "Complete", today::TaskState::Complete }
	};

	auto itr = s_names.find(utility::conversions::to_utf8string(value.as_string()));

	if (itr == s_names.cend())
	{
		throw web::json::json_exception(_XPLATSTR("not a valid TaskState value"));
	}

	return itr->second;
}

template <>
web::json::value service::ModifiedResult<today::TaskState>::convert(const today::TaskState& value, ResolverParams&&)
{
	static const std::string s_names[] = {
		"New",
		"Started",
		"Complete"
	};

	return web::json::value::string(utility::conversions::to_string_t(s_names[static_cast<size_t>(value)]));
}

template <>
today::CompleteTaskInput ModifiedArgument<today::CompleteTaskInput>::convert(const web::json::value& value)
{
	static const auto defaultValue = web::json::value::object({
		{ _XPLATSTR("isComplete"), web::json::value::parse(_XPLATSTR(R"js(true)js")) }
	});

	auto valueId = service::ModifiedArgument<std::vector<unsigned char>>::require("id", value.as_object());
	auto pairIsComplete = service::ModifiedArgument<bool>::find<service::TypeModifier::Nullable>("isComplete", value.as_object());
	auto valueIsComplete = (pairIsComplete.second
		? std::move(pairIsComplete.first)
		: service::ModifiedArgument<bool>::require<service::TypeModifier::Nullable>("isComplete", defaultValue.as_object()));
	auto valueClientMutationId = service::ModifiedArgument<std::string>::require<service::TypeModifier::Nullable>("clientMutationId", value.as_object());

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

web::json::value Query::resolveNode(service::ResolverParams&& params)
{
	auto argId = service::ModifiedArgument<std::vector<unsigned char>>::require("id", params.arguments);
	auto result = getNode(std::move(argId));

	return service::ModifiedResult<service::Object>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value Query::resolveAppointments(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<web::json::value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<web::json::value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	auto result = getAppointments(std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));

	return service::ModifiedResult<AppointmentConnection>::convert(result, std::move(params));
}

web::json::value Query::resolveTasks(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<web::json::value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<web::json::value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	auto result = getTasks(std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));

	return service::ModifiedResult<TaskConnection>::convert(result, std::move(params));
}

web::json::value Query::resolveUnreadCounts(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<web::json::value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<web::json::value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	auto result = getUnreadCounts(std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));

	return service::ModifiedResult<FolderConnection>::convert(result, std::move(params));
}

web::json::value Query::resolveAppointmentsById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<unsigned char>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getAppointmentsById(std::move(argIds));

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value Query::resolveTasksById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<unsigned char>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getTasksById(std::move(argIds));

	return service::ModifiedResult<Task>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value Query::resolveUnreadCountsById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<unsigned char>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getUnreadCountsById(std::move(argIds));

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value Query::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("Query"));
}

web::json::value Query::resolve__schema(service::ResolverParams&& params)
{
	auto result = service::ModifiedResult<introspection::Schema>::convert(_schema, std::move(params));

	return result;
}

web::json::value Query::resolve__type(service::ResolverParams&& params)
{
	auto argName = service::ModifiedArgument<std::string>::require("name", params.arguments);
	auto result = service::ModifiedResult<introspection::object::__Type>::convert<service::TypeModifier::Nullable>(_schema->LookupType(argName), std::move(params));

	return result;
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

web::json::value PageInfo::resolveHasNextPage(service::ResolverParams&& params)
{
	auto result = getHasNextPage();

	return service::ModifiedResult<bool>::convert(result, std::move(params));
}

web::json::value PageInfo::resolveHasPreviousPage(service::ResolverParams&& params)
{
	auto result = getHasPreviousPage();

	return service::ModifiedResult<bool>::convert(result, std::move(params));
}

web::json::value PageInfo::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("PageInfo"));
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

web::json::value AppointmentEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode();

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value AppointmentEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor();

	return service::ModifiedResult<web::json::value>::convert(result, std::move(params));
}

web::json::value AppointmentEdge::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("AppointmentEdge"));
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

web::json::value AppointmentConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo();

	return service::ModifiedResult<PageInfo>::convert(result, std::move(params));
}

web::json::value AppointmentConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges();

	return service::ModifiedResult<AppointmentEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value AppointmentConnection::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("AppointmentConnection"));
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

web::json::value TaskEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode();

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value TaskEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor();

	return service::ModifiedResult<web::json::value>::convert(result, std::move(params));
}

web::json::value TaskEdge::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("TaskEdge"));
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

web::json::value TaskConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo();

	return service::ModifiedResult<PageInfo>::convert(result, std::move(params));
}

web::json::value TaskConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges();

	return service::ModifiedResult<TaskEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value TaskConnection::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("TaskConnection"));
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

web::json::value FolderEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode();

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value FolderEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor();

	return service::ModifiedResult<web::json::value>::convert(result, std::move(params));
}

web::json::value FolderEdge::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("FolderEdge"));
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

web::json::value FolderConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo();

	return service::ModifiedResult<PageInfo>::convert(result, std::move(params));
}

web::json::value FolderConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges();

	return service::ModifiedResult<FolderEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value FolderConnection::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("FolderConnection"));
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

web::json::value CompleteTaskPayload::resolveTask(service::ResolverParams&& params)
{
	auto result = getTask();

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value CompleteTaskPayload::resolveClientMutationId(service::ResolverParams&& params)
{
	auto result = getClientMutationId();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value CompleteTaskPayload::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("CompleteTaskPayload"));
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

web::json::value Mutation::resolveCompleteTask(service::ResolverParams&& params)
{
	auto argInput = service::ModifiedArgument<CompleteTaskInput>::require("input", params.arguments);
	auto result = getCompleteTask(std::move(argInput));

	return service::ModifiedResult<CompleteTaskPayload>::convert(result, std::move(params));
}

web::json::value Mutation::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("Mutation"));
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

web::json::value Subscription::resolveNextAppointmentChange(service::ResolverParams&& params)
{
	auto result = getNextAppointmentChange();

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value Subscription::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("Subscription"));
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

web::json::value Appointment::resolveId(service::ResolverParams&& params)
{
	auto result = getId();

	return service::ModifiedResult<std::vector<unsigned char>>::convert(result, std::move(params));
}

web::json::value Appointment::resolveWhen(service::ResolverParams&& params)
{
	auto result = getWhen();

	return service::ModifiedResult<web::json::value>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value Appointment::resolveSubject(service::ResolverParams&& params)
{
	auto result = getSubject();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value Appointment::resolveIsNow(service::ResolverParams&& params)
{
	auto result = getIsNow();

	return service::ModifiedResult<bool>::convert(result, std::move(params));
}

web::json::value Appointment::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("Appointment"));
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

web::json::value Task::resolveId(service::ResolverParams&& params)
{
	auto result = getId();

	return service::ModifiedResult<std::vector<unsigned char>>::convert(result, std::move(params));
}

web::json::value Task::resolveTitle(service::ResolverParams&& params)
{
	auto result = getTitle();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value Task::resolveIsComplete(service::ResolverParams&& params)
{
	auto result = getIsComplete();

	return service::ModifiedResult<bool>::convert(result, std::move(params));
}

web::json::value Task::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("Task"));
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

web::json::value Folder::resolveId(service::ResolverParams&& params)
{
	auto result = getId();

	return service::ModifiedResult<std::vector<unsigned char>>::convert(result, std::move(params));
}

web::json::value Folder::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}

web::json::value Folder::resolveUnreadCount(service::ResolverParams&& params)
{
	auto result = getUnreadCount();

	return service::ModifiedResult<int>::convert(result, std::move(params));
}

web::json::value Folder::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR("Folder"));
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
	schema->AddType("ItemCursor", std::make_shared<introspection::ScalarType>("ItemCursor"));
	schema->AddType("DateTime", std::make_shared<introspection::ScalarType>("DateTime"));
	auto typeTaskState= std::make_shared<introspection::EnumType>("TaskState");
	schema->AddType("TaskState", typeTaskState);
	auto typeCompleteTaskInput= std::make_shared<introspection::InputObjectType>("CompleteTaskInput");
	schema->AddType("CompleteTaskInput", typeCompleteTaskInput);
	auto typeNode= std::make_shared<introspection::InterfaceType>("Node");
	schema->AddType("Node", typeNode);
	auto typeQuery= std::make_shared<introspection::ObjectType>("Query");
	schema->AddType("Query", typeQuery);
	auto typePageInfo= std::make_shared<introspection::ObjectType>("PageInfo");
	schema->AddType("PageInfo", typePageInfo);
	auto typeAppointmentEdge= std::make_shared<introspection::ObjectType>("AppointmentEdge");
	schema->AddType("AppointmentEdge", typeAppointmentEdge);
	auto typeAppointmentConnection= std::make_shared<introspection::ObjectType>("AppointmentConnection");
	schema->AddType("AppointmentConnection", typeAppointmentConnection);
	auto typeTaskEdge= std::make_shared<introspection::ObjectType>("TaskEdge");
	schema->AddType("TaskEdge", typeTaskEdge);
	auto typeTaskConnection= std::make_shared<introspection::ObjectType>("TaskConnection");
	schema->AddType("TaskConnection", typeTaskConnection);
	auto typeFolderEdge= std::make_shared<introspection::ObjectType>("FolderEdge");
	schema->AddType("FolderEdge", typeFolderEdge);
	auto typeFolderConnection= std::make_shared<introspection::ObjectType>("FolderConnection");
	schema->AddType("FolderConnection", typeFolderConnection);
	auto typeCompleteTaskPayload= std::make_shared<introspection::ObjectType>("CompleteTaskPayload");
	schema->AddType("CompleteTaskPayload", typeCompleteTaskPayload);
	auto typeMutation= std::make_shared<introspection::ObjectType>("Mutation");
	schema->AddType("Mutation", typeMutation);
	auto typeSubscription= std::make_shared<introspection::ObjectType>("Subscription");
	schema->AddType("Subscription", typeSubscription);
	auto typeAppointment= std::make_shared<introspection::ObjectType>("Appointment");
	schema->AddType("Appointment", typeAppointment);
	auto typeTask= std::make_shared<introspection::ObjectType>("Task");
	schema->AddType("Task", typeTask);
	auto typeFolder= std::make_shared<introspection::ObjectType>("Folder");
	schema->AddType("Folder", typeFolder);

	typeTaskState->AddEnumValues({
		"New",
		"Started",
		"Complete"
	});

	typeCompleteTaskInput->AddInputValues({
		std::make_shared<introspection::InputValue>("id", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")), web::json::value::parse(_XPLATSTR(R"js(null)js"))),
		std::make_shared<introspection::InputValue>("isComplete", schema->LookupType("Boolean"), web::json::value::parse(_XPLATSTR(R"js(true)js"))),
		std::make_shared<introspection::InputValue>("clientMutationId", schema->LookupType("String"), web::json::value::parse(_XPLATSTR(R"js(null)js")))
	});

	typeNode->AddFields({
		std::make_shared<introspection::Field>("id", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))
	});

	typeQuery->AddFields({
		std::make_shared<introspection::Field>("node", std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("id", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")), web::json::value::parse(_XPLATSTR(R"js(null)js")))
		}), schema->LookupType("Node")),
		std::make_shared<introspection::Field>("appointments", std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", schema->LookupType("Int"), web::json::value::parse(_XPLATSTR(R"js(null)js"))),
			std::make_shared<introspection::InputValue>("after", schema->LookupType("ItemCursor"), web::json::value::parse(_XPLATSTR(R"js(null)js"))),
			std::make_shared<introspection::InputValue>("last", schema->LookupType("Int"), web::json::value::parse(_XPLATSTR(R"js(null)js"))),
			std::make_shared<introspection::InputValue>("before", schema->LookupType("ItemCursor"), web::json::value::parse(_XPLATSTR(R"js(null)js")))
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("AppointmentConnection"))),
		std::make_shared<introspection::Field>("tasks", std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", schema->LookupType("Int"), web::json::value::parse(_XPLATSTR(R"js(null)js"))),
			std::make_shared<introspection::InputValue>("after", schema->LookupType("ItemCursor"), web::json::value::parse(_XPLATSTR(R"js(null)js"))),
			std::make_shared<introspection::InputValue>("last", schema->LookupType("Int"), web::json::value::parse(_XPLATSTR(R"js(null)js"))),
			std::make_shared<introspection::InputValue>("before", schema->LookupType("ItemCursor"), web::json::value::parse(_XPLATSTR(R"js(null)js")))
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("TaskConnection"))),
		std::make_shared<introspection::Field>("unreadCounts", std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", schema->LookupType("Int"), web::json::value::parse(_XPLATSTR(R"js(null)js"))),
			std::make_shared<introspection::InputValue>("after", schema->LookupType("ItemCursor"), web::json::value::parse(_XPLATSTR(R"js(null)js"))),
			std::make_shared<introspection::InputValue>("last", schema->LookupType("Int"), web::json::value::parse(_XPLATSTR(R"js(null)js"))),
			std::make_shared<introspection::InputValue>("before", schema->LookupType("ItemCursor"), web::json::value::parse(_XPLATSTR(R"js(null)js")))
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("FolderConnection"))),
		std::make_shared<introspection::Field>("appointmentsById", std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), web::json::value::parse(_XPLATSTR(R"js(null)js")))
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("Appointment")))),
		std::make_shared<introspection::Field>("tasksById", std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), web::json::value::parse(_XPLATSTR(R"js(null)js")))
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("Task")))),
		std::make_shared<introspection::Field>("unreadCountsById", std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), web::json::value::parse(_XPLATSTR(R"js(null)js")))
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("Folder"))))
	});
	typePageInfo->AddFields({
		std::make_shared<introspection::Field>("hasNextPage", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("hasPreviousPage", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeAppointmentEdge->AddFields({
		std::make_shared<introspection::Field>("node", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment")),
		std::make_shared<introspection::Field>("cursor", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeAppointmentConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("AppointmentEdge"))))
	});
	typeTaskEdge->AddFields({
		std::make_shared<introspection::Field>("node", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("cursor", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeTaskConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("TaskEdge"))))
	});
	typeFolderEdge->AddFields({
		std::make_shared<introspection::Field>("node", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Folder")),
		std::make_shared<introspection::Field>("cursor", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeFolderConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("FolderEdge"))))
	});
	typeCompleteTaskPayload->AddFields({
		std::make_shared<introspection::Field>("task", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("clientMutationId", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	typeMutation->AddFields({
		std::make_shared<introspection::Field>("completeTask", std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("input", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("CompleteTaskInput")), web::json::value::parse(_XPLATSTR(R"js(null)js")))
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("CompleteTaskPayload")))
	});
	typeSubscription->AddFields({
		std::make_shared<introspection::Field>("nextAppointmentChange", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment"))
	});
	typeAppointment->AddInterfaces({
		typeNode
	});
	typeAppointment->AddFields({
		std::make_shared<introspection::Field>("id", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("when", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("DateTime")),
		std::make_shared<introspection::Field>("subject", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isNow", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeTask->AddInterfaces({
		typeNode
	});
	typeTask->AddFields({
		std::make_shared<introspection::Field>("id", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("title", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isComplete", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeFolder->AddInterfaces({
		typeNode
	});
	typeFolder->AddFields({
		std::make_shared<introspection::Field>("id", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("name", std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("unreadCount", std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Int")))
	});

	schema->AddQueryType(typeQuery);
	schema->AddMutationType(typeMutation);
	schema->AddSubscriptionType(typeSubscription);
}

} /* namespace today */
} /* namespace graphql */
} /* namespace facebook */