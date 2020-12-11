// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayObjects.h"

#include "graphqlservice/Introspection.h"

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
	schema->AddType(R"gql(ItemCursor)gql"sv, std::make_shared<schema::ScalarType>(R"gql(ItemCursor)gql"sv, R"md()md"));
	schema->AddType(R"gql(DateTime)gql"sv, std::make_shared<schema::ScalarType>(R"gql(DateTime)gql"sv, R"md()md"));
	auto typeTaskState = std::make_shared<schema::EnumType>(R"gql(TaskState)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(TaskState)gql"sv, typeTaskState);
	auto typeCompleteTaskInput = std::make_shared<schema::InputObjectType>(R"gql(CompleteTaskInput)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(CompleteTaskInput)gql"sv, typeCompleteTaskInput);
	auto typeUnionType = std::make_shared<schema::UnionType>(R"gql(UnionType)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(UnionType)gql"sv, typeUnionType);
	auto typeNode = std::make_shared<schema::InterfaceType>(R"gql(Node)gql"sv, R"md(Node interface for Relay support)md"sv);
	schema->AddType(R"gql(Node)gql"sv, typeNode);
	auto typeQuery = std::make_shared<schema::ObjectType>(R"gql(Query)gql"sv, R"md(Root Query type)md");
	schema->AddType(R"gql(Query)gql"sv, typeQuery);
	auto typePageInfo = std::make_shared<schema::ObjectType>(R"gql(PageInfo)gql"sv, R"md()md");
	schema->AddType(R"gql(PageInfo)gql"sv, typePageInfo);
	auto typeAppointmentEdge = std::make_shared<schema::ObjectType>(R"gql(AppointmentEdge)gql"sv, R"md()md");
	schema->AddType(R"gql(AppointmentEdge)gql"sv, typeAppointmentEdge);
	auto typeAppointmentConnection = std::make_shared<schema::ObjectType>(R"gql(AppointmentConnection)gql"sv, R"md()md");
	schema->AddType(R"gql(AppointmentConnection)gql"sv, typeAppointmentConnection);
	auto typeTaskEdge = std::make_shared<schema::ObjectType>(R"gql(TaskEdge)gql"sv, R"md()md");
	schema->AddType(R"gql(TaskEdge)gql"sv, typeTaskEdge);
	auto typeTaskConnection = std::make_shared<schema::ObjectType>(R"gql(TaskConnection)gql"sv, R"md()md");
	schema->AddType(R"gql(TaskConnection)gql"sv, typeTaskConnection);
	auto typeFolderEdge = std::make_shared<schema::ObjectType>(R"gql(FolderEdge)gql"sv, R"md()md");
	schema->AddType(R"gql(FolderEdge)gql"sv, typeFolderEdge);
	auto typeFolderConnection = std::make_shared<schema::ObjectType>(R"gql(FolderConnection)gql"sv, R"md()md");
	schema->AddType(R"gql(FolderConnection)gql"sv, typeFolderConnection);
	auto typeCompleteTaskPayload = std::make_shared<schema::ObjectType>(R"gql(CompleteTaskPayload)gql"sv, R"md()md");
	schema->AddType(R"gql(CompleteTaskPayload)gql"sv, typeCompleteTaskPayload);
	auto typeMutation = std::make_shared<schema::ObjectType>(R"gql(Mutation)gql"sv, R"md()md");
	schema->AddType(R"gql(Mutation)gql"sv, typeMutation);
	auto typeSubscription = std::make_shared<schema::ObjectType>(R"gql(Subscription)gql"sv, R"md()md");
	schema->AddType(R"gql(Subscription)gql"sv, typeSubscription);
	auto typeAppointment = std::make_shared<schema::ObjectType>(R"gql(Appointment)gql"sv, R"md()md");
	schema->AddType(R"gql(Appointment)gql"sv, typeAppointment);
	auto typeTask = std::make_shared<schema::ObjectType>(R"gql(Task)gql"sv, R"md()md");
	schema->AddType(R"gql(Task)gql"sv, typeTask);
	auto typeFolder = std::make_shared<schema::ObjectType>(R"gql(Folder)gql"sv, R"md()md");
	schema->AddType(R"gql(Folder)gql"sv, typeFolder);
	auto typeNestedType = std::make_shared<schema::ObjectType>(R"gql(NestedType)gql"sv, R"md(Infinitely nestable type which can be used with nested fragments to test directive handling)md");
	schema->AddType(R"gql(NestedType)gql"sv, typeNestedType);
	auto typeExpensive = std::make_shared<schema::ObjectType>(R"gql(Expensive)gql"sv, R"md()md");
	schema->AddType(R"gql(Expensive)gql"sv, typeExpensive);

	typeTaskState->AddEnumValues({
		{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::New)], R"md()md"sv, std::nullopt },
		{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Started)], R"md()md"sv, std::nullopt },
		{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Complete)], R"md()md"sv, std::nullopt },
		{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Unassigned)], R"md()md"sv, std::make_optional(R"md(Need to deprecate an [enum value](https://facebook.github.io/graphql/June2018/#sec-Deprecation))md"sv) }
	});

	typeCompleteTaskInput->AddInputValues({
		std::make_shared<schema::InputValue>(R"gql(id)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")), R"gql()gql"sv),
		std::make_shared<schema::InputValue>(R"gql(isComplete)gql"sv, R"md()md"sv, schema->LookupType("Boolean"), R"gql(true)gql"sv),
		std::make_shared<schema::InputValue>(R"gql(clientMutationId)gql"sv, R"md()md"sv, schema->LookupType("String"), R"gql()gql"sv)
	});

	typeUnionType->AddPossibleTypes({
		schema->LookupType(R"gql(Appointment)gql"sv),
		schema->LookupType(R"gql(Task)gql"sv),
		schema->LookupType(R"gql(Folder)gql"sv)
	});

	typeNode->AddFields({
		std::make_shared<schema::Field>(R"gql(id)gql"sv, R"md()md"sv, std::nullopt, std::vector<std::shared_ptr<schema::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")))
	});

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

	schema->AddDirective(std::make_shared<schema::Directive>(R"gql(id)gql"sv, R"md()md"sv, std::vector<introspection::DirectiveLocation>({
		introspection::DirectiveLocation::FIELD_DEFINITION
	}), std::vector<std::shared_ptr<schema::InputValue>>()));
	schema->AddDirective(std::make_shared<schema::Directive>(R"gql(subscriptionTag)gql"sv, R"md()md"sv, std::vector<introspection::DirectiveLocation>({
		introspection::DirectiveLocation::SUBSCRIPTION
	}), std::vector<std::shared_ptr<schema::InputValue>>({
		std::make_shared<schema::InputValue>(R"gql(field)gql"sv, R"md()md"sv, schema->LookupType("String"), R"gql()gql"sv)
	})));
	schema->AddDirective(std::make_shared<schema::Directive>(R"gql(queryTag)gql"sv, R"md()md"sv, std::vector<introspection::DirectiveLocation>({
		introspection::DirectiveLocation::QUERY
	}), std::vector<std::shared_ptr<schema::InputValue>>({
		std::make_shared<schema::InputValue>(R"gql(query)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql"sv)
	})));
	schema->AddDirective(std::make_shared<schema::Directive>(R"gql(fieldTag)gql"sv, R"md()md"sv, std::vector<introspection::DirectiveLocation>({
		introspection::DirectiveLocation::FIELD
	}), std::vector<std::shared_ptr<schema::InputValue>>({
		std::make_shared<schema::InputValue>(R"gql(field)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql"sv)
	})));
	schema->AddDirective(std::make_shared<schema::Directive>(R"gql(fragmentDefinitionTag)gql"sv, R"md()md"sv, std::vector<introspection::DirectiveLocation>({
		introspection::DirectiveLocation::FRAGMENT_DEFINITION
	}), std::vector<std::shared_ptr<schema::InputValue>>({
		std::make_shared<schema::InputValue>(R"gql(fragmentDefinition)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql"sv)
	})));
	schema->AddDirective(std::make_shared<schema::Directive>(R"gql(fragmentSpreadTag)gql"sv, R"md()md"sv, std::vector<introspection::DirectiveLocation>({
		introspection::DirectiveLocation::FRAGMENT_SPREAD
	}), std::vector<std::shared_ptr<schema::InputValue>>({
		std::make_shared<schema::InputValue>(R"gql(fragmentSpread)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql"sv)
	})));
	schema->AddDirective(std::make_shared<schema::Directive>(R"gql(inlineFragmentTag)gql"sv, R"md()md"sv, std::vector<introspection::DirectiveLocation>({
		introspection::DirectiveLocation::INLINE_FRAGMENT
	}), std::vector<std::shared_ptr<schema::InputValue>>({
		std::make_shared<schema::InputValue>(R"gql(inlineFragment)gql"sv, R"md()md"sv, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql"sv)
	})));

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
		schema = std::make_shared<schema::Schema>();
		introspection::AddTypesToSchema(schema);
		AddTypesToSchema(schema);
		s_wpSchema = schema;
	}

	return schema;
}

} /* namespace today */
} /* namespace graphql */
