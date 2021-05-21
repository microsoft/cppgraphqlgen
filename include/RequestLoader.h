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
	void buildSchema(const SchemaLoader& schemaLoader);
	void addTypesToSchema(const SchemaLoader& schemaLoader);
	std::shared_ptr<const schema::BaseType> getSchemaType(
		std::string_view type, const TypeModifierStack& modifiers) const noexcept;
	void validateRequest() const;

	const RequestOptions _requestOptions;
	std::shared_ptr<schema::Schema> _schema;
	peg::ast _ast;
};

} /* namespace graphql::generator */

#endif // REQUESTLOADER_H
