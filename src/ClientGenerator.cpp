// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ClientGenerator.h"
#include "GeneratorUtil.h"

#include "graphqlservice/internal/Version.h"
#include "graphqlservice/introspection/IntrospectionSchema.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 26495)
#pragma warning(disable : 26812)
#endif // _MSC_VER

#include <boost/program_options.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>

using namespace std::literals;

namespace graphql::generator::client {

Generator::Generator(
	SchemaOptions&& schemaOptions, RequestOptions&& requestOptions, GeneratorOptions&& options)
	: _schemaLoader(std::move(schemaOptions))
	, _requestLoader(std::move(requestOptions), _schemaLoader)
	, _options(std::move(options))
	, _headerDir(getHeaderDir())
	, _sourceDir(getSourceDir())
	, _headerPath(getHeaderPath())
	, _modulePath(getModulePath())
	, _sourcePath(getSourcePath())
{
}

std::string Generator::getHeaderDir() const noexcept
{
	if (!_options.paths.headerPath.empty())
	{
		return std::filesystem::path { _options.paths.headerPath }.string();
	}
	else
	{
		return {};
	}
}

std::string Generator::getSourceDir() const noexcept
{
	if (!_options.paths.sourcePath.empty())
	{
		return std::filesystem::path(_options.paths.sourcePath).string();
	}
	else
	{
		return {};
	}
}

std::string Generator::getHeaderPath() const noexcept
{
	std::filesystem::path fullPath { _headerDir };

	fullPath /= (std::string { _schemaLoader.getFilenamePrefix() } + "Client.h");

	return fullPath.string();
}

std::string Generator::getModulePath() const noexcept
{
	std::filesystem::path fullPath { _headerDir };

	fullPath /= (std::string { _schemaLoader.getFilenamePrefix() } + "Client.ixx");

	return fullPath.string();
}

std::string Generator::getSourcePath() const noexcept
{
	std::filesystem::path fullPath { _sourceDir };

	fullPath /= (std::string { _schemaLoader.getFilenamePrefix() } + "Client.cpp");

	return fullPath.string();
}

const std::string& Generator::getClientNamespace() const noexcept
{
	static const auto s_namespace = R"cpp(client)cpp"s;

	return s_namespace;
}

const std::string& Generator::getOperationNamespace(const Operation& operation) const noexcept
{
	static const auto s_namespaces = [this]() noexcept {
		internal::string_view_map<std::string> result;
		const auto& operations = _requestLoader.getOperations();

		result.reserve(operations.size());

		for (const auto& entry : operations)
		{
			auto value = std::format(R"cpp({}::{})cpp",
				_requestLoader.getOperationType(entry),
				_requestLoader.getOperationNamespace(entry));

			result.emplace(entry.name, std::move(value));
		}

		return result;
	}();

	return s_namespaces.find(operation.name)->second;
}

std::string Generator::getResponseFieldCppType(
	const ResponseField& responseField, std::string_view currentScope /* = {} */) const noexcept
{
	std::string result;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			std::string prefix;

			if (!currentScope.empty())
			{
				prefix = std::format(R"cpp({}::)cpp", currentScope);
			}

			result = SchemaLoader::getSafeCppName(std::format(R"cpp({}{}_{})cpp",
				prefix,
				responseField.cppName,
				responseField.type->name()));
			break;
		}

		default:
			result = _schemaLoader.getCppType(responseField.type->name());
	}

	return result;
}

std::vector<std::string> Generator::Build() const noexcept
{
	std::vector<std::string> builtFiles;

	if (outputHeader() && _options.verbose)
	{
		builtFiles.push_back(_headerPath);
	}

	if (outputModule() && _options.verbose)
	{
		builtFiles.push_back(_modulePath);
	}

	if (outputSource())
	{
		builtFiles.push_back(_sourcePath);
	}

	return builtFiles;
}

bool Generator::outputHeader() const noexcept
{
	std::ofstream headerFile(_headerPath, std::ios_base::trunc);
	IncludeGuardScope includeGuard { headerFile,
		std::filesystem::path(_headerPath).filename().string() };

	headerFile << R"cpp(#include "graphqlservice/GraphQLClient.h"
#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLResponse.h"

#include "graphqlservice/internal/Version.h"
)cpp";

	if (_requestLoader.useSharedTypes())
	{
		headerFile << R"cpp(
#include ")cpp" << _schemaLoader.getFilenamePrefix()
				   << R"cpp(SharedTypes.h"
)cpp";
	}

	headerFile << R"cpp(
#include <optional>
#include <string>
#include <vector>

// Check if the library version is compatible with clientgen )cpp"
			   << graphql::internal::MajorVersion << R"cpp(.)cpp" << graphql::internal::MinorVersion
			   << R"cpp(.0
static_assert(graphql::internal::MajorVersion == )cpp"
			   << graphql::internal::MajorVersion
			   << R"cpp(, "regenerate with clientgen: major version mismatch");
static_assert(graphql::internal::MinorVersion == )cpp"
			   << graphql::internal::MinorVersion
			   << R"cpp(, "regenerate with clientgen: minor version mismatch");

)cpp";

	const auto schemaNamespace = std::format("graphql::{}", _schemaLoader.getSchemaNamespace());
	NamespaceScope schemaNamespaceScope { headerFile, schemaNamespace };

	outputRequestComment(headerFile);

	NamespaceScope clientNamespaceScope { headerFile, getClientNamespace() };
	PendingBlankLine pendingSeparator { headerFile };

	outputGetRequestDeclaration(headerFile);

	const auto& operations = _requestLoader.getOperations();

	if (!_requestLoader.useSharedTypes())
	{
		std::unordered_set<std::string_view> declaredEnum;

		for (const auto& operation : operations)
		{
			// Define all of the enums referenced either in variables or the response.
			for (const auto& enumType : _requestLoader.getReferencedEnums(operation))
			{
				const auto cppType = _schemaLoader.getCppType(enumType->name());

				if (!declaredEnum.insert(cppType).second)
				{
					continue;
				}

				pendingSeparator.reset();

				if (clientNamespaceScope.exit())
				{
					headerFile << std::endl;
				}

				headerFile << R"cpp(enum class [[nodiscard("unnecessary conversion")]] )cpp"
						   << cppType << R"cpp(
{
)cpp";
				for (const auto& enumValue : enumType->enumValues())
				{
					headerFile << R"cpp(	)cpp" << SchemaLoader::getSafeCppName(enumValue->name())
							   << R"cpp(,
)cpp";
				}

				headerFile << R"cpp(};
)cpp";

				pendingSeparator.add();
			}
		}

		std::unordered_set<std::string_view> declaredInput;
		std::unordered_set<std::string_view> forwardDeclaredInput;

		for (const auto& operation : operations)
		{
			// Define all of the input object structs referenced in variables.
			for (const auto& inputType : _requestLoader.getReferencedInputTypes(operation))
			{
				const auto cppType = _schemaLoader.getCppType(inputType.type->name());

				if (!declaredInput.insert(cppType).second)
				{
					continue;
				}

				pendingSeparator.reset();

				if (clientNamespaceScope.exit())
				{
					headerFile << std::endl;
				}

				if (!inputType.declarations.empty())
				{
					// Forward declare nullable dependencies
					for (auto declaration : inputType.declarations)
					{
						if (declaredInput.find(declaration) == declaredInput.end()
							&& forwardDeclaredInput.insert(declaration).second)
						{
							headerFile << R"cpp(struct )cpp" << declaration << R"cpp(;
)cpp";
							pendingSeparator.add();
						}
					}

					pendingSeparator.reset();
				}

				headerFile << R"cpp(struct [[nodiscard("unnecessary construction")]] )cpp"
						   << cppType << R"cpp(
{
	explicit )cpp" << cppType
						   << R"cpp(()cpp";

				bool firstField = true;

				for (const auto& inputField : inputType.type->inputFields())
				{
					if (firstField)
					{
						headerFile << R"cpp() noexcept;
	explicit )cpp" << cppType << R"cpp(()cpp";
					}
					else
					{
						headerFile << R"cpp(,)cpp";
					}

					firstField = false;

					const auto inputCppType =
						_requestLoader.getInputCppType(inputField->type().lock());

					headerFile << R"cpp(
		)cpp" << inputCppType << R"cpp( )cpp"
							   << SchemaLoader::getSafeCppName(inputField->name())
							   << R"cpp(Arg)cpp";
				}

				headerFile << R"cpp() noexcept;
	)cpp" << cppType << R"cpp((const )cpp"
						   << cppType << R"cpp(& other);
	)cpp" << cppType << R"cpp(()cpp"
						   << cppType << R"cpp(&& other) noexcept;
	~)cpp" << cppType << R"cpp(();

	)cpp" << cppType << R"cpp(& operator=(const )cpp"
						   << cppType << R"cpp(& other);
	)cpp" << cppType << R"cpp(& operator=()cpp"
						   << cppType << R"cpp(&& other) noexcept;

)cpp";

				for (const auto& inputField : inputType.type->inputFields())
				{
					headerFile << R"cpp(	)cpp"
							   << _requestLoader.getInputCppType(inputField->type().lock())
							   << R"cpp( )cpp" << SchemaLoader::getSafeCppName(inputField->name())
							   << R"cpp(;
)cpp";
				}

				headerFile << R"cpp(};
)cpp";

				pendingSeparator.add();
			}
		}
	}

	pendingSeparator.reset();
	if (clientNamespaceScope.enter())
	{
		pendingSeparator.add();
	}

	for (const auto& operation : operations)
	{
		pendingSeparator.reset();

		NamespaceScope operationNamespaceScope { headerFile, getOperationNamespace(operation) };

		headerFile << R"cpp(
using graphql::)cpp"
				   << _schemaLoader.getSchemaNamespace() << R"cpp(::)cpp" << getClientNamespace()
				   << R"cpp(::GetRequestText;
