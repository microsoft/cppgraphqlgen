// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <graphqlservice/GraphQLService.h>

#include <array>
#include <cstdio>

namespace facebook::graphql::schema {

// These are the set of built-in types in GraphQL.
enum class BuiltinType
{
	Int,
	Float,
	String,
	Boolean,
	ID,
};

using BuiltinTypeMap = std::unordered_map<std::string, BuiltinType>;

// These are the C++ types we'll use for them.
using CppTypeMap = std::array<std::string, static_cast<size_t>(BuiltinType::ID) + 1>;

// Types that we understand and use to generate the skeleton of a service.
enum class SchemaType
{
	Scalar,
	Enum,
	Input,
	Union,
	Interface,
	Object,
	Operation,
};

using SchemaTypeMap = std::unordered_map<std::string, SchemaType>;

// Keep track of the positions of each type declaration in the file.
using PositionMap = std::unordered_map<std::string, tao::graphqlpeg::position>;

// For all of the named types we track, we want to keep them in order in a vector but
// be able to lookup their offset quickly by name.
using TypeNameMap = std::unordered_map<std::string, size_t>;

// Any type can also have a list and/or non-nullable wrapper, and those can be nested.
// Since it's easier to express nullability than non-nullability in C++, we'll invert
// the presence of NonNull modifiers.
using TypeModifierStack = std::vector<service::TypeModifier>;

// Scalar types are opaque to the generator, it's up to the service implementation
// to handle parsing, validating, and serializing them. We just need to track which
// scalar type names have been declared so we recognize the references.
struct ScalarType
{
	std::string type;
	std::string description;
};

using ScalarTypeList = std::vector<ScalarType>;

// Enum types map a type name to a collection of valid string values.
struct EnumValueType
{
	std::string value;
	std::string description;
	std::optional<std::string> deprecationReason;
	std::optional<tao::graphqlpeg::position> position;
};

struct EnumType
{
	std::string type;
	std::vector<EnumValueType> values;
	std::string description;
};

using EnumTypeList = std::vector<EnumType>;

// Input types are complex types that have a set of named fields. Each field may be
// a scalar type (including lists or non-null wrappers) or another nested input type,
// but it cannot include output object types.
enum class InputFieldType
{
	Builtin,
	Scalar,
	Enum,
	Input,
};

struct InputField
{
	std::string type;
	std::string name;
	std::string defaultValueString;
	response::Value defaultValue;
	InputFieldType fieldType = InputFieldType::Builtin;
	TypeModifierStack modifiers;
	std::string description;
	std::optional<tao::graphqlpeg::position> position;
};

using InputFieldList = std::vector<InputField>;

struct InputType
{
	std::string type;
	InputFieldList fields;
	std::string description;
};

using InputTypeList = std::vector<InputType>;

// Directives are defined with arguments and a list of valid locations.
struct Directive
{
	std::string name;
	std::vector<std::string> locations;
	InputFieldList arguments;
	std::string description;
};

using DirectiveList = std::vector<Directive>;

// Union types map a type name to a set of potential concrete type names.
struct UnionType
{
	std::string type;
	std::vector<std::string> options;
	std::string description;
};

using UnionTypeList = std::vector<UnionType>;

// Output types are scalar types or complex types that have a set of named fields. Each
// field may be a scalar type (including lists or non-null wrappers) or another nested
// output type, but it cannot include input object types. Each field can also take
// optional arguments which are all input types.
enum class OutputFieldType
{
	Builtin,
	Scalar,
	Enum,
	Union,
	Interface,
	Object,
};

struct OutputField
{
	std::string type;
	std::string name;
	InputFieldList arguments;
	OutputFieldType fieldType = OutputFieldType::Builtin;
	TypeModifierStack modifiers;
	std::string description;
	std::optional<std::string> deprecationReason;
	std::optional<tao::graphqlpeg::position> position;
};

using OutputFieldList = std::vector<OutputField>;

// Interface types are abstract complex output types that have a set of fields. They
// are inherited by concrete object output types which support all of the fields in
// the interface, and the concrete object matches the interface for fragment type
// conditions. The fields can include any output type.
struct InterfaceType
{
	std::string type;
	OutputFieldList fields;
	std::string description;
};

using InterfaceTypeList = std::vector<InterfaceType>;

// Object types are concrete complex output types that have a set of fields. They
// may inherit multiple interfaces.
struct ObjectType
{
	std::string type;
	std::vector<std::string> interfaces;
	OutputFieldList fields;
	std::string description;
};

using ObjectTypeList = std::vector<ObjectType>;

// The schema maps operation types to named types.
struct OperationType
{
	std::string type;
	std::string operation;
};

using OperationTypeList = std::vector<OperationType>;

class Generator
{
public:
	// Initialize the generator with the introspection schema.
	explicit Generator();

	// Initialize the generator with the GraphQL schema and output parameters.
	explicit Generator(std::string&& schemaFileName, std::string&& filenamePrefix, std::string&& schemaNamespace);

	// Run the generator and return a list of filenames that were output.
	std::vector<std::string> Build() const noexcept;

private:
	void visitDefinition(const peg::ast_node& definition);

