// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ClientGenerator.h"
#include "GeneratorUtil.h"

#include "graphqlservice/internal/Version.h"

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
	: _schemaLoader(std::make_optional(std::move(schemaOptions)))
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
	if (_options.paths)
	{
		return fs::path { _options.paths->headerPath }.string();
	}
	else
	{
		return {};
	}
}

std::string Generator::getSourceDir() const noexcept
{
	if (_options.paths)
	{
		return fs::path(_options.paths->sourcePath).string();
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
#include "graphqlservice/GraphQLService.h"

// Check if the library version is compatible with clientgen )cpp"
			   << graphql::internal::MajorVersion << R"cpp(.)cpp" << graphql::internal::MinorVersion
			   << R"cpp(.0
static_assert(graphql::internal::MajorVersion == )cpp"
			   << graphql::internal::MajorVersion
			   << R"cpp(, "regenerate with clientgen: major version mismatch");
static_assert(graphql::internal::MinorVersion == )cpp"
			   << graphql::internal::MinorVersion
			   << R"cpp(, "regenerate with clientgen: minor version mismatch");

#include <memory>
#include <string>
#include <string_view>
#include <vector>

)cpp";

	NamespaceScope graphqlNamespace { headerFile, "graphql" };
	NamespaceScope schemaNamespace { headerFile, _schemaLoader.getSchemaNamespace() };
	NamespaceScope objectNamespace { headerFile, "object", true };
	PendingBlankLine pendingSeparator { headerFile };

	outputRequestComment(headerFile);

	headerFile << std::endl;

	std::string_view queryType;

	for (const auto& operation : _schemaLoader.getOperationTypes())
	{
		if (operation.operation == service::strQuery)
		{
			queryType = operation.type;
			break;
		}
	}

	if (!_schemaLoader.getEnumTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& enumType : _schemaLoader.getEnumTypes())
		{
			headerFile << R"cpp(enum class )cpp" << enumType.cppType << R"cpp(
{
)cpp";

			bool firstValue = true;

			for (const auto& value : enumType.values)
			{
				if (!firstValue)
				{
					headerFile << R"cpp(,
)cpp";
				}

				firstValue = false;
				headerFile << R"cpp(	)cpp" << value.value;
			}
			headerFile << R"cpp(
};

)cpp";
		}
	}

	if (!_schemaLoader.getInputTypes().empty())
	{
		pendingSeparator.reset();

		// Forward declare all of the input types
		if (_schemaLoader.getInputTypes().size() > 1)
		{
			for (const auto& inputType : _schemaLoader.getInputTypes())
			{
				headerFile << R"cpp(struct )cpp" << inputType.cppType << R"cpp(;
)cpp";
			}

			headerFile << std::endl;
		}

		// Output the full declarations
		for (const auto& inputType : _schemaLoader.getInputTypes())
		{
			headerFile << R"cpp(struct )cpp" << inputType.cppType << R"cpp(
{
)cpp";
			//			for (const auto& inputField : inputType.fields)
			//			{
			//				headerFile << R"cpp(	)cpp" << getFieldDeclaration(inputField) <<
			//R"cpp(; )cpp";
			//			}
			headerFile << R"cpp(};

)cpp";
		}
	}

	if (!_schemaLoader.getObjectTypes().empty())
	{
		objectNamespace.enter();
		headerFile << std::endl;

		// Forward declare all of the object types
		for (const auto& objectType : _schemaLoader.getObjectTypes())
		{
			headerFile << R"cpp(class )cpp" << objectType.cppType << R"cpp(;
)cpp";
		}

		headerFile << std::endl;
	}

	if (!_schemaLoader.getInterfaceTypes().empty())
	{
		if (objectNamespace.exit())
		{
			headerFile << std::endl;
		}

		// Forward declare all of the interface types
		if (_schemaLoader.getInterfaceTypes().size() > 1)
		{
			for (const auto& interfaceType : _schemaLoader.getInterfaceTypes())
			{
				headerFile << R"cpp(struct )cpp" << interfaceType.cppType << R"cpp(;
)cpp";
			}

			headerFile << std::endl;
		}

		// Output the full declarations
		for (const auto& interfaceType : _schemaLoader.getInterfaceTypes())
		{
			headerFile << R"cpp(struct )cpp" << interfaceType.cppType << R"cpp(
{
)cpp";

			// for (const auto& outputField : interfaceType.fields)
			//{
			//	headerFile << getFieldDeclaration(outputField);
			//}

			headerFile << R"cpp(};

)cpp";
		}
	}

	if (!_schemaLoader.getObjectTypes().empty())
	{
		if (objectNamespace.enter())
		{
			headerFile << std::endl;
		}

		// Output the full declarations
		// for (const auto& objectType : _schemaLoader.getObjectTypes())
		//{
		//	outputObjectDeclaration(headerFile, objectType, objectType.type == queryType);
		//	headerFile << std::endl;
		//}
	}

	if (objectNamespace.exit())
	{
		headerFile << std::endl;
	}

	bool firstOperation = true;

	headerFile << R"cpp(class Operations
	: public service::Request
{
public:
	explicit Operations()cpp";

	for (const auto& operation : _schemaLoader.getOperationTypes())
	{
		if (!firstOperation)
		{
			headerFile << R"cpp(, )cpp";
		}

		firstOperation = false;
		headerFile << R"cpp(std::shared_ptr<object::)cpp" << operation.cppType << R"cpp(> )cpp"
				   << operation.operation;
	}

	headerFile << R"cpp();

private:
)cpp";

	for (const auto& operation : _schemaLoader.getOperationTypes())
	{
		headerFile << R"cpp(	std::shared_ptr<object::)cpp" << operation.cppType << R"cpp(> _)cpp"
				   << operation.operation << R"cpp(;
)cpp";
	}

	headerFile << R"cpp(};

