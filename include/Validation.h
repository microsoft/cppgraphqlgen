// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef VALIDATION_H
#define VALIDATION_H

#include "graphqlservice/GraphQLService.h"
#include "graphqlservice/IntrospectionSchema.h"

namespace graphql::service {

class ValidateType
{
public:
	virtual introspection::TypeKind kind() const = 0;
	virtual const std::string_view name() const = 0;
	virtual bool isInputType() const = 0;
	virtual bool isValid() const = 0;
	virtual std::shared_ptr<const ValidateType> getInnerType() const = 0;
	virtual bool operator==(const ValidateType& other) const = 0;

	static constexpr bool isKindInput(introspection::TypeKind typeKind)
	{
		switch (typeKind)
		{
			case introspection::TypeKind::SCALAR:
			case introspection::TypeKind::ENUM:
			case introspection::TypeKind::INPUT_OBJECT:
				return true;
			default:
				return false;
		}
	}
};

class NamedValidateType
	: public ValidateType
	, public std::enable_shared_from_this<NamedValidateType>
{
public:
	std::string _name;

	NamedValidateType(const std::string_view& name)
		: _name(std::string(name))
	{
	}

	virtual const std::string_view name() const
	{
		return _name;
	}

	virtual bool isValid() const
	{
		return !name().empty();
	}

	virtual std::shared_ptr<const ValidateType> getInnerType() const
	{
		return shared_from_this();
	}

	virtual bool operator==(const ValidateType& other) const
	{
		if (this == &other)
		{
			return true;
		}

		if (kind() != other.kind())
		{
			return false;
		}

		return _name == other.name();
	}
};

template <introspection::TypeKind typeKind>
class NamedType : public NamedValidateType
{
public:
	NamedType<typeKind>(const std::string_view& name)
		: NamedValidateType(name)
	{
	}

	virtual introspection::TypeKind kind() const
	{
		return typeKind;
	}

	virtual bool isInputType() const
	{
		return ValidateType::isKindInput(typeKind);
	}
};

using ScalarType = NamedType<introspection::TypeKind::SCALAR>;

class EnumType : public NamedType<introspection::TypeKind::ENUM>
{
public:
	EnumType(const std::string_view& name, std::unordered_set<std::string>&& values)
		: NamedType<introspection::TypeKind::ENUM>(name)
		, _values(std::move(values))
	{
	}

	bool find(const std::string_view& key) const
	{
		// TODO: in the future the set will be of string_view
		return _values.find(std::string { key }) != _values.end();
	}

private:
	std::unordered_set<std::string> _values;
};

template <introspection::TypeKind typeKind>
class WrapperOfType : public ValidateType
{
public:
	WrapperOfType<typeKind>(std::shared_ptr<ValidateType> ofType)
		: _ofType(std::move(ofType))
	{
	}

	std::shared_ptr<ValidateType> ofType() const
	{
		return _ofType;
	}

	virtual introspection::TypeKind kind() const
	{
		return typeKind;
	}

	virtual const std::string_view name() const
	{
		return _name;
	}

	virtual bool isInputType() const
	{
		return _ofType ? _ofType->isInputType() : false;
	}

	virtual bool isValid() const
	{
		return _ofType ? _ofType->isValid() : false;
	}

	virtual std::shared_ptr<const ValidateType> getInnerType() const
	{
		return _ofType ? _ofType->getInnerType() : nullptr;
	}

