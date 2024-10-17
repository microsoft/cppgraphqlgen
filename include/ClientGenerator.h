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
	[[nodiscard("unnecessary memory copy")]] std::string getModulePath() const noexcept;
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

	[[nodiscard("unnecessary call")]] bool outputModule() const noexcept;

	[[nodiscard("unnecessary call")]] bool outputSource() const noexcept;
	void outputGetRequestImplementation(std::ostream& sourceFile) const noexcept;
	void outputGetOperationNameImplementation(
		std::ostream& sourceFile, const Operation& operation) const noexcept;
	bool outputModifiedResponseImplementation(std::ostream& sourceFile,
		const std::string& outerScope, const ResponseField& responseField) const noexcept;
	[[nodiscard("unnecessary memory copy")]] static std::string getTypeModifierList(
		const TypeModifierStack& modifiers) noexcept;
	void outputResponseFieldVisitorStates(std::ostream& sourceFile,
		const ResponseField& responseField, std::string_view parent = {}) const noexcept;
	void outputResponseFieldVisitorAddValue(std::ostream& sourceFile,
		const ResponseField& responseField, bool arrayElement = false,
		std::string_view parentState = {}, std::string_view parentAccessor = {},
		std::string_view parentCppType = {}) const noexcept;
	void outputResponseFieldVisitorReserve(std::ostream& sourceFile,
		const ResponseField& responseField, std::string_view parentState = {},
		std::string_view parentAccessor = {}, std::string_view parentCppType = {}) const noexcept;
	void outputResponseFieldVisitorStartObject(std::ostream& sourceFile,
		const ResponseField& responseField, std::string_view parentState = {},
		std::string_view parentAccessor = {}, std::string_view parentCppType = {}) const noexcept;
	void outputResponseFieldVisitorAddMember(std::ostream& sourceFile,
		const ResponseFieldList& children, bool arrayElement = false,
		std::string_view parentState = {}) const noexcept;
	void outputResponseFieldVisitorEndObject(std::ostream& sourceFile,
		const ResponseField& responseField, bool arrayElement = false,
		std::string_view parentState = {}) const noexcept;
	void outputResponseFieldVisitorStartArray(std::ostream& sourceFile,
		const ResponseField& responseField, std::string_view parentState = {},
		std::string_view parentAccessor = {}, std::string_view parentCppType = {}) const noexcept;
	void outputResponseFieldVisitorEndArray(std::ostream& sourceFilearrayElement,
		const ResponseField& responseField, bool arrayElement = false,
		std::string_view parentState = {}) const noexcept;
	void outputResponseFieldVisitorAddNull(std::ostream& sourceFilearrayElement,
		const ResponseField& responseField, bool arrayElement = false,
		std::string_view parentState = {}, std::string_view parentAccessor = {}) const noexcept;
	void outputResponseFieldVisitorAddMovedValue(std::ostream& sourceFile,
		const ResponseField& responseField, std::string_view movedCppType,
		bool arrayElement = false, std::string_view parentState = {},
		std::string_view parentAccessor = {}) const noexcept;
	void outputResponseFieldVisitorAddString(
		std::ostream& sourceFile, const ResponseField& responseField) const noexcept;
	void outputResponseFieldVisitorAddEnum(std::ostream& sourceFile,
		const ResponseField& responseField, bool arrayElement = false,
		std::string_view parentState = {}, std::string_view parentAccessor = {},
		std::string_view parentCppType = {}) const noexcept;
	void outputResponseFieldVisitorAddId(
		std::ostream& sourceFile, const ResponseField& responseField) const noexcept;
	void outputResponseFieldVisitorAddCopiedValue(std::ostream& sourceFile,
		const ResponseField& responseField, std::string_view copiedCppType,
		bool arrayElement = false, std::string_view parentState = {},
		std::string_view parentAccessor = {}) const noexcept;
	void outputResponseFieldVisitorAddBool(
		std::ostream& sourceFile, const ResponseField& responseField) const noexcept;
	void outputResponseFieldVisitorAddInt(
		std::ostream& sourceFile, const ResponseField& responseField) const noexcept;
	void outputResponseFieldVisitorAddFloat(
		std::ostream& sourceFile, const ResponseField& responseField) const noexcept;

	const SchemaLoader _schemaLoader;
	const RequestLoader _requestLoader;
	const GeneratorOptions _options;
	const std::string _headerDir;
	const std::string _sourceDir;
	const std::string _headerPath;
	const std::string _modulePath;
	const std::string _sourcePath;
};

} // namespace graphql::generator::client

#endif // CLIENTGENERATOR_H
