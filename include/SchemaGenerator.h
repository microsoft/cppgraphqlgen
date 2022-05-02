// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef SCHEMAGENERATOR_H
#define SCHEMAGENERATOR_H

#include "SchemaLoader.h"

namespace graphql::generator::schema {

struct [[nodiscard]] GeneratorPaths
{
	const std::string headerPath;
	const std::string sourcePath;
};

struct [[nodiscard]] GeneratorOptions
{
	const GeneratorPaths paths;
	const bool verbose = false;
	const bool stubs = false;
	const bool noIntrospection = false;
};

class [[nodiscard]] Generator
{
public:
	// Initialize the generator with the introspection schema or a custom GraphQL schema.
	explicit Generator(SchemaOptions&& schemaOptions, GeneratorOptions&& options);

	// Run the generator and return a list of filenames that were output.
	[[nodiscard]] std::vector<std::string> Build() const noexcept;

private:
	[[nodiscard]] std::string getHeaderDir() const noexcept;
	[[nodiscard]] std::string getSourceDir() const noexcept;
	[[nodiscard]] std::string getHeaderPath() const noexcept;
	[[nodiscard]] std::string getSourcePath() const noexcept;

	[[nodiscard]] bool outputHeader() const noexcept;
	void outputInterfaceDeclaration(std::ostream& headerFile, std::string_view cppType) const;
	void outputObjectImplements(std::ostream& headerFile, const ObjectType& objectType) const;
	void outputObjectStubs(std::ostream& headerFile, const ObjectType& objectType) const;
	void outputObjectDeclaration(
		std::ostream& headerFile, const ObjectType& objectType, bool isQueryType) const;
	[[nodiscard]] std::string getFieldDeclaration(const InputField& inputField) const noexcept;
	[[nodiscard]] std::string getFieldDeclaration(const OutputField& outputField) const noexcept;
	[[nodiscard]] std::string getResolverDeclaration(const OutputField& outputField) const noexcept;

	[[nodiscard]] bool outputSource() const noexcept;
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
	[[nodiscard]] std::string getArgumentDefaultValue(
		size_t level, const response::Value& defaultValue) const noexcept;
	[[nodiscard]] std::string getArgumentDeclaration(const InputField& argument,
		const char* prefixToken, const char* argumentsToken,
		const char* defaultToken) const noexcept;
	[[nodiscard]] std::string getArgumentAccessType(const InputField& argument) const noexcept;
	[[nodiscard]] std::string getResultAccessType(const OutputField& result) const noexcept;
	[[nodiscard]] std::string getTypeModifiers(const TypeModifierStack& modifiers) const noexcept;
	[[nodiscard]] std::string getIntrospectionType(
		std::string_view type, const TypeModifierStack& modifiers) const noexcept;

	[[nodiscard]] std::vector<std::string> outputSeparateFiles() const noexcept;

	static const std::string s_currentDirectory;

	SchemaLoader _loader;
	const GeneratorOptions _options;
	const std::string _headerDir;
	const std::string _sourceDir;
	const std::string _headerPath;
	const std::string _sourcePath;
};

} // namespace graphql::generator::schema

#endif // SCHEMAGENERATOR_H
