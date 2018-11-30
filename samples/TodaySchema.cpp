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
today::TaskState ModifiedArgument<today::TaskState>::convert(const rapidjson::Value& value)
{
	static const std::unordered_map<std::string, today::TaskState> s_names = {
		{ "New", today::TaskState::New },
		{ "Started", today::TaskState::Started },
		{ "Complete", today::TaskState::Complete },
		{ "Unassigned", today::TaskState::Unassigned }
	};

	auto itr = s_names.find(value.GetString());

	if (itr == s_names.cend())
	{
		throw service::schema_exception({ "not a valid TaskState value" });
	}

	return itr->second;
}

template <>
rapidjson::Document service::ModifiedResult<today::TaskState>::convert(today::TaskState&& value, ResolverParams&&)
{
	static const std::string s_names[] = {
		"New",
		"Started",
		"Complete",
		"Unassigned"
	};

	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef(s_names[static_cast<size_t>(value)].c_str()));

	return result;
}

template <>
today::CompleteTaskInput ModifiedArgument<today::CompleteTaskInput>::convert(const rapidjson::Value& value)
{
	const auto defaultValue = []()
	{
		rapidjson::Document values(rapidjson::Type::kObjectType);
		auto& allocator = values.GetAllocator();
		rapidjson::Document parsed;
		rapidjson::Value entry;

		parsed.Parse(R"js(true)js");
		entry.CopyFrom(parsed, allocator);
		values.AddMember(rapidjson::StringRef("isComplete"), entry, allocator);

		return values;
	}();

	auto valueId = service::ModifiedArgument<std::vector<uint8_t>>::require("id", value.GetObject());
	auto pairIsComplete = service::ModifiedArgument<bool>::find<service::TypeModifier::Nullable>("isComplete", value.GetObject());
	auto valueIsComplete = (pairIsComplete.second
		? std::move(pairIsComplete.first)
		: service::ModifiedArgument<bool>::require<service::TypeModifier::Nullable>("isComplete", defaultValue.GetObject()));
	auto valueClientMutationId = service::ModifiedArgument<std::string>::require<service::TypeModifier::Nullable>("clientMutationId", value.GetObject());

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

rapidjson::Document Query::resolveNode(service::ResolverParams&& params)
{
	auto argId = service::ModifiedArgument<std::vector<uint8_t>>::require("id", params.arguments);
	auto result = getNode(std::move(argId));

	return service::ModifiedResult<service::Object>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document Query::resolveAppointments(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<rapidjson::Document>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<rapidjson::Document>::require<service::TypeModifier::Nullable>("before", params.arguments);
	auto result = getAppointments(std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));

	return service::ModifiedResult<AppointmentConnection>::convert(std::move(result), std::move(params));
}

rapidjson::Document Query::resolveTasks(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<rapidjson::Document>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<rapidjson::Document>::require<service::TypeModifier::Nullable>("before", params.arguments);
	auto result = getTasks(std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));

	return service::ModifiedResult<TaskConnection>::convert(std::move(result), std::move(params));
}

rapidjson::Document Query::resolveUnreadCounts(service::ResolverParams&& params)
{
	auto argFirst = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<rapidjson::Document>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<rapidjson::Document>::require<service::TypeModifier::Nullable>("before", params.arguments);
	auto result = getUnreadCounts(std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));

	return service::ModifiedResult<FolderConnection>::convert(std::move(result), std::move(params));
}

rapidjson::Document Query::resolveAppointmentsById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<uint8_t>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getAppointmentsById(std::move(argIds));

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document Query::resolveTasksById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<uint8_t>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getTasksById(std::move(argIds));

	return service::ModifiedResult<Task>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document Query::resolveUnreadCountsById(service::ResolverParams&& params)
{
	auto argIds = service::ModifiedArgument<std::vector<uint8_t>>::require<service::TypeModifier::List>("ids", params.arguments);
	auto result = getUnreadCountsById(std::move(argIds));

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document Query::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("Query"));

	return result;
}

rapidjson::Document Query::resolve__schema(service::ResolverParams&& params)
{
	return service::ModifiedResult<service::Object>::convert(std::static_pointer_cast<service::Object>(_schema), std::move(params));
}

