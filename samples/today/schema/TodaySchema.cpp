// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "QueryObject.h"
#include "MutationObject.h"
#include "SubscriptionObject.h"

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

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

static const auto s_namesTaskState = today::getTaskStateNames();

template <>
today::TaskState ModifiedArgument<today::TaskState>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid TaskState value)ex" } };
	}

	const auto itr = std::find(s_namesTaskState.cbegin(), s_namesTaskState.cend(), value.get<std::string>());

	if (itr == s_namesTaskState.cend())
	{
		throw service::schema_exception { { R"ex(not a valid TaskState value)ex" } };
	}

	return static_cast<today::TaskState>(itr - s_namesTaskState.cbegin());
}

template <>
service::AwaitableResolver ModifiedResult<today::TaskState>::convert(service::AwaitableScalar<today::TaskState> result, ResolverParams params)
{
	return resolve(std::move(result), std::move(params),
		[](today::TaskState value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<std::string>(std::string { s_namesTaskState[static_cast<size_t>(value)] });

			return result;
		});
}

template <>
void ModifiedResult<today::TaskState>::validateScalar(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid TaskState value)ex" } };
	}

	const auto itr = std::find(s_namesTaskState.cbegin(), s_namesTaskState.cend(), value.get<std::string>());

	if (itr == s_namesTaskState.cend())
	{
		throw service::schema_exception { { R"ex(not a valid TaskState value)ex" } };
	}
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
	auto valueTestTaskState = service::ModifiedArgument<today::TaskState>::require<service::TypeModifier::Nullable>("testTaskState", value);
	auto pairIsComplete = service::ModifiedArgument<bool>::find<service::TypeModifier::Nullable>("isComplete", value);
	auto valueIsComplete = (pairIsComplete.second
		? std::move(pairIsComplete.first)
		: service::ModifiedArgument<bool>::require<service::TypeModifier::Nullable>("isComplete", defaultValue));
	auto valueClientMutationId = service::ModifiedArgument<std::string>::require<service::TypeModifier::Nullable>("clientMutationId", value);

	return {
		std::move(valueId),
		std::move(valueTestTaskState),
		std::move(valueIsComplete),
		std::move(valueClientMutationId)
	};
}

template <>
today::ThirdNestedInput ModifiedArgument<today::ThirdNestedInput>::convert(const response::Value& value)
{
	auto valueId = service::ModifiedArgument<response::IdType>::require("id", value);

	return {
		std::move(valueId)
	};
}

template <>
today::FourthNestedInput ModifiedArgument<today::FourthNestedInput>::convert(const response::Value& value)
{
	auto valueId = service::ModifiedArgument<response::IdType>::require("id", value);

	return {
		std::move(valueId)
	};
}

template <>
today::SecondNestedInput ModifiedArgument<today::SecondNestedInput>::convert(const response::Value& value)
{
	auto valueId = service::ModifiedArgument<response::IdType>::require("id", value);
	auto valueThird = service::ModifiedArgument<today::ThirdNestedInput>::require("third", value);

	return {
		std::move(valueId),
		std::move(valueThird)
	};
}

template <>
today::FirstNestedInput ModifiedArgument<today::FirstNestedInput>::convert(const response::Value& value)
{
	auto valueId = service::ModifiedArgument<response::IdType>::require("id", value);
	auto valueSecond = service::ModifiedArgument<today::SecondNestedInput>::require("second", value);
	auto valueThird = service::ModifiedArgument<today::ThirdNestedInput>::require("third", value);

	return {
		std::move(valueId),
		std::move(valueSecond),
		std::move(valueThird)
	};
}

} // namespace service

