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

// clang-format off
#ifdef USE_STD_FILESYSTEM
	#include <filesystem>
	namespace fs = std::filesystem;
#else
	#ifdef USE_STD_EXPERIMENTAL_FILESYSTEM
		#include <experimental/filesystem>
		namespace fs = std::experimental::filesystem;
	#else
		#ifdef USE_BOOST_FILESYSTEM
			#include <boost/filesystem.hpp>
			namespace fs = boost::filesystem;
		#else
			#error "No std::filesystem implementation defined"
		#endif
	#endif
#endif
// clang-format on

#include <cctype>
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
	, _sourcePath(getSourcePath())
{
}

std::string Generator::getHeaderDir() const noexcept
{
	if (!_options.paths.headerPath.empty())
	{
		return fs::path { _options.paths.headerPath }.string();
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
		return fs::path(_options.paths.sourcePath).string();
	}
	else
	{
		return {};
	}
}

std::string Generator::getHeaderPath() const noexcept
{
	fs::path fullPath { _headerDir };

	fullPath /= (std::string { _schemaLoader.getFilenamePrefix() } + "Client.h");

	return fullPath.string();
}

std::string Generator::getSourcePath() const noexcept
{
	fs::path fullPath { _sourceDir };

	fullPath /= (std::string { _schemaLoader.getFilenamePrefix() } + "Client.cpp");

	return fullPath.string();
}

const std::string& Generator::getClientNamespace() const noexcept
{
	static const auto s_namespace = R"cpp(graphql::client)cpp"s;

	return s_namespace;
}

const std::string& Generator::getRequestNamespace() const noexcept
{
	static const auto s_namespace = [this]() noexcept {
		std::ostringstream oss;

		oss << _requestLoader.getOperationType() << R"cpp(::)cpp"
			<< _requestLoader.getOperationNamespace();

		return oss.str();
	}();

	return s_namespace;
}

const std::string& Generator::getFullNamespace() const noexcept
{
	static const auto s_namespace = [this]() noexcept {
		std::ostringstream oss;

		oss << getClientNamespace() << R"cpp(::)cpp" << getRequestNamespace();

		return oss.str();
	}();

	return s_namespace;
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
			std::ostringstream oss;

			if (!currentScope.empty())
			{
				oss << currentScope << R"cpp(::)cpp";
			}

			oss << responseField.cppName << R"cpp(_)cpp" << responseField.type->name();
			result = SchemaLoader::getSafeCppName(oss.str());
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

	if (outputSource())
	{
		builtFiles.push_back(_sourcePath);
	}

	return builtFiles;
}

bool Generator::outputHeader() const noexcept
{
	std::ofstream headerFile(_headerPath, std::ios_base::trunc);
	IncludeGuardScope includeGuard { headerFile, fs::path(_headerPath).filename().string() };

	headerFile << R"cpp(#include "graphqlservice/GraphQLClient.h"
#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLResponse.h"

#include "graphqlservice/internal/Version.h"

// Check if the library version is compatible with clientgen )cpp"
			   << graphql::internal::MajorVersion << R"cpp(.)cpp" << graphql::internal::MinorVersion
			   << R"cpp(.0
static_assert(graphql::internal::MajorVersion == )cpp"
			   << graphql::internal::MajorVersion
			   << R"cpp(, "regenerate with clientgen: major version mismatch");
static_assert(graphql::internal::MinorVersion == )cpp"
			   << graphql::internal::MinorVersion
			   << R"cpp(, "regenerate with clientgen: minor version mismatch");

#include <optional>
#include <string>
#include <vector>
)cpp";

	outputRequestComment(headerFile);

	NamespaceScope fullNamespaceScope { headerFile, getFullNamespace() };
	PendingBlankLine pendingSeparator { headerFile };

	outputGetRequestDeclaration(headerFile);

	// Define all of the enums referenced either in variables or the response.
	for (const auto& enumType : _requestLoader.getReferencedEnums())
	{
		pendingSeparator.reset();

		headerFile << R"cpp(enum class )cpp" << _schemaLoader.getCppType(enumType->name()) << R"cpp(
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

	pendingSeparator.reset();

	const auto& variables = _requestLoader.getVariables();

	if (!variables.empty())
	{
		headerFile << R"cpp(struct Variables
{
)cpp";

		// Define all of the input object structs referenced in variables.
		for (const auto& inputType : _requestLoader.getReferencedInputTypes())
		{
			pendingSeparator.reset();

			headerFile << R"cpp(	struct )cpp" << _schemaLoader.getCppType(inputType->name())
					   << R"cpp(
	{
)cpp";

			for (const auto& inputField : inputType->inputFields())
			{

				headerFile << R"cpp(		)cpp"
						   << _requestLoader.getInputCppType(inputField->type().lock())
						   << R"cpp( )cpp" << SchemaLoader::getSafeCppName(inputField->name())
						   << R"cpp( {};
)cpp";
			}

			headerFile << R"cpp(	};
)cpp";

			pendingSeparator.add();
		}

		pendingSeparator.reset();

		for (const auto& variable : variables)
		{
			headerFile << R"cpp(	)cpp" << _schemaLoader.getCppType(variable.type->name())
					   << R"cpp( )cpp" << variable.cppName << R"cpp( {};
)cpp";
		}

		headerFile << R"cpp(};

