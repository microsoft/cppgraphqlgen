// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef CLIENTGENERATOR_H
#define CLIENTGENERATOR_H

#include "RequestLoader.h"

namespace graphql::generator::client {

struct GeneratorPaths
{
	const std::string headerPath;
	const std::string sourcePath;
};

struct GeneratorOptions
{
	const std::optional<GeneratorPaths> paths;
	const bool verbose = false;
};

class Generator
{
public:
	// Initialize the generator with the introspection client or a custom GraphQL client.
	explicit Generator(
		SchemaOptions&& schemaOptions, RequestOptions&& requestOptions, GeneratorOptions&& options);

	// Run the generator and return a list of filenames that were output.
	std::vector<std::string> Build() const noexcept;

private:
	std::string getHeaderDir() const noexcept;
	std::string getSourceDir() const noexcept;
	std::string getHeaderPath() const noexcept;
	std::string getSourcePath() const noexcept;

	bool outputHeader() const noexcept;
	void outputRequestComment(std::ostream& headerFile) const noexcept;
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

	const SchemaLoader _schemaLoader;
	const RequestLoader _requestLoader;
	const GeneratorOptions _options;
	const std::string _headerDir;
	const std::string _sourceDir;
	const std::string _headerPath;
	const std::string _sourcePath;
};

} /* namespace graphql::generator::client */

#endif // CLIENTGENERATOR_H
