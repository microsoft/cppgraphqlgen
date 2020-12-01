// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef VALIDATION_H
#define VALIDATION_H

#include "graphqlservice/GraphQLService.h"
#include "graphqlservice/IntrospectionSchema.h"

namespace graphql::service {

using ValidateType = response::Value;

struct ValidateArgument
{
	bool defaultValue = false;
	bool nonNullDefaultValue = false;
	ValidateType type;
};

using ValidateTypeFieldArguments = std::map<std::string, ValidateArgument>;

struct ValidateTypeField
{
	ValidateType returnType;
	ValidateTypeFieldArguments arguments;
};

using ValidateDirectiveArguments = std::map<std::string, ValidateArgument>;

struct ValidateDirective
{
	std::set<introspection::DirectiveLocation> locations;
	ValidateDirectiveArguments arguments;
};

struct ValidateArgumentVariable
{
	bool operator==(const ValidateArgumentVariable& other) const;

	std::string name;
};

struct ValidateArgumentEnumValue
{
	bool operator==(const ValidateArgumentEnumValue& other) const;

	std::string value;
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

	std::map<std::string, ValidateArgumentValuePtr> values;
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

using ValidateFieldArguments = std::map<std::string, ValidateArgumentValuePtr>;

struct ValidateField
{
	ValidateField(std::string&& returnType, std::optional<std::string>&& objectType,
		const std::string& fieldName, ValidateFieldArguments&& arguments);

	bool operator==(const ValidateField& other) const;

	std::string returnType;
	std::optional<std::string> objectType;
	std::string fieldName;
	ValidateFieldArguments arguments;
};

using ValidateTypeKinds = std::map<std::string, introspection::TypeKind>;

// ValidateVariableTypeVisitor visits the AST and builds a ValidateType structure representing
// a variable type in an operation definition as if it came from an Introspection query.
class ValidateVariableTypeVisitor
{
public:
	ValidateVariableTypeVisitor(const ValidateTypeKinds& typeKinds);

	void visit(const peg::ast_node& typeName);

	bool isInputType() const;
	ValidateType getType();

private:
	void visitNamedType(const peg::ast_node& namedType);
	void visitListType(const peg::ast_node& listType);
	void visitNonNullType(const peg::ast_node& nonNullType);

	const ValidateTypeKinds& _typeKinds;

	bool _isInputType = false;
	ValidateType _variableType;
};

class ValidationContext
{
public:
	ValidationContext(const Request& service);
	ValidationContext(const response::Value& introspectionQuery);

	using FieldTypes = std::map<std::string, ValidateTypeField>;
	using InputFieldTypes = ValidateTypeFieldArguments;

	struct OperationTypes
	{
		std::string queryType;
		std::string mutationType;
		std::string subscriptionType;
	};

	std::optional<introspection::TypeKind> getTypeKind(const std::string& name) const;
	const ValidateTypeKinds& getTypeKinds() const;
	std::optional<std::reference_wrapper<const std::set<std::string>>> getMatchingTypes(
		const std::string& name) const;
	std::optional<std::reference_wrapper<const FieldTypes>> getTypeFields(
		const std::string& name) const;
	std::optional<std::reference_wrapper<const InputFieldTypes>> getInputTypeFields(
		const std::string& name) const;
	std::optional<std::reference_wrapper<const std::set<std::string>>> getEnumValues(
		const std::string& name) const;
	std::optional<std::reference_wrapper<const ValidateDirective>> getDirective(
		const std::string& name) const;
	std::optional<std::reference_wrapper<const std::string>> getOperationType(
		const std::string_view& name) const;

	bool isKnownScalar(const std::string& name) const;

	template <class _FieldTypes>
	static std::string getFieldType(const _FieldTypes& fields, const std::string& name);
	template <class _FieldTypes>
	static std::string getWrappedFieldType(const _FieldTypes& fields, const std::string& name);
	static std::string getWrappedFieldType(const ValidateType& returnType);

	static constexpr bool isScalarType(introspection::TypeKind kind);

private:
	void populate(const response::Value& introspectionQuery);

	static ValidateTypeFieldArguments getArguments(const response::ListType& argumentsMember);
	static const ValidateType& getValidateFieldType(const FieldTypes::mapped_type& value);
	static const ValidateType& getValidateFieldType(const InputFieldTypes::mapped_type& value);

	using TypeFields = std::map<std::string, FieldTypes>;
	using InputTypeFields = std::map<std::string, InputFieldTypes>;
	using EnumValues = std::map<std::string, std::set<std::string>>;

	using Directives = std::map<std::string, ValidateDirective>;
	using MatchingTypes = std::map<std::string, std::set<std::string>>;
	using ScalarTypes = std::set<std::string>;

	// These members store Introspection schema information which does not change between queries.
	OperationTypes _operationTypes;
	ValidateTypeKinds _typeKinds;
	MatchingTypes _matchingTypes;
	Directives _directives;
	EnumValues _enumValues;
	ScalarTypes _scalarTypes;

	TypeFields _typeFields;
	InputTypeFields _inputTypeFields;

	// builds the validation context (lookup maps)
	void addScalar(const std::string& scalarName);
	void addEnum(const std::string& enumName, const response::Value& enumDescriptionMap);
	void addObject(const std::string& name, const response::Value& typeDescriptionMap);
	void addInputObject(const std::string& name, const response::Value& typeDescriptionMap);
	void addInterfaceOrUnion(const std::string& name, const response::Value& typeDescriptionMap);
	void addDirective(const std::string& name, const response::ListType& locations,
		const response::Value& descriptionMap);
	void addTypeFields(const std::string& typeName, const response::Value& typeDescriptionMap);
	void addInputTypeFields(const std::string& typeName, const response::Value& typeDescriptionMap);
};

// ValidateExecutableVisitor visits the AST and validates that it is executable against the service
// schema.
class ValidateExecutableVisitor
{
public:
	// Legacy, left for compatibility reasons. Services should create a ValidationContext and pass
	// it
	ValidateExecutableVisitor(const Request& service);
	ValidateExecutableVisitor(std::shared_ptr<const ValidationContext> validationContext);

	void visit(const peg::ast_node& root);

	std::vector<schema_error> getStructuredErrors();

private:
	std::optional<introspection::TypeKind> getScopedTypeKind() const;
	bool matchesScopedType(const std::string& name) const;

	std::optional<std::reference_wrapper<const ValidationContext::FieldTypes>> getScopedTypeFields()
		const;

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

	std::shared_ptr<const ValidationContext> _validationContext;

	std::vector<schema_error> _errors;

	using ExecutableNodes = std::map<std::string, const peg::ast_node&>;
	using FragmentSet = std::unordered_set<std::string>;
	using VariableDefinitions = std::map<std::string, const peg::ast_node&>;
	using VariableTypes = std::map<std::string, ValidateArgument>;
	using OperationVariables = std::optional<VariableTypes>;
	using VariableSet = std::set<std::string>;

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
	size_t _fieldCount = 0;
	std::string _scopedType;
	std::map<std::string, ValidateField> _selectionFields;
};

} /* namespace graphql::service */

#endif // VALIDATION_H