response::Value serializeVariables(Variables&& variables);
)cpp";

		pendingSeparator.add();
	}

	pendingSeparator.reset();

	const auto& responseType = _requestLoader.getResponseType();

	headerFile << R"cpp(struct Response
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

Response parseResponse(response::Value response);
)cpp";

	pendingSeparator.add();
	pendingSeparator.reset();

	return true;
}

void Generator::outputRequestComment(std::ostream& headerFile) const noexcept
{
	headerFile << R"cpp(
/// <summary>
/// Operation: )cpp"
			   << _requestLoader.getOperationType() << ' '
			   << _requestLoader.getOperationDisplayName() << R"cpp(
/// </summary>
/// <code class="language-graphql">
)cpp";

	std::istringstream request { std::string { _requestLoader.getRequestText() } };

	for (std::string line; std::getline(request, line);)
	{
		headerFile << R"cpp(/// )cpp" << line << std::endl;
	}

	headerFile << R"cpp(/// </code>
)cpp";
}

void Generator::outputGetRequestDeclaration(std::ostream& headerFile) const noexcept
{
	headerFile << R"cpp(
// Return the original text of the request document.
const std::string& GetRequestText() noexcept;

// Return a pre-parsed, pre-validated request object.
const peg::ast& GetRequestObject() noexcept;
)cpp";
}

bool Generator::outputResponseFieldType(std::ostream& headerFile,
	const ResponseField& responseField, size_t indent /* = 0 */) const noexcept
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

	headerFile << indentTabs << R"cpp(struct )cpp" << getResponseFieldCppType(responseField)
			   << R"cpp(
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

bool Generator::outputSource() const noexcept
{
	std::ofstream sourceFile(_sourcePath, std::ios_base::trunc);

	sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include ")cpp" << _schemaLoader.getFilenamePrefix()
			   << R"cpp(Client.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <string_view>

using namespace std::literals;

)cpp";

	NamespaceScope clientNamespaceScope { sourceFile, getClientNamespace() };
	PendingBlankLine pendingSeparator { sourceFile };

	sourceFile << R"cpp(
