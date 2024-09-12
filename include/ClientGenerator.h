// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef CLIENTGENERATOR_H
#define CLIENTGENERATOR_H

#include "RequestLoader.h"
#include "SchemaLoader.h"

#include <cstddef>

namespace graphql::generator::client {

struct [[nodiscard("unnecessary construction")]] GeneratorPaths
{
	const std::string headerPath;
	const std::string sourcePath;
};

struct [[nodiscard("unnecessary construction")]] GeneratorOptions
{
	const GeneratorPaths paths;
	const bool verbose = false;
};

class [[nodiscard("unnecessary construction")]] Generator
{
public:
	// Initialize the generator with the introspection client or a custom GraphQL client.
	explicit Generator(
		SchemaOptions&& schemaOptions, RequestOptions&& requestOptions, GeneratorOptions&& options);

	// Run the generator and return a list of filenames that were output.
	[[nodiscard("unnecessary memory copy")]] std::vector<std::string> Build() const noexcept;

private:
	[[nodiscard("unnecessary memory copy")]] std::string getHeaderDir() const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getSourceDir() const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getHeaderPath() const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getSourcePath() const noexcept;
	[[nodiscard("unnecessary call")]] const std::string& getClientNamespace() const noexcept;
	[[nodiscard("unnecessary call")]] const std::string& getOperationNamespace(
		const Operation& operation) const noexcept;
	[[nodiscard("unnecessary memory copy")]] std::string getResponseFieldCppType(
		const ResponseField& responseField, std::string_view currentScope = {}) const noexcept;

	[[nodiscard("unnecessary call")]] bool outputHeader() const noexcept;
	void outputRequestComment(std::ostream& headerFile) const noexcept;
	void outputGetRequestDeclaration(std::ostream& headerFile) const noexcept;
	void outputGetOperationNameDeclaration(std::ostream& headerFile) const noexcept;
	[[nodiscard("unnecessary call")]] bool outputResponseFieldType(std::ostream& headerFile,
		const ResponseField& responseField, std::size_t indent = 0) const noexcept;

	[[nodiscard("unnecessary call")]] bool outputSource() const noexcept;
	void outputGetRequestImplementation(std::ostream& sourceFile) const noexcept;
	void outputGetOperationNameImplementation(
		std::ostream& sourceFile, const Operation& operation) const noexcept;
	bool outputModifiedResponseImplementation(std::ostream& sourceFile,
		const std::string& outerScope, const ResponseField& responseField) const noexcept;
	[[nodiscard("unnecessary memory copy")]] static std::string getTypeModifierList(
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