using graphql::)cpp"
				   << _schemaLoader.getSchemaNamespace() << R"cpp(::)cpp" << getClientNamespace()
				   << R"cpp(::GetRequestObject;
)cpp";

		outputGetOperationNameDeclaration(headerFile);

		// Alias all of the enums referenced either in variables or the response.
		for (const auto& enumType : _requestLoader.getReferencedEnums(operation))
		{
			headerFile << R"cpp(using graphql::)cpp" << _schemaLoader.getSchemaNamespace()
					   << R"cpp(::)cpp" << _schemaLoader.getCppType(enumType->name()) << R"cpp(;
)cpp";

			pendingSeparator.add();
		}

		pendingSeparator.reset();

		// Alias all of the input object structs referenced in variables.
		for (const auto& inputType : _requestLoader.getReferencedInputTypes(operation))
		{
			headerFile << R"cpp(using graphql::)cpp" << _schemaLoader.getSchemaNamespace()
					   << R"cpp(::)cpp" << _schemaLoader.getCppType(inputType.type->name())
					   << R"cpp(;
)cpp";

			pendingSeparator.add();
		}

		pendingSeparator.reset();

		const auto& variables = _requestLoader.getVariables(operation);

		if (!variables.empty())
		{
			headerFile << R"cpp(struct [[nodiscard("unnecessary construction")]] Variables
{
)cpp";

			for (const auto& variable : variables)
			{
				headerFile << R"cpp(	)cpp"
						   << _requestLoader.getInputCppType(variable.inputType.type,
								  variable.modifiers)
						   << R"cpp( )cpp" << variable.cppName << R"cpp( {};
)cpp";
			}

			headerFile << R"cpp(};

[[nodiscard("unnecessary conversion")]] response::Value serializeVariables(Variables&& variables);
)cpp";

			pendingSeparator.add();
		}

		pendingSeparator.reset();

		const auto& responseType = _requestLoader.getResponseType(operation);

		headerFile << R"cpp(struct [[nodiscard("unnecessary construction")]] Response
{)cpp";

		pendingSeparator.add();

		for (const auto& responseField : responseType.fields)
		{
			pendingSeparator.reset();

			if (outputResponseFieldType(headerFile, responseField, 1))
			{
				pendingSeparator.add();
			}
		}

		for (const auto& responseField : responseType.fields)
		{
			pendingSeparator.reset();

			headerFile << R"cpp(	)cpp"
					   << RequestLoader::getOutputCppType(getResponseFieldCppType(responseField),
							  responseField.modifiers)
					   << R"cpp( )cpp" << responseField.cppName << R"cpp( {};
)cpp";
		}

		headerFile << R"cpp(};

class ResponseVisitor
	: public std::enable_shared_from_this<ResponseVisitor>
{
public:
	ResponseVisitor() noexcept;
	~ResponseVisitor();

	void add_value(std::shared_ptr<const response::Value>&&);
	void reserve(std::size_t count);
	void start_object();
	void add_member(std::string&& key);
	void end_object();
	void start_array();
	void end_array();
	void add_null();
	void add_string(std::string&& value);
	void add_enum(std::string&& value);
	void add_id(response::IdType&& value);
	void add_bool(bool value);
	void add_int(int value);
	void add_float(double value);
	void complete();

	Response response();

private:
	struct impl;

	std::unique_ptr<impl> _pimpl;
};

[[nodiscard("unnecessary conversion")]] Response parseResponse(response::Value&& response);

struct Traits
{
	[[nodiscard("unnecessary call")]] static const std::string& GetRequestText() noexcept;
	[[nodiscard("unnecessary call")]] static const peg::ast& GetRequestObject() noexcept;
	[[nodiscard("unnecessary call")]] static const std::string& GetOperationName() noexcept;
)cpp";

		if (!variables.empty())
		{
			headerFile << R"cpp(
	using Variables = )cpp"
					   << _requestLoader.getOperationNamespace(operation) << R"cpp(::Variables;

	[[nodiscard("unnecessary conversion")]] static response::Value serializeVariables(Variables&& variables);
)cpp";
		}

		headerFile << R"cpp(
	using Response = )cpp"
				   << _requestLoader.getOperationNamespace(operation) << R"cpp(::Response;
	using ResponseVisitor = )cpp"
				   << _requestLoader.getOperationNamespace(operation) << R"cpp(::ResponseVisitor;

	[[nodiscard("unnecessary conversion")]] static Response parseResponse(response::Value&& response);
};

)cpp";

		pendingSeparator.add();
	}

	return true;
}

void Generator::outputRequestComment(std::ostream& headerFile) const noexcept
{
	headerFile << R"cpp(
/// # Operation)cpp";

	const auto& operations = _requestLoader.getOperations();

	if (operations.size() > 1)
	{
		headerFile << R"cpp(s)cpp";
	}

	headerFile << R"cpp(: )cpp";

	bool firstOperation = true;

	for (const auto& operation : operations)
	{
		if (!firstOperation)
		{
			headerFile << R"cpp(, )cpp";
		}

		firstOperation = false;

		headerFile << _requestLoader.getOperationType(operation) << R"cpp( )cpp"
				   << _requestLoader.getOperationDisplayName(operation);
	}

	headerFile << R"cpp(
/// ```graphql
)cpp";

	std::istringstream request { std::string { _requestLoader.getRequestText() } };

	for (std::string line; std::getline(request, line);)
	{
		headerFile << R"cpp(/// )cpp" << line << std::endl;
	}

	headerFile << R"cpp(/// ```
)cpp";
}

void Generator::outputGetRequestDeclaration(std::ostream& headerFile) const noexcept
{
	headerFile << R"cpp(
// Return the original text of the request document.
[[nodiscard("unnecessary call")]] const std::string& GetRequestText() noexcept;

// Return a pre-parsed, pre-validated request object.
[[nodiscard("unnecessary call")]] const peg::ast& GetRequestObject() noexcept;
)cpp";
}

void Generator::outputGetOperationNameDeclaration(std::ostream& headerFile) const noexcept
{
	headerFile << R"cpp(
// Return the name of this operation in the shared request document.
[[nodiscard("unnecessary call")]] const std::string& GetOperationName() noexcept;

)cpp";
}

bool Generator::outputResponseFieldType(std::ostream& headerFile,
	const ResponseField& responseField, std::size_t indent /* = 0 */) const noexcept
{
	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
			// This is a complex type that requires a custom struct declaration.
			break;

		default:
			// This is a scalar type, it doesn't require a type declaration.
			return false;
	}

	const std::string indentTabs(indent, '\t');
	std::unordered_set<std::string_view> fieldNames;
	PendingBlankLine pendingSeparator { headerFile };

	headerFile << indentTabs << R"cpp(struct [[nodiscard("unnecessary construction")]] )cpp"
			   << getResponseFieldCppType(responseField) << R"cpp(
)cpp" << indentTabs
			   << R"cpp({)cpp";

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			for (const auto& field : responseField.children)
			{
				pendingSeparator.reset();

				if (fieldNames.emplace(field.name).second
					&& outputResponseFieldType(headerFile, field, indent + 1))
				{
					pendingSeparator.add();
				}
			}

			fieldNames.clear();

			for (const auto& field : responseField.children)
			{
				pendingSeparator.reset();

				if (fieldNames.emplace(field.name).second)
				{
					headerFile << indentTabs << R"cpp(	)cpp"
							   << RequestLoader::getOutputCppType(getResponseFieldCppType(field),
									  field.modifiers)
							   << R"cpp( )cpp" << field.cppName << R"cpp( {};
)cpp";
				}
			}

			break;
		}

		default:
			break;
	}

	headerFile << indentTabs << R"cpp(};
)cpp";

	return true;
}

bool Generator::outputModule() const noexcept
{
	std::ofstream moduleFile(_modulePath, std::ios_base::trunc);

	moduleFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

module;

#include ")cpp" << _schemaLoader.getFilenamePrefix()
			   <<
		R"cpp(Client.h"

export module GraphQL.)cpp"
			   << _schemaLoader.getFilenamePrefix() << R"cpp(.)cpp"
			   << _schemaLoader.getFilenamePrefix() <<
		R"cpp(Client;

export )cpp";

	const auto schemaNamespace = std::format("graphql::{}", _schemaLoader.getSchemaNamespace());
	NamespaceScope schemaNamespaceScope { moduleFile, schemaNamespace };

	moduleFile << std::endl;

	NamespaceScope clientNamespaceScope { moduleFile, getClientNamespace() };
	PendingBlankLine pendingSeparator { moduleFile };

	moduleFile << R"cpp(
using )cpp" << getClientNamespace()
			   << R"cpp(::GetRequestText;
using )cpp" << getClientNamespace()
			   << R"cpp(::GetRequestObject;
)cpp";

	const auto& operations = _requestLoader.getOperations();
	std::unordered_set<std::string_view> declaredEnum;

	for (const auto& operation : operations)
	{
		if (!_requestLoader.getReferencedEnums(operation).empty())
		{
			pendingSeparator.reset();

			if (clientNamespaceScope.exit())
			{
				moduleFile << std::endl;
			}

			// Define all of the enums referenced either in variables or the response.
			for (const auto& enumType : _requestLoader.getReferencedEnums(operation))
			{
				const auto cppType = _schemaLoader.getCppType(enumType->name());

				if (!declaredEnum.insert(cppType).second)
				{
					continue;
				}

				moduleFile << R"cpp(using )cpp" << _schemaLoader.getSchemaNamespace()
						   << R"cpp(::)cpp" << cppType << R"cpp(;
)cpp";
			}
		}
	}

	if (!declaredEnum.empty())
	{
		pendingSeparator.add();
	}

	std::unordered_set<std::string_view> declaredInput;
	std::unordered_set<std::string_view> forwardDeclaredInput;

	for (const auto& operation : operations)
	{
		if (!_requestLoader.getReferencedInputTypes(operation).empty())
		{
			pendingSeparator.reset();

			if (clientNamespaceScope.exit())
			{
				moduleFile << std::endl;
			}

			// Define all of the input object structs referenced in variables.
			for (const auto& inputType : _requestLoader.getReferencedInputTypes(operation))
			{
				const auto cppType = _schemaLoader.getCppType(inputType.type->name());

				if (!declaredInput.insert(cppType).second)
				{
					continue;
				}

				moduleFile << R"cpp(using )cpp" << _schemaLoader.getSchemaNamespace()
						   << R"cpp(::)cpp" << cppType << R"cpp(;
)cpp";
			}
		}
	}

	if (!declaredInput.empty())
	{
		pendingSeparator.add();
	}

	pendingSeparator.reset();
	if (clientNamespaceScope.enter())
	{
		pendingSeparator.add();
	}

	for (const auto& operation : operations)
	{
		pendingSeparator.reset();

		NamespaceScope operationNamespaceScope { moduleFile, getOperationNamespace(operation) };
		const auto operationNamespace = _requestLoader.getOperationNamespace(operation);

		moduleFile << R"cpp(
using graphql::)cpp"
				   << _schemaLoader.getSchemaNamespace() << R"cpp(::)cpp" << getClientNamespace()
				   << R"cpp(::GetRequestText;
