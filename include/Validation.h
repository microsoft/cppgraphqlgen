// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef VALIDATION_H
#define VALIDATION_H

#include "graphqlservice/GraphQLService.h"

#include "graphqlservice/internal/Schema.h"

#include <cstddef>

namespace graphql::service {

using ValidateType = std::optional<std::reference_wrapper<const schema::BaseType>>;
using SharedType = std::shared_ptr<const schema::BaseType>;

[[nodiscard("unnecessary call")]] SharedType getSharedType(const ValidateType& type) noexcept;
[[nodiscard("unnecessary call")]] ValidateType getValidateType(const SharedType& type) noexcept;

struct [[nodiscard("unnecessary construction")]] ValidateArgument
{
	bool defaultValue = false;
	bool nonNullDefaultValue = false;
	ValidateType type;
};

using ValidateTypeFieldArguments = internal::string_view_map<ValidateArgument>;

struct [[nodiscard("unnecessary construction")]] ValidateTypeField
{
	ValidateType returnType;
	ValidateTypeFieldArguments arguments;
};

using ValidateDirectiveArguments = internal::string_view_map<ValidateArgument>;

struct [[nodiscard("unnecessary construction")]] ValidateDirective
{
	bool isRepeatable = false;
	internal::sorted_set<introspection::DirectiveLocation> locations;
	ValidateDirectiveArguments arguments;
};

struct [[nodiscard("unnecessary construction")]] ValidateArgumentVariable
{
	[[nodiscard("unnecessary call")]] bool operator==(const ValidateArgumentVariable& other) const;

	std::string_view name;
};

struct [[nodiscard("unnecessary construction")]] ValidateArgumentEnumValue
{
	[[nodiscard("unnecessary call")]] bool operator==(const ValidateArgumentEnumValue& other) const;

	std::string_view value;
};

struct ValidateArgumentValue;

struct [[nodiscard("unnecessary construction")]] ValidateArgumentValuePtr
{
	[[nodiscard("unnecessary call")]] bool operator==(const ValidateArgumentValuePtr& other) const;

	std::unique_ptr<ValidateArgumentValue> value;
	schema_location position;
};

struct [[nodiscard("unnecessary construction")]] ValidateArgumentList
{
	[[nodiscard("unnecessary call")]] bool operator==(const ValidateArgumentList& other) const;

	std::vector<ValidateArgumentValuePtr> values;
};

struct [[nodiscard("unnecessary construction")]] ValidateArgumentMap
{
	[[nodiscard("unnecessary call")]] bool operator==(const ValidateArgumentMap& other) const;

	internal::string_view_map<ValidateArgumentValuePtr> values;
};

using ValidateArgumentVariant = std::variant<ValidateArgumentVariable, int, double,
	std::string_view, bool, ValidateArgumentEnumValue, ValidateArgumentList, ValidateArgumentMap>;

struct [[nodiscard("unnecessary construction")]] ValidateArgumentValue
{
	ValidateArgumentValue(ValidateArgumentVariable&& value);
	ValidateArgumentValue(int value);
	ValidateArgumentValue(double value);
	ValidateArgumentValue(std::string_view value);
	ValidateArgumentValue(bool value);
	ValidateArgumentValue(ValidateArgumentEnumValue&& value);
	ValidateArgumentValue(ValidateArgumentList&& value);
	ValidateArgumentValue(ValidateArgumentMap&& value);

	ValidateArgumentVariant data;
};

// ValidateArgumentValueVisitor visits the AST and builds a record of a field return type and map
// of the arguments for comparison to see if 2 fields with the same result name can be merged.
class [[nodiscard("unnecessary construction")]] ValidateArgumentValueVisitor
{
public:
	ValidateArgumentValueVisitor(std::list<schema_error>& errors);

	void visit(const peg::ast_node& value);

	[[nodiscard("unnecessary call")]] ValidateArgumentValuePtr getArgumentValue();

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
	std::list<schema_error>& _errors;
};

using ValidateFieldArguments = internal::string_view_map<ValidateArgumentValuePtr>;

struct [[nodiscard("unnecessary construction")]] ValidateField
{
	ValidateField(ValidateType&& returnType, ValidateType&& objectType, std::string_view fieldName,
		ValidateFieldArguments&& arguments);

	[[nodiscard("unnecessary call")]] bool operator==(const ValidateField& other) const;

	ValidateType returnType;
	ValidateType objectType;
	std::string_view fieldName;
	ValidateFieldArguments arguments;
};

using ValidateTypes = internal::string_view_map<ValidateType>;

// ValidateVariableTypeVisitor visits the AST and builds a ValidateType structure representing
// a variable type in an operation definition as if it came from an Introspection query.
class [[nodiscard("unnecessary construction")]] ValidateVariableTypeVisitor
{
public:
	ValidateVariableTypeVisitor(
		const std::shared_ptr<schema::Schema>& schema, const ValidateTypes& types);

	void visit(const peg::ast_node& typeName);