	void visitSchemaDefinition(const peg::ast_node& schemaDefinition);
	void visitSchemaExtension(const peg::ast_node& schemaExtension);
	void visitScalarTypeDefinition(const peg::ast_node& scalarTypeDefinition);
	void visitEnumTypeDefinition(const peg::ast_node& enumTypeDefinition);
	void visitEnumTypeExtension(const peg::ast_node& enumTypeExtension);
	void visitInputObjectTypeDefinition(const peg::ast_node& inputObjectTypeDefinition);
	void visitInputObjectTypeExtension(const peg::ast_node& inputObjectTypeExtension);
	void visitUnionTypeDefinition(const peg::ast_node& unionTypeDefinition);
	void visitUnionTypeExtension(const peg::ast_node& unionTypeExtension);
	void visitInterfaceTypeDefinition(const peg::ast_node& interfaceTypeDefinition);
	void visitInterfaceTypeExtension(const peg::ast_node& interfaceTypeExtension);
	void visitObjectTypeDefinition(const peg::ast_node& objectTypeDefinition);
	void visitObjectTypeExtension(const peg::ast_node& objectTypeExtension);
	void visitDirectiveDefinition(const peg::ast_node& directiveDefinition);

	static OutputFieldList getOutputFields(const std::vector<std::unique_ptr<peg::ast_node>>& fields);
	static InputFieldList getInputFields(const std::vector<std::unique_ptr<peg::ast_node>>& fields);

	// Recursively visit a Type node until we reach a NamedType and we've
	// taken stock of all of the modifier wrappers.
	class TypeVisitor
	{
	public:
		std::pair<std::string, TypeModifierStack> getType();

		void visit(const peg::ast_node& typeName);

	private:
		void visitNamedType(const peg::ast_node& namedType);
		void visitListType(const peg::ast_node& listType);
		void visitNonNullType(const peg::ast_node& nonNullType);

		std::string _type;
		TypeModifierStack _modifiers;
		bool _nonNull = false;
	};

	// Recursively visit a Value node representing the default value on an input field
	// and build a JSON representation of the hardcoded value.
	class DefaultValueVisitor
	{
	public:
		response::Value getValue();

		void visit(const peg::ast_node& value);

	private:
		void visitIntValue(const peg::ast_node& intValue);
		void visitFloatValue(const peg::ast_node& floatValue);
		void visitStringValue(const peg::ast_node& stringValue);
		void visitBooleanValue(const peg::ast_node& booleanValue);
		void visitNullValue(const peg::ast_node& nullValue);
		void visitEnumValue(const peg::ast_node& enumValue);
		void visitListValue(const peg::ast_node& listValue);
		void visitObjectValue(const peg::ast_node& objectValue);

		response::Value _value;
	};

	void validateSchema();
	void fixupOutputFieldList(OutputFieldList& fields);
	void fixupInputFieldList(InputFieldList& fields);

	const std::string& getCppType(const std::string& type) const noexcept;
	std::string getInputCppType(const InputField& field) const noexcept;
	std::string getOutputCppType(const OutputField& field, bool interfaceField) const noexcept;

	bool outputHeader() const noexcept;
	std::string getFieldDeclaration(const InputField& inputField) const noexcept;
	std::string getFieldDeclaration(const OutputField& outputField, bool interfaceField, bool inheritedField) const noexcept;
	std::string getResolverDeclaration(const OutputField& outputField) const noexcept;

	bool outputSource() const noexcept;
	std::string getArgumentDefaultValue(size_t level, const response::Value& defaultValue) const noexcept;
	std::string getArgumentDeclaration(const InputField& argument, const char* prefixToken, const char* argumentsToken, const char* defaultToken) const noexcept;
	std::string getArgumentAccessType(const InputField& argument) const noexcept;
	std::string getResultAccessType(const OutputField& result) const noexcept;
	std::string getTypeModifiers(const TypeModifierStack& modifiers) const noexcept;
	std::string getIntrospectionType(const std::string& type, const TypeModifierStack& modifiers) const noexcept;

	static const std::string s_introspectionNamespace;
	static const BuiltinTypeMap s_builtinTypes;
	static const CppTypeMap s_builtinCppTypes;
	static const std::string s_scalarCppType;

	const bool _isIntrospection;
	const std::string _filenamePrefix;
	const std::string _schemaNamespace;

	SchemaTypeMap _schemaTypes;
	PositionMap _typePositions;
	TypeNameMap _scalarNames;
	ScalarTypeList _scalarTypes;
	TypeNameMap _enumNames;
	EnumTypeList _enumTypes;
	TypeNameMap _inputNames;
	InputTypeList _inputTypes;
	TypeNameMap _unionNames;
	UnionTypeList _unionTypes;
	TypeNameMap _interfaceNames;
	InterfaceTypeList _interfaceTypes;
	TypeNameMap _objectNames;
	ObjectTypeList _objectTypes;
	DirectiveList _directives;
	PositionMap _directivePositions;
	OperationTypeList _operationTypes;
};

} /* namespace facebook::graphql::schema */
