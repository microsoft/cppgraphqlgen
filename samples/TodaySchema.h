// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <cpprest/json.h>

#include "GraphQLService.h"

namespace facebook {
namespace graphql {
namespace introspection {

class Schema;

} /* namespace introspection */

namespace today {

enum class TaskState
{
	New,
	Started,
	Complete
};

struct CompleteTaskInput
{
	std::vector<unsigned char> id;
	std::unique_ptr<bool> isComplete;
	std::unique_ptr<std::string> clientMutationId;
};

struct Node
{
	virtual std::vector<unsigned char> getId() const = 0;
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

class Query
	: public service::Object
{
protected:
	Query();

public:
	virtual std::shared_ptr<service::Object> getNode(std::vector<unsigned char>&& id) const = 0;
	virtual std::shared_ptr<AppointmentConnection> getAppointments(std::unique_ptr<int>&& first, std::unique_ptr<web::json::value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<web::json::value>&& before) const = 0;
	virtual std::shared_ptr<TaskConnection> getTasks(std::unique_ptr<int>&& first, std::unique_ptr<web::json::value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<web::json::value>&& before) const = 0;
	virtual std::shared_ptr<FolderConnection> getUnreadCounts(std::unique_ptr<int>&& first, std::unique_ptr<web::json::value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<web::json::value>&& before) const = 0;
	virtual std::vector<std::shared_ptr<Appointment>> getAppointmentsById(std::vector<std::vector<unsigned char>>&& ids) const = 0;
	virtual std::vector<std::shared_ptr<Task>> getTasksById(std::vector<std::vector<unsigned char>>&& ids) const = 0;
	virtual std::vector<std::shared_ptr<Folder>> getUnreadCountsById(std::vector<std::vector<unsigned char>>&& ids) const = 0;

private:
	web::json::value resolveNode(service::ResolverParams&& params);
	web::json::value resolveAppointments(service::ResolverParams&& params);
	web::json::value resolveTasks(service::ResolverParams&& params);
	web::json::value resolveUnreadCounts(service::ResolverParams&& params);
	web::json::value resolveAppointmentsById(service::ResolverParams&& params);
	web::json::value resolveTasksById(service::ResolverParams&& params);
	web::json::value resolveUnreadCountsById(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
	web::json::value resolve__schema(service::ResolverParams&& params);
	web::json::value resolve__type(service::ResolverParams&& params);

	std::shared_ptr<introspection::Schema> _schema;
};

class PageInfo
	: public service::Object
{
protected:
	PageInfo();

public:
	virtual bool getHasNextPage() const = 0;
	virtual bool getHasPreviousPage() const = 0;

private:
	web::json::value resolveHasNextPage(service::ResolverParams&& params);
	web::json::value resolveHasPreviousPage(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class AppointmentEdge
	: public service::Object
{
protected:
	AppointmentEdge();

public:
	virtual std::shared_ptr<Appointment> getNode() const = 0;
	virtual web::json::value getCursor() const = 0;

private:
	web::json::value resolveNode(service::ResolverParams&& params);
	web::json::value resolveCursor(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class AppointmentConnection
	: public service::Object
{
protected:
	AppointmentConnection();

public:
	virtual std::shared_ptr<PageInfo> getPageInfo() const = 0;
	virtual std::unique_ptr<std::vector<std::shared_ptr<AppointmentEdge>>> getEdges() const = 0;

private:
	web::json::value resolvePageInfo(service::ResolverParams&& params);
	web::json::value resolveEdges(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class TaskEdge
	: public service::Object
{
protected:
	TaskEdge();

public:
	virtual std::shared_ptr<Task> getNode() const = 0;
	virtual web::json::value getCursor() const = 0;

private:
	web::json::value resolveNode(service::ResolverParams&& params);
	web::json::value resolveCursor(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class TaskConnection
	: public service::Object
{
protected:
	TaskConnection();

public:
	virtual std::shared_ptr<PageInfo> getPageInfo() const = 0;
	virtual std::unique_ptr<std::vector<std::shared_ptr<TaskEdge>>> getEdges() const = 0;

private:
	web::json::value resolvePageInfo(service::ResolverParams&& params);
	web::json::value resolveEdges(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class FolderEdge
	: public service::Object
{
protected:
	FolderEdge();

public:
	virtual std::shared_ptr<Folder> getNode() const = 0;
	virtual web::json::value getCursor() const = 0;

private:
	web::json::value resolveNode(service::ResolverParams&& params);
	web::json::value resolveCursor(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class FolderConnection
	: public service::Object
{
protected:
	FolderConnection();

public:
	virtual std::shared_ptr<PageInfo> getPageInfo() const = 0;
	virtual std::unique_ptr<std::vector<std::shared_ptr<FolderEdge>>> getEdges() const = 0;

private:
	web::json::value resolvePageInfo(service::ResolverParams&& params);
	web::json::value resolveEdges(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class CompleteTaskPayload
	: public service::Object
{
protected:
	CompleteTaskPayload();

public:
	virtual std::shared_ptr<Task> getTask() const = 0;
	virtual std::unique_ptr<std::string> getClientMutationId() const = 0;

private:
	web::json::value resolveTask(service::ResolverParams&& params);
	web::json::value resolveClientMutationId(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class Mutation
	: public service::Object
{
protected:
	Mutation();

public:
	virtual std::shared_ptr<CompleteTaskPayload> getCompleteTask(CompleteTaskInput&& input) const = 0;

private:
	web::json::value resolveCompleteTask(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class Subscription
	: public service::Object
{
protected:
	Subscription();

public:
	virtual std::shared_ptr<Appointment> getNextAppointmentChange() const = 0;

private:
	web::json::value resolveNextAppointmentChange(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class Appointment
	: public service::Object
	, public Node
{
protected:
	Appointment();

public:
	virtual std::unique_ptr<web::json::value> getWhen() const = 0;
	virtual std::unique_ptr<std::string> getSubject() const = 0;
	virtual bool getIsNow() const = 0;

private:
	web::json::value resolveId(service::ResolverParams&& params);
	web::json::value resolveWhen(service::ResolverParams&& params);
	web::json::value resolveSubject(service::ResolverParams&& params);
	web::json::value resolveIsNow(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class Task
	: public service::Object
	, public Node
{
protected:
	Task();

public:
	virtual std::unique_ptr<std::string> getTitle() const = 0;
	virtual bool getIsComplete() const = 0;

private:
	web::json::value resolveId(service::ResolverParams&& params);
	web::json::value resolveTitle(service::ResolverParams&& params);
	web::json::value resolveIsComplete(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
};

class Folder
	: public service::Object
	, public Node
{
protected:
	Folder();

public:
	virtual std::unique_ptr<std::string> getName() const = 0;
	virtual int getUnreadCount() const = 0;

private:
	web::json::value resolveId(service::ResolverParams&& params);
	web::json::value resolveName(service::ResolverParams&& params);
	web::json::value resolveUnreadCount(service::ResolverParams&& params);

	web::json::value resolve__typename(service::ResolverParams&& params);
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
} /* namespace facebook */