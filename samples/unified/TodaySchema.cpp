// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodaySchema.h"

#include "graphqlservice/Introspection.h"
#include "graphqlservice/GraphQLValidation.h"

#include <algorithm>
#include <array>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

using namespace std::literals;

namespace graphql {
namespace service {

static const std::array<std::string_view, 4> s_namesTaskState = {
	"New",
	"Started",
	"Complete",
	"Unassigned"
};

template <>
today::TaskState ModifiedArgument<today::TaskState>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { "not a valid TaskState value" } };
	}

	auto itr = std::find(s_namesTaskState.cbegin(), s_namesTaskState.cend(), value.get<response::StringType>());

	if (itr == s_namesTaskState.cend())
	{
		throw service::schema_exception { { "not a valid TaskState value" } };
	}

	return static_cast<today::TaskState>(itr - s_namesTaskState.cbegin());
}

template <>
std::future<response::Value> ModifiedResult<today::TaskState>::convert(service::FieldResult<today::TaskState>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[](today::TaskState&& value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<response::StringType>(std::string(s_namesTaskState[static_cast<size_t>(value)]));

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

	auto valueId = service::ModifiedArgument<response::IdType>::require("id", value);
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
#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
		{ R"gql(__schema)gql"sv, [this](service::ResolverParams&& params) { return resolve_schema(std::move(params)); } },
		{ R"gql(__type)gql"sv, [this](service::ResolverParams&& params) { return resolve_type(std::move(params)); } },
#endif
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(appointments)gql"sv, [this](service::ResolverParams&& params) { return resolveAppointments(std::move(params)); } },
		{ R"gql(appointmentsById)gql"sv, [this](service::ResolverParams&& params) { return resolveAppointmentsById(std::move(params)); } },
		{ R"gql(expensive)gql"sv, [this](service::ResolverParams&& params) { return resolveExpensive(std::move(params)); } },
		{ R"gql(nested)gql"sv, [this](service::ResolverParams&& params) { return resolveNested(std::move(params)); } },
		{ R"gql(node)gql"sv, [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } },
		{ R"gql(tasks)gql"sv, [this](service::ResolverParams&& params) { return resolveTasks(std::move(params)); } },
		{ R"gql(tasksById)gql"sv, [this](service::ResolverParams&& params) { return resolveTasksById(std::move(params)); } },
		{ R"gql(unimplemented)gql"sv, [this](service::ResolverParams&& params) { return resolveUnimplemented(std::move(params)); } },
		{ R"gql(unreadCounts)gql"sv, [this](service::ResolverParams&& params) { return resolveUnreadCounts(std::move(params)); } },
		{ R"gql(unreadCountsById)gql"sv, [this](service::ResolverParams&& params) { return resolveUnreadCountsById(std::move(params)); } }
	})
#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
	, _schema(std::make_shared<introspection::Schema>())