	virtual bool operator==(const ValidateType& otherType) const
	{
		if (this == &otherType)
		{
			return true;
		}

		if (typeKind != otherType.kind())
		{
			return false;
		}

		const auto& other = static_cast<const WrapperOfType<typeKind>&>(otherType);
		if (_ofType == other.ofType())
		{
			return true;
		}
		if (_ofType && other.ofType())
		{
			return *_ofType == *other.ofType();
		}
		return false;
	}

private:
	std::shared_ptr<ValidateType> _ofType;
	static inline const std::string_view _name = ""sv;
};

using ListOfType = WrapperOfType<introspection::TypeKind::LIST>;
using NonNullOfType = WrapperOfType<introspection::TypeKind::NON_NULL>;

struct ValidateArgument
{
	std::shared_ptr<ValidateType> type;
	bool defaultValue = false;
	bool nonNullDefaultValue = false;
};

using ValidateTypeFieldArguments = std::unordered_map<std::string, ValidateArgument>;

struct VariableDefinition : public ValidateArgument
{
	schema_location position;
};

struct ValidateTypeField
{
	std::shared_ptr<ValidateType> returnType;
	ValidateTypeFieldArguments arguments;
};

using ValidateDirectiveArguments = std::unordered_map<std::string, ValidateArgument>;

template <typename FieldType>
class ContainerValidateType : public NamedValidateType
{
public:
	ContainerValidateType<FieldType>(const std::string_view& name)
		: NamedValidateType(name)
	{
	}

	using FieldsContainer = std::unordered_map<std::string, FieldType>;

	std::optional<std::reference_wrapper<const FieldType>> getField(
		const std::string_view& name) const
	{
		// TODO: string is a work around, the _fields set will be moved to string_view soon
		const auto& itr = _fields.find(std::string { name });
		if (itr == _fields.cend())
		{
			return std::nullopt;
		}

		return std::optional<std::reference_wrapper<const FieldType>>(itr->second);
	}

	void setFields(FieldsContainer&& fields)
	{
		_fields = std::move(fields);
	}

	typename FieldsContainer::const_iterator begin() const
	{
		return _fields.cbegin();
	}

	typename FieldsContainer::const_iterator end() const
	{
		return _fields.cend();
	}

	virtual bool matchesType(const ValidateType& other) const
	{
		if (*this == other)
		{
			return true;
		}

		switch (other.kind())
		{
			case introspection::TypeKind::INTERFACE:
			case introspection::TypeKind::UNION:
				return static_cast<const ContainerValidateType&>(other).matchesType(*this);

			default:
				return false;
		}
	}

private:
	FieldsContainer _fields;
};

template <introspection::TypeKind typeKind, typename FieldType>
class ContainerType : public ContainerValidateType<FieldType>
{
public:
	ContainerType<typeKind, FieldType>(const std::string_view& name)
		: ContainerValidateType<FieldType>(name)
	{
	}

	virtual introspection::TypeKind kind() const
	{
		return typeKind;
	}

	virtual bool isInputType() const
	{
		return ValidateType::isKindInput(typeKind);
	}
};

using ObjectType = ContainerType<introspection::TypeKind::OBJECT, ValidateTypeField>;
using InputObjectType = ContainerType<introspection::TypeKind::INPUT_OBJECT, ValidateArgument>;

class PossibleTypesContainerValidateType : public ContainerValidateType<ValidateTypeField>
{
public:
	PossibleTypesContainerValidateType(const std::string_view& name)
		: ContainerValidateType<ValidateTypeField>(name)
	{
	}

	const std::set<const ValidateType*>& possibleTypes() const
	{
		return _possibleTypes;
	}

	virtual bool matchesType(const ValidateType& other) const
	{
		if (*this == other)
		{
			return true;
		}

		switch (other.kind())
		{
			case introspection::TypeKind::OBJECT:
				return _possibleTypes.find(&other) != _possibleTypes.cend();

			case introspection::TypeKind::INTERFACE:
			case introspection::TypeKind::UNION:
			{
				const auto& types =
					static_cast<const PossibleTypesContainerValidateType&>(other).possibleTypes();
				for (const auto& itr : _possibleTypes)
				{
					if (types.find(itr) != types.cend())
					{
						return true;
					}
				}
				return false;
			}

			default:
				return false;
		}
	}

	void setPossibleTypes(std::set<const ValidateType*>&& possibleTypes)
	{
		_possibleTypes = std::move(possibleTypes);
	}

private:
	std::set<const ValidateType*> _possibleTypes;
};

template <introspection::TypeKind typeKind>
class PossibleTypesContainer : public PossibleTypesContainerValidateType
{
public:
	PossibleTypesContainer<typeKind>(const std::string_view& name)
		: PossibleTypesContainerValidateType(name)
	{
	}

	virtual introspection::TypeKind kind() const
	{
		return typeKind;
	}