rapidjson::Document Query::resolve__type(service::ResolverParams&& params)
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

rapidjson::Document PageInfo::resolveHasNextPage(service::ResolverParams&& params)
{
	auto result = getHasNextPage();

	return service::ModifiedResult<bool>::convert(std::move(result), std::move(params));
}

rapidjson::Document PageInfo::resolveHasPreviousPage(service::ResolverParams&& params)
{
	auto result = getHasPreviousPage();

	return service::ModifiedResult<bool>::convert(std::move(result), std::move(params));
}

rapidjson::Document PageInfo::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("PageInfo"));

	return result;
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

rapidjson::Document AppointmentEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode();

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document AppointmentEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor();

	return service::ModifiedResult<rapidjson::Document>::convert(std::move(result), std::move(params));
}

rapidjson::Document AppointmentEdge::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("AppointmentEdge"));

	return result;
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

rapidjson::Document AppointmentConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo();

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

rapidjson::Document AppointmentConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges();

	return service::ModifiedResult<AppointmentEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document AppointmentConnection::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("AppointmentConnection"));

	return result;
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

rapidjson::Document TaskEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode();

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document TaskEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor();

	return service::ModifiedResult<rapidjson::Document>::convert(std::move(result), std::move(params));
}

rapidjson::Document TaskEdge::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("TaskEdge"));

	return result;
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

rapidjson::Document TaskConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo();

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

rapidjson::Document TaskConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges();

	return service::ModifiedResult<TaskEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document TaskConnection::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("TaskConnection"));

	return result;
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

rapidjson::Document FolderEdge::resolveNode(service::ResolverParams&& params)
{
	auto result = getNode();

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document FolderEdge::resolveCursor(service::ResolverParams&& params)
{
	auto result = getCursor();

	return service::ModifiedResult<rapidjson::Document>::convert(std::move(result), std::move(params));
}

rapidjson::Document FolderEdge::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("FolderEdge"));

	return result;
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