#endif
{
#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
	introspection::AddTypesToSchema(_schema);
	today::AddTypesToSchema(_schema);
#endif
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

#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
std::future<response::Value> Query::resolve_schema(service::ResolverParams&& params)
{
	return service::ModifiedResult<service::Object>::convert(std::static_pointer_cast<service::Object>(_schema), std::move(params));
}

std::future<response::Value> Query::resolve_type(service::ResolverParams&& params)
{
	auto argName = service::ModifiedArgument<response::StringType>::require("name", params.arguments);

	return service::ModifiedResult<introspection::object::Type>::convert<service::TypeModifier::Nullable>(_schema->LookupType(argName), std::move(params));
}
#endif

PageInfo::PageInfo()
	: service::Object({
		"PageInfo"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(hasNextPage)gql"sv, [this](service::ResolverParams&& params) { return resolveHasNextPage(std::move(params)); } },
		{ R"gql(hasPreviousPage)gql"sv, [this](service::ResolverParams&& params) { return resolveHasPreviousPage(std::move(params)); } }
	})
{
}

service::FieldResult<response::BooleanType> PageInfo::getHasNextPage(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(PageInfo::getHasNextPage is not implemented)ex");
}

std::future<response::Value> PageInfo::resolveHasNextPage(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getHasNextPage(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> PageInfo::getHasPreviousPage(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(PageInfo::getHasPreviousPage is not implemented)ex");
}

std::future<response::Value> PageInfo::resolveHasPreviousPage(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getHasPreviousPage(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> PageInfo::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(PageInfo)gql" }, std::move(params));
}

AppointmentEdge::AppointmentEdge()
	: service::Object({
		"AppointmentEdge"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(cursor)gql"sv, [this](service::ResolverParams&& params) { return resolveCursor(std::move(params)); } },
		{ R"gql(node)gql"sv, [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } }
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

AppointmentConnection::AppointmentConnection()
	: service::Object({
		"AppointmentConnection"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(edges)gql"sv, [this](service::ResolverParams&& params) { return resolveEdges(std::move(params)); } },
		{ R"gql(pageInfo)gql"sv, [this](service::ResolverParams&& params) { return resolvePageInfo(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<PageInfo>> AppointmentConnection::getPageInfo(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(AppointmentConnection::getPageInfo is not implemented)ex");
}

std::future<response::Value> AppointmentConnection::resolvePageInfo(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<AppointmentEdge>>>> AppointmentConnection::getEdges(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(AppointmentConnection::getEdges is not implemented)ex");
}

std::future<response::Value> AppointmentConnection::resolveEdges(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<AppointmentEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> AppointmentConnection::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(AppointmentConnection)gql" }, std::move(params));
}

TaskEdge::TaskEdge()
	: service::Object({
		"TaskEdge"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(cursor)gql"sv, [this](service::ResolverParams&& params) { return resolveCursor(std::move(params)); } },
		{ R"gql(node)gql"sv, [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<Task>> TaskEdge::getNode(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(TaskEdge::getNode is not implemented)ex");
}

std::future<response::Value> TaskEdge::resolveNode(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::Value> TaskEdge::getCursor(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(TaskEdge::getCursor is not implemented)ex");
}

std::future<response::Value> TaskEdge::resolveCursor(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> TaskEdge::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(TaskEdge)gql" }, std::move(params));
}

TaskConnection::TaskConnection()
	: service::Object({
		"TaskConnection"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(edges)gql"sv, [this](service::ResolverParams&& params) { return resolveEdges(std::move(params)); } },
		{ R"gql(pageInfo)gql"sv, [this](service::ResolverParams&& params) { return resolvePageInfo(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<PageInfo>> TaskConnection::getPageInfo(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(TaskConnection::getPageInfo is not implemented)ex");
}

std::future<response::Value> TaskConnection::resolvePageInfo(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<TaskEdge>>>> TaskConnection::getEdges(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(TaskConnection::getEdges is not implemented)ex");
}

std::future<response::Value> TaskConnection::resolveEdges(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<TaskEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> TaskConnection::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(TaskConnection)gql" }, std::move(params));
}

FolderEdge::FolderEdge()
	: service::Object({
		"FolderEdge"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(cursor)gql"sv, [this](service::ResolverParams&& params) { return resolveCursor(std::move(params)); } },
		{ R"gql(node)gql"sv, [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<Folder>> FolderEdge::getNode(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(FolderEdge::getNode is not implemented)ex");
}

std::future<response::Value> FolderEdge::resolveNode(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNode(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::Value> FolderEdge::getCursor(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(FolderEdge::getCursor is not implemented)ex");
}

std::future<response::Value> FolderEdge::resolveCursor(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getCursor(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::Value>::convert(std::move(result), std::move(params));
}

std::future<response::Value> FolderEdge::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(FolderEdge)gql" }, std::move(params));
}

FolderConnection::FolderConnection()
	: service::Object({
		"FolderConnection"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(edges)gql"sv, [this](service::ResolverParams&& params) { return resolveEdges(std::move(params)); } },
		{ R"gql(pageInfo)gql"sv, [this](service::ResolverParams&& params) { return resolvePageInfo(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<PageInfo>> FolderConnection::getPageInfo(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(FolderConnection::getPageInfo is not implemented)ex");
}

std::future<response::Value> FolderConnection::resolvePageInfo(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getPageInfo(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<PageInfo>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<FolderEdge>>>> FolderConnection::getEdges(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(FolderConnection::getEdges is not implemented)ex");
}

std::future<response::Value> FolderConnection::resolveEdges(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getEdges(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<FolderEdge>::convert<service::TypeModifier::Nullable, service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> FolderConnection::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(FolderConnection)gql" }, std::move(params));
}

CompleteTaskPayload::CompleteTaskPayload()
	: service::Object({
		"CompleteTaskPayload"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(clientMutationId)gql"sv, [this](service::ResolverParams&& params) { return resolveClientMutationId(std::move(params)); } },
		{ R"gql(task)gql"sv, [this](service::ResolverParams&& params) { return resolveTask(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<Task>> CompleteTaskPayload::getTask(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(CompleteTaskPayload::getTask is not implemented)ex");
}

std::future<response::Value> CompleteTaskPayload::resolveTask(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getTask(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<Task>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> CompleteTaskPayload::getClientMutationId(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(CompleteTaskPayload::getClientMutationId is not implemented)ex");
}

std::future<response::Value> CompleteTaskPayload::resolveClientMutationId(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getClientMutationId(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

std::future<response::Value> CompleteTaskPayload::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(CompleteTaskPayload)gql" }, std::move(params));
}

Mutation::Mutation()
	: service::Object({
		"Mutation"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(completeTask)gql"sv, [this](service::ResolverParams&& params) { return resolveCompleteTask(std::move(params)); } },
		{ R"gql(setFloat)gql"sv, [this](service::ResolverParams&& params) { return resolveSetFloat(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<CompleteTaskPayload>> Mutation::applyCompleteTask(service::FieldParams&&, CompleteTaskInput&&) const
{
	throw std::runtime_error(R"ex(Mutation::applyCompleteTask is not implemented)ex");
}

std::future<response::Value> Mutation::resolveCompleteTask(service::ResolverParams&& params)
{
	auto argInput = service::ModifiedArgument<today::CompleteTaskInput>::require("input", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = applyCompleteTask(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argInput));
	resolverLock.unlock();

	return service::ModifiedResult<CompleteTaskPayload>::convert(std::move(result), std::move(params));
}

service::FieldResult<response::FloatType> Mutation::applySetFloat(service::FieldParams&&, response::FloatType&&) const
{
	throw std::runtime_error(R"ex(Mutation::applySetFloat is not implemented)ex");
}

std::future<response::Value> Mutation::resolveSetFloat(service::ResolverParams&& params)
{
	auto argValue = service::ModifiedArgument<response::FloatType>::require("value", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = applySetFloat(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argValue));
	resolverLock.unlock();

	return service::ModifiedResult<response::FloatType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Mutation::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Mutation)gql" }, std::move(params));
}

Subscription::Subscription()
	: service::Object({
		"Subscription"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(nextAppointmentChange)gql"sv, [this](service::ResolverParams&& params) { return resolveNextAppointmentChange(std::move(params)); } },
		{ R"gql(nodeChange)gql"sv, [this](service::ResolverParams&& params) { return resolveNodeChange(std::move(params)); } }
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

Task::Task()
	: service::Object({
		"Node",
		"UnionType",
		"Task"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(id)gql"sv, [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ R"gql(isComplete)gql"sv, [this](service::ResolverParams&& params) { return resolveIsComplete(std::move(params)); } },
		{ R"gql(title)gql"sv, [this](service::ResolverParams&& params) { return resolveTitle(std::move(params)); } }
	})
{
}

service::FieldResult<response::IdType> Task::getId(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Task::getId is not implemented)ex");
}

std::future<response::Value> Task::resolveId(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> Task::getTitle(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Task::getTitle is not implemented)ex");
}

std::future<response::Value> Task::resolveTitle(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getTitle(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::BooleanType> Task::getIsComplete(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Task::getIsComplete is not implemented)ex");
}

std::future<response::Value> Task::resolveIsComplete(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getIsComplete(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::BooleanType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Task::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Task)gql" }, std::move(params));
}

Folder::Folder()
	: service::Object({
		"Node",
		"UnionType",
		"Folder"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(id)gql"sv, [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(unreadCount)gql"sv, [this](service::ResolverParams&& params) { return resolveUnreadCount(std::move(params)); } }
	})
{
}

service::FieldResult<response::IdType> Folder::getId(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Folder::getId is not implemented)ex");
}

std::future<response::Value> Folder::resolveId(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getId(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::optional<response::StringType>> Folder::getName(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Folder::getName is not implemented)ex");
}

std::future<response::Value> Folder::resolveName(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getName(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::StringType>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::FieldResult<response::IntType> Folder::getUnreadCount(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Folder::getUnreadCount is not implemented)ex");
}

std::future<response::Value> Folder::resolveUnreadCount(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getUnreadCount(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Folder::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Folder)gql" }, std::move(params));
}

NestedType::NestedType()
	: service::Object({
		"NestedType"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(depth)gql"sv, [this](service::ResolverParams&& params) { return resolveDepth(std::move(params)); } },
		{ R"gql(nested)gql"sv, [this](service::ResolverParams&& params) { return resolveNested(std::move(params)); } }
	})
{
}

service::FieldResult<response::IntType> NestedType::getDepth(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(NestedType::getDepth is not implemented)ex");
}

std::future<response::Value> NestedType::resolveDepth(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getDepth(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

service::FieldResult<std::shared_ptr<NestedType>> NestedType::getNested(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(NestedType::getNested is not implemented)ex");
}

std::future<response::Value> NestedType::resolveNested(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getNested(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<NestedType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> NestedType::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(NestedType)gql" }, std::move(params));
}

Expensive::Expensive()
	: service::Object({
		"Expensive"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(order)gql"sv, [this](service::ResolverParams&& params) { return resolveOrder(std::move(params)); } }
	})
{
}

service::FieldResult<response::IntType> Expensive::getOrder(service::FieldParams&&) const
{
	throw std::runtime_error(R"ex(Expensive::getOrder is not implemented)ex");
}

std::future<response::Value> Expensive::resolveOrder(service::ResolverParams&& params)
{
	std::unique_lock resolverLock(_resolverMutex);
	auto result = getOrder(service::FieldParams(params, std::move(params.fieldDirectives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IntType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Expensive::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Expensive)gql" }, std::move(params));
}

} /* namespace object */

class ValidationContext : public service::ValidationContext
{
public:
	ValidationContext()
	{
		auto typeBoolean = makeNamedValidateType(service::ScalarType { "Boolean" });
		auto typeFloat = makeNamedValidateType(service::ScalarType { "Float" });
		auto typeID = makeNamedValidateType(service::ScalarType { "ID" });
		auto typeInt = makeNamedValidateType(service::ScalarType { "Int" });
		auto typeString = makeNamedValidateType(service::ScalarType { "String" });


#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
#endif
		auto typeItemCursor = makeNamedValidateType(service::ScalarType { "ItemCursor" });
		auto typeDateTime = makeNamedValidateType(service::ScalarType { "DateTime" });


#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
		auto type__TypeKind = makeNamedValidateType(service::EnumType { "__TypeKind", {
				"SCALAR",
				"OBJECT",
				"INTERFACE",
				"UNION",
				"ENUM",
				"INPUT_OBJECT",
				"LIST",
				"NON_NULL"
			} });
		auto type__DirectiveLocation = makeNamedValidateType(service::EnumType { "__DirectiveLocation", {
				"QUERY",
				"MUTATION",
				"SUBSCRIPTION",
				"FIELD",
				"FRAGMENT_DEFINITION",
				"FRAGMENT_SPREAD",
				"INLINE_FRAGMENT",
				"SCHEMA",
				"SCALAR",
				"OBJECT",
				"FIELD_DEFINITION",
				"ARGUMENT_DEFINITION",
				"INTERFACE",
				"UNION",
				"ENUM",
				"ENUM_VALUE",
				"INPUT_OBJECT",
				"INPUT_FIELD_DEFINITION"
			} });
#endif
		auto typeTaskState = makeNamedValidateType(service::EnumType { "TaskState", {
				"New",
				"Started",
				"Complete",
				"Unassigned"
			} });


#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
#endif
		auto typeCompleteTaskInput = makeNamedValidateType(service::InputObjectType { "CompleteTaskInput" });


#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
#endif
		auto typeUnionType = makeNamedValidateType(service::UnionType { "UnionType" });


#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
#endif
		auto typeNode = makeNamedValidateType(service::InterfaceType { "Node" });


#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
		auto type__Schema = makeNamedValidateType(service::ObjectType { "__Schema" });
		auto type__Type = makeNamedValidateType(service::ObjectType { "__Type" });
		auto type__Field = makeNamedValidateType(service::ObjectType { "__Field" });
		auto type__InputValue = makeNamedValidateType(service::ObjectType { "__InputValue" });
		auto type__EnumValue = makeNamedValidateType(service::ObjectType { "__EnumValue" });
		auto type__Directive = makeNamedValidateType(service::ObjectType { "__Directive" });
#endif
		auto typeQuery = makeNamedValidateType(service::ObjectType { "Query" });
		auto typePageInfo = makeNamedValidateType(service::ObjectType { "PageInfo" });
		auto typeAppointmentEdge = makeNamedValidateType(service::ObjectType { "AppointmentEdge" });
		auto typeAppointmentConnection = makeNamedValidateType(service::ObjectType { "AppointmentConnection" });
		auto typeTaskEdge = makeNamedValidateType(service::ObjectType { "TaskEdge" });
		auto typeTaskConnection = makeNamedValidateType(service::ObjectType { "TaskConnection" });
		auto typeFolderEdge = makeNamedValidateType(service::ObjectType { "FolderEdge" });
		auto typeFolderConnection = makeNamedValidateType(service::ObjectType { "FolderConnection" });
		auto typeCompleteTaskPayload = makeNamedValidateType(service::ObjectType { "CompleteTaskPayload" });
		auto typeMutation = makeNamedValidateType(service::ObjectType { "Mutation" });
		auto typeSubscription = makeNamedValidateType(service::ObjectType { "Subscription" });
		auto typeAppointment = makeNamedValidateType(service::ObjectType { "Appointment" });
		auto typeTask = makeNamedValidateType(service::ObjectType { "Task" });
		auto typeFolder = makeNamedValidateType(service::ObjectType { "Folder" });
		auto typeNestedType = makeNamedValidateType(service::ObjectType { "NestedType" });
		auto typeExpensive = makeNamedValidateType(service::ObjectType { "Expensive" });


#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
#endif
		typeCompleteTaskInput->setFields({
				{ "id", { makeNonNullOfType(typeID), 0, 0 } },
				{ "isComplete", { typeBoolean, 1, 1 } },
				{ "clientMutationId", { typeString, 0, 0 } }
			});


#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
#endif
		typeUnionType->setPossibleTypes({
				typeAppointment.get(),
				typeTask.get(),
				typeFolder.get()
			});
		typeUnionType->setFields({
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});


#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
#endif
		typeNode->setPossibleTypes({
				typeAppointment.get(),
				typeTask.get(),
				typeFolder.get()
			});
		typeNode->setFields({
				{ "id", { makeNonNullOfType(typeID), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});


#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
		type__Schema->setFields({
				{ "types", { makeNonNullOfType(makeListOfType(makeNonNullOfType(type__Type))), {  } } },
				{ "queryType", { makeNonNullOfType(type__Type), {  } } },
				{ "mutationType", { type__Type, {  } } },
				{ "subscriptionType", { type__Type, {  } } },
				{ "directives", { makeNonNullOfType(makeListOfType(makeNonNullOfType(type__Directive))), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		type__Type->setFields({
				{ "kind", { makeNonNullOfType(type__TypeKind), {  } } },
				{ "name", { typeString, {  } } },
				{ "description", { typeString, {  } } },
				{ "fields", { makeListOfType(makeNonNullOfType(type__Field)), { { "includeDeprecated", { typeBoolean, 1, 1 } } } } },
				{ "interfaces", { makeListOfType(makeNonNullOfType(type__Type)), {  } } },
				{ "possibleTypes", { makeListOfType(makeNonNullOfType(type__Type)), {  } } },
				{ "enumValues", { makeListOfType(makeNonNullOfType(type__EnumValue)), { { "includeDeprecated", { typeBoolean, 1, 1 } } } } },
				{ "inputFields", { makeListOfType(makeNonNullOfType(type__InputValue)), {  } } },
				{ "ofType", { type__Type, {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		type__Field->setFields({
				{ "name", { makeNonNullOfType(typeString), {  } } },
				{ "description", { typeString, {  } } },
				{ "args", { makeNonNullOfType(makeListOfType(makeNonNullOfType(type__InputValue))), {  } } },
				{ "type", { makeNonNullOfType(type__Type), {  } } },
				{ "isDeprecated", { makeNonNullOfType(typeBoolean), {  } } },
				{ "deprecationReason", { typeString, {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		type__InputValue->setFields({
				{ "name", { makeNonNullOfType(typeString), {  } } },
				{ "description", { typeString, {  } } },
				{ "type", { makeNonNullOfType(type__Type), {  } } },
				{ "defaultValue", { typeString, {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		type__EnumValue->setFields({
				{ "name", { makeNonNullOfType(typeString), {  } } },
				{ "description", { typeString, {  } } },
				{ "isDeprecated", { makeNonNullOfType(typeBoolean), {  } } },
				{ "deprecationReason", { typeString, {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		type__Directive->setFields({
				{ "name", { makeNonNullOfType(typeString), {  } } },
				{ "description", { typeString, {  } } },
				{ "locations", { makeNonNullOfType(makeListOfType(makeNonNullOfType(type__DirectiveLocation))), {  } } },
				{ "args", { makeNonNullOfType(makeListOfType(makeNonNullOfType(type__InputValue))), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
#endif
		typeQuery->setFields({
				{ "node", { typeNode, { { "id", { makeNonNullOfType(typeID), 0, 0 } } } } },
				{ "appointments", { makeNonNullOfType(typeAppointmentConnection), { { "first", { typeInt, 0, 0 } }, { "after", { typeItemCursor, 0, 0 } }, { "last", { typeInt, 0, 0 } }, { "before", { typeItemCursor, 0, 0 } } } } },
				{ "tasks", { makeNonNullOfType(typeTaskConnection), { { "first", { typeInt, 0, 0 } }, { "after", { typeItemCursor, 0, 0 } }, { "last", { typeInt, 0, 0 } }, { "before", { typeItemCursor, 0, 0 } } } } },
				{ "unreadCounts", { makeNonNullOfType(typeFolderConnection), { { "first", { typeInt, 0, 0 } }, { "after", { typeItemCursor, 0, 0 } }, { "last", { typeInt, 0, 0 } }, { "before", { typeItemCursor, 0, 0 } } } } },
				{ "appointmentsById", { makeNonNullOfType(makeListOfType(typeAppointment)), { { "ids", { makeNonNullOfType(makeListOfType(makeNonNullOfType(typeID))), 1, 1 } } } } },
				{ "tasksById", { makeNonNullOfType(makeListOfType(typeTask)), { { "ids", { makeNonNullOfType(makeListOfType(makeNonNullOfType(typeID))), 0, 0 } } } } },
				{ "unreadCountsById", { makeNonNullOfType(makeListOfType(typeFolder)), { { "ids", { makeNonNullOfType(makeListOfType(makeNonNullOfType(typeID))), 0, 0 } } } } },
				{ "nested", { makeNonNullOfType(typeNestedType), {  } } },
				{ "unimplemented", { makeNonNullOfType(typeString), {  } } },
				{ "expensive", { makeNonNullOfType(makeListOfType(makeNonNullOfType(typeExpensive))), {  } } }
#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
				, { "__schema", { makeNonNullOfType(type__Schema), {  } } }
#endif
#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
				, { "__type", { type__Type, { { "name", { makeNonNullOfType(typeString), 0, 0 } } } } }
#endif
				, { "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typePageInfo->setFields({
				{ "hasNextPage", { makeNonNullOfType(typeBoolean), {  } } },
				{ "hasPreviousPage", { makeNonNullOfType(typeBoolean), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeAppointmentEdge->setFields({
				{ "node", { typeAppointment, {  } } },
				{ "cursor", { makeNonNullOfType(typeItemCursor), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeAppointmentConnection->setFields({
				{ "pageInfo", { makeNonNullOfType(typePageInfo), {  } } },
				{ "edges", { makeListOfType(typeAppointmentEdge), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeTaskEdge->setFields({
				{ "node", { typeTask, {  } } },
				{ "cursor", { makeNonNullOfType(typeItemCursor), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeTaskConnection->setFields({
				{ "pageInfo", { makeNonNullOfType(typePageInfo), {  } } },
				{ "edges", { makeListOfType(typeTaskEdge), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeFolderEdge->setFields({
				{ "node", { typeFolder, {  } } },
				{ "cursor", { makeNonNullOfType(typeItemCursor), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeFolderConnection->setFields({
				{ "pageInfo", { makeNonNullOfType(typePageInfo), {  } } },
				{ "edges", { makeListOfType(typeFolderEdge), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeCompleteTaskPayload->setFields({
				{ "task", { typeTask, {  } } },
				{ "clientMutationId", { typeString, {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeMutation->setFields({
				{ "completeTask", { makeNonNullOfType(typeCompleteTaskPayload), { { "input", { makeNonNullOfType(typeCompleteTaskInput), 0, 0 } } } } },
				{ "setFloat", { makeNonNullOfType(typeFloat), { { "value", { makeNonNullOfType(typeFloat), 0, 0 } } } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeSubscription->setFields({
				{ "nextAppointmentChange", { typeAppointment, {  } } },
				{ "nodeChange", { makeNonNullOfType(typeNode), { { "id", { makeNonNullOfType(typeID), 0, 0 } } } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeAppointment->setFields({
				{ "id", { makeNonNullOfType(typeID), {  } } },
				{ "when", { typeDateTime, {  } } },
				{ "subject", { typeString, {  } } },
				{ "isNow", { makeNonNullOfType(typeBoolean), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeTask->setFields({
				{ "id", { makeNonNullOfType(typeID), {  } } },
				{ "title", { typeString, {  } } },
				{ "isComplete", { makeNonNullOfType(typeBoolean), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeFolder->setFields({
				{ "id", { makeNonNullOfType(typeID), {  } } },
				{ "name", { typeString, {  } } },
				{ "unreadCount", { makeNonNullOfType(typeInt), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeNestedType->setFields({
				{ "depth", { makeNonNullOfType(typeInt), {  } } },
				{ "nested", { makeNonNullOfType(typeNestedType), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});
		typeExpensive->setFields({
				{ "order", { makeNonNullOfType(typeInt), {  } } },
				{ "__typename", { makeNonNullOfType(typeString), {  } } }
			});

		_directives = {
#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
			{ "skip", { { introspection::DirectiveLocation::FIELD, introspection::DirectiveLocation::FRAGMENT_SPREAD, introspection::DirectiveLocation::INLINE_FRAGMENT }, { { "if", { makeNonNullOfType(typeBoolean), 0, 0 } } } } },
			{ "include", { { introspection::DirectiveLocation::FIELD, introspection::DirectiveLocation::FRAGMENT_SPREAD, introspection::DirectiveLocation::INLINE_FRAGMENT }, { { "if", { makeNonNullOfType(typeBoolean), 0, 0 } } } } },
			{ "deprecated", { { introspection::DirectiveLocation::FIELD_DEFINITION, introspection::DirectiveLocation::ENUM_VALUE }, { { "reason", { typeString, 1, 1 } } } } },
#endif
			{ "id", { { introspection::DirectiveLocation::FIELD_DEFINITION }, {  } } },
			{ "subscriptionTag", { { introspection::DirectiveLocation::SUBSCRIPTION }, { { "field", { typeString, 0, 0 } } } } },
			{ "queryTag", { { introspection::DirectiveLocation::QUERY }, { { "query", { makeNonNullOfType(typeString), 0, 0 } } } } },
			{ "fieldTag", { { introspection::DirectiveLocation::FIELD }, { { "field", { makeNonNullOfType(typeString), 0, 0 } } } } },
			{ "fragmentDefinitionTag", { { introspection::DirectiveLocation::FRAGMENT_DEFINITION }, { { "fragmentDefinition", { makeNonNullOfType(typeString), 0, 0 } } } } },
			{ "fragmentSpreadTag", { { introspection::DirectiveLocation::FRAGMENT_SPREAD }, { { "fragmentSpread", { makeNonNullOfType(typeString), 0, 0 } } } } },
			{ "inlineFragmentTag", { { introspection::DirectiveLocation::INLINE_FRAGMENT }, { { "inlineFragment", { makeNonNullOfType(typeString), 0, 0 } } } } }
		};

		_operationTypes.queryType = "Query";
		_operationTypes.mutationType = "Mutation";
		_operationTypes.subscriptionType = "Subscription";
	}
};


Operations::Operations(std::shared_ptr<object::Query> query, std::shared_ptr<object::Mutation> mutation, std::shared_ptr<object::Subscription> subscription)
	: service::Request({
		{ "query", query },
		{ "mutation", mutation },
		{ "subscription", subscription }
	}, std::make_unique<ValidationContext>())
	, _query(std::move(query))
	, _mutation(std::move(mutation))
	, _subscription(std::move(subscription))
{
}

#ifndef SCHEMAGEN_DISABLE_INTROSPECTION
void AddTypesToSchema(const std::shared_ptr<introspection::Schema>& schema)
{
	schema->AddType("ItemCursor", std::make_shared<introspection::ScalarType>("ItemCursor", R"md()md"));
	schema->AddType("DateTime", std::make_shared<introspection::ScalarType>("DateTime", R"md()md"));
	auto typeTaskState = std::make_shared<introspection::EnumType>("TaskState", R"md()md");
	schema->AddType("TaskState", typeTaskState);
	auto typeCompleteTaskInput = std::make_shared<introspection::InputObjectType>("CompleteTaskInput", R"md()md");
	schema->AddType("CompleteTaskInput", typeCompleteTaskInput);
	auto typeUnionType = std::make_shared<introspection::UnionType>("UnionType", R"md()md");
	schema->AddType("UnionType", typeUnionType);
	auto typeNode = std::make_shared<introspection::InterfaceType>("Node", R"md(Node interface for Relay support)md");
	schema->AddType("Node", typeNode);
	auto typeQuery = std::make_shared<introspection::ObjectType>("Query", R"md(Root Query type)md");
	schema->AddType("Query", typeQuery);
	auto typePageInfo = std::make_shared<introspection::ObjectType>("PageInfo", R"md()md");
	schema->AddType("PageInfo", typePageInfo);
	auto typeAppointmentEdge = std::make_shared<introspection::ObjectType>("AppointmentEdge", R"md()md");
	schema->AddType("AppointmentEdge", typeAppointmentEdge);
	auto typeAppointmentConnection = std::make_shared<introspection::ObjectType>("AppointmentConnection", R"md()md");
	schema->AddType("AppointmentConnection", typeAppointmentConnection);
	auto typeTaskEdge = std::make_shared<introspection::ObjectType>("TaskEdge", R"md()md");
	schema->AddType("TaskEdge", typeTaskEdge);
	auto typeTaskConnection = std::make_shared<introspection::ObjectType>("TaskConnection", R"md()md");
	schema->AddType("TaskConnection", typeTaskConnection);
	auto typeFolderEdge = std::make_shared<introspection::ObjectType>("FolderEdge", R"md()md");
	schema->AddType("FolderEdge", typeFolderEdge);
	auto typeFolderConnection = std::make_shared<introspection::ObjectType>("FolderConnection", R"md()md");
	schema->AddType("FolderConnection", typeFolderConnection);
	auto typeCompleteTaskPayload = std::make_shared<introspection::ObjectType>("CompleteTaskPayload", R"md()md");
	schema->AddType("CompleteTaskPayload", typeCompleteTaskPayload);
	auto typeMutation = std::make_shared<introspection::ObjectType>("Mutation", R"md()md");
	schema->AddType("Mutation", typeMutation);
	auto typeSubscription = std::make_shared<introspection::ObjectType>("Subscription", R"md()md");
	schema->AddType("Subscription", typeSubscription);
	auto typeAppointment = std::make_shared<introspection::ObjectType>("Appointment", R"md()md");
	schema->AddType("Appointment", typeAppointment);
	auto typeTask = std::make_shared<introspection::ObjectType>("Task", R"md()md");
	schema->AddType("Task", typeTask);
	auto typeFolder = std::make_shared<introspection::ObjectType>("Folder", R"md()md");
	schema->AddType("Folder", typeFolder);
	auto typeNestedType = std::make_shared<introspection::ObjectType>("NestedType", R"md(Infinitely nestable type which can be used with nested fragments to test directive handling)md");
	schema->AddType("NestedType", typeNestedType);
	auto typeExpensive = std::make_shared<introspection::ObjectType>("Expensive", R"md()md");
	schema->AddType("Expensive", typeExpensive);

	typeTaskState->AddEnumValues({
		{ std::string{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::New)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Started)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Complete)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Unassigned)] }, R"md()md", std::make_optional<response::StringType>(R"md(Need to deprecate an [enum value](https://facebook.github.io/graphql/June2018/#sec-Deprecation))md") }
	});

	typeCompleteTaskInput->AddInputValues({
		std::make_shared<introspection::InputValue>("id", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")), R"gql()gql"),
		std::make_shared<introspection::InputValue>("isComplete", R"md()md", schema->LookupType("Boolean"), R"gql(true)gql"),
		std::make_shared<introspection::InputValue>("clientMutationId", R"md()md", schema->LookupType("String"), R"gql()gql")
	});

	typeUnionType->AddPossibleTypes({
		schema->LookupType("Appointment"),
		schema->LookupType("Task"),
		schema->LookupType("Folder")
	});

	typeNode->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")))
	});

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
	typePageInfo->AddFields({
		std::make_shared<introspection::Field>("hasNextPage", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean"))),
		std::make_shared<introspection::Field>("hasPreviousPage", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeAppointmentEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeAppointmentConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("AppointmentEdge")))
	});
	typeTaskEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeTaskConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("TaskEdge")))
	});
	typeFolderEdge->AddFields({
		std::make_shared<introspection::Field>("node", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Folder")),
		std::make_shared<introspection::Field>("cursor", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ItemCursor")))
	});
	typeFolderConnection->AddFields({
		std::make_shared<introspection::Field>("pageInfo", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("PageInfo"))),
		std::make_shared<introspection::Field>("edges", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::LIST, schema->LookupType("FolderEdge")))
	});
	typeCompleteTaskPayload->AddFields({
		std::make_shared<introspection::Field>("task", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Task")),
		std::make_shared<introspection::Field>("clientMutationId", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String"))
	});
	typeMutation->AddFields({
		std::make_shared<introspection::Field>("completeTask", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("input", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("CompleteTaskInput")), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("CompleteTaskPayload"))),
		std::make_shared<introspection::Field>("setFloat", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("value", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Float")), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Float")))
	});
	typeSubscription->AddFields({
		std::make_shared<introspection::Field>("nextAppointmentChange", R"md()md", std::make_optional<response::StringType>(R"md(Need to deprecate a [field](https://facebook.github.io/graphql/June2018/#sec-Deprecation))md"), std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("Appointment")),
		std::make_shared<introspection::Field>("nodeChange", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("id", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Node")))
	});
	typeAppointment->AddInterfaces({
		typeNode
	});
	typeAppointment->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("when", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("DateTime")),
		std::make_shared<introspection::Field>("subject", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isNow", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeTask->AddInterfaces({
		typeNode
	});
	typeTask->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("title", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("isComplete", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Boolean")))
	});
	typeFolder->AddInterfaces({
		typeNode
	});
	typeFolder->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID"))),
		std::make_shared<introspection::Field>("name", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->LookupType("String")),
		std::make_shared<introspection::Field>("unreadCount", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int")))
	});
	typeNestedType->AddFields({
		std::make_shared<introspection::Field>("depth", R"md(Depth of the nested element)md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int"))),
		std::make_shared<introspection::Field>("nested", R"md(Link to the next level)md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("NestedType")))
	});
	typeExpensive->AddFields({
		std::make_shared<introspection::Field>("order", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Int")))
	});

	schema->AddDirective(std::make_shared<introspection::Directive>("id", R"md()md", std::vector<response::StringType>({
		R"gql(FIELD_DEFINITION)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>()));
	schema->AddDirective(std::make_shared<introspection::Directive>("subscriptionTag", R"md()md", std::vector<response::StringType>({
		R"gql(SUBSCRIPTION)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("field", R"md()md", schema->LookupType("String"), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("queryTag", R"md()md", std::vector<response::StringType>({
		R"gql(QUERY)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("query", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("fieldTag", R"md()md", std::vector<response::StringType>({
		R"gql(FIELD)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("field", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("fragmentDefinitionTag", R"md()md", std::vector<response::StringType>({
		R"gql(FRAGMENT_DEFINITION)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("fragmentDefinition", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("fragmentSpreadTag", R"md()md", std::vector<response::StringType>({
		R"gql(FRAGMENT_SPREAD)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("fragmentSpread", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("inlineFragmentTag", R"md()md", std::vector<response::StringType>({
		R"gql(INLINE_FRAGMENT)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("inlineFragment", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));

	schema->AddQueryType(typeQuery);
	schema->AddMutationType(typeMutation);
	schema->AddSubscriptionType(typeSubscription);
}
#endif

} /* namespace today */
} /* namespace graphql */
