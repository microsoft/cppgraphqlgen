// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef APPOINTMENTCONNECTIONOBJECT_H
#define APPOINTMENTCONNECTIONOBJECT_H

#include "TodaySchema.h"

namespace graphql::today::object {

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

} /* namespace graphql::today::object */

#endif // APPOINTMENTCONNECTIONOBJECT_H
