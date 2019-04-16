// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

namespace facebook::graphql::today::object {

class CompleteTaskPayload
	: public service::Object
{
protected:
	CompleteTaskPayload();

public:
	virtual std::future<std::shared_ptr<Task>> getTask(service::FieldParams&& params) const;
	virtual std::future<std::optional<response::StringType>> getClientMutationId(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveTask(service::ResolverParams&& params);
	std::future<response::Value> resolveClientMutationId(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace facebook::graphql::today::object */