using graphql::)cpp"
				   << _schemaLoader.getSchemaNamespace() << R"cpp(::)cpp" << getClientNamespace()
				   << R"cpp(::GetRequestObject;
using )cpp" << operationNamespace
				   << R"cpp(::GetOperationName;

)cpp";

		// Alias all of the enums referenced either in variables or the response.
		for (const auto& enumType : _requestLoader.getReferencedEnums(operation))
		{
			moduleFile << R"cpp(using graphql::)cpp" << _schemaLoader.getSchemaNamespace()
					   << R"cpp(::)cpp" << _schemaLoader.getCppType(enumType->name()) << R"cpp(;
)cpp";

			pendingSeparator.add();
		}

		pendingSeparator.reset();

		// Alias all of the input object structs referenced in variables.
		for (const auto& inputType : _requestLoader.getReferencedInputTypes(operation))
		{
			moduleFile << R"cpp(using graphql::)cpp" << _schemaLoader.getSchemaNamespace()
					   << R"cpp(::)cpp" << _schemaLoader.getCppType(inputType.type->name())
					   << R"cpp(;
)cpp";

			pendingSeparator.add();
		}

		pendingSeparator.reset();

		const auto& variables = _requestLoader.getVariables(operation);

		if (!variables.empty())
		{
			moduleFile << R"cpp(using )cpp" << operationNamespace << R"cpp(::Variables;
using )cpp" << operationNamespace
					   << R"cpp(::serializeVariables;
)cpp";

			pendingSeparator.add();
		}

		pendingSeparator.reset();

		moduleFile << R"cpp(using )cpp" << operationNamespace << R"cpp(::Response;
using )cpp" << operationNamespace
				   << R"cpp(::ResponseVisitor;
using )cpp" << operationNamespace
				   << R"cpp(::parseResponse;

using )cpp" << operationNamespace
				   << R"cpp(::Traits;

)cpp";

		pendingSeparator.add();
	}

	pendingSeparator.reset();

	return true;
}

bool Generator::outputSource() const noexcept
{
	std::ofstream sourceFile(_sourcePath, std::ios_base::trunc);

	sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include ")cpp" << _schemaLoader.getFilenamePrefix()
			   << R"cpp(Client.h"

#include "graphqlservice/internal/SortedMap.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace std::literals;

)cpp";

	NamespaceScope graphqlNamespaceScope { sourceFile, "graphql" };
	NamespaceScope schemaNamespaceScope { sourceFile, _schemaLoader.getSchemaNamespace() };
	NamespaceScope clientNamespaceScope { sourceFile, getClientNamespace() };
	PendingBlankLine pendingSeparator { sourceFile };

	outputGetRequestImplementation(sourceFile);

	const auto& operations = _requestLoader.getOperations();

	if (!_requestLoader.useSharedTypes())
	{

		std::unordered_set<std::string_view> outputInputMethods;

		for (const auto& operation : operations)
		{
			for (const auto& inputType : _requestLoader.getReferencedInputTypes(operation))
			{
				const auto cppType = _schemaLoader.getCppType(inputType.type->name());

				if (!outputInputMethods.insert(cppType).second)
				{
					continue;
				}

				pendingSeparator.reset();

				if (clientNamespaceScope.exit())
				{
					sourceFile << R"cpp(
using namespace graphql::)cpp" << getClientNamespace()
							   << R"cpp(;

)cpp";
				}

				sourceFile << cppType << R"cpp(::)cpp" << cppType << R"cpp(() noexcept)cpp";

				bool firstField = true;

				for (const auto& inputField : inputType.type->inputFields())
				{
					sourceFile << R"cpp(
	)cpp" << (firstField ? R"cpp(:)cpp" : R"cpp(,)cpp")
							   << R"cpp( )cpp" << SchemaLoader::getSafeCppName(inputField->name())
							   << R"cpp( {})cpp";
					firstField = false;
				}

				sourceFile << R"cpp(
{
	// Explicit definition to prevent ODR violations when LTO is enabled.
}

)cpp" << cppType << R"cpp(::)cpp"
						   << cppType << R"cpp(()cpp";

				firstField = true;

				for (const auto& inputField : inputType.type->inputFields())
				{
					if (!firstField)
					{
						sourceFile << R"cpp(,)cpp";
					}

					firstField = false;
					sourceFile << R"cpp(
		)cpp" << _requestLoader.getInputCppType(inputField->type().lock())
							   << R"cpp( )cpp" << SchemaLoader::getSafeCppName(inputField->name())
							   << R"cpp(Arg)cpp";
				}

				sourceFile << R"cpp() noexcept
)cpp";

				firstField = true;

				for (const auto& inputField : inputType.type->inputFields())
				{
					sourceFile << (firstField ? R"cpp(	: )cpp" : R"cpp(	, )cpp");
					firstField = false;

					const auto name = SchemaLoader::getSafeCppName(inputField->name());

					sourceFile << name << R"cpp( { std::move()cpp" << name << R"cpp(Arg) }
)cpp";
				}

				sourceFile << R"cpp({
}

)cpp" << cppType << R"cpp(::)cpp"
						   << cppType << R"cpp((const )cpp" << cppType << R"cpp(& other)
)cpp";

				firstField = true;

				for (const auto& inputField : inputType.type->inputFields())
				{
					sourceFile << (firstField ? R"cpp(	: )cpp" : R"cpp(	, )cpp");
					firstField = false;

					const auto name = SchemaLoader::getSafeCppName(inputField->name());
					const auto [type, modifiers] =
						RequestLoader::unwrapSchemaType(inputField->type().lock());

					sourceFile << name << R"cpp( { ModifiedVariable<)cpp"
							   << _schemaLoader.getCppType(type->name()) << R"cpp(>::duplicate)cpp"
							   << getTypeModifierList(modifiers) << R"cpp((other.)cpp" << name
							   << R"cpp() }
)cpp";
				}

				sourceFile << R"cpp({
}

)cpp" << cppType << R"cpp(::)cpp"
						   << cppType << R"cpp(()cpp" << cppType << R"cpp(&& other) noexcept
)cpp";

				firstField = true;

				for (const auto& inputField : inputType.type->inputFields())
				{
					sourceFile << (firstField ? R"cpp(	: )cpp" : R"cpp(	, )cpp");
					firstField = false;

					const auto name = SchemaLoader::getSafeCppName(inputField->name());

					sourceFile << name << R"cpp( { std::move(other.)cpp" << name << R"cpp() }
)cpp";
				}

				sourceFile << R"cpp({
}

)cpp" << cppType << R"cpp(::~)cpp"
						   << cppType << R"cpp(()
{
	// Explicit definition to prevent ODR violations when LTO is enabled.
}

)cpp" << cppType << R"cpp(& )cpp"
						   << cppType << R"cpp(::operator=(const )cpp" << cppType << R"cpp(& other)
{
	return *this = )cpp" << cppType
						   << R"cpp( { other };
}

)cpp" << cppType << R"cpp(& )cpp"
						   << cppType << R"cpp(::operator=()cpp" << cppType
						   << R"cpp(&& other) noexcept
{
)cpp";

				for (const auto& inputField : inputType.type->inputFields())
				{
					const auto name = SchemaLoader::getSafeCppName(inputField->name());

					sourceFile << R"cpp(	)cpp" << name << R"cpp( = std::move(other.)cpp" << name
							   << R"cpp();
)cpp";
				}

				sourceFile << R"cpp(
	return *this;
}
)cpp";

				pendingSeparator.add();
			}
		}
	}

	pendingSeparator.reset();
	clientNamespaceScope.exit();
	if (schemaNamespaceScope.exit())
	{
		pendingSeparator.add();
	}

	std::unordered_set<std::string_view> outputModifiedVariableEnum;
	std::unordered_set<std::string_view> outputModifiedVariableInput;
	std::unordered_set<std::string_view> outputModifiedResponseEnum;

	for (const auto& operation : operations)
	{
		const auto& variables = _requestLoader.getVariables(operation);

		if (!variables.empty())
		{
			for (const auto& enumType : _requestLoader.getReferencedEnums(operation))
			{
				const auto cppType = _schemaLoader.getCppType(enumType->name());

				if (!outputModifiedVariableEnum.insert(cppType).second)
				{
					continue;
				}

				pendingSeparator.reset();

				if (clientNamespaceScope.enter())
				{
					sourceFile << R"cpp(
using namespace )cpp" << _schemaLoader.getSchemaNamespace()
							   << R"cpp(;

)cpp";
				}

				const auto& enumValues = enumType->enumValues();

				sourceFile << R"cpp(template <>
response::Value Variable<)cpp"
						   << cppType << R"cpp(>::serialize()cpp" << cppType << R"cpp(&& value)
{
	static const std::array<std::string_view, )cpp"
						   << enumValues.size() << R"cpp(> s_names = {)cpp";

				bool firstValue = true;

				for (const auto& enumValue : enumValues)
				{
					if (!firstValue)
					{
						sourceFile << R"cpp(,)cpp";
					}

					firstValue = false;
					sourceFile << R"cpp(
		R"gql()cpp" << enumValue->name()
							   << R"cpp()gql"sv)cpp";
					pendingSeparator.add();
				}

				pendingSeparator.reset();
				sourceFile << R"cpp(	};

	response::Value result { response::Type::EnumValue };

	result.set<std::string>(std::string { s_names[static_cast<std::size_t>(value)] });

	return result;
}
)cpp";
				pendingSeparator.add();
			}

			for (const auto& inputType : _requestLoader.getReferencedInputTypes(operation))
			{
				const auto cppType = _schemaLoader.getCppType(inputType.type->name());

				if (!outputModifiedVariableInput.insert(cppType).second)
				{
					continue;
				}

				pendingSeparator.reset();

				if (clientNamespaceScope.enter())
				{
					sourceFile << R"cpp(
using namespace )cpp" << _schemaLoader.getSchemaNamespace()
							   << R"cpp(;

)cpp";
				}

				sourceFile << R"cpp(template <>
