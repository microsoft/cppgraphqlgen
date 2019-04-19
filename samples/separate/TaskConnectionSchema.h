// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

namespace facebook::graphql::today::object {

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

} /* namespace facebook::graphql::today::object */
