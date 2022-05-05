// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef CLIENTGENERATOR_H
#define CLIENTGENERATOR_H

#include "RequestLoader.h"
#include "SchemaLoader.h"

namespace graphql::generator::client {

struct [[nodiscard]] GeneratorPaths
{
	const std::string headerPath;
	const std::string sourcePath;
};

struct [[nodiscard]] GeneratorOptions
{
	const GeneratorPaths paths;
	const bool verbose = false;
};

class [[nodiscard]] Generator
{
public:
	// Initialize the generator with the introspection client or a custom GraphQL client.
	explicit Generator(
		SchemaOptions&& schemaOptions, RequestOptions&& requestOptions, GeneratorOptions&& options);

	// Run the generator and return a list of filenames that were output.
	[[nodiscard]] std::vector<std::string> Build() const noexcept;

private:
	[[nodiscard]] std::string getHeaderDir() const noexcept;
	[[nodiscard]] std::string getSourceDir() const noexcept;
	[[nodiscard]] std::string getHeaderPath() const noexcept;
	[[nodiscard]] std::string getSourcePath() const noexcept;
	[[nodiscard]] const std::string& getClientNamespace() const noexcept;
	[[nodiscard]] const std::string& getOperationNamespace(
		const Operation& operation) const noexcept;
	[[nodiscard]] std::string getResponseFieldCppType(
		const ResponseField& responseField, std::string_view currentScope = {}) const noexcept;

	[[nodiscard]] bool outputHeader() const noexcept;
	void outputRequestComment(std::ostream& headerFile) const noexcept;
	void outputGetRequestDeclaration(std::ostream& headerFile) const noexcept;
	void outputGetOperationNameDeclaration(std::ostream& headerFile) const noexcept;
	[[nodiscard]] bool outputResponseFieldType(std::ostream& headerFile,
		const ResponseField& responseField, size_t indent = 0) const noexcept;

	[[nodiscard]] bool outputSource() const noexcept;
	void outputGetRequestImplementation(std::ostream& sourceFile) const noexcept;
	void outputGetOperationNameImplementation(
		std::ostream& sourceFile, const Operation& operation) const noexcept;
	bool outputModifiedResponseImplementation(std::ostream& sourceFile,
		const std::string& outerScope, const ResponseField& responseField) const noexcept;
	[[nodiscard]] static std::string getTypeModifierList(
		const TypeModifierStack& modifiers) noexcept;

	const SchemaLoader _schemaLoader;
	const RequestLoader _requestLoader;
	const GeneratorOptions _options;
	const std::string _headerDir;
	const std::string _sourceDir;
	const std::string _headerPath;
	const std::string _sourcePath;
};

} // namespace graphql::generator::client

#endif // CLIENTGENERATOR_H
