// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef EXPENSIVEOBJECT_H
#define EXPENSIVEOBJECT_H

#include "TodaySchema.h"

namespace graphql::today::object {

class Expensive
	: public service::Object
{
protected:
	explicit Expensive();

public:
	virtual service::FieldResult<response::IntType> getOrder(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveOrder(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace graphql::today::object */

#endif // EXPENSIVEOBJECT_H
