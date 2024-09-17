// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "QueryObject.h"

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace graphql {
namespace proxy {

Operations::Operations(std::shared_ptr<object::Query> query)
	: service::Request({
		{ service::strQuery, query }
	}, GetSchema())
	, _query(std::move(query))
{
}

void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema)
{
	auto typeQuery = schema::ObjectType::Make(R"gql(Query)gql"sv, R"md()md"sv);
	schema->AddType(R"gql(Query)gql"sv, typeQuery);

	AddQueryDetails(typeQuery, schema);

	schema->AddQueryType(typeQuery);
}

std::shared_ptr<schema::Schema> GetSchema()
{
	static std::weak_ptr<schema::Schema> s_wpSchema;
	auto schema = s_wpSchema.lock();

	if (!schema)
	{
		schema = std::make_shared<schema::Schema>(false, R"md()md"sv);
		introspection::AddTypesToSchema(schema);
		AddTypesToSchema(schema);
		s_wpSchema = schema;
	}

	return schema;
}

} // namespace proxy
} // namespace graphql
