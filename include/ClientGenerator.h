// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef CLIENTGENERATOR_H
#define CLIENTGENERATOR_H

#include "SchemaLoader.h"

#include "graphqlservice/GraphQLSchema.h"

namespace graphql::generator::client {

struct GeneratorPaths
{
	const std::string headerPath;
	const std::string sourcePath;
};

struct GeneratorOptions
{
	const std::string requestFilename;
	const std::optional<GeneratorPaths> paths;
	const bool verbose = false;
	const bool noIntrospection = false;
};

class Generator
{
public:
	// Initialize the generator with the introspection client or a custom GraphQL client.
	explicit Generator(SchemaOptions&& schemaOptions, GeneratorOptions&& options);

	// Run the generator and return a list of filenames that were output.
	std::vector<std::string> Build() const noexcept;

private:
	std::string getHeaderDir() const noexcept;
	std::string getSourceDir() const noexcept;
	std::string getHeaderPath() const noexcept;
	std::string getSourcePath() const noexcept;

	void validateRequest() const;
	std::shared_ptr<schema::Schema> buildSchema() const;
	void addTypesToSchema(const std::shared_ptr<schema::Schema>& schema) const;

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

	SchemaLoader _loader;
	const GeneratorOptions _options;
	const std::string _headerDir;
	const std::string _sourceDir;
	const std::string _headerPath;
	const std::string _sourcePath;
	peg::ast _request;
};

} /* namespace graphql::generator::client */

#endif // CLIENTGENERATOR_H
