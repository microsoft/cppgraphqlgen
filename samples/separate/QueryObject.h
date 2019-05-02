// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

namespace graphql::today::object {

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

} /* namespace graphql::today::object */
