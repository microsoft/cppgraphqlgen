// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef SCHEMAGENERATOR_H
#define SCHEMAGENERATOR_H

#include "SchemaLoader.h"

#include <cstddef>

namespace graphql::generator::schema {

struct [[nodiscard("unnecessary construction")]] GeneratorPaths
{
	const std::string headerPath;
	const std::string sourcePath;
};

struct [[nodiscard("unnecessary construction")]] GeneratorOptions
{
	const GeneratorPaths paths;
	const bool verbose = false;
	const bool stubs = false;
	const bool noIntrospection = false;
};

class [[nodiscard("unnecessary construction")]] Generator
{
public:
	// Initialize the generator with the introspection schema or a custom GraphQL schema.
	explicit Generator(SchemaOptions&& schemaOptions, GeneratorOptions&& options);

	// Run the generator and return a list of filenames that were output.
	[[nodiscard("unnecessary construction")]] std::vector<std::string> Build() const noexcept;

private:
	[[nodiscard("unnecessary memory copy")]] std::string getHeaderDir() const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getSourceDir() const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getSchemaHeaderPath() const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getSchemaModulePath() const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getSchemaSourcePath() const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getSharedTypesHeaderPath() const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getSharedTypesModulePath() const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getSharedTypesSourcePath() const noexcept;

	[[nodiscard("unnecessary call")]] bool outputSchemaHeader() const noexcept;
	[[nodiscard("unnecessary call")]] bool outputSchemaModule() const noexcept;
	[[nodiscard("unnecessary call")]] bool outputSharedTypesHeader() const noexcept;
	[[nodiscard("unnecessary call")]] bool outputSharedTypesModule() const noexcept;
	void outputInterfaceDeclaration(std::ostream& headerFile, std::string_view cppType) const;
	void outputObjectModule(
		std::ostream& moduleFile, std::string_view objectNamespace, std::string_view cppType) const;
	void outputObjectImplements(std::ostream& headerFile, const ObjectType& objectType) const;
	void outputObjectStubs(std::ostream& headerFile, const ObjectType& objectType) const;
	void outputObjectDeclaration(std::ostream& headerFile, const ObjectType& objectType,
		bool isQueryType, bool isSubscriptionType) const;
	[[nodiscard("unnecessary memory copy")]] std::string getFieldDeclaration(
		const InputField& inputField) const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getFieldDeclaration(
		const OutputField& outputField) const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getResolverDeclaration(
		const OutputField& outputField) const noexcept;

	[[nodiscard("unnecessary call")]] bool outputSchemaSource() const noexcept;
	[[nodiscard("unnecessary call")]] bool outputSharedTypesSource() const noexcept;
	void outputInterfaceImplementation(std::ostream& sourceFile, std::string_view cppType) const;
	void outputInterfaceIntrospection(
		std::ostream& sourceFile, const InterfaceType& interfaceType) const;
	void outputUnionIntrospection(std::ostream& sourceFile, const UnionType& unionType) const;
	void outputObjectImplementation(
		std::ostream& sourceFile, const ObjectType& objectType, bool isQueryType) const;
	void outputObjectIntrospection(std::ostream& sourceFile, const ObjectType& objectType) const;
	void outputIntrospectionInterfaces(std::ostream& sourceFile, std::string_view cppType,
		const std::vector<std::string_view>& interfaces) const;
	void outputIntrospectionFields(
		std::ostream& sourceFile, std::string_view cppType, const OutputFieldList& fields) const;
	[[nodiscard("unnecessary memory copy")]] std::string getArgumentDefaultValue(
		std::size_t level, const response::Value& defaultValue) const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getArgumentDeclaration(
		const InputField& argument, const char* prefixToken, const char* argumentsToken,
		const char* defaultToken) const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getArgumentAccessType(
		const InputField& argument) const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getResultAccessType(
		const OutputField& result) const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getTypeModifiers(
		const TypeModifierStack& modifiers) const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getIntrospectionType(
		std::string_view type, const TypeModifierStack& modifiers) const noexcept;

	[[nodiscard("unnecessary memory copy")]] std::vector<std::string> outputSeparateFiles()
		const noexcept;

	static const std::string s_currentDirectory;

	SchemaLoader _loader;
	const GeneratorOptions _options;
	const std::string _headerDir;
	const std::string _moduleDir;
	const std::string _sourceDir;
	const std::string _schemaHeaderPath;
	const std::string _schemaModulePath;
	const std::string _schemaSourcePath;
	const std::string _sharedTypesHeaderPath;
	const std::string _sharedTypesModulePath;
	const std::string _sharedTypesSourcePath;
};

} // namespace graphql::generator::schema

#endif // SCHEMAGENERATOR_H
