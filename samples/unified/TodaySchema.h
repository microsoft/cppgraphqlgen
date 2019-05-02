// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <graphqlservice/GraphQLService.h>

#include <memory>
#include <string>
#include <vector>

namespace graphql {
namespace introspection {

class Schema;

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

namespace object {

class Query
	: public service::Object
{
protected:
	Query();

public:
	virtual service::FieldResult<std::shared_ptr<service::Object>> getNode(service::FieldParams&& params, response::IdType&& idArg) const;
	virtual service::FieldResult<std::shared_ptr<AppointmentConnection>> getAppointments(service::FieldParams&& params, std::optional<response::IntType>&& firstArg, std::optional<response::Value>&& afterArg, std::optional<response::IntType>&& lastArg, std::optional<response::Value>&& beforeArg) const;
	virtual service::FieldResult<std::shared_ptr<TaskConnection>> getTasks(service::FieldParams&& params, std::optional<response::IntType>&& firstArg, std::optional<response::Value>&& afterArg, std::optional<response::IntType>&& lastArg, std::optional<response::Value>&& beforeArg) const;
	virtual service::FieldResult<std::shared_ptr<FolderConnection>> getUnreadCounts(service::FieldParams&& params, std::optional<response::IntType>&& firstArg, std::optional<response::Value>&& afterArg, std::optional<response::IntType>&& lastArg, std::optional<response::Value>&& beforeArg) const;
	virtual service::FieldResult<std::vector<std::shared_ptr<Appointment>>> getAppointmentsById(service::FieldParams&& params, std::vector<response::IdType>&& idsArg) const;
	virtual service::FieldResult<std::vector<std::shared_ptr<Task>>> getTasksById(service::FieldParams&& params, std::vector<response::IdType>&& idsArg) const;
	virtual service::FieldResult<std::vector<std::shared_ptr<Folder>>> getUnreadCountsById(service::FieldParams&& params, std::vector<response::IdType>&& idsArg) const;
	virtual service::FieldResult<std::shared_ptr<NestedType>> getNested(service::FieldParams&& params) const;
	virtual service::FieldResult<response::StringType> getUnimplemented(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveNode(service::ResolverParams&& params);
	std::future<response::Value> resolveAppointments(service::ResolverParams&& params);
	std::future<response::Value> resolveTasks(service::ResolverParams&& params);
	std::future<response::Value> resolveUnreadCounts(service::ResolverParams&& params);
	std::future<response::Value> resolveAppointmentsById(service::ResolverParams&& params);
	std::future<response::Value> resolveTasksById(service::ResolverParams&& params);
	std::future<response::Value> resolveUnreadCountsById(service::ResolverParams&& params);
	std::future<response::Value> resolveNested(service::ResolverParams&& params);
	std::future<response::Value> resolveUnimplemented(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
	std::future<response::Value> resolve_schema(service::ResolverParams&& params);
	std::future<response::Value> resolve_type(service::ResolverParams&& params);

	std::shared_ptr<introspection::Schema> _schema;
};

class PageInfo
	: public service::Object
{
protected:
	PageInfo();

public:
	virtual service::FieldResult<response::BooleanType> getHasNextPage(service::FieldParams&& params) const;
	virtual service::FieldResult<response::BooleanType> getHasPreviousPage(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveHasNextPage(service::ResolverParams&& params);
	std::future<response::Value> resolveHasPreviousPage(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class AppointmentEdge
	: public service::Object
{
protected:
	AppointmentEdge();

public:
	virtual service::FieldResult<std::shared_ptr<Appointment>> getNode(service::FieldParams&& params) const;
	virtual service::FieldResult<response::Value> getCursor(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveNode(service::ResolverParams&& params);
	std::future<response::Value> resolveCursor(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class AppointmentConnection
	: public service::Object
{
protected:
	AppointmentConnection();

public:
	virtual service::FieldResult<std::shared_ptr<PageInfo>> getPageInfo(service::FieldParams&& params) const;
	virtual service::FieldResult<std::optional<std::vector<std::shared_ptr<AppointmentEdge>>>> getEdges(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolvePageInfo(service::ResolverParams&& params);
	std::future<response::Value> resolveEdges(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class TaskEdge
	: public service::Object
{
protected:
	TaskEdge();

public:
	virtual service::FieldResult<std::shared_ptr<Task>> getNode(service::FieldParams&& params) const;
	virtual service::FieldResult<response::Value> getCursor(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveNode(service::ResolverParams&& params);
	std::future<response::Value> resolveCursor(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class TaskConnection
	: public service::Object
{
protected:
	TaskConnection();

public:
	virtual service::FieldResult<std::shared_ptr<PageInfo>> getPageInfo(service::FieldParams&& params) const;
	virtual service::FieldResult<std::optional<std::vector<std::shared_ptr<TaskEdge>>>> getEdges(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolvePageInfo(service::ResolverParams&& params);
	std::future<response::Value> resolveEdges(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class FolderEdge
	: public service::Object
{
protected:
	FolderEdge();

public:
	virtual service::FieldResult<std::shared_ptr<Folder>> getNode(service::FieldParams&& params) const;
	virtual service::FieldResult<response::Value> getCursor(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveNode(service::ResolverParams&& params);
	std::future<response::Value> resolveCursor(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class FolderConnection
	: public service::Object
{
protected:
	FolderConnection();

public:
	virtual service::FieldResult<std::shared_ptr<PageInfo>> getPageInfo(service::FieldParams&& params) const;
	virtual service::FieldResult<std::optional<std::vector<std::shared_ptr<FolderEdge>>>> getEdges(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolvePageInfo(service::ResolverParams&& params);
	std::future<response::Value> resolveEdges(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class CompleteTaskPayload
	: public service::Object
{
protected:
	CompleteTaskPayload();

public:
	virtual service::FieldResult<std::shared_ptr<Task>> getTask(service::FieldParams&& params) const;
	virtual service::FieldResult<std::optional<response::StringType>> getClientMutationId(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveTask(service::ResolverParams&& params);
	std::future<response::Value> resolveClientMutationId(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Mutation
	: public service::Object
{
protected:
	Mutation();

public:
	virtual service::FieldResult<std::shared_ptr<CompleteTaskPayload>> applyCompleteTask(service::FieldParams&& params, CompleteTaskInput&& inputArg) const;

private:
	std::future<response::Value> resolveCompleteTask(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Subscription
	: public service::Object
{
protected:
	Subscription();

public:
	virtual service::FieldResult<std::shared_ptr<Appointment>> getNextAppointmentChange(service::FieldParams&& params) const;
	virtual service::FieldResult<std::shared_ptr<service::Object>> getNodeChange(service::FieldParams&& params, response::IdType&& idArg) const;

private:
	std::future<response::Value> resolveNextAppointmentChange(service::ResolverParams&& params);
	std::future<response::Value> resolveNodeChange(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Appointment
	: public service::Object
	, public Node
{
protected:
	Appointment();

public:
	virtual service::FieldResult<response::IdType> getId(service::FieldParams&& params) const override;
	virtual service::FieldResult<std::optional<response::Value>> getWhen(service::FieldParams&& params) const;
	virtual service::FieldResult<std::optional<response::StringType>> getSubject(service::FieldParams&& params) const;
	virtual service::FieldResult<response::BooleanType> getIsNow(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveId(service::ResolverParams&& params);
	std::future<response::Value> resolveWhen(service::ResolverParams&& params);
	std::future<response::Value> resolveSubject(service::ResolverParams&& params);
	std::future<response::Value> resolveIsNow(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Task
	: public service::Object
	, public Node
{
protected:
	Task();

public:
	virtual service::FieldResult<response::IdType> getId(service::FieldParams&& params) const override;
	virtual service::FieldResult<std::optional<response::StringType>> getTitle(service::FieldParams&& params) const;
	virtual service::FieldResult<response::BooleanType> getIsComplete(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveId(service::ResolverParams&& params);
	std::future<response::Value> resolveTitle(service::ResolverParams&& params);
	std::future<response::Value> resolveIsComplete(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class Folder
	: public service::Object
	, public Node
{
protected:
	Folder();

public:
	virtual service::FieldResult<response::IdType> getId(service::FieldParams&& params) const override;
	virtual service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&& params) const;
	virtual service::FieldResult<response::IntType> getUnreadCount(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveId(service::ResolverParams&& params);
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveUnreadCount(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

class NestedType
	: public service::Object
{
protected:
	NestedType();

public:
	virtual service::FieldResult<response::IntType> getDepth(service::FieldParams&& params) const;
	virtual service::FieldResult<std::shared_ptr<NestedType>> getNested(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveDepth(service::ResolverParams&& params);
	std::future<response::Value> resolveNested(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace object */

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

void AddTypesToSchema(std::shared_ptr<introspection::Schema> schema);

} /* namespace today */
} /* namespace graphql */