response::Value Variable<)cpp"
						   << cppType << R"cpp(>::serialize()cpp" << cppType << R"cpp(&& inputValue)
{
	response::Value result { response::Type::Map };

)cpp";
				for (const auto& inputField : inputType.type->inputFields())
				{
					const auto [type, modifiers] =
						RequestLoader::unwrapSchemaType(inputField->type().lock());

					sourceFile << R"cpp(	result.emplace_back(R"js()cpp" << inputField->name()
							   << R"cpp()js"s, ModifiedVariable<)cpp";

					sourceFile << _schemaLoader.getCppType(type->name()) << R"cpp(>::serialize)cpp"
							   << getTypeModifierList(modifiers)
							   << R"cpp((std::move(inputValue.)cpp"
							   << SchemaLoader::getSafeCppName(inputField->name()) << R"cpp()));
)cpp";
				}

				sourceFile << R"cpp(
	return result;
}
)cpp";

				pendingSeparator.add();
			}
		}

		for (const auto& enumType : _requestLoader.getReferencedEnums(operation))
		{
			const auto cppType = _schemaLoader.getCppType(enumType->name());

			if (!outputModifiedResponseEnum.insert(cppType).second)
			{
				continue;
			}

			pendingSeparator.reset();

			if (clientNamespaceScope.enter())
			{
				sourceFile << R"cpp(
using namespace )cpp" << _schemaLoader.getSchemaNamespace()
						   << R"cpp(;

)cpp";
			}

			const auto& enumValues = enumType->enumValues();

			sourceFile << R"cpp(static const std::array<std::pair<std::string_view, )cpp" << cppType
					   << R"cpp(>, )cpp" << enumValues.size() << R"cpp(> s_values)cpp" << cppType
					   << R"cpp( = {)cpp";

			std::vector<std::pair<std::string_view, std::string_view>> sortedValues(
				enumValues.size());

			std::ranges::transform(enumValues,
				sortedValues.begin(),
				[](const auto& value) noexcept {
					return std::make_pair(value->name(),
						SchemaLoader::getSafeCppName(value->name()));
				});
			std::ranges::sort(sortedValues, [](const auto& lhs, const auto& rhs) noexcept {
				return internal::shorter_or_less {}(lhs.first, rhs.first);
			});

			bool firstValue = true;

			for (const auto& enumValue : sortedValues)
			{
				if (!firstValue)
				{
					sourceFile << R"cpp(,)cpp";
				}

				firstValue = false;
				sourceFile << R"cpp(
	std::make_pair(R"gql()cpp"
						   << enumValue.first << R"cpp()gql"sv, )cpp" << cppType << R"cpp(::)cpp"
						   << enumValue.second << R"cpp())cpp";
				pendingSeparator.add();
			}

			pendingSeparator.reset();
			sourceFile << R"cpp(};
			
template <>
)cpp" << cppType << R"cpp( Response<)cpp"
					   << cppType << R"cpp(>::parse(response::Value&& value)
{
	if (!value.maybe_enum())
	{
		throw std::logic_error { R"ex(not a valid )cpp"
					   << enumType->name() << R"cpp( value)ex" };
	}

	const auto result = internal::sorted_map_lookup<internal::shorter_or_less>(
		s_values)cpp" << cppType
					   << R"cpp(,
		std::string_view { value.get<std::string>() });

	if (!result)
	{
		throw std::logic_error { R"ex(not a valid )cpp"
					   << enumType->name() << R"cpp( value)ex" };
	}

	return *result;
}
)cpp";

			pendingSeparator.add();
		}

		const auto operationNamespace = std::format("{}::client::{}",
			_schemaLoader.getSchemaNamespace(),
			getOperationNamespace(operation));
		const auto graphqlCurrentScope =
			std::format(R"cpp(graphql::{}::Response)cpp", operationNamespace);
		const auto& responseType = _requestLoader.getResponseType(operation);

		for (const auto& responseField : responseType.fields)
		{
			if (clientNamespaceScope.enter())
			{
				sourceFile << R"cpp(
using namespace )cpp" << _schemaLoader.getSchemaNamespace()
						   << R"cpp(;
)cpp";
			}

			if (outputModifiedResponseImplementation(sourceFile,
					graphqlCurrentScope,
					responseField))
			{
				pendingSeparator.add();
			}
		}

		pendingSeparator.reset();
		if (clientNamespaceScope.exit())
		{
			sourceFile << std::endl;
		}

		NamespaceScope operationNamespaceScope { sourceFile, operationNamespace };
		const auto schemaCurrentScope =
			std::format("{}::Response", getOperationNamespace(operation));

		outputGetOperationNameImplementation(sourceFile, operation);

		if (!variables.empty())
		{
			sourceFile << R"cpp(
response::Value serializeVariables(Variables&& variables)
{
	using namespace graphql::)cpp"
					   << getClientNamespace() << R"cpp(;

	response::Value result { response::Type::Map };

)cpp";

			for (const auto& variable : variables)
			{
				sourceFile << R"cpp(	result.emplace_back(R"js()cpp" << variable.name
						   << R"cpp()js"s, ModifiedVariable<)cpp"
						   << _schemaLoader.getCppType(variable.inputType.type->name())
						   << R"cpp(>::serialize)cpp" << getTypeModifierList(variable.modifiers)
						   << R"cpp((std::move(variables.)cpp" << variable.cppName << R"cpp()));
)cpp";
			}

			sourceFile << R"cpp(
	return result;
}
)cpp";
		}

		sourceFile << R"cpp(
struct ResponseVisitor::impl
{
	enum class VisitorState
	{
		Start,
)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorStates(sourceFile, responseField);
		}

		sourceFile << R"cpp(		Complete,
	};

	VisitorState state { VisitorState::Start };
	Response response {};
};

ResponseVisitor::ResponseVisitor() noexcept
	: _pimpl { std::make_unique<impl>() }
{
}

ResponseVisitor::~ResponseVisitor()
{
}

void ResponseVisitor::add_value([[maybe_unused]] std::shared_ptr<const response::Value>&& value)
{
	using namespace graphql::client;

	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorAddValue(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::reserve([[maybe_unused]] std::size_t count)
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorReserve(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::start_object()
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorStartObject(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::add_member([[maybe_unused]] std::string&& key)
{
	switch (_pimpl->state)
	{)cpp";

		outputResponseFieldVisitorAddMember(sourceFile, responseType.fields);

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::end_object()
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorEndObject(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::start_array()
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorStartArray(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::end_array()
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorEndArray(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::add_null()
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorAddNull(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::add_string([[maybe_unused]] std::string&& value)
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorAddString(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::add_enum([[maybe_unused]] std::string&& value)
{
	using namespace graphql::client;

	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorAddEnum(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::add_id([[maybe_unused]] response::IdType&& value)
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorAddId(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::add_bool([[maybe_unused]] bool value)
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorAddBool(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::add_int([[maybe_unused]] int value)
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorAddInt(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::add_float([[maybe_unused]] double value)
{
	switch (_pimpl->state)
	{)cpp";

		for (const auto& responseField : responseType.fields)
		{
			outputResponseFieldVisitorAddFloat(sourceFile, responseField);
		}

		sourceFile << R"cpp(
		default:
			break;
	}
}

void ResponseVisitor::complete()
{
	_pimpl->state = impl::VisitorState::Complete;
}

Response ResponseVisitor::response()
{
	Response response {};

	switch (_pimpl->state)
	{
		case impl::VisitorState::Complete:
			_pimpl->state = impl::VisitorState::Start;
			std::swap(_pimpl->response, response);
			break;

		default:
			break;
	}

	return response;
}

Response parseResponse(response::Value&& response)
{
	using namespace graphql::)cpp"
				   << getClientNamespace() << R"cpp(;

	Response result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
)cpp";

		std::unordered_set<std::string_view> fieldNames;

		for (const auto& responseField : responseType.fields)
		{
			if (fieldNames.emplace(responseField.name).second)
			{
				sourceFile << R"cpp(			if (member.first == R"js()cpp" << responseField.name
						   << R"cpp()js"sv)
			{
				result.)cpp"
						   << responseField.cppName << R"cpp( = ModifiedResponse<)cpp"
						   << getResponseFieldCppType(responseField, schemaCurrentScope)
						   << R"cpp(>::parse)cpp" << getTypeModifierList(responseField.modifiers)
						   << R"cpp((std::move(member.second));
				continue;
			}
)cpp";
			}
		}

		sourceFile << R"cpp(		}
	}

	return result;
}

[[nodiscard("unnecessary call")]] const std::string& Traits::GetRequestText() noexcept
{
	return )cpp" << getClientNamespace()
				   << R"cpp(::GetRequestText();
}

[[nodiscard("unnecessary call")]] const peg::ast& Traits::GetRequestObject() noexcept
{
	return )cpp" << getClientNamespace()
				   << R"cpp(::GetRequestObject();
}

[[nodiscard("unnecessary call")]] const std::string& Traits::GetOperationName() noexcept
{
	return )cpp" << _requestLoader.getOperationNamespace(operation)
				   << R"cpp(::GetOperationName();
}
)cpp";

		if (!variables.empty())
		{
			sourceFile << R"cpp(
[[nodiscard("unnecessary conversion")]] response::Value Traits::serializeVariables(Traits::Variables&& variables)
{
	return )cpp" << _requestLoader.getOperationNamespace(operation)
					   << R"cpp(::serializeVariables(std::move(variables));
}
)cpp";
		}

		sourceFile << R"cpp(
[[nodiscard("unnecessary conversion")]] Traits::Response Traits::parseResponse(response::Value&& response)
{
	return )cpp" << _requestLoader.getOperationNamespace(operation)
				   << R"cpp(::parseResponse(std::move(response));
}

)cpp";

		pendingSeparator.add();
	}

	return true;
}

