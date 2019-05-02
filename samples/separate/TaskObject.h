// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

namespace graphql::today::object {

class Task
	: public service::Object
	, public Node
{
protected:
	Task();

public:
	virtual service::FieldResult<response::IdType> getId(service::FieldParams&& params) const override;
	virtual service::FieldResult<std::optional<response::StringType>> getTitle(service::FieldParams&& params) const;
	virtual service::FieldResult<response::BooleanType> getIsComplete(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveId(service::ResolverParams&& params);
	std::future<response::Value> resolveTitle(service::ResolverParams&& params);
	std::future<response::Value> resolveIsComplete(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace graphql::today::object */