	virtual bool isInputType() const
	{
		return ValidateType::isKindInput(typeKind);
	}
};

using InterfaceType = PossibleTypesContainer<introspection::TypeKind::INTERFACE>;
using UnionType = PossibleTypesContainer<introspection::TypeKind::UNION>;

struct ValidateDirective
{
	std::set<introspection::DirectiveLocation> locations;
	ValidateDirectiveArguments arguments;
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

class ValidationContext
{
public:
	ValidationContext(const Request& service);
	ValidationContext(const response::Value& introspectionQuery);

	struct OperationTypes
	{
		std::string queryType;
		std::string mutationType;
		std::string subscriptionType;
	};

	std::optional<std::reference_wrapper<const ValidateDirective>> getDirective(
		const std::string_view& name) const;
	std::optional<std::reference_wrapper<const std::string>> getOperationType(
		const std::string_view& name) const;

	template <typename T = NamedValidateType,
		typename std::enable_if<std::is_base_of<NamedValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<T> getNamedValidateType(const std::string_view& name) const;
	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<ListOfType> getListOfType(std::shared_ptr<T>&& ofType) const;
	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<NonNullOfType> getNonNullOfType(std::shared_ptr<T>&& ofType) const;

private:
	void populate(const response::Value& introspectionQuery);

	struct
	{
		std::shared_ptr<ScalarType> string;
		std::shared_ptr<NonNullOfType> nonNullString;
	} commonTypes;

	ValidateTypeFieldArguments getArguments(const response::ListType& argumentsMember);

	using Directives = std::unordered_map<std::string, ValidateDirective>;

	// These members store Introspection schema information which does not change between queries.
	OperationTypes _operationTypes;
	Directives _directives;

	std::unordered_map<const ValidateType*, std::shared_ptr<ListOfType>> _listOfCache;
	std::unordered_map<const ValidateType*, std::shared_ptr<NonNullOfType>> _nonNullCache;
	std::unordered_map<std::string_view, std::shared_ptr<NamedValidateType>> _namedCache;

	template <typename T,
		typename std::enable_if<std::is_base_of<NamedValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<T> makeNamedValidateType(T&& typeDef);

	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<ListOfType> makeListOfType(std::shared_ptr<T>&& ofType);

	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<ListOfType> makeListOfType(std::shared_ptr<T>& ofType)
	{
		return makeListOfType(std::shared_ptr<T>(ofType));
	}

	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<NonNullOfType> makeNonNullOfType(std::shared_ptr<T>&& ofType);

	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<NonNullOfType> makeNonNullOfType(std::shared_ptr<T>& ofType)
	{
		return makeNonNullOfType(std::shared_ptr<T>(ofType));
	}

	std::shared_ptr<ScalarType> makeScalarType(const std::string_view& name)
	{
		return makeNamedValidateType(ScalarType { name });
	}

	std::shared_ptr<ObjectType> makeObjectType(const std::string_view& name)
	{
		return makeNamedValidateType(ObjectType { name });
	}

	std::shared_ptr<ValidateType> getTypeFromMap(const response::Value& typeMap);

	// builds the validation context (lookup maps)
	void addScalar(const std::string_view& scalarName);
	void addEnum(const std::string_view& enumName, const response::Value& enumDescriptionMap);
	void addObject(const std::string_view& name);
	void addInputObject(const std::string_view& name);
	void addInterface(const std::string_view& name, const response::Value& typeDescriptionMap);
	void addUnion(const std::string_view& name, const response::Value& typeDescriptionMap);
	void addDirective(const std::string_view& name, const response::ListType& locations,
		const response::Value& descriptionMap);

	void addTypeFields(std::shared_ptr<ContainerValidateType<ValidateTypeField>> type,
		const response::Value& typeDescriptionMap);
	void addPossibleTypes(std::shared_ptr<PossibleTypesContainerValidateType> type,
		const response::Value& typeDescriptionMap);
	void addInputTypeFields(
		std::shared_ptr<InputObjectType> type, const response::Value& typeDescriptionMap);
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

	std::shared_ptr<const ValidationContext> _validationContext;

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