void Generator::outputGetRequestImplementation(std::ostream& sourceFile) const noexcept
{
	sourceFile << R"cpp(
const std::string& GetRequestText() noexcept
{
	static const auto s_request = R"gql(
)cpp";

	std::istringstream request { std::string { _requestLoader.getRequestText() } };

	for (std::string line; std::getline(request, line);)
	{
		sourceFile << R"cpp(		)cpp" << line << std::endl;
	}

	sourceFile << R"cpp(	)gql"s;

	return s_request;
}

const peg::ast& GetRequestObject() noexcept
{
	static const auto s_request = []() noexcept {
		auto ast = peg::parseString(GetRequestText());

		// This has already been validated against the schema by clientgen.
		ast.validated = true;

		return ast;
	}();

	return s_request;
}
)cpp";
}

void Generator::outputGetOperationNameImplementation(
	std::ostream& sourceFile, const Operation& operation) const noexcept
{
	sourceFile << R"cpp(
const std::string& GetOperationName() noexcept
{
	static const auto s_name = R"gql()cpp"
			   << operation.name << R"cpp()gql"s;

	return s_name;
}
)cpp";
}

bool Generator::outputModifiedResponseImplementation(std::ostream& sourceFile,
	const std::string& outerScope, const ResponseField& responseField) const noexcept
{
	const auto cppType =
		std::format(R"cpp({}::{})cpp", outerScope, getResponseFieldCppType(responseField));
	std::unordered_set<std::string_view> fieldNames;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					outputModifiedResponseImplementation(sourceFile, cppType, field);
				}
			}
			break;
		}

		default:
			// This is a scalar type, it doesn't require a type declaration.
			return false;
	}

	fieldNames.clear();

	// This is a complex type that requires a custom ModifiedResponse implementation.
	sourceFile << R"cpp(
template <>
)cpp" << cppType
			   << R"cpp( Response<)cpp" << cppType << R"cpp(>::parse(response::Value&& response)
{
	)cpp" << cppType
			   << R"cpp( result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
)cpp";

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					sourceFile << R"cpp(			if (member.first == R"js()cpp" << field.name
							   << R"cpp()js"sv)
			{
				result.)cpp" << field.cppName
							   << R"cpp( = ModifiedResponse<)cpp"
							   << getResponseFieldCppType(field, cppType) << R"cpp(>::parse)cpp"
							   << getTypeModifierList(field.modifiers)
							   << R"cpp((std::move(member.second));
				continue;
			}
)cpp";
				}
			}
			break;
		}

		default:
			break;
	}

	sourceFile << R"cpp(		}
	}

	return result;
}
)cpp";

	return true;
}

std::string Generator::getTypeModifierList(const TypeModifierStack& modifiers) noexcept
{
	if (modifiers.empty())
	{
		return {};
	}

	bool firstModifier = true;
	std::ostringstream oss;

	oss << '<';

	for (auto modifier : modifiers)
	{
		if (!firstModifier)
		{
			oss << R"cpp(, )cpp";
		}

		firstModifier = false;

		switch (modifier)
		{
			case service::TypeModifier::None:
				oss << R"cpp(TypeModifier::None)cpp";
				break;

			case service::TypeModifier::Nullable:
				oss << R"cpp(TypeModifier::Nullable)cpp";
				break;

			case service::TypeModifier::List:
				oss << R"cpp(TypeModifier::List)cpp";
				break;
		}
	}

	oss << '>';

	return oss.str();
}

void Generator::outputResponseFieldVisitorStates(std::ostream& sourceFile,
	const ResponseField& responseField, std::string_view parent /* = {} */) const noexcept
{
	auto state =
		std::format("{}_{}", parent.empty() ? R"cpp(Member)cpp"sv : parent, responseField.cppName);

	sourceFile << R"cpp(		)cpp" << state << R"cpp(,
)cpp";

	std::size_t arrayDimensions = 0;

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
			case service::TypeModifier::Nullable:
				break;

			case service::TypeModifier::List:
				state = std::format("{}_{}", state, arrayDimensions++);
				sourceFile << R"cpp(		)cpp" << state << R"cpp(,
)cpp";
				break;
		}
	}

	if (arrayDimensions > 0)
	{
		sourceFile << R"cpp(		)cpp" << state << R"cpp(_,
)cpp";
	}

	std::unordered_set<std::string_view> fieldNames;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					outputResponseFieldVisitorStates(sourceFile, field, state);
				}
			}

			break;
		}

		default:
			break;
	}
}

void Generator::outputResponseFieldVisitorAddValue(std::ostream& sourceFile,
	const ResponseField& responseField, bool arrayElement /* = false */,
	std::string_view parentState /* = {} */, std::string_view parentAccessor /* = {} */,
	std::string_view parentCppType /* = {} */) const noexcept
{
	auto state = std::format("{}_{}",
		parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
		responseField.cppName);
	auto accessor = std::format("{}{}", parentAccessor, responseField.cppName);
	auto cppType = getResponseFieldCppType(responseField,
		parentCppType.empty() ? R"cpp(Response)cpp"sv : parentCppType);

	bool isNullable = false;
	std::size_t arrayDimensions = 0;
	std::optional<std::size_t> lastNullableDimension {};

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
				break;

			case service::TypeModifier::Nullable:
				isNullable = true;
				break;

			case service::TypeModifier::List:
				if (isNullable)
				{
					lastNullableDimension = arrayDimensions;
					isNullable = false;
				}

				state = std::format("{}_{}", state, arrayDimensions++);
				break;
		}
	}

	sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
			   << state << R"cpp(:)cpp";

	if (arrayDimensions == 0)
	{
		sourceFile << R"cpp(
			_pimpl->state = impl::VisitorState::)cpp"
				   << (parentState.empty() ? "Start"sv : parentState);

		if (arrayElement)
		{
			sourceFile << R"cpp(_)cpp";
		}

		sourceFile << R"cpp(;)cpp";
	}

	sourceFile << R"cpp(
			_pimpl->response.)cpp"
			   << accessor;

	if (arrayDimensions > 0)
	{
		sourceFile << ((lastNullableDimension && *lastNullableDimension + 1 == arrayDimensions)
				? R"cpp(->)cpp"
				: R"cpp(.)cpp")
				   << R"cpp(push_back()cpp";
	}
	else
	{
		sourceFile << R"cpp( = )cpp";
	}

	sourceFile << R"cpp(ModifiedResponse<)cpp" << cppType << R"cpp(>::parse)cpp";

	if (arrayDimensions > 0)
	{
		TypeModifierStack skippedModifiers;

		skippedModifiers.reserve(responseField.modifiers.size() - arrayDimensions);
		std::ranges::copy(std::views::all(responseField.modifiers) | std::views::reverse
				| std::views::take_while([](auto modifier) noexcept {
					  return modifier != service::TypeModifier::List;
				  })
				| std::views::reverse,
			std::back_inserter(skippedModifiers));
		sourceFile << getTypeModifierList(skippedModifiers);
	}
	else
	{
		sourceFile << getTypeModifierList(responseField.modifiers);
	}

	sourceFile << R"cpp((response::Value { *value }))cpp";

	if (arrayDimensions > 0)
	{
		sourceFile << R"cpp())cpp";
	}

	sourceFile << R"cpp(;
			break;
)cpp";

	std::unordered_set<std::string_view> fieldNames;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			bool dereference = true;

			for (auto modifier : responseField.modifiers)
			{
				switch (modifier)
				{
					case service::TypeModifier::None:
						break;

					case service::TypeModifier::Nullable:
						accessor.append(R"cpp(->)cpp");
						dereference = false;
						break;

					case service::TypeModifier::List:
						if (dereference)
						{
							accessor.append(R"cpp(.)cpp");
						}

						accessor.append(R"cpp(back())cpp");
						dereference = true;
						break;
				}
			}

			if (dereference)
			{
				accessor.append(R"cpp(.)cpp");
			}

			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					outputResponseFieldVisitorAddValue(sourceFile,
						field,
						arrayDimensions > 0,
						state,
						accessor,
						cppType);
				}
			}

			break;
		}

		default:
			break;
	}
}

