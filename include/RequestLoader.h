// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef REQUESTLOADER_H
#define REQUESTLOADER_H

#include "SchemaLoader.h"

#include "graphqlservice/GraphQLSchema.h"

namespace graphql::generator {

using ResponseField = InputField;
using ResponseFieldList = InputFieldList;

struct ResponseType
{
	std::string_view type;
	std::string_view cppType;
	ResponseFieldList fields;
};

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

	std::string_view getRequestFilename() const noexcept;
	std::string_view getOperationName() const noexcept;
	std::string_view getOperationType() const noexcept;
	std::string_view getRequestText() const noexcept;

	const ResponseType& getVariablesType() const noexcept;
	const ResponseType& getResponseType() const noexcept;

private:
	void buildSchema();
	void addTypesToSchema();
	std::shared_ptr<const schema::BaseType> getSchemaType(
		std::string_view type, const TypeModifierStack& modifiers) const noexcept;
	void validateRequest() const;

	static std::string_view trimWhitespace(std::string_view content) noexcept;

	void findOperation();

	const RequestOptions _requestOptions;
	const SchemaLoader& _schemaLoader;
	std::shared_ptr<schema::Schema> _schema;
	peg::ast _ast;

	std::string _requestText;
	const peg::ast_node* _operation = nullptr;
	std::string_view _operationName;
	std::string_view _operationType;
	ResponseType _variablesType;
	ResponseType _responseType;
};

} /* namespace graphql::generator */

#endif // REQUESTLOADER_H