rapidjson::Document FolderConnection::resolvePageInfo(service::ResolverParams&& params)
{
	auto result = getPageInfo();

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

rapidjson::Document FolderConnection::resolveEdges(service::ResolverParams&& params)
{
	auto result = getEdges();

	return service::ModifiedResult<FolderEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document FolderConnection::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("FolderConnection"));

	return result;
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

rapidjson::Document CompleteTaskPayload::resolveTask(service::ResolverParams&& params)
{
	auto result = getTask();

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document CompleteTaskPayload::resolveClientMutationId(service::ResolverParams&& params)
{
	auto result = getClientMutationId();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document CompleteTaskPayload::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("CompleteTaskPayload"));

	return result;
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

rapidjson::Document Mutation::resolveCompleteTask(service::ResolverParams&& params)
{
	auto argInput = service::ModifiedArgument<CompleteTaskInput>::require("input", params.arguments);
	auto result = getCompleteTask(std::move(argInput));

	return service::ModifiedResult<CompleteTaskPayload>::convert(std::move(result), std::move(params));
}

rapidjson::Document Mutation::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("Mutation"));

	return result;
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

rapidjson::Document Subscription::resolveNextAppointmentChange(service::ResolverParams&& params)
{
	auto result = getNextAppointmentChange();

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document Subscription::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("Subscription"));

	return result;
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

rapidjson::Document Appointment::resolveId(service::ResolverParams&& params)
{
	auto result = getId();

	return service::ModifiedResult<std::vector<uint8_t>>::convert(std::move(result), std::move(params));
}

rapidjson::Document Appointment::resolveWhen(service::ResolverParams&& params)
{
	auto result = getWhen();

	return service::ModifiedResult<rapidjson::Document>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document Appointment::resolveSubject(service::ResolverParams&& params)
{
	auto result = getSubject();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document Appointment::resolveIsNow(service::ResolverParams&& params)
{
	auto result = getIsNow();

	return service::ModifiedResult<bool>::convert(std::move(result), std::move(params));
}

rapidjson::Document Appointment::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("Appointment"));

	return result;
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

rapidjson::Document Task::resolveId(service::ResolverParams&& params)
{
	auto result = getId();

	return service::ModifiedResult<std::vector<uint8_t>>::convert(std::move(result), std::move(params));
}

rapidjson::Document Task::resolveTitle(service::ResolverParams&& params)
{
	auto result = getTitle();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document Task::resolveIsComplete(service::ResolverParams&& params)
{
	auto result = getIsComplete();

	return service::ModifiedResult<bool>::convert(std::move(result), std::move(params));
}

rapidjson::Document Task::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("Task"));

	return result;
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

rapidjson::Document Folder::resolveId(service::ResolverParams&& params)
{
	auto result = getId();

	return service::ModifiedResult<std::vector<uint8_t>>::convert(std::move(result), std::move(params));
}

rapidjson::Document Folder::resolveName(service::ResolverParams&& params)
{
	auto result = getName();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

rapidjson::Document Folder::resolveUnreadCount(service::ResolverParams&& params)
{
	auto result = getUnreadCount();

	return service::ModifiedResult<int>::convert(std::move(result), std::move(params));
}

rapidjson::Document Folder::resolve__typename(service::ResolverParams&&)
{
	rapidjson::Document result(rapidjson::Type::kStringType);

	result.SetString(rapidjson::StringRef("Folder"));

	return result;
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

	rapidjson::Document defaultCompleteTaskInputid;
	defaultCompleteTaskInputid.Parse(R"js(null)js");
	rapidjson::Document defaultCompleteTaskInputisComplete;
	defaultCompleteTaskInputisComplete.Parse(R"js(true)js");
	rapidjson::Document defaultCompleteTaskInputclientMutationId;
	defaultCompleteTaskInputclientMutationId.Parse(R"js(null)js");
	typeCompleteTaskInput->AddInputValues({
		std::make_shared<introspection::InputValue>("id", R"md()md", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")), defaultCompleteTaskInputid),
		std::make_shared<introspection::InputValue>("isComplete", R"md()md", schema->LookupType("Boolean"), defaultCompleteTaskInputisComplete),
		std::make_shared<introspection::InputValue>("clientMutationId", R"md()md", schema->LookupType("String"), defaultCompleteTaskInputclientMutationId)
	});

	typeNode->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))
	});

	rapidjson::Document defaultQuerynodeid;
	defaultQuerynodeid.Parse(R"js(null)js");
	rapidjson::Document defaultQueryappointmentsfirst;
	defaultQueryappointmentsfirst.Parse(R"js(null)js");
	rapidjson::Document defaultQueryappointmentsafter;
	defaultQueryappointmentsafter.Parse(R"js(null)js");
	rapidjson::Document defaultQueryappointmentslast;
	defaultQueryappointmentslast.Parse(R"js(null)js");
	rapidjson::Document defaultQueryappointmentsbefore;
	defaultQueryappointmentsbefore.Parse(R"js(null)js");
	rapidjson::Document defaultQuerytasksfirst;
	defaultQuerytasksfirst.Parse(R"js(null)js");
	rapidjson::Document defaultQuerytasksafter;
	defaultQuerytasksafter.Parse(R"js(null)js");
	rapidjson::Document defaultQuerytaskslast;
	defaultQuerytaskslast.Parse(R"js(null)js");
	rapidjson::Document defaultQuerytasksbefore;
	defaultQuerytasksbefore.Parse(R"js(null)js");
	rapidjson::Document defaultQueryunreadCountsfirst;
	defaultQueryunreadCountsfirst.Parse(R"js(null)js");
	rapidjson::Document defaultQueryunreadCountsafter;
	defaultQueryunreadCountsafter.Parse(R"js(null)js");
	rapidjson::Document defaultQueryunreadCountslast;
	defaultQueryunreadCountslast.Parse(R"js(null)js");
	rapidjson::Document defaultQueryunreadCountsbefore;
	defaultQueryunreadCountsbefore.Parse(R"js(null)js");
	rapidjson::Document defaultQueryappointmentsByIdids;
	defaultQueryappointmentsByIdids.Parse(R"js(null)js");
	rapidjson::Document defaultQuerytasksByIdids;
	defaultQuerytasksByIdids.Parse(R"js(null)js");
	rapidjson::Document defaultQueryunreadCountsByIdids;
	defaultQueryunreadCountsByIdids.Parse(R"js(null)js");
	typeQuery->AddFields({
		std::make_shared<introspection::Field>("node", R"md([Object Identification](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#object-identification))md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("id", R"md()md", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")), defaultQuerynodeid)
		}), schema->LookupType("Node")),
		std::make_shared<introspection::Field>("appointments", R"md(Appointments [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), defaultQueryappointmentsfirst),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), defaultQueryappointmentsafter),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), defaultQueryappointmentslast),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), defaultQueryappointmentsbefore)
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("AppointmentConnection"))),
		std::make_shared<introspection::Field>("tasks", R"md(Tasks [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), defaultQuerytasksfirst),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), defaultQuerytasksafter),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), defaultQuerytaskslast),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), defaultQuerytasksbefore)
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("TaskConnection"))),
		std::make_shared<introspection::Field>("unreadCounts", R"md(Folder unread counts [Connection](https://facebook.github.io/relay/docs/en/graphql-server-specification.html#connections))md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("first", R"md()md", schema->LookupType("Int"), defaultQueryunreadCountsfirst),
			std::make_shared<introspection::InputValue>("after", R"md()md", schema->LookupType("ItemCursor"), defaultQueryunreadCountsafter),
			std::make_shared<introspection::InputValue>("last", R"md()md", schema->LookupType("Int"), defaultQueryunreadCountslast),
			std::make_shared<introspection::InputValue>("before", R"md()md", schema->LookupType("ItemCursor"), defaultQueryunreadCountsbefore)
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("FolderConnection"))),
		std::make_shared<introspection::Field>("appointmentsById", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), defaultQueryappointmentsByIdids)
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("Appointment")))),
		std::make_shared<introspection::Field>("tasksById", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), defaultQuerytasksByIdids)
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("Task")))),
		std::make_shared<introspection::Field>("unreadCountsById", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("ids", R"md()md", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID")))), defaultQueryunreadCountsByIdids)
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("Folder"))))
	});
	typePageInfo->AddFields({
		std::make_shared<introspection::Field>("hasNextPage", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("hasPreviousPage", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeAppointmentEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeAppointmentConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("AppointmentEdge"))))
	});
	typeTaskEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeTaskConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("TaskEdge"))))
	});
	typeFolderEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Folder")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeFolderConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, std::make_shared<introspection::WrapperType>(introspection::__TypeKind::LIST, schema->LookupType("FolderEdge"))))
	});
	typeCompleteTaskPayload->AddFields({
		std::make_shared<introspection::Field>("task", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("clientMutationId", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	rapidjson::Document defaultMutationcompleteTaskinput;
	defaultMutationcompleteTaskinput.Parse(R"js(null)js");
	typeMutation->AddFields({
		std::make_shared<introspection::Field>("completeTask", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("input", R"md()md", std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("CompleteTaskInput")), defaultMutationcompleteTaskinput)
		}), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("CompleteTaskPayload")))
	});
	typeSubscription->AddFields({
		std::make_shared<introspection::Field>("nextAppointmentChange", R"md()md", std::unique_ptr<std::string>(new std::string(R"md(Need to deprecate a [field](https://facebook.github.io/graphql/June2018/#sec-Deprecation))md")), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment"))
	});
	typeAppointment->AddInterfaces({
		typeNode
	});
	typeAppointment->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("when", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("DateTime")),
		std::make_shared<introspection::Field>("subject", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isNow", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeTask->AddInterfaces({
		typeNode
	});
	typeTask->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("title", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isComplete", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeFolder->AddInterfaces({
		typeNode
	});
	typeFolder->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("name", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("unreadCount", R"md()md", std::unique_ptr<std::string>(nullptr), std::vector<std::shared_ptr<introspection::InputValue>>(), std::make_shared<introspection::WrapperType>(introspection::__TypeKind::NON_NULL, schema->LookupType("Int")))
	});

	schema->AddQueryType(typeQuery);
	schema->AddMutationType(typeMutation);
	schema->AddSubscriptionType(typeSubscription);
}

} /* namespace today */
} /* namespace graphql */
} /* namespace facebook */