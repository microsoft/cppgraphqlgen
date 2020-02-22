// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef MUTATIONOBJECT_H
#define MUTATIONOBJECT_H

#include "TodaySchema.h"

namespace graphql::today::object {

class Mutation
	: public service::Object
{
protected:
	explicit Mutation();

public:
	virtual service::FieldResult<std::shared_ptr<CompleteTaskPayload>> applyCompleteTask(service::FieldParams&& params, CompleteTaskInput&& inputArg) const;

private:
	std::future<response::Value> resolveCompleteTask(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace graphql::today::object */

#endif // MUTATIONOBJECT_H