std::shared_ptr<client::Client> GetClient();

)cpp";

	return true;
}

void Generator::outputRequestComment(std::ostream& headerFile) const noexcept
{
	headerFile << R"cpp(
/** Operation: )cpp"
			   << _requestLoader.getOperationType() << ' ' << _requestLoader.getOperationName()
			   << R"cpp(

)cpp" << _requestLoader.getRequestText()
			   << R"cpp(

**/

// Return the original text of the request document.
std::string_view GetRequestText() noexcept;
)cpp";
}

bool Generator::outputSource() const noexcept
{
	std::ofstream sourceFile(_sourcePath, std::ios_base::trunc);

	sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "graphqlservice/introspection/Introspection.h"

#include "graphqlservice/GraphQLParse.h"

#include <algorithm>
#include <array>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

using namespace std::literals;

)cpp";

	NamespaceScope graphqlNamespace { sourceFile, "graphql" };

	if (!_schemaLoader.getEnumTypes().empty() || !_schemaLoader.getInputTypes().empty())
	{
		NamespaceScope serviceNamespace { sourceFile, "service" };

		sourceFile << std::endl;

		for (const auto& enumType : _schemaLoader.getEnumTypes())
		{
			bool firstValue = true;

			sourceFile << R"cpp(static const std::array<std::string_view, )cpp"
					   << enumType.values.size() << R"cpp(> s_names)cpp" << enumType.cppType
					   << R"cpp( = {
)cpp";

			for (const auto& value : enumType.values)
			{
				if (!firstValue)
				{
					sourceFile << R"cpp(,
)cpp";
				}

				firstValue = false;
				sourceFile << R"cpp(	")cpp" << value.value << R"cpp(")cpp";
			}

			sourceFile << R"cpp(
};

template <>
)cpp" << _schemaLoader.getSchemaNamespace()
					   << R"cpp(::)cpp" << enumType.cppType << R"cpp( ModifiedArgument<)cpp"
					   << _schemaLoader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { "not a valid )cpp"
					   << enumType.type << R"cpp( value" } };
	}

	auto itr = std::find(s_names)cpp"
					   << enumType.cppType << R"cpp(.cbegin(), s_names)cpp" << enumType.cppType
					   << R"cpp(.cend(), value.get<response::StringType>());

	if (itr == s_names)cpp"
					   << enumType.cppType << R"cpp(.cend())
	{
		throw service::schema_exception { { "not a valid )cpp"
					   << enumType.type << R"cpp( value" } };
	}

	return static_cast<)cpp"
					   << _schemaLoader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>(itr - s_names)cpp" << enumType.cppType << R"cpp(.cbegin());
}

