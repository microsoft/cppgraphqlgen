// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef SCHEMALOADER_H
#define SCHEMALOADER_H

#include "GeneratorLoader.h"

#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLService.h"

#include "graphqlservice/internal/Grammar.h"

#include <array>
#include <unordered_map>
#include <unordered_set>

namespace graphql::generator {

// These are the set of built-in types in GraphQL.
enum class BuiltinType
{
	Int,
	Float,
	String,
	Boolean,
	ID,
};

using BuiltinTypeMap = std::map<std::string_view, BuiltinType>;

// These are the C++ types we'll use for them.
using CppTypeMap = std::array<std::string_view, static_cast<size_t>(BuiltinType::ID) + 1>;

// Keep track of the positions of each type declaration in the file.
using PositionMap = std::unordered_map<std::string_view, tao::graphqlpeg::position>;

// For all of the named types we track, we want to keep them in order in a vector but
// be able to lookup their offset quickly by name.
using TypeNameMap = std::unordered_map<std::string_view, size_t>;

// Scalar types are opaque to the generator, it's up to the service implementation
// to handle parsing, validating, and serializing them. We just need to track which
// scalar type names have been declared so we recognize the references.
struct ScalarType
{
	std::string_view type;
	std::string_view description;
};

using ScalarTypeList = std::vector<ScalarType>;

// Enum types map a type name to a collection of valid string values.
struct EnumValueType
{
	std::string_view value;
	std::string_view cppValue;
	std::string_view description;
	std::optional<std::string_view> deprecationReason;
	std::optional<tao::graphqlpeg::position> position;
};

struct EnumType
{
	std::string_view type;
	std::string_view cppType;
	std::vector<EnumValueType> values;
	std::string_view description;
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
	std::string_view type;
	std::string_view name;
	std::string_view cppName;
	std::string_view defaultValueString;
	response::Value defaultValue;
	InputFieldType fieldType = InputFieldType::Builtin;
	TypeModifierStack modifiers;
	std::string_view description;
	std::optional<tao::graphqlpeg::position> position;
};

using InputFieldList = std::vector<InputField>;

struct InputType
{
	std::string_view type;
	std::string_view cppType;
	InputFieldList fields;
	std::string_view description;
	std::unordered_set<std::string_view> dependencies;
};

using InputTypeList = std::vector<InputType>;

// Directives are defined with arguments and a list of valid locations.
struct Directive
{
	std::string_view name;
	std::vector<std::string_view> locations;
	InputFieldList arguments;
	std::string_view description;
};

using DirectiveList = std::vector<Directive>;

// Union types map a type name to a set of potential concrete type names.
struct UnionType
{
	std::string_view type;
	std::string_view cppType;
	std::vector<std::string_view> options;
	std::string_view description;
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

constexpr std::string_view strGet = "get";
constexpr std::string_view strApply = "apply";

struct OutputField
{
	std::string_view type;
	std::string_view name;
	std::string_view cppName;
	InputFieldList arguments;
	OutputFieldType fieldType = OutputFieldType::Builtin;
	TypeModifierStack modifiers;
	std::string_view description;
	std::optional<std::string_view> deprecationReason;
	std::optional<tao::graphqlpeg::position> position;
	bool interfaceField = false;
	bool inheritedField = false;
	std::string_view accessor { strGet };
};

using OutputFieldList = std::vector<OutputField>;

// Interface types are abstract complex output types that have a set of fields. They
// are inherited by concrete object output types which support all of the fields in
// the interface, and the concrete object matches the interface for fragment type
// conditions. The fields can include any output type.
struct InterfaceType
{
	std::string_view type;
	std::string_view cppType;
	OutputFieldList fields;
	std::string_view description;
};

using InterfaceTypeList = std::vector<InterfaceType>;

// Object types are concrete complex output types that have a set of fields. They
// may inherit multiple interfaces.
struct ObjectType
{
	std::string_view type;
	std::string_view cppType;
	std::vector<std::string_view> interfaces;
	std::vector<std::string_view> unions;
	OutputFieldList fields;
	std::string_view description;
};

using ObjectTypeList = std::vector<ObjectType>;

// The schema maps operation types to named types.
struct OperationType
{
	std::string_view type;
	std::string_view cppType;
	std::string_view operation;
};

using OperationTypeList = std::vector<OperationType>;

struct SchemaOptions
{
	const std::string schemaFilename;
	const std::string filenamePrefix;
	const std::string schemaNamespace;
	const bool isIntrospection = false;
};

class SchemaLoader
{
public:
	// Initialize the loader with the introspection schema or a custom GraphQL schema.
	explicit SchemaLoader(SchemaOptions&& schemaOptions);

	bool isIntrospection() const noexcept;
	std::string_view getFilenamePrefix() const noexcept;
	std::string_view getSchemaNamespace() const noexcept;

	static std::string_view getIntrospectionNamespace() noexcept;
	static const BuiltinTypeMap& getBuiltinTypes() noexcept;
	static const CppTypeMap& getBuiltinCppTypes() noexcept;
	static std::string_view getScalarCppType() noexcept;

	SchemaType getSchemaType(std::string_view type) const;
	const tao::graphqlpeg::position& getTypePosition(std::string_view type) const;

	size_t getScalarIndex(std::string_view type) const;
	const ScalarTypeList& getScalarTypes() const noexcept;

	size_t getEnumIndex(std::string_view type) const;
	const EnumTypeList& getEnumTypes() const noexcept;

	size_t getInputIndex(std::string_view type) const;
	const InputTypeList& getInputTypes() const noexcept;

	size_t getUnionIndex(std::string_view type) const;
	const UnionTypeList& getUnionTypes() const noexcept;

	size_t getInterfaceIndex(std::string_view type) const;
	const InterfaceTypeList& getInterfaceTypes() const noexcept;

	size_t getObjectIndex(std::string_view type) const;
	const ObjectTypeList& getObjectTypes() const noexcept;

	const DirectiveList& getDirectives() const noexcept;
	const tao::graphqlpeg::position& getDirectivePosition(std::string_view type) const;

	const OperationTypeList& getOperationTypes() const noexcept;

	static std::string_view getSafeCppName(std::string_view type) noexcept;

	std::string_view getCppType(std::string_view type) const noexcept;
	std::string getInputCppType(const InputField& field) const noexcept;
	std::string getOutputCppType(const OutputField& field) const noexcept;

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

	static OutputFieldList getOutputFields(const peg::ast_node::children_t& fields);
	static InputFieldList getInputFields(const peg::ast_node::children_t& fields);

	void validateSchema();
	void fixupOutputFieldList(OutputFieldList& fields,
		const std::optional<std::unordered_set<std::string_view>>& interfaceFields,
		const std::optional<std::string_view>& accessor);
	void fixupInputFieldList(InputFieldList& fields);
	void reorderInputTypeDependencies();

	static const std::string_view s_introspectionNamespace;
	static const BuiltinTypeMap s_builtinTypes;
	static const CppTypeMap s_builtinCppTypes;
	static const std::string_view s_scalarCppType;

	const SchemaOptions _schemaOptions;
	const bool _isIntrospection;
	std::string_view _schemaNamespace;
	peg::ast _ast;

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

} // namespace graphql::generator

#endif // SCHEMALOADER_H