void Generator::outputResponseFieldVisitorReserve(std::ostream& sourceFile,
	const ResponseField& responseField, std::string_view parentState /* = {} */,
	std::string_view parentAccessor /* = {} */,
	std::string_view parentCppType /* = {} */) const noexcept
{
	auto state = std::format("{}_{}",
		parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
		responseField.cppName);
	auto accessor = std::format("{}{}", parentAccessor, responseField.cppName);
	auto cppType = getResponseFieldCppType(responseField,
		parentCppType.empty() ? R"cpp(Response)cpp"sv : parentCppType);

	std::size_t arrayDimensions = 0;
	bool dereference = true;

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
				break;

			case service::TypeModifier::Nullable:
				accessor.append(R"cpp(->)cpp");
				dereference = false;
				break;

			case service::TypeModifier::List:
				if (dereference)
				{
					accessor.append(R"cpp(.)cpp");
				}
				state = std::format("{}_{}", state, arrayDimensions++);

				sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
						   << state << R"cpp(:
			_pimpl->response.)cpp"
						   << accessor << R"cpp(reserve(count);
			break;
)cpp";

				accessor.append(R"cpp(back())cpp");
				dereference = true;
				break;
		}
	}

	std::unordered_set<std::string_view> fieldNames;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			dereference = true;

			for (auto modifier : responseField.modifiers)
			{
				switch (modifier)
				{
					case service::TypeModifier::None:
						break;

					case service::TypeModifier::Nullable:
						accessor.append(R"cpp(->)cpp");
						dereference = false;
						break;

					case service::TypeModifier::List:
						if (dereference)
						{
							accessor.append(R"cpp(.)cpp");
						}

						accessor.append(R"cpp(back())cpp");
						dereference = true;
						break;
				}
			}

			if (dereference)
			{
				accessor.append(R"cpp(.)cpp");
			}

			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					outputResponseFieldVisitorReserve(sourceFile, field, state, accessor, cppType);
				}
			}

			break;
		}

		default:
			break;
	}
}

void Generator::outputResponseFieldVisitorStartObject(std::ostream& sourceFile,
	const ResponseField& responseField, std::string_view parentState /* = {} */,
	std::string_view parentAccessor /* = {} */,
	std::string_view parentCppType /* = {} */) const noexcept
{
	auto state = std::format("{}_{}",
		parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
		responseField.cppName);
	auto accessor = std::format("{}{}", parentAccessor, responseField.cppName);
	auto cppType = getResponseFieldCppType(responseField,
		parentCppType.empty() ? R"cpp(Response)cpp"sv : parentCppType);

	bool isNullable = false;
	std::size_t arrayDimensions = 0;
	std::optional<std::size_t> lastNullableDimension {};

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
				break;

			case service::TypeModifier::Nullable:
				isNullable = true;
				break;

			case service::TypeModifier::List:
				if (isNullable)
				{
					lastNullableDimension = arrayDimensions;
					isNullable = false;
				}

				state = std::format("{}_{}", state, arrayDimensions++);
				break;
		}
	}

	if (isNullable && arrayDimensions == 0)
	{
		switch (responseField.type->kind())
		{
			case introspection::TypeKind::OBJECT:
			case introspection::TypeKind::INTERFACE:
			case introspection::TypeKind::UNION:
				break;

			default:
				isNullable = false;
				break;
		}
	}

	if (isNullable || arrayDimensions > 0)
	{
		sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
				   << state << R"cpp(:)cpp";

		if (arrayDimensions > 0)
		{
			sourceFile << R"cpp(
			_pimpl->state = impl::VisitorState::)cpp"
					   << state << R"cpp(_;)cpp";
		}

		sourceFile << R"cpp(
			_pimpl->response.)cpp"
				   << accessor;

		if (arrayDimensions > 0)
		{
			sourceFile << ((lastNullableDimension && *lastNullableDimension + 1 == arrayDimensions)
					? R"cpp(->)cpp"
					: R"cpp(.)cpp")
					   << R"cpp(push_back()cpp";
		}
		else
		{
			sourceFile << R"cpp( = )cpp";
		}

		if (isNullable)
		{
			sourceFile << R"cpp(std::make_optional<)cpp" << cppType << R"cpp(>({}))cpp";
		}
		else
		{
			sourceFile << R"cpp({})cpp";
		}

		if (arrayDimensions > 0)
		{
			sourceFile << R"cpp())cpp";
		}

		sourceFile << R"cpp(;
			break;
)cpp";
	}

	std::unordered_set<std::string_view> fieldNames;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			bool dereference = true;

			for (auto modifier : responseField.modifiers)
			{
				switch (modifier)
				{
					case service::TypeModifier::None:
						break;

					case service::TypeModifier::Nullable:
						accessor.append(R"cpp(->)cpp");
						dereference = false;
						break;

					case service::TypeModifier::List:
						if (dereference)
						{
							accessor.append(R"cpp(.)cpp");
						}

						accessor.append(R"cpp(back())cpp");
						dereference = true;
						break;
				}
			}

			if (dereference)
			{
				accessor.append(R"cpp(.)cpp");
			}

			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					outputResponseFieldVisitorStartObject(sourceFile,
						field,
						state,
						accessor,
						cppType);
				}
			}

			break;
		}

		default:
			break;
	}
}

void Generator::outputResponseFieldVisitorAddMember(std::ostream& sourceFile,
	const ResponseFieldList& children, bool arrayElement /* = false */,
	std::string_view parentState /* = {} */) const noexcept
{
	sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
			   << (parentState.empty() ? R"cpp(Start)cpp"sv : parentState);

	if (arrayElement)
	{
		sourceFile << R"cpp(_)cpp";
	}

	sourceFile << R"cpp(:
			)cpp";

	std::unordered_set<std::string_view> fieldNames;
	bool firstField = true;

	for (const auto& field : children)
	{
		if (!fieldNames.emplace(field.name).second)
		{
			continue;
		}

		auto state = std::format("{}_{}",
			parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
			field.cppName);

		if (!firstField)
		{
			sourceFile << R"cpp(else )cpp";
		}

		firstField = false;

		sourceFile << R"cpp(if (key == ")cpp" << field.name << R"cpp("sv)
			{
				_pimpl->state = impl::VisitorState::)cpp"
				   << state << R"cpp(;
			}
			)cpp";
	}

	sourceFile << R"cpp(break;
)cpp";

	fieldNames.clear();

	for (const auto& field : children)
	{
		switch (field.type->kind())
		{
			case introspection::TypeKind::OBJECT:
			case introspection::TypeKind::INTERFACE:
			case introspection::TypeKind::UNION:
				break;

			default:
				continue;
		}

		if (!fieldNames.emplace(field.name).second)
		{
			continue;
		}

		auto state = std::format("{}_{}",
			parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
			field.cppName);
		std::size_t arrayDimensions = 0;

		for (auto modifier : field.modifiers)
		{
			switch (modifier)
			{
				case service::TypeModifier::None:
				case service::TypeModifier::Nullable:
					break;

				case service::TypeModifier::List:
					state = std::format("{}_{}", state, arrayDimensions++);
					break;
			}
		}

		outputResponseFieldVisitorAddMember(sourceFile, field.children, arrayDimensions > 0, state);
	}
}

void Generator::outputResponseFieldVisitorEndObject(std::ostream& sourceFile,
	const ResponseField& responseField, bool arrayElement /* = false */,
	std::string_view parentState /* = {} */) const noexcept
{
	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
			break;

		default:
			return;
	}

	auto state = std::format("{}_{}",
		parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
		responseField.cppName);

	std::size_t arrayDimensions = 0;
	std::string arrayState;

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
			case service::TypeModifier::Nullable:
				break;

			case service::TypeModifier::List:
				state = std::format("{}_{}", state, arrayDimensions++);
				break;
		}
	}

	std::unordered_set<std::string_view> fieldNames;

	for (const auto& field : responseField.children)
	{
		if (fieldNames.emplace(field.name).second)
		{
			outputResponseFieldVisitorEndObject(sourceFile, field, arrayDimensions > 0, state);
		}
	}

	if (arrayDimensions > 0)
	{
		sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
				   << state << R"cpp(_:
			_pimpl->state = impl::VisitorState::)cpp"
				   << state;
	}
	else
	{
		sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
				   << state << R"cpp(:
			_pimpl->state = impl::VisitorState::)cpp"
				   << (parentState.empty() ? "Start"sv : parentState);

		if (arrayElement)
		{
			sourceFile << R"cpp(_)cpp";
		}
	}

	sourceFile << R"cpp(;
			break;
)cpp";
}

void Generator::outputResponseFieldVisitorStartArray(std::ostream& sourceFile,
	const ResponseField& responseField, std::string_view parentState /* = {} */,
	std::string_view parentAccessor /* = {} */,
	std::string_view parentCppType /* = {} */) const noexcept
{
	auto state = std::format("{}_{}",
		parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
		responseField.cppName);
	auto accessor = std::format("{}{}", parentAccessor, responseField.cppName);
	auto cppType = getResponseFieldCppType(responseField,
		parentCppType.empty() ? R"cpp(Response)cpp"sv : parentCppType);

	bool dereference = true;
	std::size_t arrayDimensions = 0;
	std::size_t skipModifiers = 0;

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
				break;

			case service::TypeModifier::Nullable:
				dereference = false;
				break;

			case service::TypeModifier::List:
				sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
						   << state << R"cpp(:)cpp";

				state = std::format("{}_{}", state, arrayDimensions++);

				sourceFile << R"cpp(
			_pimpl->state = impl::VisitorState::)cpp"
						   << state << R"cpp(;)cpp";

				if (!dereference)
				{
					sourceFile << R"cpp(
			_pimpl->response.)cpp"
							   << accessor;

					if (arrayDimensions > 1)
					{
						sourceFile << R"cpp(push_back()cpp";
					}
					else
					{
						sourceFile << R"cpp( = )cpp";
					}

					TypeModifierStack skippedModifiers;

					skippedModifiers.reserve(responseField.modifiers.size() - skipModifiers);
					std::ranges::copy(
						std::views::all(responseField.modifiers) | std::views::drop(skipModifiers),
						std::back_inserter(skippedModifiers));

					sourceFile << R"cpp(std::make_optional<)cpp"
							   << RequestLoader::getOutputCppType(cppType, skippedModifiers)
							   << R"cpp(>({}))cpp";

					if (arrayDimensions > 1)
					{
						sourceFile << R"cpp();)cpp";
					}
					else
					{
						sourceFile << R"cpp(;)cpp";
					}
				}

				sourceFile << R"cpp(
			break;
)cpp";

				if (dereference)
				{
					accessor.append(R"cpp(.)cpp");
				}
				else
				{
					accessor.append(R"cpp(->)cpp");
				}

				accessor.append(R"cpp(back())cpp");
				dereference = true;
				break;
		}

		++skipModifiers;
	}

	if (dereference)
	{
		accessor.append(R"cpp(.)cpp");
	}

	std::unordered_set<std::string_view> fieldNames;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					outputResponseFieldVisitorStartArray(sourceFile,
						field,
						state,
						accessor,
						cppType);
				}
			}

			break;
		}

		default:
			break;
	}
}

