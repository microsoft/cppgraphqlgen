// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "TodayObjects.h"

#include "graphqlservice/introspection/Introspection.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

namespace graphql::today {
namespace object {

Query::Query(std::unique_ptr<Concept>&& pimpl) noexcept
	: service::Object{ getTypeNames(), getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}

service::TypeNames Query::getTypeNames() const noexcept
{
	return {
		R"gql(Query)gql"sv
	};
}

service::ResolverMap Query::getResolvers() const noexcept
{
	return {
		{ R"gql(node)gql"sv, [this](service::ResolverParams&& params) { return resolveNode(std::move(params)); } },
		{ R"gql(tasks)gql"sv, [this](service::ResolverParams&& params) { return resolveTasks(std::move(params)); } },
		{ R"gql(nested)gql"sv, [this](service::ResolverParams&& params) { return resolveNested(std::move(params)); } },
		{ R"gql(anyType)gql"sv, [this](service::ResolverParams&& params) { return resolveAnyType(std::move(params)); } },
		{ R"gql(expensive)gql"sv, [this](service::ResolverParams&& params) { return resolveExpensive(std::move(params)); } },
		{ R"gql(tasksById)gql"sv, [this](service::ResolverParams&& params) { return resolveTasksById(std::move(params)); } },
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(appointments)gql"sv, [this](service::ResolverParams&& params) { return resolveAppointments(std::move(params)); } },
		{ R"gql(unreadCounts)gql"sv, [this](service::ResolverParams&& params) { return resolveUnreadCounts(std::move(params)); } },
		{ R"gql(testTaskState)gql"sv, [this](service::ResolverParams&& params) { return resolveTestTaskState(std::move(params)); } },
		{ R"gql(unimplemented)gql"sv, [this](service::ResolverParams&& params) { return resolveUnimplemented(std::move(params)); } },
		{ R"gql(appointmentsById)gql"sv, [this](service::ResolverParams&& params) { return resolveAppointmentsById(std::move(params)); } },
		{ R"gql(unreadCountsById)gql"sv, [this](service::ResolverParams&& params) { return resolveUnreadCountsById(std::move(params)); } }
	};
}

void Query::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void Query::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}

service::AwaitableResolver Query::resolveNode(service::ResolverParams&& params) const
{
	auto argId = service::ModifiedArgument<response::IdType>::require("id", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getNode(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)), std::move(argId));
	resolverLock.unlock();

