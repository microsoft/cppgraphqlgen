// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

namespace facebook::graphql::today::object {

class Subscription
	: public service::Object
{
protected:
	Subscription();

public:
	virtual std::future<std::shared_ptr<Appointment>> getNextAppointmentChange(service::FieldParams&& params) const;
	virtual std::future<std::shared_ptr<service::Object>> getNodeChange(service::FieldParams&& params, response::IdType&& idArg) const;

private:
	std::future<response::Value> resolveNextAppointmentChange(service::ResolverParams&& params);
	std::future<response::Value> resolveNodeChange(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace facebook::graphql::today::object */
