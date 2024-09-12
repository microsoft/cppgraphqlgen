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
#include <cstddef>
#include <unordered_map>
#include <unordered_set>

namespace graphql::generator {

// These are the set of built-in types in GraphQL.
enum class [[nodiscard("unnecessary conversion")]] BuiltinType {
	Int,
	Float,
	String,
	Boolean,
	ID,
};

using BuiltinTypeMap = std::map<std::string_view, BuiltinType>;

// These are the C++ types we'll use for them.
using CppTypeMap = std::array<std::string_view, static_cast<std::size_t>(BuiltinType::ID) + 1>;

// Keep track of the positions of each type declaration in the file.
using PositionMap = std::unordered_map<std::string_view, tao::graphqlpeg::position>;

// For all of the named types we track, we want to keep them in order in a vector but
// be able to lookup their offset quickly by name.
using TypeNameMap = std::unordered_map<std::string_view, std::size_t>;

// Scalar types are opaque to the generator, it's up to the service implementation
// to handle parsing, validating, and serializing them. We just need to track which
// scalar type names have been declared so we recognize the references.
struct [[nodiscard("unnecessary construction")]] ScalarType
{
	std::string_view type;
	std::string_view description;
	std::string_view specifiedByURL {};
};

using ScalarTypeList = std::vector<ScalarType>;

// Enum types map a type name to a collection of valid string values.
struct [[nodiscard("unnecessary construction")]] EnumValueType
{
	std::string_view value;
	std::string_view cppValue;
	std::string_view description;
	std::optional<std::string_view> deprecationReason;
	std::optional<tao::graphqlpeg::position> position;
};

struct [[nodiscard("unnecessary construction")]] EnumType
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
enum class [[nodiscard("unnecessary conversion")]] InputFieldType {
	Builtin,
	Scalar,
	Enum,
	Input,
};

struct [[nodiscard("unnecessary construction")]] InputField
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

struct [[nodiscard("unnecessary construction")]] InputType
{
	std::string_view type;
	std::string_view cppType;
	InputFieldList fields;
	std::string_view description;
	std::unordered_set<std::string_view> dependencies {};
	std::vector<std::string_view> declarations {};
};

using InputTypeList = std::vector<InputType>;

// Directives are defined with arguments and a list of valid locations.
struct [[nodiscard("unnecessary construction")]] Directive
{
	std::string_view name;
	bool isRepeatable = false;
	std::vector<std::string_view> locations;
	InputFieldList arguments;
	std::string_view description;
};

using DirectiveList = std::vector<Directive>;

// Union types map a type name to a set of potential concrete type names.
struct [[nodiscard("unnecessary construction")]] UnionType
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
enum class [[nodiscard("unnecessary conversion")]] OutputFieldType {
	Builtin,
	Scalar,
	Enum,
	Union,
	Interface,
	Object,
};

constexpr std::string_view strGet = "get";
constexpr std::string_view strApply = "apply";

struct [[nodiscard("unnecessary construction")]] OutputField
{
	std::string_view type;
	std::string_view name;
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
struct [[nodiscard("unnecessary construction")]] InterfaceType
{
	std::string_view type;
	std::string_view cppType;
	std::vector<std::string_view> interfaces;
	OutputFieldList fields;
	std::string_view description;
};

using InterfaceTypeList = std::vector<InterfaceType>;

// Object types are concrete complex output types that have a set of fields. They
// may inherit multiple interfaces.
struct [[nodiscard("unnecessary construction")]] ObjectType
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
struct [[nodiscard("unnecessary construction")]] OperationType
{
	std::string_view type;
	std::string_view cppType;
	std::string_view operation;
};

using OperationTypeList = std::vector<OperationType>;

struct [[nodiscard("unnecessary construction")]] SchemaOptions
{
	const std::string schemaFilename;
	const std::string filenamePrefix;
	const std::string schemaNamespace;
	const bool isIntrospection = false;
};

class [[nodiscard("unnecessary construction")]] SchemaLoader
{
public:
	// Initialize the loader with the introspection schema or a custom GraphQL schema.
	explicit SchemaLoader(SchemaOptions&& schemaOptions);

	[[nodiscard("unnecessary call")]] bool isIntrospection() const noexcept;
	[[nodiscard("unnecessary call")]] std::string_view getSchemaDescription() const noexcept;
	[[nodiscard("unnecessary call")]] std::string_view getFilenamePrefix() const noexcept;
	[[nodiscard("unnecessary call")]] std::string_view getSchemaNamespace() const noexcept;

	[[nodiscard("unnecessary call")]] static std::string_view getIntrospectionNamespace() noexcept;
	[[nodiscard("unnecessary call")]] static const BuiltinTypeMap& getBuiltinTypes() noexcept;
	[[nodiscard("unnecessary call")]] static const CppTypeMap& getBuiltinCppTypes() noexcept;
	[[nodiscard("unnecessary call")]] static std::string_view getScalarCppType() noexcept;

