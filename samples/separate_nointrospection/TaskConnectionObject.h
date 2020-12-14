// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef TASKCONNECTIONOBJECT_H
#define TASKCONNECTIONOBJECT_H

#include "TodaySchema.h"

namespace graphql::today::object {

class TaskConnection
	: public service::Object
{
protected:
	explicit TaskConnection();

public:
	virtual service::FieldResult<std::shared_ptr<PageInfo>> getPageInfo(service::FieldParams&& params) const;
	virtual service::FieldResult<std::optional<std::vector<std::shared_ptr<TaskEdge>>>> getEdges(service::FieldParams&& params) const;

private:
	std::future<service::ResolverResult> resolvePageInfo(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolveEdges(service::ResolverParams&& params);

	std::future<service::ResolverResult> resolve_typename(service::ResolverParams&& params);
};

} /* namespace graphql::today::object */

#endif // TASKCONNECTIONOBJECT_H