using namespace )cpp"
			   << getRequestNamespace() << R"cpp(;
)cpp";

	const auto& variables = _requestLoader.getVariables();

	for (const auto& enumType : _requestLoader.getReferencedEnums())
	{
		pendingSeparator.reset();

		const auto& enumValues = enumType->enumValues();
		const auto cppType = _schemaLoader.getCppType(enumType->name());
		bool firstValue = true;

		sourceFile << R"cpp(static const std::array<std::string_view, )cpp" << enumValues.size()
				   << R"cpp(> s_names)cpp" << cppType << R"cpp( = {
)cpp";

		for (const auto& enumValue : enumValues)
		{
			firstValue = false;
			sourceFile << R"cpp(	")cpp" << SchemaLoader::getSafeCppName(enumValue->name())
					   << R"cpp("sv,
)cpp";
		}

		sourceFile << R"cpp(};
)cpp";

		pendingSeparator.add();
	}

	if (!variables.empty())
	{
		for (const auto& enumType : _requestLoader.getReferencedEnums())
		{
			pendingSeparator.reset();

			const auto& enumValues = enumType->enumValues();
			const auto cppType = _schemaLoader.getCppType(enumType->name());

			if (!variables.empty())
			{
				sourceFile << R"cpp(template <>
response::Value ModifiedVariable<)cpp"
						   << cppType << R"cpp(>::serialize()cpp" << cppType << R"cpp(&& value)
{
	response::Value result { response::Type::EnumValue };

	result.set<std::string>(std::string { s_names)cpp"
						   << cppType << R"cpp([static_cast<size_t>(value)] });

	return result;
}
)cpp";
			}

			pendingSeparator.add();
		}

		for (const auto& inputType : _requestLoader.getReferencedInputTypes())
		{
			pendingSeparator.reset();

			const auto cppType = _schemaLoader.getCppType(inputType->name());

			sourceFile << R"cpp(template <>
response::Value ModifiedVariable<Variables::)cpp"
					   << cppType << R"cpp(>::serialize(Variables::)cpp" << cppType
					   << R"cpp(&& inputValue)
{
	response::Value result { response::Type::Map };

)cpp";
			for (const auto& inputField : inputType->inputFields())
			{
				const auto [type, modifiers] =
					RequestLoader::unwrapSchemaType(inputField->type().lock());

				sourceFile << R"cpp(	result.emplace_back(R"js()cpp" << inputField->name()
						   << R"cpp()js"s, ModifiedVariable<)cpp"
						   << _schemaLoader.getCppType(type->name()) << R"cpp(>::serialize)cpp"
						   << getTypeModifierList(modifiers) << R"cpp((std::move(inputValue.)cpp"
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

	for (const auto& enumType : _requestLoader.getReferencedEnums())
	{
		pendingSeparator.reset();

		const auto cppType = _schemaLoader.getCppType(enumType->name());

		sourceFile << R"cpp(template <>
)cpp" << cppType << R"cpp( ModifiedResponse<)cpp"
				   << cppType << R"cpp(>::parse(response::Value value)
{
	if (!value.maybe_enum())
	{
		throw std::logic_error { "not a valid )cpp"
				   << cppType << R"cpp( value" };
	}

	const auto itr = std::find(s_names)cpp"
				   << cppType << R"cpp(.cbegin(), s_names)cpp" << cppType
				   << R"cpp(.cend(), value.release<std::string>());

	if (itr == s_names)cpp"
				   << cppType << R"cpp(.cend())
	{
		throw std::logic_error { "not a valid )cpp"
				   << cppType << R"cpp( value" };
	}

	return static_cast<)cpp"
				   << cppType << R"cpp(>(itr - s_names)cpp" << cppType << R"cpp(.cbegin());
}
)cpp";

		pendingSeparator.add();
	}

	const auto& responseType = _requestLoader.getResponseType();
	const auto currentScope = R"cpp(Response)cpp"s;

	for (const auto& responseField : responseType.fields)
	{
		if (outputModifiedResponseImplementation(sourceFile, currentScope, responseField))
		{
			pendingSeparator.add();
		}
	}

	pendingSeparator.reset();

	NamespaceScope requestNamespaceScope { sourceFile, getRequestNamespace() };

	pendingSeparator.add();

	outputGetRequestImplementation(sourceFile);

	if (!variables.empty())
	{
		sourceFile << R"cpp(
response::Value serializeVariables(Variables&& variables)
{
	response::Value result { response::Type::Map };

)cpp";

		for (const auto& variable : variables)
		{
			sourceFile << R"cpp(	result.emplace_back(R"js()cpp" << variable.name
					   << R"cpp()js"s, ModifiedVariable<Variables::)cpp"
					   << _schemaLoader.getCppType(variable.type->name()) << R"cpp(>::serialize)cpp"
					   << getTypeModifierList(variable.modifiers)
					   << R"cpp((std::move(variables.)cpp" << variable.cppName << R"cpp()));
)cpp";
		}

		sourceFile << R"cpp(
	return result;
}
)cpp";
	}

	sourceFile << R"cpp(
Response parseResponse(response::Value response)
{
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
					   << getResponseFieldCppType(responseField, currentScope)
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
)cpp";

	pendingSeparator.reset();

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

bool Generator::outputModifiedResponseImplementation(std::ostream& sourceFile,
	const std::string& outerScope, const ResponseField& responseField) const noexcept
{
	std::ostringstream oss;

	oss << outerScope << R"cpp(::)cpp" << getResponseFieldCppType(responseField);

	const auto cppType = oss.str();
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
			   << R"cpp( ModifiedResponse<)cpp" << cppType
			   << R"cpp(>::parse(response::Value response)
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
		"Do not expect support for Introspection");
	positional.add("schema", 1).add("request", 1).add("prefix", 1).add("namespace", 1);

	try
	{
		po::store(po::command_line_parser(argc, argv).options(options).positional(positional).run(),
			variables);
		po::notify(variables);

		// If you specify any of these parameters, you must specify all three.
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
				std::move(operationName),
				noIntrospection,
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
					if (std::holds_alternative<size_t>(segment))
					{
						std::cerr << '[' << std::get<size_t>(segment) << ']';
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