template <>
std::future<service::ResolverResult> ModifiedResult<)cpp"
					   << _schemaLoader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>::convert(service::FieldResult<)cpp"
					   << _schemaLoader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[]()cpp" << _schemaLoader.getSchemaNamespace()
					   << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(&& value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<response::StringType>(std::string(s_names)cpp"
					   << enumType.cppType << R"cpp([static_cast<size_t>(value)]));

			return result;
		});
}

)cpp";
		}

		for (const auto& inputType : _schemaLoader.getInputTypes())
		{
			bool firstField = true;

			sourceFile << R"cpp(template <>
)cpp" << _schemaLoader.getSchemaNamespace()
					   << R"cpp(::)cpp" << inputType.cppType << R"cpp( ModifiedArgument<)cpp"
					   << _schemaLoader.getSchemaNamespace() << R"cpp(::)cpp" << inputType.cppType
					   << R"cpp(>::convert(const response::Value& value)
{
)cpp";

			for (const auto& inputField : inputType.fields)
			{
				if (inputField.defaultValue.type() != response::Type::Null)
				{
					if (firstField)
					{
						firstField = false;
						sourceFile << R"cpp(	const auto defaultValue = []()
	{
		response::Value values(response::Type::Map);
		response::Value entry;

)cpp";
					}

					//					sourceFile << getArgumentDefaultValue(0,
					//inputField.defaultValue)
					//							   << R"cpp(		values.emplace_back(")cpp" <<
					//inputField.name
					//							   << R"cpp(", std::move(entry));
					//)cpp";
				}
			}

			if (!firstField)
			{
				sourceFile << R"cpp(
		return values;
	}();

)cpp";
			}

			// for (const auto& inputField : inputType.fields)
			//{
			//	sourceFile << getArgumentDeclaration(inputField, "value", "value", "defaultValue");
			//}

			if (!inputType.fields.empty())
			{
				sourceFile << std::endl;
			}

			sourceFile << R"cpp(	return {
)cpp";

			firstField = true;

			for (const auto& inputField : inputType.fields)
			{
				std::string fieldName(inputField.cppName);

				if (!firstField)
				{
					sourceFile << R"cpp(,
)cpp";
				}

				firstField = false;
				fieldName[0] =
					static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
				sourceFile << R"cpp(		std::move(value)cpp" << fieldName << R"cpp())cpp";
			}

			sourceFile << R"cpp(
	};
}

)cpp";
		}

		serviceNamespace.exit();
		sourceFile << std::endl;
	}

	NamespaceScope schemaNamespace { sourceFile, _schemaLoader.getSchemaNamespace() };
	std::string_view queryType;

	outputGetRequest(sourceFile);

	for (const auto& operation : _schemaLoader.getOperationTypes())
	{
		if (operation.operation == service::strQuery)
		{
			queryType = operation.type;
			break;
		}
	}

	if (!_schemaLoader.getObjectTypes().empty())
	{
		NamespaceScope objectNamespace { sourceFile, "object" };

		sourceFile << std::endl;

		// for (const auto& objectType : _schemaLoader.getObjectTypes())
		//{
		//	outputObjectImplementation(sourceFile, objectType, objectType.type == queryType);
		//	sourceFile << std::endl;
		//}
	}

	bool firstOperation = true;

	sourceFile << R"cpp(
Operations::Operations()cpp";

	for (const auto& operation : _schemaLoader.getOperationTypes())
	{
		if (!firstOperation)
		{
			sourceFile << R"cpp(, )cpp";
		}

		firstOperation = false;
		sourceFile << R"cpp(std::shared_ptr<object::)cpp" << operation.cppType << R"cpp(> )cpp"
				   << operation.operation;
	}

	sourceFile << R"cpp()
	: service::Request({
)cpp";

	firstOperation = true;

	for (const auto& operation : _schemaLoader.getOperationTypes())
	{
		if (!firstOperation)
		{
			sourceFile << R"cpp(,
)cpp";
		}

		firstOperation = false;
		sourceFile << R"cpp(		{ ")cpp" << operation.operation << R"cpp(", )cpp"
				   << operation.operation << R"cpp( })cpp";
	}

	sourceFile << R"cpp(
	}, GetClient())
)cpp";

	for (const auto& operation : _schemaLoader.getOperationTypes())
	{
		sourceFile << R"cpp(	, _)cpp" << operation.operation << R"cpp((std::move()cpp"
				   << operation.operation << R"cpp())
)cpp";
	}

	sourceFile << R"cpp({
}

)cpp";

	sourceFile << R"cpp(std::shared_ptr<client::Client> GetClient()
{
	static std::weak_ptr<client::Client> s_wpClient;
	auto client = s_wpClient.lock();

	if (!client)
	{
		client = std::make_shared<client::Client>(false);
		AddTypesToClient(client);
		s_wpClient = client;
	}

	return client;
}

)cpp";

	return true;
}

void Generator::outputGetRequest(std::ostream& sourceFile) const noexcept
{
	sourceFile << R"cpp(
std::string_view GetRequestText() noexcept
{
	return R"gql()cpp" << _requestLoader.getRequestText() << R"cpp()gql"sv;
}
)cpp";
}

} /* namespace graphql::generator::client */

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
				graphql::generator::client::GeneratorPaths { std::move(headerDir),
					std::move(sourceDir) },
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
