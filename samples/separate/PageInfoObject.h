// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

namespace graphql::today::object {

class PageInfo
	: public service::Object
{
protected:
	PageInfo();

public:
	virtual service::FieldResult<response::BooleanType> getHasNextPage(service::FieldParams&& params) const;
	virtual service::FieldResult<response::BooleanType> getHasPreviousPage(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveHasNextPage(service::ResolverParams&& params);
	std::future<response::Value> resolveHasPreviousPage(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace graphql::today::object */