	[[nodiscard("unnecessary call")]] bool isInputType() const;
	[[nodiscard("unnecessary call")]] ValidateType getType();

private:
	void visitNamedType(const peg::ast_node& namedType);
	void visitListType(const peg::ast_node& listType);
	void visitNonNullType(const peg::ast_node& nonNullType);

	const std::shared_ptr<schema::Schema>& _schema;
	const ValidateTypes& _types;

	bool _isInputType = false;
	ValidateType _variableType;
};

// ValidateExecutableVisitor visits the AST and validates that it is executable against the service
// schema.
class [[nodiscard("unnecessary construction")]] ValidateExecutableVisitor
{
public:
	GRAPHQLSERVICE_EXPORT ValidateExecutableVisitor(std::shared_ptr<schema::Schema> schema);

	GRAPHQLSERVICE_EXPORT void visit(const peg::ast_node& root);

	[[nodiscard("unnecessary construction")]] GRAPHQLSERVICE_EXPORT std::list<schema_error>
	getStructuredErrors();

private:
	[[nodiscard("unnecessary call")]] static ValidateTypeFieldArguments getArguments(
		const std::vector<std::shared_ptr<const schema::InputValue>>& args);

	using FieldTypes = internal::string_view_map<ValidateTypeField>;
	using TypeFields = internal::string_view_map<FieldTypes>;
	using InputFieldTypes = ValidateTypeFieldArguments;
	using InputTypeFields = internal::string_view_map<InputFieldTypes>;
	using EnumValues = internal::string_view_map<internal::string_view_set>;

	[[nodiscard("unnecessary call")]] constexpr bool isScalarType(introspection::TypeKind kind);

	[[nodiscard("unnecessary call")]] bool matchesScopedType(std::string_view name) const;

	[[nodiscard("unnecessary call")]] TypeFields::const_iterator getScopedTypeFields();
	[[nodiscard("unnecessary call")]] InputTypeFields::const_iterator getInputTypeFields(
		std::string_view name);
	[[nodiscard("unnecessary call")]] static const ValidateType& getValidateFieldType(
		const FieldTypes::mapped_type& value);
	[[nodiscard("unnecessary call")]] static const ValidateType& getValidateFieldType(
		const InputFieldTypes::mapped_type& value);
	template <class _FieldTypes>
	[[nodiscard("unnecessary call")]] static ValidateType getFieldType(
		const _FieldTypes& fields, std::string_view name);
	template <class _FieldTypes>
	[[nodiscard("unnecessary call")]] static ValidateType getWrappedFieldType(
		const _FieldTypes& fields, std::string_view name);

	void visitFragmentDefinition(const peg::ast_node& fragmentDefinition);
	void visitOperationDefinition(const peg::ast_node& operationDefinition);

	void visitSelection(const peg::ast_node& selection);

	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	void visitDirectives(
		introspection::DirectiveLocation location, const peg::ast_node& directives);

	[[nodiscard("unnecessary call")]] bool validateInputValue(bool hasNonNullDefaultValue,
		const ValidateArgumentValuePtr& argument, const ValidateType& type);
	[[nodiscard("unnecessary call")]] bool validateVariableType(bool isNonNull,
		const ValidateType& variableType, const schema_location& position,
		const ValidateType& inputType);

	const std::shared_ptr<schema::Schema> _schema;
	std::list<schema_error> _errors;

	using Directives = internal::string_view_map<ValidateDirective>;
	using AstNodeRef = std::reference_wrapper<const peg::ast_node>;
	using ExecutableNodes = internal::string_view_map<AstNodeRef>;
	using FragmentSet = internal::string_view_set;
	using MatchingTypes = internal::string_view_map<internal::string_view_set>;
	using ScalarTypes = internal::string_view_set;
	using VariableDefinitions = internal::string_view_map<AstNodeRef>;
	using VariableTypes = internal::string_view_map<ValidateArgument>;
	using OperationVariables = std::optional<VariableTypes>;
	using VariableSet = internal::string_view_set;

	// These members store Introspection schema information which does not change between queries.
	ValidateTypes _operationTypes;
	ValidateTypes _types;
	MatchingTypes _matchingTypes;
	Directives _directives;
	EnumValues _enumValues;
	ScalarTypes _scalarTypes;

	// These members store information that's specific to a single query and changes every time we
	// visit a new one. They must be reset in between queries.
	ExecutableNodes _fragmentDefinitions;
	ExecutableNodes _operationDefinitions;
	FragmentSet _referencedFragments;
	FragmentSet _fragmentCycles;

	// These members store state for the visitor. They implicitly reset each time we call visit.
	OperationVariables _operationVariables;
	VariableDefinitions _variableDefinitions;
	VariableSet _referencedVariables;
	FragmentSet _fragmentStack;
	std::size_t _fieldCount = 0;
	std::size_t _introspectionFieldCount = 0;
	TypeFields _typeFields;
	InputTypeFields _inputTypeFields;
	ValidateType _scopedType;
	internal::string_view_map<ValidateField> _selectionFields;
};

} // namespace graphql::service

#endif // VALIDATION_H
