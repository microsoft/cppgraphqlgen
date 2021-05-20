// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef REQUESTLOADER_H
#define REQUESTLOADER_H

#include "SchemaLoader.h"

#include "graphqlservice/GraphQLSchema.h"

namespace graphql::generator {

struct RequestOptions
{
	const std::string requestFilename;
	const std::string operationName;
	const bool noIntrospection = false;
};

class RequestLoader
{
public:
	explicit RequestLoader(RequestOptions&& requestOptions, const SchemaLoader& schemaLoader);

private:
	std::shared_ptr<schema::Schema> buildSchema(const SchemaLoader& schemaLoader) const;
	void addTypesToSchema(const SchemaLoader& schemaLoader, const std::shared_ptr<schema::Schema>& schema) const;
	void validateRequest() const;

	static std::shared_ptr<const schema::BaseType> getIntrospectionType(
		const std::shared_ptr<schema::Schema>& schema, std::string_view type,
		const TypeModifierStack& modifiers) noexcept;

	const RequestOptions _requestOptions;
	peg::ast _ast;
	std::shared_ptr<schema::Schema> _schema;
};

} /* namespace graphql::generator */

#endif // REQUESTLOADER_H
