// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

namespace graphql::today::object {

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

} /* namespace graphql::today::object */
