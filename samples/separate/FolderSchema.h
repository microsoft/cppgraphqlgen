// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

namespace facebook::graphql::today::object {

class Folder
	: public service::Object
	, public Node
{
protected:
	Folder();

public:
	virtual service::FieldResult<response::IdType> getId(service::FieldParams&& params) const override;
	virtual service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&& params) const;
	virtual service::FieldResult<response::IntType> getUnreadCount(service::FieldParams&& params) const;

private:
	std::future<response::Value> resolveId(service::ResolverParams&& params);
	std::future<response::Value> resolveName(service::ResolverParams&& params);
	std::future<response::Value> resolveUnreadCount(service::ResolverParams&& params);

	std::future<response::Value> resolve_typename(service::ResolverParams&& params);
};

} /* namespace facebook::graphql::today::object */
