// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef PROXYSCHEMA_H
#define PROXYSCHEMA_H

#include "graphqlservice/GraphQLResponse.h"
#include "graphqlservice/GraphQLService.h"

#include "graphqlservice/internal/Schema.h"

// Check if the library version is compatible with schemagen 4.5.0
static_assert(graphql::internal::MajorVersion == 4, "regenerate with schemagen: major version mismatch");
static_assert(graphql::internal::MinorVersion == 5, "regenerate with schemagen: minor version mismatch");

#include <array>
#include <memory>
#include <string>
#include <string_view>

namespace graphql {
namespace proxy {
namespace object {

class Query;

} // namespace object

class [[nodiscard("unnecessary construction")]] Operations final
	: public service::Request
{
public:
	explicit Operations(std::shared_ptr<object::Query> query);

	template <class TQuery>
	explicit Operations(std::shared_ptr<TQuery> query)
		: Operations {
			std::make_shared<object::Query>(std::move(query))
		}
	{
	}

private:
	std::shared_ptr<object::Query> _query;
};

void AddQueryDetails(const std::shared_ptr<schema::ObjectType>& typeQuery, const std::shared_ptr<schema::Schema>& schema);

std::shared_ptr<schema::Schema> GetSchema();

} // namespace proxy
} // namespace graphql

#endif // PROXYSCHEMA_H
