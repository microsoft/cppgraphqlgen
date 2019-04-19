// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <graphqlservice/GraphQLService.h>

#include <memory>
#include <string>
#include <vector>

namespace facebook::graphql {
namespace introspection {

class Schema;
class ObjectType;

} /* namespace introspection */

namespace today {

enum class TaskState
{
	New,
	Started,
	Complete,
	Unassigned
};

struct CompleteTaskInput
{
	response::IdType id;
	std::optional<response::BooleanType> isComplete;
	std::optional<response::StringType> clientMutationId;
};

namespace object {

class Query;
class PageInfo;
class AppointmentEdge;
class AppointmentConnection;
class TaskEdge;
class TaskConnection;
class FolderEdge;
class FolderConnection;
class CompleteTaskPayload;
class Mutation;
class Subscription;
class Appointment;
class Task;
class Folder;
class NestedType;

} /* namespace object */

struct Node
{
	virtual service::FieldResult<response::IdType> getId(service::FieldParams&& params) const = 0;
};

class Operations
	: public service::Request
{
public:
	Operations(std::shared_ptr<object::Query> query, std::shared_ptr<object::Mutation> mutation, std::shared_ptr<object::Subscription> subscription);

private:
	std::shared_ptr<object::Query> _query;
	std::shared_ptr<object::Mutation> _mutation;
	std::shared_ptr<object::Subscription> _subscription;
};

void AddQueryDetails(std::shared_ptr<introspection::ObjectType> typeQuery, std::shared_ptr<introspection::Schema> schema);
void AddPageInfoDetails(std::shared_ptr<introspection::ObjectType> typePageInfo, std::shared_ptr<introspection::Schema> schema);
void AddAppointmentEdgeDetails(std::shared_ptr<introspection::ObjectType> typeAppointmentEdge, std::shared_ptr<introspection::Schema> schema);
void AddAppointmentConnectionDetails(std::shared_ptr<introspection::ObjectType> typeAppointmentConnection, std::shared_ptr<introspection::Schema> schema);
void AddTaskEdgeDetails(std::shared_ptr<introspection::ObjectType> typeTaskEdge, std::shared_ptr<introspection::Schema> schema);
void AddTaskConnectionDetails(std::shared_ptr<introspection::ObjectType> typeTaskConnection, std::shared_ptr<introspection::Schema> schema);
void AddFolderEdgeDetails(std::shared_ptr<introspection::ObjectType> typeFolderEdge, std::shared_ptr<introspection::Schema> schema);
void AddFolderConnectionDetails(std::shared_ptr<introspection::ObjectType> typeFolderConnection, std::shared_ptr<introspection::Schema> schema);
void AddCompleteTaskPayloadDetails(std::shared_ptr<introspection::ObjectType> typeCompleteTaskPayload, std::shared_ptr<introspection::Schema> schema);
void AddMutationDetails(std::shared_ptr<introspection::ObjectType> typeMutation, std::shared_ptr<introspection::Schema> schema);
void AddSubscriptionDetails(std::shared_ptr<introspection::ObjectType> typeSubscription, std::shared_ptr<introspection::Schema> schema);
void AddAppointmentDetails(std::shared_ptr<introspection::ObjectType> typeAppointment, std::shared_ptr<introspection::Schema> schema);
void AddTaskDetails(std::shared_ptr<introspection::ObjectType> typeTask, std::shared_ptr<introspection::Schema> schema);
void AddFolderDetails(std::shared_ptr<introspection::ObjectType> typeFolder, std::shared_ptr<introspection::Schema> schema);
void AddNestedTypeDetails(std::shared_ptr<introspection::ObjectType> typeNestedType, std::shared_ptr<introspection::Schema> schema);

void AddTypesToSchema(std::shared_ptr<introspection::Schema> schema);

} /* namespace today */
} /* namespace facebook::graphql */