	return service::ModifiedResult<Node>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveAppointments(service::ResolverParams&& params) const
{
	auto argFirst = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getAppointments(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)), std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));
	resolverLock.unlock();

	return service::ModifiedResult<AppointmentConnection>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveTasks(service::ResolverParams&& params) const
{
	auto argFirst = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getTasks(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)), std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));
	resolverLock.unlock();

	return service::ModifiedResult<TaskConnection>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveUnreadCounts(service::ResolverParams&& params) const
{
	auto argFirst = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("first", params.arguments);
	auto argAfter = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("after", params.arguments);
	auto argLast = service::ModifiedArgument<int>::require<service::TypeModifier::Nullable>("last", params.arguments);
	auto argBefore = service::ModifiedArgument<response::Value>::require<service::TypeModifier::Nullable>("before", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getUnreadCounts(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)), std::move(argFirst), std::move(argAfter), std::move(argLast), std::move(argBefore));
	resolverLock.unlock();

	return service::ModifiedResult<FolderConnection>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveAppointmentsById(service::ResolverParams&& params) const
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
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getAppointmentsById(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)), std::move(argIds));
	resolverLock.unlock();

	return service::ModifiedResult<Appointment>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveTasksById(service::ResolverParams&& params) const
{
	auto argIds = service::ModifiedArgument<response::IdType>::require<service::TypeModifier::List>("ids", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getTasksById(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)), std::move(argIds));
	resolverLock.unlock();

	return service::ModifiedResult<Task>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveUnreadCountsById(service::ResolverParams&& params) const
{
	auto argIds = service::ModifiedArgument<response::IdType>::require<service::TypeModifier::List>("ids", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getUnreadCountsById(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)), std::move(argIds));
	resolverLock.unlock();

	return service::ModifiedResult<Folder>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveNested(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getNested(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<NestedType>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveUnimplemented(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getUnimplemented(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<std::string>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveExpensive(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getExpensive(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<Expensive>::convert<service::TypeModifier::List>(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveTestTaskState(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getTestTaskState(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<TaskState>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolveAnyType(service::ResolverParams&& params) const
{
	auto argIds = service::ModifiedArgument<response::IdType>::require<service::TypeModifier::List>("ids", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getAnyType(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)), std::move(argIds));
	resolverLock.unlock();

	return service::ModifiedResult<service::Object>::convert<service::TypeModifier::List, service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Query::resolve_typename(service::ResolverParams&& params) const
{
	return service::ModifiedResult<std::string>::convert(std::string{ R"gql(Query)gql" }, std::move(params));
}

} // namespace object

void AddQueryDetails(const std::shared_ptr<schema::ObjectType>& typeQuery, const std::shared_ptr<schema::Schema>& schema)
{
	typeQuery->AddFields({
		schema::Field::Make(R"gql(node)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType(R"gql(Node)gql"sv), {
			schema::InputValue::Make(R"gql(id)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv)), R"gql()gql"sv)
		}),
		schema::Field::Make(R"gql(appointments)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(AppointmentConnection)gql"sv)), {
			schema::InputValue::Make(R"gql(first)gql"sv, R"md()md"sv, schema->LookupType(R"gql(Int)gql"sv), R"gql()gql"sv),
			schema::InputValue::Make(R"gql(after)gql"sv, R"md()md"sv, schema->LookupType(R"gql(ItemCursor)gql"sv), R"gql()gql"sv),
			schema::InputValue::Make(R"gql(last)gql"sv, R"md()md"sv, schema->LookupType(R"gql(Int)gql"sv), R"gql()gql"sv),
			schema::InputValue::Make(R"gql(before)gql"sv, R"md()md"sv, schema->LookupType(R"gql(ItemCursor)gql"sv), R"gql()gql"sv)
		}),
		schema::Field::Make(R"gql(tasks)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(TaskConnection)gql"sv)), {
			schema::InputValue::Make(R"gql(first)gql"sv, R"md()md"sv, schema->LookupType(R"gql(Int)gql"sv), R"gql()gql"sv),
			schema::InputValue::Make(R"gql(after)gql"sv, R"md()md"sv, schema->LookupType(R"gql(ItemCursor)gql"sv), R"gql()gql"sv),
			schema::InputValue::Make(R"gql(last)gql"sv, R"md()md"sv, schema->LookupType(R"gql(Int)gql"sv), R"gql()gql"sv),
			schema::InputValue::Make(R"gql(before)gql"sv, R"md()md"sv, schema->LookupType(R"gql(ItemCursor)gql"sv), R"gql()gql"sv)
		}),
		schema::Field::Make(R"gql(unreadCounts)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(FolderConnection)gql"sv)), {
			schema::InputValue::Make(R"gql(first)gql"sv, R"md()md"sv, schema->LookupType(R"gql(Int)gql"sv), R"gql()gql"sv),
			schema::InputValue::Make(R"gql(after)gql"sv, R"md()md"sv, schema->LookupType(R"gql(ItemCursor)gql"sv), R"gql()gql"sv),
			schema::InputValue::Make(R"gql(last)gql"sv, R"md()md"sv, schema->LookupType(R"gql(Int)gql"sv), R"gql()gql"sv),
			schema::InputValue::Make(R"gql(before)gql"sv, R"md()md"sv, schema->LookupType(R"gql(ItemCursor)gql"sv), R"gql()gql"sv)
		}),
		schema::Field::Make(R"gql(appointmentsById)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType(R"gql(Appointment)gql"sv))), {
			schema::InputValue::Make(R"gql(ids)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv)))), R"gql(["ZmFrZUFwcG9pbnRtZW50SWQ="])gql"sv)
		}),
		schema::Field::Make(R"gql(tasksById)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType(R"gql(Task)gql"sv))), {
			schema::InputValue::Make(R"gql(ids)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv)))), R"gql()gql"sv)
		}),
		schema::Field::Make(R"gql(unreadCountsById)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType(R"gql(Folder)gql"sv))), {
			schema::InputValue::Make(R"gql(ids)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv)))), R"gql()gql"sv)
		}),
		schema::Field::Make(R"gql(nested)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(NestedType)gql"sv))),
		schema::Field::Make(R"gql(unimplemented)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(String)gql"sv))),
		schema::Field::Make(R"gql(expensive)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Expensive)gql"sv))))),
		schema::Field::Make(R"gql(testTaskState)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(TaskState)gql"sv))),
		schema::Field::Make(R"gql(anyType)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->LookupType(R"gql(UnionType)gql"sv))), {
			schema::InputValue::Make(R"gql(ids)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->WrapType(introspection::TypeKind::LIST, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv)))), R"gql()gql"sv)
		})
	});
}

} // namespace graphql::today