void Generator::outputResponseFieldVisitorEndArray(std::ostream& sourceFile,
	const ResponseField& responseField, bool arrayElement /* = false */,
	std::string_view parentState /* = {} */) const noexcept
{
	auto state = std::format("{}_{}",
		parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
		responseField.cppName);

	std::size_t arrayDimensions = 0;

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
			case service::TypeModifier::Nullable:
				break;

			case service::TypeModifier::List:
			{
				auto child = std::format("{}_{}", state, arrayDimensions++);
				std::string_view parent { state };

				if (arrayDimensions == 1)
				{
					parent = parentState.empty() ? "Start"sv : parentState;
				}

				sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
						   << child << R"cpp(:
			_pimpl->state = impl::VisitorState::)cpp"
						   << parent;

				if (arrayElement)
				{
					sourceFile << R"cpp(_)cpp";
				}

				sourceFile << R"cpp(;
			break;
)cpp";

				state = std::move(child);
				break;
			}
		}
	}

	std::unordered_set<std::string_view> fieldNames;

	for (const auto& field : responseField.children)
	{
		if (fieldNames.emplace(field.name).second)
		{
			outputResponseFieldVisitorEndArray(sourceFile, field, arrayDimensions > 0, state);
		}
	}
}

void Generator::outputResponseFieldVisitorAddNull(std::ostream& sourceFile,
	const ResponseField& responseField, bool arrayElement /* = false */,
	std::string_view parentState /* = {} */,
	std::string_view parentAccessor /* = {} */) const noexcept
{
	auto state = std::format("{}_{}",
		parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
		responseField.cppName);
	auto accessor = std::format("{}{}", parentAccessor, responseField.cppName);

	bool isNullable = false;
	std::size_t arrayDimensions = 0;
	std::optional<std::size_t> lastNullableDimension {};

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
				break;

			case service::TypeModifier::Nullable:
				isNullable = true;
				break;

			case service::TypeModifier::List:
				if (isNullable)
				{
					lastNullableDimension = arrayDimensions;
					isNullable = false;
				}

				state = std::format("{}_{}", state, arrayDimensions++);
				break;
		}
	}

	if (isNullable)
	{
		sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
				   << state << R"cpp(:)cpp";

		if (arrayDimensions == 0)
		{
			sourceFile << R"cpp(
			_pimpl->state = impl::VisitorState::)cpp"
					   << (parentState.empty() ? "Start"sv : parentState);

			if (arrayElement)
			{
				sourceFile << R"cpp(_)cpp";
			}

			sourceFile << R"cpp(;)cpp";
		}

		sourceFile << R"cpp(
			_pimpl->response.)cpp"
				   << accessor;

		if (arrayDimensions > 0)
		{
			sourceFile << ((lastNullableDimension && *lastNullableDimension + 1 == arrayDimensions)
					? R"cpp(->)cpp"
					: R"cpp(.)cpp")
					   << R"cpp(push_back()cpp";
		}
		else
		{
			sourceFile << R"cpp( = )cpp";
		}

		sourceFile << R"cpp(std::nullopt)cpp";

		if (arrayDimensions > 0)
		{
			sourceFile << R"cpp())cpp";
		}

		sourceFile << R"cpp(;
			break;
)cpp";
	}

	std::unordered_set<std::string_view> fieldNames;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			bool dereference = true;

			for (auto modifier : responseField.modifiers)
			{
				switch (modifier)
				{
					case service::TypeModifier::None:
						break;

					case service::TypeModifier::Nullable:
						accessor.append(R"cpp(->)cpp");
						dereference = false;
						break;

					case service::TypeModifier::List:
						if (dereference)
						{
							accessor.append(R"cpp(.)cpp");
						}

						accessor.append(R"cpp(back())cpp");
						dereference = true;
						break;
				}
			}

			if (dereference)
			{
				accessor.append(R"cpp(.)cpp");
			}

			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					outputResponseFieldVisitorAddNull(sourceFile,
						field,
						arrayDimensions > 0,
						state,
						accessor);
				}
			}

			break;
		}

		default:
			break;
	}
}

void Generator::outputResponseFieldVisitorAddMovedValue(std::ostream& sourceFile,
	const ResponseField& responseField, std::string_view movedCppType,
	bool arrayElement /* = false */, std::string_view parentState /* = {} */,
	std::string_view parentAccessor /* = {} */) const noexcept
{
	auto state = std::format("{}_{}",
		parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
		responseField.cppName);
	auto accessor = std::format("{}{}", parentAccessor, responseField.cppName);

	bool isNullable = false;
	std::size_t arrayDimensions = 0;
	std::optional<std::size_t> lastNullableDimension {};

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
				break;

			case service::TypeModifier::Nullable:
				isNullable = true;
				break;

			case service::TypeModifier::List:
				if (isNullable)
				{
					lastNullableDimension = arrayDimensions;
					isNullable = false;
				}

				state = std::format("{}_{}", state, arrayDimensions++);
				break;
		}
	}

	if (getResponseFieldCppType(responseField) == movedCppType)
	{
		sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
				   << state << R"cpp(:)cpp";

		if (arrayDimensions == 0)
		{
			sourceFile << R"cpp(
			_pimpl->state = impl::VisitorState::)cpp"
					   << (parentState.empty() ? "Start"sv : parentState);

			if (arrayElement)
			{
				sourceFile << R"cpp(_)cpp";
			}

			sourceFile << R"cpp(;)cpp";
		}

		sourceFile << R"cpp(
			_pimpl->response.)cpp"
				   << accessor;

		if (arrayDimensions > 0)
		{
			sourceFile << ((lastNullableDimension && *lastNullableDimension + 1 == arrayDimensions)
					? R"cpp(->)cpp"
					: R"cpp(.)cpp")
					   << R"cpp(push_back()cpp";
		}
		else
		{
			sourceFile << R"cpp( = )cpp";
		}

		sourceFile << R"cpp(std::move(value))cpp";

		if (arrayDimensions > 0)
		{
			sourceFile << R"cpp())cpp";
		}

		sourceFile << R"cpp(;
			break;
)cpp";
	}

	std::unordered_set<std::string_view> fieldNames;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			bool dereference = true;

			for (auto modifier : responseField.modifiers)
			{
				switch (modifier)
				{
					case service::TypeModifier::None:
						break;

					case service::TypeModifier::Nullable:
						accessor.append(R"cpp(->)cpp");
						dereference = false;
						break;

					case service::TypeModifier::List:
						if (dereference)
						{
							accessor.append(R"cpp(.)cpp");
						}

						accessor.append(R"cpp(back())cpp");
						dereference = true;
						break;
				}
			}

			if (dereference)
			{
				accessor.append(R"cpp(.)cpp");
			}

			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					outputResponseFieldVisitorAddMovedValue(sourceFile,
						field,
						movedCppType,
						arrayDimensions > 0,
						state,
						accessor);
				}
			}

			break;
		}

		default:
			break;
	}
}

void Generator::outputResponseFieldVisitorAddString(
	std::ostream& sourceFile, const ResponseField& responseField) const noexcept
{
	outputResponseFieldVisitorAddMovedValue(sourceFile, responseField, R"cpp(std::string)cpp"sv);
}

void Generator::outputResponseFieldVisitorAddEnum(std::ostream& sourceFile,
	const ResponseField& responseField, bool arrayElement /* = false */,
	std::string_view parentState /* = {} */, std::string_view parentAccessor /* = {} */,
	std::string_view parentCppType /* = {} */) const noexcept
{
	auto state = std::format("{}_{}",
		parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
		responseField.cppName);
	auto accessor = std::format("{}{}", parentAccessor, responseField.cppName);
	auto cppType = getResponseFieldCppType(responseField,
		parentCppType.empty() ? R"cpp(Response)cpp"sv : parentCppType);

	bool isNullable = false;
	std::size_t arrayDimensions = 0;
	std::optional<std::size_t> lastNullableDimension {};

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
				break;

			case service::TypeModifier::Nullable:
				isNullable = true;
				break;

			case service::TypeModifier::List:
				if (isNullable)
				{
					lastNullableDimension = arrayDimensions;
					isNullable = false;
				}

				state = std::format("{}_{}", state, arrayDimensions++);
				break;
		}
	}

	if (responseField.type->kind() == introspection::TypeKind::ENUM)
	{
		sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
				   << state << R"cpp(:)cpp";

		if (arrayDimensions == 0)
		{
			sourceFile << R"cpp(
			_pimpl->state = impl::VisitorState::)cpp"
					   << (parentState.empty() ? "Start"sv : parentState);

			if (arrayElement)
			{
				sourceFile << R"cpp(_)cpp";
			}

			sourceFile << R"cpp(;)cpp";
		}

		sourceFile << R"cpp(
			if (const auto enumValue = internal::sorted_map_lookup<internal::shorter_or_less>(s_values)cpp"
				   << cppType << R"cpp(, std::string_view { value }))
			{
				_pimpl->response.)cpp"
				   << accessor;

		if (arrayDimensions > 0)
		{
			sourceFile << ((lastNullableDimension && *lastNullableDimension + 1 == arrayDimensions)
					? R"cpp(->)cpp"
					: R"cpp(.)cpp")
					   << R"cpp(push_back()cpp";
		}
		else
		{
			sourceFile << R"cpp( = )cpp";
		}

		sourceFile << R"cpp(*enumValue)cpp";

		if (arrayDimensions > 0)
		{
			sourceFile << R"cpp())cpp";
		}

		sourceFile << R"cpp(;
			}
			break;
)cpp";
	}

	std::unordered_set<std::string_view> fieldNames;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			bool dereference = true;

			for (auto modifier : responseField.modifiers)
			{
				switch (modifier)
				{
					case service::TypeModifier::None:
						break;

					case service::TypeModifier::Nullable:
						accessor.append(R"cpp(->)cpp");
						dereference = false;
						break;

					case service::TypeModifier::List:
						if (dereference)
						{
							accessor.append(R"cpp(.)cpp");
						}

						accessor.append(R"cpp(back())cpp");
						dereference = true;
						break;
				}
			}

			if (dereference)
			{
				accessor.append(R"cpp(.)cpp");
			}

			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					outputResponseFieldVisitorAddEnum(sourceFile,
						field,
						arrayDimensions > 0,
						state,
						accessor,
						cppType);
				}
			}

			break;
		}

		default:
			break;
	}
}