	[[nodiscard("unnecessary call")]] SchemaType getSchemaType(std::string_view type) const;
	[[nodiscard("unnecessary call")]] const tao::graphqlpeg::position& getTypePosition(
		std::string_view type) const;

	[[nodiscard("unnecessary call")]] std::size_t getScalarIndex(std::string_view type) const;
	[[nodiscard("unnecessary call")]] const ScalarTypeList& getScalarTypes() const noexcept;

	[[nodiscard("unnecessary call")]] std::size_t getEnumIndex(std::string_view type) const;
	[[nodiscard("unnecessary call")]] const EnumTypeList& getEnumTypes() const noexcept;

	[[nodiscard("unnecessary call")]] std::size_t getInputIndex(std::string_view type) const;
	[[nodiscard("unnecessary call")]] const InputTypeList& getInputTypes() const noexcept;

	[[nodiscard("unnecessary call")]] std::size_t getUnionIndex(std::string_view type) const;
	[[nodiscard("unnecessary call")]] const UnionTypeList& getUnionTypes() const noexcept;

	[[nodiscard("unnecessary call")]] std::size_t getInterfaceIndex(std::string_view type) const;
	[[nodiscard("unnecessary call")]] const InterfaceTypeList& getInterfaceTypes() const noexcept;

	[[nodiscard("unnecessary call")]] std::size_t getObjectIndex(std::string_view type) const;
	[[nodiscard("unnecessary call")]] const ObjectTypeList& getObjectTypes() const noexcept;

	[[nodiscard("unnecessary call")]] const DirectiveList& getDirectives() const noexcept;
	[[nodiscard("unnecessary call")]] const tao::graphqlpeg::position& getDirectivePosition(
		std::string_view type) const;

	[[nodiscard("unnecessary call")]] const OperationTypeList& getOperationTypes() const noexcept;

	[[nodiscard("unnecessary call")]] static std::string_view getSafeCppName(
		std::string_view type) noexcept;

	[[nodiscard("unnecessary call")]] std::string_view getCppType(
		std::string_view type) const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getInputCppType(
		const InputField& field) const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getOutputCppType(
		const OutputField& field) const noexcept;

	[[nodiscard("unnecessary memory copy")]] static std::string getOutputCppAccessor(
		const OutputField& field) noexcept;
	[[nodiscard("unnecessary memory copy")]] static std::string getOutputCppResolver(
		const OutputField& field) noexcept;

	[[nodiscard("unnecessary call")]] static bool shouldMoveInputField(
		const InputField& field) noexcept;

private:
	[[nodiscard("unnecessary call")]] static bool isExtension(
		const peg::ast_node& definition) noexcept;

	void visitDefinition(const peg::ast_node& definition);

	void visitSchemaDefinition(const peg::ast_node& schemaDefinition);
	void visitSchemaExtension(const peg::ast_node& schemaExtension);
	void visitScalarTypeDefinition(const peg::ast_node& scalarTypeDefinition);
	void visitScalarTypeExtension(const peg::ast_node& scalarTypeExtension);
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

	static void blockReservedName(
		std::string_view name, std::optional<tao::graphqlpeg::position> position = std::nullopt);
	[[nodiscard("unnecessary memory copy")]] static OutputFieldList getOutputFields(
		const peg::ast_node::children_t& fields);
	[[nodiscard("unnecessary memory copy")]] static InputFieldList getInputFields(
		const peg::ast_node::children_t& fields);

	void validateSchema();
	void fixupOutputFieldList(OutputFieldList& fields,
		const std::optional<std::unordered_set<std::string_view>>& interfaceFields,
		const std::optional<std::string_view>& accessor);
	void fixupInputFieldList(InputFieldList& fields);
	void reorderInputTypeDependencies();
	void validateImplementedInterfaces() const;
	[[nodiscard("unnecessary call")]] const InterfaceType& findInterfaceType(
		std::string_view typeName, std::string_view interfaceName) const;
	void validateInterfaceFields(std::string_view typeName, std::string_view interfaceName,
		const OutputFieldList& typeFields) const;
	void validateTransitiveInterfaces(
		std::string_view typeName, const std::vector<std::string_view>& interfaces) const;

	[[nodiscard("unnecessary memory copy")]] static std::string getJoinedCppName(
		std::string_view prefix, std::string_view fieldName) noexcept;

	static const std::string_view s_introspectionNamespace;
	static const BuiltinTypeMap s_builtinTypes;
	static const CppTypeMap s_builtinCppTypes;
	static const std::string_view s_scalarCppType;

	const SchemaOptions _schemaOptions;
	const bool _isIntrospection;
	std::string_view _schemaDescription;
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
