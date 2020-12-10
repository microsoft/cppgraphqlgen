// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef NESTEDTYPEOBJECT_H
#define NESTEDTYPEOBJECT_H

#include "TodaySchema.h"

namespace graphql::today::object {

class NestedType
	: public service::Object
{
protected:
	explicit NestedType();

public:
	virtual service::FieldResult<response::IntType> getDepth(service::FieldParams&& params) const;
	virtual service::FieldResult<std::shared_ptr<NestedType>> getNested(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveDepth(service::ResolverParams&& params);
	std::future<response::Value> resolveNested(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace graphql::today::object */

#endif // NESTEDTYPEOBJECT_H
