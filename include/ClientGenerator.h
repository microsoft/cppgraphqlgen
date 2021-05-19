// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef CLIENTGENERATOR_H
#define CLIENTGENERATOR_H

#include "graphqlservice/GraphQLGrammar.h"
#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLResponse.h"
#include "graphqlservice/GraphQLSchema.h"

#include <array>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>

namespace graphql::client {

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

using SchemaTypeMap = std::map<std::string_view, SchemaType>;

// Keep track of the positions of each type declaration in the file.
using PositionMap = std::unordered_map<std::string_view, tao::graphqlpeg::position>;

// For all of the named types we track, we want to keep them in order in a vector but
// be able to lookup their offset quickly by name.
using TypeNameMap = std::unordered_map<std::string_view, size_t>;

// Any type can also have a list and/or non-nullable wrapper, and those can be nested.
// Since it's easier to express nullability than non-nullability in C++, we'll invert
// the presence of NonNull modifiers.
using TypeModifierStack = std::vector<service::TypeModifier>;

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

// The client maps operation types to named types.
struct OperationType
{
	std::string_view type;
	std::string_view cppType;
	std::string_view operation;
};

using OperationTypeList = std::vector<OperationType>;

struct GeneratorClient
{
	const std::string schemaFilename;
	const std::string requestFilename;
	const std::string filenamePrefix;
	const std::string clientNamespace;
};

struct GeneratorPaths
{
	const std::string headerPath;
	const std::string sourcePath;
};

struct GeneratorOptions
{
	const GeneratorClient client;
	const std::optional<GeneratorPaths> paths;
	const bool verbose = false;
	const bool noIntrospection = false;
};

class Generator
{
public:
	// Initialize the generator with the introspection client or a custom GraphQL client.
	explicit Generator(GeneratorOptions&& options);

	// Run the generator and return a list of filenames that were output.
	std::vector<std::string> Build() const noexcept;

private:
	std::string getHeaderDir() const noexcept;
	std::string getSourceDir() const noexcept;
	std::string getHeaderPath() const noexcept;
	std::string getSourcePath() const noexcept;

	void visitDefinition(const peg::ast_node& definition);

	void visitClientDefinition(const peg::ast_node& clientDefinition);
	void visitClientExtension(const peg::ast_node& clientExtension);
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

	static std::string_view getSafeCppName(std::string_view type) noexcept;
	static OutputFieldList getOutputFields(const peg::ast_node::children_t& fields);
	static InputFieldList getInputFields(const peg::ast_node::children_t& fields);

	// Recursively visit a Type node until we reach a NamedType and we've
	// taken stock of all of the modifier wrappers.
	class TypeVisitor
	{
	public:
		std::pair<std::string_view, TypeModifierStack> getType();

		void visit(const peg::ast_node& typeName);

	private:
		void visitNamedType(const peg::ast_node& namedType);
		void visitListType(const peg::ast_node& listType);
		void visitNonNullType(const peg::ast_node& nonNullType);

		std::string_view _type;
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
	void fixupOutputFieldList(OutputFieldList& fields,
		const std::optional<std::unordered_set<std::string_view>>& interfaceFields,
		const std::optional<std::string_view>& accessor);
	void fixupInputFieldList(InputFieldList& fields);

	void validateQuery() const;
	std::shared_ptr<schema::Schema> buildSchema() const;
	void addTypesToSchema(const std::shared_ptr<schema::Schema>& schema) const;

	std::string_view getCppType(std::string_view type) const noexcept;
	std::string getInputCppType(const InputField& field) const noexcept;
	std::string getOutputCppType(const OutputField& field) const noexcept;

	bool outputHeader() const noexcept;
	void outputObjectDeclaration(
		std::ostream& headerFile, const ObjectType& objectType, bool isQueryType) const;
	std::string getFieldDeclaration(const InputField& inputField) const noexcept;
	std::string getFieldDeclaration(const OutputField& outputField) const noexcept;
	std::string getResolverDeclaration(const OutputField& outputField) const noexcept;

	bool outputSource() const noexcept;
	void outputObjectImplementation(
		std::ostream& sourceFile, const ObjectType& objectType, bool isQueryType) const;
	std::string getArgumentDefaultValue(
		size_t level, const response::Value& defaultValue) const noexcept;
	std::string getArgumentDeclaration(const InputField& argument, const char* prefixToken,
		const char* argumentsToken, const char* defaultToken) const noexcept;
	std::string getArgumentAccessType(const InputField& argument) const noexcept;
	std::string getResultAccessType(const OutputField& result) const noexcept;
	std::string getTypeModifiers(const TypeModifierStack& modifiers) const noexcept;

	static std::shared_ptr<const schema::BaseType> getIntrospectionType(
		const std::shared_ptr<schema::Schema>& schema, std::string_view type,
		TypeModifierStack modifiers, bool nonNull = true) noexcept;

	static const BuiltinTypeMap s_builtinTypes;
	static const CppTypeMap s_builtinCppTypes;
	static const std::string_view s_scalarCppType;
	static const std::string s_currentDirectory;

	const GeneratorOptions _options;
	std::string_view _clientNamespace;
	const std::string _headerDir;
	const std::string _sourceDir;
	const std::string _headerPath;
	const std::string _sourcePath;
	peg::ast _schema;
	peg::ast _request;

	SchemaTypeMap _clientTypes;
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

} /* namespace graphql::client */

#endif // CLIENTGENERATOR_H
