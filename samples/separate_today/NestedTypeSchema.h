// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

namespace facebook::graphql::today::object {

class NestedType
	: public service::Object
{
protected:
	NestedType();

public:
	virtual std::future<response::IntType> getDepth(service::FieldParams&& params) const;
	virtual std::future<std::shared_ptr<NestedType>> getNested(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveDepth(service::ResolverParams&& params);
	std::future<response::Value> resolveNested(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace facebook::graphql::today::object */
