// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef CLIENTGENERATOR_H
#define CLIENTGENERATOR_H

#include "RequestLoader.h"
#include "SchemaLoader.h"

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
	const std::string& getClientNamespace() const noexcept;

	bool outputHeader() const noexcept;
	void outputGetRequestDeclaration(std::ostream& headerFile) const noexcept;

	bool outputSource() const noexcept;
	void outputGetRequestImplementation(std::ostream& sourceFile) const noexcept;

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
