// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef REQUESTLOADER_H
#define REQUESTLOADER_H

#include "GeneratorLoader.h"

#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLService.h"

#include "graphqlservice/internal/Grammar.h"
#include "graphqlservice/internal/Schema.h"

#include <unordered_set>

namespace graphql::generator {

using RequestSchemaType = std::shared_ptr<const schema::BaseType>;
using RequestSchemaTypeList = std::vector<RequestSchemaType>;

struct ResponseField;

using ResponseFieldList = std::vector<ResponseField>;

struct [[nodiscard]] ResponseType
{
	RequestSchemaType type;
	std::string_view cppType;
	ResponseFieldList fields;
};

struct [[nodiscard]] ResponseField
{
	RequestSchemaType type;
	TypeModifierStack modifiers;
	std::string_view name;
	std::string_view cppName;
	std::optional<tao::graphqlpeg::position> position;
	ResponseFieldList children;
};

struct [[nodiscard]] RequestInputType
{
	RequestSchemaType type;
	std::unordered_set<std::string_view> dependencies {};
	std::vector<std::string_view> declarations {};
};

using RequestInputTypeList = std::vector<RequestInputType>;

struct [[nodiscard]] RequestVariable
{
	RequestInputType inputType;
	TypeModifierStack modifiers;
	std::string_view name;
	std::string_view cppName;
	std::string_view defaultValueString;
	response::Value defaultValue;
	std::optional<tao::graphqlpeg::position> position;
};

using RequestVariableList = std::vector<RequestVariable>;

struct [[nodiscard]] Operation
{
	const peg::ast_node* operation;
	std::string_view name;
	std::string_view type;
	ResponseType responseType {};
	RequestVariableList variables {};
	internal::string_view_set inputTypeNames {};
	RequestInputTypeList referencedInputTypes {};
	internal::string_view_set enumNames {};
	RequestSchemaTypeList referencedEnums {};
};

using OperationList = std::vector<Operation>;

struct [[nodiscard]] RequestOptions
{
	const std::string requestFilename;
	const std::optional<std::string> operationName;
	const bool noIntrospection = false;
};

class SchemaLoader;

class [[nodiscard]] RequestLoader
{
public:
	explicit RequestLoader(RequestOptions&& requestOptions, const SchemaLoader& schemaLoader);

	[[nodiscard]] std::string_view getRequestFilename() const noexcept;
	[[nodiscard]] const OperationList& getOperations() const noexcept;
	[[nodiscard]] std::string_view getOperationDisplayName(
		const Operation& operation) const noexcept;
	[[nodiscard]] std::string getOperationNamespace(const Operation& operation) const noexcept;
	[[nodiscard]] std::string_view getOperationType(const Operation& operation) const noexcept;
	[[nodiscard]] std::string_view getRequestText() const noexcept;

	[[nodiscard]] const ResponseType& getResponseType(const Operation& operation) const noexcept;
	[[nodiscard]] const RequestVariableList& getVariables(
		const Operation& operation) const noexcept;

	[[nodiscard]] const RequestInputTypeList& getReferencedInputTypes(
		const Operation& operation) const noexcept;
	[[nodiscard]] const RequestSchemaTypeList& getReferencedEnums(
		const Operation& operation) const noexcept;

	[[nodiscard]] std::string getInputCppType(
		const RequestSchemaType& wrappedInputType) const noexcept;
	[[nodiscard]] std::string getInputCppType(
		const RequestSchemaType& inputType, const TypeModifierStack& modifiers) const noexcept;
	[[nodiscard]] static std::string getOutputCppType(
		std::string_view outputCppType, const TypeModifierStack& modifiers) noexcept;

	[[nodiscard]] static std::pair<RequestSchemaType, TypeModifierStack> unwrapSchemaType(
		RequestSchemaType&& type) noexcept;

private:
	void buildSchema();
	void addTypesToSchema();
	[[nodiscard]] RequestSchemaType getSchemaType(
		std::string_view type, const TypeModifierStack& modifiers) const noexcept;
	void validateRequest() const;

	[[nodiscard]] static std::string_view trimWhitespace(std::string_view content) noexcept;

	void findOperation();
	void collectFragments() noexcept;
	void collectVariables(Operation& operation) noexcept;
	void collectInputTypes(Operation& operation, const RequestSchemaType& variableType) noexcept;
	void reorderInputTypeDependencies(Operation& operation);
	void collectEnums(Operation& operation, const RequestSchemaType& variableType) noexcept;
	void collectEnums(Operation& operation, const ResponseField& responseField) noexcept;

	using FragmentDefinitionMap = std::map<std::string_view, const peg::ast_node*>;

	// SelectionVisitor visits the AST and fills in the ResponseType for the request.
	class [[nodiscard]] SelectionVisitor
	{
	public:
		explicit SelectionVisitor(const SchemaLoader& schemaLoader,
			const FragmentDefinitionMap& fragments, const std::shared_ptr<schema::Schema>& schema,
			const RequestSchemaType& type);

		void visit(const peg::ast_node& selection);

		[[nodiscard]] ResponseFieldList getFields();

	private:
		void visitField(const peg::ast_node& field);
		void visitFragmentSpread(const peg::ast_node& fragmentSpread);
		void visitInlineFragment(const peg::ast_node& inlineFragment);

		void mergeFragmentFields(ResponseFieldList&& fragmentFields) noexcept;

		const SchemaLoader& _schemaLoader;
		const FragmentDefinitionMap& _fragments;
		const std::shared_ptr<schema::Schema>& _schema;
		const RequestSchemaType& _type;

		internal::string_view_set _names;
		ResponseFieldList _fields;
	};

	const RequestOptions _requestOptions;
	const SchemaLoader& _schemaLoader;
	std::shared_ptr<schema::Schema> _schema;
	peg::ast _ast;

	std::string _requestText;
	OperationList _operations;
	FragmentDefinitionMap _fragments;
};

} // namespace graphql::generator

#endif // REQUESTLOADER_H