void Generator::outputResponseFieldVisitorAddId(
	std::ostream& sourceFile, const ResponseField& responseField) const noexcept
{
	outputResponseFieldVisitorAddMovedValue(sourceFile,
		responseField,
		R"cpp(response::IdType)cpp"sv);
}

void Generator::outputResponseFieldVisitorAddCopiedValue(std::ostream& sourceFile,
	const ResponseField& responseField, std::string_view copiedCppType,
	bool arrayElement /* = false */, std::string_view parentState /* = {} */,
	std::string_view parentAccessor /* = {} */) const noexcept
{
	auto state = std::format("{}_{}",
		parentState.empty() ? R"cpp(Member)cpp"sv : parentState,
		responseField.cppName);
	auto accessor = std::format("{}{}", parentAccessor, responseField.cppName);

	bool isNullable = false;
	std::size_t arrayDimensions = 0;
	std::optional<std::size_t> lastNullableDimension {};

	for (auto modifier : responseField.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::None:
				break;

			case service::TypeModifier::Nullable:
				isNullable = true;
				break;

			case service::TypeModifier::List:
				if (isNullable)
				{
					lastNullableDimension = arrayDimensions;
					isNullable = false;
				}

				state = std::format("{}_{}", state, arrayDimensions++);
				break;
		}
	}

	if (getResponseFieldCppType(responseField) == copiedCppType)
	{
		sourceFile << R"cpp(
		case impl::VisitorState::)cpp"
				   << state << R"cpp(:)cpp";

		if (arrayDimensions == 0)
		{
			sourceFile << R"cpp(
			_pimpl->state = impl::VisitorState::)cpp"
					   << (parentState.empty() ? "Start"sv : parentState);

			if (arrayElement)
			{
				sourceFile << R"cpp(_)cpp";
			}

			sourceFile << R"cpp(;)cpp";
		}

		sourceFile << R"cpp(
			_pimpl->response.)cpp"
				   << accessor;

		if (arrayDimensions > 0)
		{
			sourceFile << ((lastNullableDimension && *lastNullableDimension + 1 == arrayDimensions)
					? R"cpp(->)cpp"
					: R"cpp(.)cpp")
					   << R"cpp(push_back()cpp";
		}
		else
		{
			sourceFile << R"cpp( = )cpp";
		}

		sourceFile << R"cpp(value)cpp";

		if (arrayDimensions > 0)
		{
			sourceFile << R"cpp())cpp";
		}

		sourceFile << R"cpp(;
			break;
)cpp";
	}

	std::unordered_set<std::string_view> fieldNames;

	switch (responseField.type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			bool dereference = true;

			for (auto modifier : responseField.modifiers)
			{
				switch (modifier)
				{
					case service::TypeModifier::None:
						break;

					case service::TypeModifier::Nullable:
						accessor.append(R"cpp(->)cpp");
						dereference = false;
						break;

					case service::TypeModifier::List:
						if (dereference)
						{
							accessor.append(R"cpp(.)cpp");
						}

						accessor.append(R"cpp(back())cpp");
						dereference = true;
						break;
				}
			}

			if (dereference)
			{
				accessor.append(R"cpp(.)cpp");
			}

			for (const auto& field : responseField.children)
			{
				if (fieldNames.emplace(field.name).second)
				{
					outputResponseFieldVisitorAddCopiedValue(sourceFile,
						field,
						copiedCppType,
						arrayDimensions > 0,
						state,
						accessor);
				}
			}

			break;
		}

		default:
			break;
	}
}

void Generator::outputResponseFieldVisitorAddBool(
	std::ostream& sourceFile, const ResponseField& responseField) const noexcept
{
	outputResponseFieldVisitorAddCopiedValue(sourceFile, responseField, R"cpp(bool)cpp"sv);
}

void Generator::outputResponseFieldVisitorAddInt(
	std::ostream& sourceFile, const ResponseField& responseField) const noexcept
{
	outputResponseFieldVisitorAddCopiedValue(sourceFile, responseField, R"cpp(int)cpp"sv);
}

void Generator::outputResponseFieldVisitorAddFloat(
	std::ostream& sourceFile, const ResponseField& responseField) const noexcept
{
	outputResponseFieldVisitorAddCopiedValue(sourceFile, responseField, R"cpp(double)cpp"sv);
}

} // namespace graphql::generator::client

namespace po = boost::program_options;

void outputVersion(std::ostream& ostm)
{
	ostm << graphql::internal::FullVersion << std::endl;
}

void outputUsage(std::ostream& ostm, const po::options_description& options)
{
	ostm << "Usage:\tclientgen [options] <schema file> <request file> <output filename prefix> "
			"<output namespace>"
		 << std::endl;
	ostm << options;
}

int main(int argc, char** argv)
{
	po::options_description options("Command line options");
	po::positional_options_description positional;
	po::variables_map variables;
	bool showUsage = false;
	bool showVersion = false;
	bool buildCustom = false;
	bool verbose = false;
	bool noIntrospection = false;
	bool sharedTypes = false;
	std::string schemaFileName;
	std::string requestFileName;
	std::string operationName;
	std::string filenamePrefix;
	std::string schemaNamespace;
	std::string sourceDir;
	std::string headerDir;

	options.add_options()("version", po::bool_switch(&showVersion), "Print the version number")(
		"help,?",
		po::bool_switch(&showUsage),
		"Print the command line options")("verbose,v",
		po::bool_switch(&verbose),
		"Verbose output including generated header names as well as sources")("schema,s",
		po::value(&schemaFileName),
		"Schema definition file path")("request,r",
		po::value(&requestFileName),
		"Request document file path")("operation,o",
		po::value(&operationName),
		"Operation name if the request document contains more than one")("prefix,p",
		po::value(&filenamePrefix),
		"Prefix to use for the generated C++ filenames")("namespace,n",
		po::value(&schemaNamespace),
		"C++ sub-namespace for the generated types")("source-dir",
		po::value(&sourceDir),
		"Target path for the <prefix>Client.cpp source file")("header-dir",
		po::value(&headerDir),
		"Target path for the <prefix>Client.h header file")("no-introspection",
		po::bool_switch(&noIntrospection),
		"Do not expect support for Introspection")("shared-types",
		po::bool_switch(&sharedTypes),
		"Re-use shared types from <prefix>SharedTypes.h");
	positional.add("schema", 1).add("request", 1).add("prefix", 1).add("namespace", 1);

	try
	{
		po::store(po::command_line_parser(argc, argv).options(options).positional(positional).run(),
			variables);
		po::notify(variables);

		// If you specify any of these parameters, you must specify all four.
		buildCustom = !schemaFileName.empty() || !requestFileName.empty() || !filenamePrefix.empty()
			|| !schemaNamespace.empty();

		if (buildCustom)
		{
			if (schemaFileName.empty())
			{
				throw po::required_option("schema");
			}
			else if (requestFileName.empty())
			{
				throw po::required_option("request");
			}
			else if (filenamePrefix.empty())
			{
				throw po::required_option("prefix");
			}
			else if (schemaNamespace.empty())
			{
				throw po::required_option("namespace");
			}
		}
	}
	catch (const po::error& oe)
	{
		std::cerr << "Command line errror: " << oe.what() << std::endl;
		outputUsage(std::cerr, options);
		return 1;
	}

	if (showVersion)
	{
		outputVersion(std::cout);
		return 0;
	}
	else if (showUsage || !buildCustom)
	{
		outputUsage(std::cout, options);
		return 0;
	}

	try
	{
		const auto files = graphql::generator::client::Generator(
			graphql::generator::SchemaOptions { std::move(schemaFileName),
				std::move(filenamePrefix),
				std::move(schemaNamespace) },
			graphql::generator::RequestOptions {
				std::move(requestFileName),
				{ operationName.empty() ? std::nullopt
										: std::make_optional(std::move(operationName)) },
				noIntrospection,
				sharedTypes,
			},
			graphql::generator::client::GeneratorOptions {
				{ std::move(headerDir), std::move(sourceDir) },
				verbose,
			})
							   .Build();

		for (const auto& file : files)
		{
			std::cout << file << std::endl;
		}
	}
	catch (const graphql::peg::parse_error& pe)
	{
		std::cerr << "Invalid GraphQL: " << pe.what() << std::endl;

		for (const auto& position : pe.positions())
		{
			std::cerr << "\tline: " << position.line << " column: " << position.column << std::endl;
		}

		return 1;
	}
	catch (graphql::service::schema_exception& scx)
	{
		auto errors = scx.getStructuredErrors();

		std::cerr << "Invalid Request:" << std::endl;

		for (const auto& error : errors)
		{
			std::cerr << "\tmessage: " << error.message << ", line: " << error.location.line
					  << ", column: " << error.location.column << std::endl;

			if (!error.path.empty())
			{
				bool addSeparator = false;

				std::cerr << "\tpath: ";

				for (const auto& segment : error.path)
				{
					if (std::holds_alternative<std::size_t>(segment))
					{
						std::cerr << '[' << std::get<std::size_t>(segment) << ']';
					}
					else
					{
						if (addSeparator)
						{
							std::cerr << '.';
						}

						std::cerr << std::get<std::string_view>(segment);
					}

					addSeparator = true;
				}

				std::cerr << std::endl;
			}
		}

		return 1;
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
