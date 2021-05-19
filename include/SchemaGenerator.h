// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef SCHEMAGENERATOR_H
#define SCHEMAGENERATOR_H

#include "SchemaLoader.h"

namespace graphql::generator::schema {

struct GeneratorPaths
{
	const std::string headerPath;
	const std::string sourcePath;
};

struct GeneratorOptions
{
	const std::optional<GeneratorPaths> paths;
	const bool verbose = false;
	const bool separateFiles = false;
	const bool noStubs = false;
	const bool noIntrospection = false;
};

class Generator
{
public:
	// Initialize the generator with the introspection schema or a custom GraphQL schema.
	explicit Generator(std::optional<SchemaOptions>&& customSchema, GeneratorOptions&& options);

	// Run the generator and return a list of filenames that were output.
	std::vector<std::string> Build() const noexcept;

private:
	std::string getHeaderDir() const noexcept;
	std::string getSourceDir() const noexcept;
	std::string getHeaderPath() const noexcept;
	std::string getObjectHeaderPath() const noexcept;
	std::string getSourcePath() const noexcept;

	bool outputHeader() const noexcept;
	void outputObjectDeclaration(
		std::ostream& headerFile, const ObjectType& objectType, bool isQueryType) const;
	std::string getFieldDeclaration(const InputField& inputField) const noexcept;
	std::string getFieldDeclaration(const OutputField& outputField) const noexcept;
	std::string getResolverDeclaration(const OutputField& outputField) const noexcept;

	bool outputSource() const noexcept;
	void outputObjectImplementation(
		std::ostream& sourceFile, const ObjectType& objectType, bool isQueryType) const;
	void outputObjectIntrospection(std::ostream& sourceFile, const ObjectType& objectType) const;
	std::string getArgumentDefaultValue(
		size_t level, const response::Value& defaultValue) const noexcept;
	std::string getArgumentDeclaration(const InputField& argument, const char* prefixToken,
		const char* argumentsToken, const char* defaultToken) const noexcept;
	std::string getArgumentAccessType(const InputField& argument) const noexcept;
	std::string getResultAccessType(const OutputField& result) const noexcept;
	std::string getTypeModifiers(const TypeModifierStack& modifiers) const noexcept;
	std::string getIntrospectionType(
		std::string_view type, const TypeModifierStack& modifiers) const noexcept;

	std::vector<std::string> outputSeparateFiles() const noexcept;

	static const std::string s_currentDirectory;

	SchemaLoader _loader;
	const GeneratorOptions _options;
	const std::string _headerDir;
	const std::string _sourceDir;
	const std::string _headerPath;
	const std::string _objectHeaderPath;
	const std::string _sourcePath;
};

} /* namespace graphql::generator::schema */

#endif // SCHEMAGENERATOR_H
