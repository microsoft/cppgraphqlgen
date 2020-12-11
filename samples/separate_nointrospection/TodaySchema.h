// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef TODAYSCHEMA_H
#define TODAYSCHEMA_H

#include "graphqlservice/GraphQLSchema.h"
#include "graphqlservice/GraphQLService.h"

#include <memory>
#include <string>
#include <vector>

namespace graphql {
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
class Expensive;

} /* namespace object */

struct Node
{
	virtual service::FieldResult<response::IdType> getId(service::FieldParams&& params) const = 0;
};

class Operations
	: public service::Request
{
public:
	explicit Operations(std::shared_ptr<object::Query> query, std::shared_ptr<object::Mutation> mutation, std::shared_ptr<object::Subscription> subscription);

private:
	std::shared_ptr<object::Query> _query;
	std::shared_ptr<object::Mutation> _mutation;
	std::shared_ptr<object::Subscription> _subscription;
};

void AddQueryDetails(std::shared_ptr<schema::ObjectType> typeQuery, const std::shared_ptr<schema::Schema>& schema);
void AddPageInfoDetails(std::shared_ptr<schema::ObjectType> typePageInfo, const std::shared_ptr<schema::Schema>& schema);
void AddAppointmentEdgeDetails(std::shared_ptr<schema::ObjectType> typeAppointmentEdge, const std::shared_ptr<schema::Schema>& schema);
void AddAppointmentConnectionDetails(std::shared_ptr<schema::ObjectType> typeAppointmentConnection, const std::shared_ptr<schema::Schema>& schema);
void AddTaskEdgeDetails(std::shared_ptr<schema::ObjectType> typeTaskEdge, const std::shared_ptr<schema::Schema>& schema);
void AddTaskConnectionDetails(std::shared_ptr<schema::ObjectType> typeTaskConnection, const std::shared_ptr<schema::Schema>& schema);
void AddFolderEdgeDetails(std::shared_ptr<schema::ObjectType> typeFolderEdge, const std::shared_ptr<schema::Schema>& schema);
void AddFolderConnectionDetails(std::shared_ptr<schema::ObjectType> typeFolderConnection, const std::shared_ptr<schema::Schema>& schema);
void AddCompleteTaskPayloadDetails(std::shared_ptr<schema::ObjectType> typeCompleteTaskPayload, const std::shared_ptr<schema::Schema>& schema);
void AddMutationDetails(std::shared_ptr<schema::ObjectType> typeMutation, const std::shared_ptr<schema::Schema>& schema);
void AddSubscriptionDetails(std::shared_ptr<schema::ObjectType> typeSubscription, const std::shared_ptr<schema::Schema>& schema);
void AddAppointmentDetails(std::shared_ptr<schema::ObjectType> typeAppointment, const std::shared_ptr<schema::Schema>& schema);
void AddTaskDetails(std::shared_ptr<schema::ObjectType> typeTask, const std::shared_ptr<schema::Schema>& schema);
void AddFolderDetails(std::shared_ptr<schema::ObjectType> typeFolder, const std::shared_ptr<schema::Schema>& schema);
void AddNestedTypeDetails(std::shared_ptr<schema::ObjectType> typeNestedType, const std::shared_ptr<schema::Schema>& schema);
void AddExpensiveDetails(std::shared_ptr<schema::ObjectType> typeExpensive, const std::shared_ptr<schema::Schema>& schema);

void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema);

} /* namespace today */
} /* namespace graphql */

#endif // TODAYSCHEMA_H
