// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef VALIDATION_H
#define VALIDATION_H

#include "graphqlservice/GraphQLValidation.h"

namespace graphql::service {

struct VariableDefinition : public ValidateArgument
{
	schema_location position;
};

struct ValidateArgumentVariable
{
	bool operator==(const ValidateArgumentVariable& other) const;

	const std::string_view name;
};

struct ValidateArgumentEnumValue
{
	bool operator==(const ValidateArgumentEnumValue& other) const;

	const std::string_view value;
};

struct ValidateArgumentValue;

struct ValidateArgumentValuePtr
{
	bool operator==(const ValidateArgumentValuePtr& other) const;

	std::unique_ptr<ValidateArgumentValue> value;
	schema_location position;
};

struct ValidateArgumentList
{
	bool operator==(const ValidateArgumentList& other) const;

	std::vector<ValidateArgumentValuePtr> values;
};

struct ValidateArgumentMap
{
	bool operator==(const ValidateArgumentMap& other) const;

	std::unordered_map<std::string_view, ValidateArgumentValuePtr> values;
};

using ValidateArgumentVariant = std::variant<ValidateArgumentVariable, response::IntType,
	response::FloatType, response::StringType, response::BooleanType, ValidateArgumentEnumValue,
	ValidateArgumentList, ValidateArgumentMap>;

struct ValidateArgumentValue
{
	ValidateArgumentValue(ValidateArgumentVariable&& value);
	ValidateArgumentValue(response::IntType value);
	ValidateArgumentValue(response::FloatType value);
	ValidateArgumentValue(response::StringType&& value);
	ValidateArgumentValue(response::BooleanType value);
	ValidateArgumentValue(ValidateArgumentEnumValue&& value);
	ValidateArgumentValue(ValidateArgumentList&& value);
	ValidateArgumentValue(ValidateArgumentMap&& value);

	ValidateArgumentVariant data;
};

// ValidateArgumentValueVisitor visits the AST and builds a record of a field return type and map
// of the arguments for comparison to see if 2 fields with the same result name can be merged.
class ValidateArgumentValueVisitor
{
public:
	ValidateArgumentValueVisitor(std::vector<schema_error>& errors);

	void visit(const peg::ast_node& value);

	ValidateArgumentValuePtr getArgumentValue();

private:
	void visitVariable(const peg::ast_node& variable);
	void visitIntValue(const peg::ast_node& intValue);
	void visitFloatValue(const peg::ast_node& floatValue);
	void visitStringValue(const peg::ast_node& stringValue);
	void visitBooleanValue(const peg::ast_node& booleanValue);
	void visitNullValue(const peg::ast_node& nullValue);
	void visitEnumValue(const peg::ast_node& enumValue);
	void visitListValue(const peg::ast_node& listValue);
	void visitObjectValue(const peg::ast_node& objectValue);

	ValidateArgumentValuePtr _argumentValue;
	std::vector<schema_error>& _errors;
};

using ValidateFieldArguments = std::unordered_map<std::string_view, ValidateArgumentValuePtr>;

struct ValidateField
{
	ValidateField(std::shared_ptr<const ValidateType> returnType,
		std::shared_ptr<const ValidateType>&& objectType, const std::string_view& fieldName,
		ValidateFieldArguments&& arguments);

	bool operator==(const ValidateField& other) const;

	std::shared_ptr<const ValidateType> returnType;
	std::shared_ptr<const ValidateType> objectType;
	std::string_view fieldName;
	ValidateFieldArguments arguments;
};

class ValidationContext;

// ValidateVariableTypeVisitor visits the AST and builds a ValidateType structure representing
// a variable type in an operation definition as if it came from an Introspection query.
class ValidateVariableTypeVisitor
{
public:
	ValidateVariableTypeVisitor(const ValidationContext& validationContext);

	void visit(const peg::ast_node& typeName);

	bool isInputType() const;
	std::shared_ptr<ValidateType> getType();

private:
	void visitNamedType(const peg::ast_node& namedType);
	void visitListType(const peg::ast_node& listType);
	void visitNonNullType(const peg::ast_node& nonNullType);

	const ValidationContext& _validationContext;

	std::shared_ptr<ValidateType> _variableType;
};

// ValidateExecutableVisitor visits the AST and validates that it is executable against the service
// schema.
class ValidateExecutableVisitor
{
public:
	ValidateExecutableVisitor(const ValidationContext& validationContext);

	void visit(const peg::ast_node& root);

	std::vector<schema_error> getStructuredErrors();

private:
	bool matchesScopedType(const ValidateType& name) const;

	void visitFragmentDefinition(const peg::ast_node& fragmentDefinition);
	void visitOperationDefinition(const peg::ast_node& operationDefinition);

	void visitSelection(const peg::ast_node& selection);

	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	void visitDirectives(
		introspection::DirectiveLocation location, const peg::ast_node& directives);

	bool validateInputValue(bool hasNonNullDefaultValue, const ValidateArgumentValuePtr& argument,
		const ValidateType& type);
	bool validateVariableType(bool isNonNull, const ValidateType& variableType,
		const schema_location& position, const ValidateType& inputType);

	const ValidationContext& _validationContext;

	std::vector<schema_error> _errors;

	using ExecutableNodes = std::unordered_map<std::string_view, const peg::ast_node&>;
	using FragmentSet = std::unordered_set<std::string_view>;
	using VariableTypes = std::unordered_map<std::string_view, VariableDefinition>;
	using OperationVariables = std::optional<VariableTypes>;
	using VariableSet = std::unordered_set<const VariableDefinition*>;

	// These members store information that's specific to a single query and changes every time we
	// visit a new one. They must be reset in between queries.
	ExecutableNodes _fragmentDefinitions;
	FragmentSet _referencedFragments;
	FragmentSet _fragmentCycles;

	// These members store state for the visitor. They implicitly reset each time we call visit.
	OperationVariables _operationVariables;
	VariableSet _referencedVariables;
	FragmentSet _fragmentStack;
	size_t _fieldCount = 0;
	std::shared_ptr<const ValidateType> _scopedType;
	std::unordered_map<std::string_view, ValidateField> _selectionFields;
	struct
	{
		std::shared_ptr<ValidateType> nonNullString;
	} commonTypes;
};

} /* namespace graphql::service */

#endif // VALIDATION_H
