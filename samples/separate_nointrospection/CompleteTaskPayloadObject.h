// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef COMPLETETASKPAYLOADOBJECT_H
#define COMPLETETASKPAYLOADOBJECT_H

#include "TodaySchema.h"

namespace graphql::today::object {

class CompleteTaskPayload
	: public service::Object
{
protected:
	explicit CompleteTaskPayload();

public:
	virtual service::FieldResult<std::shared_ptr<Task>> getTask(service::FieldParams&& params) const;
	virtual service::FieldResult<std::optional<response::StringType>> getClientMutationId(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveTask(service::ResolverParams&& params);
	std::future<response::Value> resolveClientMutationId(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace graphql::today::object */

#endif // COMPLETETASKPAYLOADOBJECT_H