namespace today {

Operations::Operations(std::shared_ptr<object::Query> query, std::shared_ptr<object::Mutation> mutation, std::shared_ptr<object::Subscription> subscription)
	: service::Request({
		{ "query", query },
		{ "mutation", mutation },
		{ "subscription", subscription }
	}, GetSchema())
	, _query(std::move(query))
	, _mutation(std::move(mutation))
	, _subscription(std::move(subscription))
{
}

void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema)
{
	schema->AddType(R"gql(ItemCursor)gql"sv, schema::ScalarType::Make(R"gql(ItemCursor)gql"sv, R"md()md", R"url()url"sv));
	schema->AddType(R"gql(DateTime)gql"sv, schema::ScalarType::Make(R"gql(DateTime)gql"sv, R"md()md", R"url(https://en.wikipedia.org/wiki/ISO_8601)url"sv));
	auto typeTaskState = schema::EnumType::Make(R"gql(TaskState)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(TaskState)gql"sv, typeTaskState);
	auto typeCompleteTaskInput = schema::InputObjectType::Make(R"gql(CompleteTaskInput)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(CompleteTaskInput)gql"sv, typeCompleteTaskInput);
	auto typeThirdNestedInput = schema::InputObjectType::Make(R"gql(ThirdNestedInput)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(ThirdNestedInput)gql"sv, typeThirdNestedInput);
	auto typeFourthNestedInput = schema::InputObjectType::Make(R"gql(FourthNestedInput)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(FourthNestedInput)gql"sv, typeFourthNestedInput);
	auto typeSecondNestedInput = schema::InputObjectType::Make(R"gql(SecondNestedInput)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(SecondNestedInput)gql"sv, typeSecondNestedInput);
	auto typeFirstNestedInput = schema::InputObjectType::Make(R"gql(FirstNestedInput)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(FirstNestedInput)gql"sv, typeFirstNestedInput);
	auto typeNode = schema::InterfaceType::Make(R"gql(Node)gql"sv, R"md(Node interface for Relay support)md"sv);
	schema->AddType(R"gql(Node)gql"sv, typeNode);
	auto typeUnionType = schema::UnionType::Make(R"gql(UnionType)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(UnionType)gql"sv, typeUnionType);
	auto typeQuery = schema::ObjectType::Make(R"gql(Query)gql"sv, R"md(Root Query type)md"sv);
	schema->AddType(R"gql(Query)gql"sv, typeQuery);
	auto typePageInfo = schema::ObjectType::Make(R"gql(PageInfo)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(PageInfo)gql"sv, typePageInfo);
	auto typeAppointmentEdge = schema::ObjectType::Make(R"gql(AppointmentEdge)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(AppointmentEdge)gql"sv, typeAppointmentEdge);
	auto typeAppointmentConnection = schema::ObjectType::Make(R"gql(AppointmentConnection)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(AppointmentConnection)gql"sv, typeAppointmentConnection);
	auto typeTaskEdge = schema::ObjectType::Make(R"gql(TaskEdge)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(TaskEdge)gql"sv, typeTaskEdge);
	auto typeTaskConnection = schema::ObjectType::Make(R"gql(TaskConnection)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(TaskConnection)gql"sv, typeTaskConnection);
	auto typeFolderEdge = schema::ObjectType::Make(R"gql(FolderEdge)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(FolderEdge)gql"sv, typeFolderEdge);
	auto typeFolderConnection = schema::ObjectType::Make(R"gql(FolderConnection)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(FolderConnection)gql"sv, typeFolderConnection);
	auto typeCompleteTaskPayload = schema::ObjectType::Make(R"gql(CompleteTaskPayload)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(CompleteTaskPayload)gql"sv, typeCompleteTaskPayload);
	auto typeMutation = schema::ObjectType::Make(R"gql(Mutation)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Mutation)gql"sv, typeMutation);
	auto typeSubscription = schema::ObjectType::Make(R"gql(Subscription)gql"sv, R"md(Subscription type:

2nd line...
    3rd line goes here!)md"sv);
	schema->AddType(R"gql(Subscription)gql"sv, typeSubscription);
	auto typeAppointment = schema::ObjectType::Make(R"gql(Appointment)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Appointment)gql"sv, typeAppointment);
	auto typeTask = schema::ObjectType::Make(R"gql(Task)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Task)gql"sv, typeTask);
	auto typeFolder = schema::ObjectType::Make(R"gql(Folder)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Folder)gql"sv, typeFolder);
	auto typeNestedType = schema::ObjectType::Make(R"gql(NestedType)gql"sv, R"md(Infinitely nestable type which can be used with nested fragments to test directive handling)md"sv);
	schema->AddType(R"gql(NestedType)gql"sv, typeNestedType);
	auto typeExpensive = schema::ObjectType::Make(R"gql(Expensive)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Expensive)gql"sv, typeExpensive);

	typeTaskState->AddEnumValues({
		{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::New)], R"md()md"sv, std::nullopt },
		{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Started)], R"md()md"sv, std::nullopt },
		{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Complete)], R"md()md"sv, std::nullopt },
		{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Unassigned)], R"md()md"sv, std::make_optional(R"md(Need to deprecate an [enum value](https://spec.graphql.org/October2021/#sec-Schema-Introspection.Deprecation))md"sv) }
	});

	typeCompleteTaskInput->AddInputValues({
		schema::InputValue::Make(R"gql(id)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv)), R"gql()gql"sv),
		schema::InputValue::Make(R"gql(testTaskState)gql"sv, R"md()md"sv, schema->LookupType(R"gql(TaskState)gql"sv), R"gql()gql"sv),
		schema::InputValue::Make(R"gql(isComplete)gql"sv, R"md()md"sv, schema->LookupType(R"gql(Boolean)gql"sv), R"gql(true)gql"sv),
		schema::InputValue::Make(R"gql(clientMutationId)gql"sv, R"md()md"sv, schema->LookupType(R"gql(String)gql"sv), R"gql()gql"sv)
	});
	typeThirdNestedInput->AddInputValues({
		schema::InputValue::Make(R"gql(id)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv)), R"gql()gql"sv)
	});
	typeFourthNestedInput->AddInputValues({
		schema::InputValue::Make(R"gql(id)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv)), R"gql()gql"sv)
	});
	typeSecondNestedInput->AddInputValues({
		schema::InputValue::Make(R"gql(id)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv)), R"gql()gql"sv),
		schema::InputValue::Make(R"gql(third)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ThirdNestedInput)gql"sv)), R"gql()gql"sv)
	});
	typeFirstNestedInput->AddInputValues({
		schema::InputValue::Make(R"gql(id)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv)), R"gql()gql"sv),
		schema::InputValue::Make(R"gql(second)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(SecondNestedInput)gql"sv)), R"gql()gql"sv),
		schema::InputValue::Make(R"gql(third)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ThirdNestedInput)gql"sv)), R"gql()gql"sv)
	});

	AddNodeDetails(typeNode, schema);

	AddUnionTypeDetails(typeUnionType, schema);

	AddQueryDetails(typeQuery, schema);
	AddPageInfoDetails(typePageInfo, schema);
	AddAppointmentEdgeDetails(typeAppointmentEdge, schema);
	AddAppointmentConnectionDetails(typeAppointmentConnection, schema);
	AddTaskEdgeDetails(typeTaskEdge, schema);
	AddTaskConnectionDetails(typeTaskConnection, schema);
	AddFolderEdgeDetails(typeFolderEdge, schema);
	AddFolderConnectionDetails(typeFolderConnection, schema);
	AddCompleteTaskPayloadDetails(typeCompleteTaskPayload, schema);
	AddMutationDetails(typeMutation, schema);
	AddSubscriptionDetails(typeSubscription, schema);
	AddAppointmentDetails(typeAppointment, schema);
	AddTaskDetails(typeTask, schema);
	AddFolderDetails(typeFolder, schema);
	AddNestedTypeDetails(typeNestedType, schema);
	AddExpensiveDetails(typeExpensive, schema);

	schema->AddDirective(schema::Directive::Make(R"gql(id)gql"sv, R"md()md"sv, {
		introspection::DirectiveLocation::FIELD_DEFINITION
	}, {}, false));
	schema->AddDirective(schema::Directive::Make(R"gql(queryTag)gql"sv, R"md()md"sv, {
		introspection::DirectiveLocation::QUERY
	}, {
		schema::InputValue::Make(R"gql(query)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(String)gql"sv)), R"gql()gql"sv)
	}, false));
	schema->AddDirective(schema::Directive::Make(R"gql(fieldTag)gql"sv, R"md()md"sv, {
		introspection::DirectiveLocation::FIELD
	}, {
		schema::InputValue::Make(R"gql(field)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(String)gql"sv)), R"gql()gql"sv)
	}, false));
	schema->AddDirective(schema::Directive::Make(R"gql(fragmentDefinitionTag)gql"sv, R"md()md"sv, {
		introspection::DirectiveLocation::FRAGMENT_DEFINITION
	}, {
		schema::InputValue::Make(R"gql(fragmentDefinition)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(String)gql"sv)), R"gql()gql"sv)
	}, false));
	schema->AddDirective(schema::Directive::Make(R"gql(fragmentSpreadTag)gql"sv, R"md()md"sv, {
		introspection::DirectiveLocation::FRAGMENT_SPREAD
	}, {
		schema::InputValue::Make(R"gql(fragmentSpread)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(String)gql"sv)), R"gql()gql"sv)
	}, false));
	schema->AddDirective(schema::Directive::Make(R"gql(inlineFragmentTag)gql"sv, R"md()md"sv, {
		introspection::DirectiveLocation::INLINE_FRAGMENT
	}, {
		schema::InputValue::Make(R"gql(inlineFragment)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(String)gql"sv)), R"gql()gql"sv)
	}, false));
	schema->AddDirective(schema::Directive::Make(R"gql(repeatableOnField)gql"sv, R"md()md"sv, {
		introspection::DirectiveLocation::FIELD
	}, {}, true));

	schema->AddQueryType(typeQuery);
	schema->AddMutationType(typeMutation);
	schema->AddSubscriptionType(typeSubscription);
}

std::shared_ptr<schema::Schema> GetSchema()
{
	static std::weak_ptr<schema::Schema> s_wpSchema;
	auto schema = s_wpSchema.lock();

	if (!schema)
	{
		schema = std::make_shared<schema::Schema>(false, R"md(Test Schema based on a dashboard showing daily appointments, tasks, and email folders with unread counts.)md"sv);
		introspection::AddTypesToSchema(schema);
		AddTypesToSchema(schema);
		s_wpSchema = schema;
	}

	return schema;
}

} // namespace today
} // namespace graphql
