// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "SchemaGenerator.h"
#include "GeneratorUtil.h"

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

namespace graphql::generator::schema {

Generator::Generator(std::optional<SchemaOptions>&& customSchema, GeneratorOptions&& options)
	: _loader(std::move(customSchema))
	, _options(std::move(options))
	, _headerDir(getHeaderDir())
	, _sourceDir(getSourceDir())
	, _headerPath(getHeaderPath())
	, _objectHeaderPath(getObjectHeaderPath())
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

	fullPath /= (std::string { _loader.getFilenamePrefix() } + "Schema.h");
	return fullPath.string();
}

std::string Generator::getObjectHeaderPath() const noexcept
{
	if (_options.separateFiles)
	{
		fs::path fullPath { _headerDir };

		fullPath /= (std::string { _loader.getFilenamePrefix() } + "Objects.h");
		return fullPath.string();
	}

	return _headerPath;
}

std::string Generator::getSourcePath() const noexcept
{
	fs::path fullPath { _sourceDir };

	fullPath /= (std::string { _loader.getFilenamePrefix() } + "Schema.cpp");
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

	if (_options.separateFiles)
	{
		auto separateFiles = outputSeparateFiles();

		for (auto& file : separateFiles)
		{
			builtFiles.push_back(std::move(file));
		}
	}

	return builtFiles;
}

bool Generator::outputHeader() const noexcept
{
	std::ofstream headerFile(_headerPath, std::ios_base::trunc);
	IncludeGuardScope includeGuard { headerFile, fs::path(_headerPath).filename().string() };

	headerFile << R"cpp(#include "graphqlservice/internal/Schema.h"

// Check if the library version is compatible with schemagen )cpp"
			   << graphql::internal::MajorVersion << R"cpp(.)cpp" << graphql::internal::MinorVersion
			   << R"cpp(.0
static_assert(graphql::internal::MajorVersion == )cpp"
			   << graphql::internal::MajorVersion
			   << R"cpp(, "regenerate with schemagen: major version mismatch");
static_assert(graphql::internal::MinorVersion == )cpp"
			   << graphql::internal::MinorVersion
			   << R"cpp(, "regenerate with schemagen: minor version mismatch");

)cpp";
	if (_loader.isIntrospection())
	{
		headerFile <<
			R"cpp(// clang-format off
#ifdef GRAPHQL_DLLEXPORTS
	#ifdef IMPL_GRAPHQLINTROSPECTION_DLL
		#define GRAPHQLINTROSPECTION_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLINTROSPECTION_DLL
		#define GRAPHQLINTROSPECTION_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLINTROSPECTION_DLL
#else // !GRAPHQL_DLLEXPORTS
	#define GRAPHQLINTROSPECTION_EXPORT
#endif // !GRAPHQL_DLLEXPORTS
// clang-format on

)cpp";
	}

	headerFile <<
		R"cpp(#include <memory>
#include <string>
#include <vector>

)cpp";

	NamespaceScope graphqlNamespace { headerFile, "graphql" };
	NamespaceScope schemaNamespace { headerFile, _loader.getSchemaNamespace() };
	NamespaceScope objectNamespace { headerFile, "object", true };
	PendingBlankLine pendingSeparator { headerFile };

	std::string_view queryType;

	if (!_loader.isIntrospection())
	{
		for (const auto& operation : _loader.getOperationTypes())
		{
			if (operation.operation == service::strQuery)
			{
				queryType = operation.type;
				break;
			}
		}
	}

	if (!_loader.getEnumTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& enumType : _loader.getEnumTypes())
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

	if (!_loader.getInputTypes().empty())
	{
		pendingSeparator.reset();

		// Output the full declarations
		for (const auto& inputType : _loader.getInputTypes())
		{
			headerFile << R"cpp(struct )cpp" << inputType.cppType << R"cpp(
{
)cpp";
			for (const auto& inputField : inputType.fields)
			{
				headerFile << R"cpp(	)cpp" << getFieldDeclaration(inputField) << R"cpp(;
)cpp";
			}
			headerFile << R"cpp(};

)cpp";
		}
	}

	if (!_loader.getObjectTypes().empty())
	{
		objectNamespace.enter();
		headerFile << std::endl;

		// Forward declare all of the object types
		for (const auto& objectType : _loader.getObjectTypes())
		{
			headerFile << R"cpp(class )cpp" << objectType.cppType << R"cpp(;
)cpp";
		}

		headerFile << std::endl;
	}

	if (!_loader.getInterfaceTypes().empty())
	{
		if (objectNamespace.exit())
		{
			headerFile << std::endl;
		}

		// Forward declare all of the interface types
		if (_loader.getInterfaceTypes().size() > 1)
		{
			for (const auto& interfaceType : _loader.getInterfaceTypes())
			{
				headerFile << R"cpp(struct )cpp" << interfaceType.cppType << R"cpp(;
)cpp";
			}

			headerFile << std::endl;
		}

		// Output the full declarations
		for (const auto& interfaceType : _loader.getInterfaceTypes())
		{
			headerFile << R"cpp(struct )cpp" << interfaceType.cppType << R"cpp(
{
)cpp";

			for (const auto& outputField : interfaceType.fields)
			{
				headerFile << getFieldDeclaration(outputField);
			}

			headerFile << R"cpp(};

)cpp";
		}
	}

	if (!_loader.getObjectTypes().empty() && !_options.separateFiles)
	{
		if (objectNamespace.enter())
		{
			headerFile << std::endl;
		}

		// Output the full declarations
		for (const auto& objectType : _loader.getObjectTypes())
		{
			outputObjectDeclaration(headerFile, objectType, objectType.type == queryType);
			headerFile << std::endl;
		}
	}

	if (objectNamespace.exit())
	{
		headerFile << std::endl;
	}

	if (!_loader.isIntrospection())
	{
		bool firstOperation = true;

		headerFile << R"cpp(class Operations
	: public service::Request
{
public:
	explicit Operations()cpp";

		for (const auto& operation : _loader.getOperationTypes())
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

		for (const auto& operation : _loader.getOperationTypes())
		{
			headerFile << R"cpp(	std::shared_ptr<object::)cpp" << operation.cppType
					   << R"cpp(> _)cpp" << operation.operation << R"cpp(;
)cpp";
		}

		headerFile << R"cpp(};

)cpp";
	}

	if (!_loader.getObjectTypes().empty() && _options.separateFiles)
	{
		for (const auto& objectType : _loader.getObjectTypes())
		{
			headerFile << R"cpp(void Add)cpp" << objectType.cppType
					   << R"cpp(Details(std::shared_ptr<schema::ObjectType> type)cpp"
					   << objectType.cppType
					   << R"cpp(, const std::shared_ptr<schema::Schema>& schema);
)cpp";
		}

		headerFile << std::endl;
	}

	if (_loader.isIntrospection())
	{
		headerFile
			<< R"cpp(GRAPHQLINTROSPECTION_EXPORT void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema);

)cpp";

		if (!_loader.getEnumTypes().empty() || !_loader.getInputTypes().empty())
		{
			if (schemaNamespace.exit())
			{
				headerFile << std::endl;
			}

			NamespaceScope serviceNamespace { headerFile, "service" };

			headerFile << R"cpp(
#ifdef GRAPHQL_DLLEXPORTS
// Export all of the built-in converters
)cpp";

			for (const auto& enumType : _loader.getEnumTypes())
			{
				headerFile << R"cpp(template <>
GRAPHQLINTROSPECTION_EXPORT )cpp"
						   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
						   << R"cpp( ModifiedArgument<)cpp" << _loader.getSchemaNamespace()
						   << R"cpp(::)cpp" << enumType.cppType << R"cpp(>::convert(
	const response::Value& value);
template <>
GRAPHQLINTROSPECTION_EXPORT std::future<ResolverResult> ModifiedResult<)cpp"
						   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
						   << R"cpp(>::convert(
	FieldResult<)cpp" << _loader.getSchemaNamespace()
						   << R"cpp(::)cpp" << enumType.cppType
						   << R"cpp(>&& result, ResolverParams&& params);
)cpp";
			}

			for (const auto& inputType : _loader.getInputTypes())
			{
				headerFile << R"cpp(template <>
GRAPHQLINTROSPECTION_EXPORT )cpp"
						   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << inputType.cppType
						   << R"cpp( ModifiedArgument<)cpp" << inputType.cppType
						   << R"cpp(>::convert(
	const response::Value& value);
)cpp";
			}

			headerFile << R"cpp(#endif // GRAPHQL_DLLEXPORTS

)cpp";
		}
	}
	else
	{
		headerFile << R"cpp(std::shared_ptr<schema::Schema> GetSchema();

)cpp";
	}

	return true;
}

void Generator::outputObjectDeclaration(
	std::ostream& headerFile, const ObjectType& objectType, bool isQueryType) const
{
	headerFile << R"cpp(class )cpp" << objectType.cppType << R"cpp(
	: public service::Object)cpp";

	for (const auto& interfaceName : objectType.interfaces)
	{
		headerFile << R"cpp(
	, public )cpp" << _loader.getSafeCppName(interfaceName);
	}

	headerFile << R"cpp(
{
protected:
	explicit )cpp"
			   << objectType.cppType << R"cpp(();
)cpp";

	if (!objectType.fields.empty())
	{
		bool firstField = true;

		for (const auto& outputField : objectType.fields)
		{
			if (outputField.inheritedField && (_loader.isIntrospection() || _options.noStubs))
			{
				continue;
			}

			if (firstField)
			{
				headerFile << R"cpp(
public:
)cpp";
				firstField = false;
			}

			headerFile << getFieldDeclaration(outputField);
		}

		headerFile << R"cpp(
private:
)cpp";

		for (const auto& outputField : objectType.fields)
		{
			headerFile << getResolverDeclaration(outputField);
		}

		headerFile << R"cpp(
	std::future<service::ResolverResult> resolve_typename(service::ResolverParams&& params);
)cpp";

		if (!_options.noIntrospection && isQueryType)
		{
			headerFile
				<< R"cpp(	std::future<service::ResolverResult> resolve_schema(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolve_type(service::ResolverParams&& params);

	std::shared_ptr<schema::Schema> _schema;
)cpp";
		}
	}

	headerFile << R"cpp(};
)cpp";
}

std::string Generator::getFieldDeclaration(const InputField& inputField) const noexcept
{
	std::ostringstream output;

	output << _loader.getInputCppType(inputField) << R"cpp( )cpp" << inputField.cppName;

	return output.str();
}

std::string Generator::getFieldDeclaration(const OutputField& outputField) const noexcept
{
	std::ostringstream output;
	std::string fieldName { outputField.cppName };

	fieldName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
	output << R"cpp(	virtual service::FieldResult<)cpp" << _loader.getOutputCppType(outputField)
		   << R"cpp(> )cpp" << outputField.accessor << fieldName
		   << R"cpp((service::FieldParams&& params)cpp";

	for (const auto& argument : outputField.arguments)
	{
		output << R"cpp(, )cpp" << _loader.getInputCppType(argument) << R"cpp(&& )cpp"
			   << argument.cppName << "Arg";
	}

	output << R"cpp() const)cpp";
	if (outputField.interfaceField || _loader.isIntrospection() || _options.noStubs)
	{
		output << R"cpp( = 0)cpp";
	}
	else if (outputField.inheritedField)
	{
		output << R"cpp( override)cpp";
	}
	output << R"cpp(;
)cpp";

	return output.str();
}

std::string Generator::getResolverDeclaration(const OutputField& outputField) const noexcept
{
	std::ostringstream output;
	std::string fieldName(outputField.cppName);

	fieldName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
	output << R"cpp(	std::future<service::ResolverResult> resolve)cpp" << fieldName
		   << R"cpp((service::ResolverParams&& params);
)cpp";

	return output.str();
}

bool Generator::outputSource() const noexcept
{
	std::ofstream sourceFile(_sourcePath, std::ios_base::trunc);

	sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

)cpp";

	if (!_loader.isIntrospection())
	{
		sourceFile << R"cpp(#include ")cpp" << fs::path(_objectHeaderPath).filename().string()
				   << R"cpp("

)cpp";
	}

	sourceFile << R"cpp(#include "graphqlservice/introspection/Introspection.h"

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

	if (!_loader.getEnumTypes().empty() || !_loader.getInputTypes().empty())
	{
		NamespaceScope serviceNamespace { sourceFile, "service" };

		sourceFile << std::endl;

		for (const auto& enumType : _loader.getEnumTypes())
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
				sourceFile << R"cpp(	")cpp" << value.value << R"cpp("sv)cpp";
			}

			sourceFile << R"cpp(
};

template <>
)cpp" << _loader.getSchemaNamespace()
					   << R"cpp(::)cpp" << enumType.cppType << R"cpp( ModifiedArgument<)cpp"
					   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { "not a valid )cpp"
					   << enumType.type << R"cpp( value" } };
	}

	const auto itr = std::find(s_names)cpp"
					   << enumType.cppType << R"cpp(.cbegin(), s_names)cpp" << enumType.cppType
					   << R"cpp(.cend(), value.get<response::StringType>());

	if (itr == s_names)cpp"
					   << enumType.cppType << R"cpp(.cend())
	{
		throw service::schema_exception { { "not a valid )cpp"
					   << enumType.type << R"cpp( value" } };
	}

	return static_cast<)cpp"
					   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>(itr - s_names)cpp" << enumType.cppType << R"cpp(.cbegin());
}

template <>
std::future<service::ResolverResult> ModifiedResult<)cpp"
					   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>::convert(service::FieldResult<)cpp"
					   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[]()cpp" << _loader.getSchemaNamespace()
					   << R"cpp(::)cpp" << enumType.cppType << R"cpp( value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<response::StringType>(response::StringType { s_names)cpp"
					   << enumType.cppType << R"cpp([static_cast<size_t>(value)] });

			return result;
		});
}

)cpp";
		}

		for (const auto& inputType : _loader.getInputTypes())
		{
			bool firstField = true;

			sourceFile << R"cpp(template <>
)cpp" << _loader.getSchemaNamespace()
					   << R"cpp(::)cpp" << inputType.cppType << R"cpp( ModifiedArgument<)cpp"
					   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << inputType.cppType
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

					sourceFile << getArgumentDefaultValue(0, inputField.defaultValue)
							   << R"cpp(		values.emplace_back(")cpp" << inputField.name
							   << R"cpp(", std::move(entry));
)cpp";
				}
			}

			if (!firstField)
			{
				sourceFile << R"cpp(
		return values;
	}();

)cpp";
			}

			for (const auto& inputField : inputType.fields)
			{
				sourceFile << getArgumentDeclaration(inputField, "value", "value", "defaultValue");
			}

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

	NamespaceScope schemaNamespace { sourceFile, _loader.getSchemaNamespace() };
	std::string_view queryType;

	if (!_loader.isIntrospection())
	{
		for (const auto& operation : _loader.getOperationTypes())
		{
			if (operation.operation == service::strQuery)
			{
				queryType = operation.type;
				break;
			}
		}
	}

	if (!_loader.getObjectTypes().empty() && !_options.separateFiles)
	{
		NamespaceScope objectNamespace { sourceFile, "object" };

		sourceFile << std::endl;

		for (const auto& objectType : _loader.getObjectTypes())
		{
			outputObjectImplementation(sourceFile, objectType, objectType.type == queryType);
			sourceFile << std::endl;
		}
	}

	if (!_loader.isIntrospection())
	{
		bool firstOperation = true;

		sourceFile << R"cpp(
Operations::Operations()cpp";

		for (const auto& operation : _loader.getOperationTypes())
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

		for (const auto& operation : _loader.getOperationTypes())
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
	}, GetSchema())
)cpp";

		for (const auto& operation : _loader.getOperationTypes())
		{
			sourceFile << R"cpp(	, _)cpp" << operation.operation << R"cpp((std::move()cpp"
					   << operation.operation << R"cpp())
)cpp";
		}

		sourceFile << R"cpp({
}

)cpp";
	}
	else
	{
		sourceFile << std::endl;
	}

	sourceFile << R"cpp(void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema)
{
)cpp";

	if (_loader.isIntrospection())
	{
		// Add SCALAR types for each of the built-in types
		for (const auto& builtinType : SchemaLoader::getBuiltinTypes())
		{
			sourceFile << R"cpp(	schema->AddType(R"gql()cpp" << builtinType.first
					   << R"cpp()gql"sv, schema::ScalarType::Make(R"gql()cpp" << builtinType.first
					   << R"cpp()gql"sv, R"md()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << R"cpp(Built-in type)cpp";
			}

			sourceFile << R"cpp()md"));
)cpp";
		}
	}

	if (!_loader.getScalarTypes().empty())
	{
		for (const auto& scalarType : _loader.getScalarTypes())
		{
			sourceFile << R"cpp(	schema->AddType(R"gql()cpp" << scalarType.type
					   << R"cpp()gql"sv, schema::ScalarType::Make(R"gql()cpp" << scalarType.type
					   << R"cpp()gql"sv, R"md()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << scalarType.description;
			}

			sourceFile << R"cpp()md"));
)cpp";
		}
	}

	if (!_loader.getEnumTypes().empty())
	{
		for (const auto& enumType : _loader.getEnumTypes())
		{
			sourceFile << R"cpp(	auto type)cpp" << enumType.cppType
					   << R"cpp( = schema::EnumType::Make(R"gql()cpp" << enumType.type
					   << R"cpp()gql"sv, R"md()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << enumType.description;
			}

			sourceFile << R"cpp()md"sv);
	schema->AddType(R"gql()cpp"
					   << enumType.type << R"cpp()gql"sv, type)cpp" << enumType.cppType << R"cpp();
)cpp";
		}
	}

	if (!_loader.getInputTypes().empty())
	{
		for (const auto& inputType : _loader.getInputTypes())
		{
			sourceFile << R"cpp(	auto type)cpp" << inputType.cppType
					   << R"cpp( = schema::InputObjectType::Make(R"gql()cpp" << inputType.type
					   << R"cpp()gql"sv, R"md()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << inputType.description;
			}

			sourceFile << R"cpp()md"sv);
	schema->AddType(R"gql()cpp"
					   << inputType.type << R"cpp()gql"sv, type)cpp" << inputType.cppType
					   << R"cpp();
)cpp";
		}
	}

	if (!_loader.getUnionTypes().empty())
	{
		for (const auto& unionType : _loader.getUnionTypes())
		{
			sourceFile << R"cpp(	auto type)cpp" << unionType.cppType
					   << R"cpp( = schema::UnionType::Make(R"gql()cpp" << unionType.type
					   << R"cpp()gql"sv, R"md()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << unionType.description;
			}

			sourceFile << R"cpp()md"sv);
	schema->AddType(R"gql()cpp"
					   << unionType.type << R"cpp()gql"sv, type)cpp" << unionType.cppType
					   << R"cpp();
)cpp";
		}
	}

	if (!_loader.getInterfaceTypes().empty())
	{
		for (const auto& interfaceType : _loader.getInterfaceTypes())
		{
			sourceFile << R"cpp(	auto type)cpp" << interfaceType.cppType
					   << R"cpp( = schema::InterfaceType::Make(R"gql()cpp" << interfaceType.type
					   << R"cpp()gql"sv, R"md()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << interfaceType.description;
			}

			sourceFile << R"cpp()md"sv);
	schema->AddType(R"gql()cpp"
					   << interfaceType.type << R"cpp()gql"sv, type)cpp" << interfaceType.cppType
					   << R"cpp();
)cpp";
		}
	}

	if (!_loader.getObjectTypes().empty())
	{
		for (const auto& objectType : _loader.getObjectTypes())
		{
			sourceFile << R"cpp(	auto type)cpp" << objectType.cppType
					   << R"cpp( = schema::ObjectType::Make(R"gql()cpp" << objectType.type
					   << R"cpp()gql"sv, R"md()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << objectType.description;
			}

			sourceFile << R"cpp()md");
	schema->AddType(R"gql()cpp"
					   << objectType.type << R"cpp()gql"sv, type)cpp" << objectType.cppType
					   << R"cpp();
)cpp";
		}
	}

	if (!_loader.getEnumTypes().empty())
	{
		sourceFile << std::endl;

		for (const auto& enumType : _loader.getEnumTypes())
		{
			if (!enumType.values.empty())
			{
				bool firstValue = true;

				sourceFile << R"cpp(	type)cpp" << enumType.cppType << R"cpp(->AddEnumValues({
)cpp";

				for (const auto& enumValue : enumType.values)
				{
					if (!firstValue)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstValue = false;
					sourceFile << R"cpp(		{ service::s_names)cpp" << enumType.cppType
							   << R"cpp([static_cast<size_t>()cpp" << _loader.getSchemaNamespace()
							   << R"cpp(::)cpp" << enumType.cppType << R"cpp(::)cpp"
							   << enumValue.cppValue << R"cpp()], R"md()cpp";

					if (!_options.noIntrospection)
					{
						sourceFile << enumValue.description;
					}

					sourceFile << R"cpp()md"sv, )cpp";

					if (enumValue.deprecationReason)
					{
						sourceFile << R"cpp(std::make_optional(R"md()cpp"
								   << *enumValue.deprecationReason << R"cpp()md"sv))cpp";
					}
					else
					{
						sourceFile << R"cpp(std::nullopt)cpp";
					}

					sourceFile << R"cpp( })cpp";
				}

				sourceFile << R"cpp(
	});
)cpp";
			}
		}
	}

	if (!_loader.getInputTypes().empty())
	{
		sourceFile << std::endl;

		for (const auto& inputType : _loader.getInputTypes())
		{
			if (!inputType.fields.empty())
			{
				bool firstValue = true;

				sourceFile << R"cpp(	type)cpp" << inputType.cppType << R"cpp(->AddInputValues({
)cpp";

				for (const auto& inputField : inputType.fields)
				{
					if (!firstValue)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstValue = false;
					sourceFile << R"cpp(		schema::InputValue::Make(R"gql()cpp"
							   << inputField.name << R"cpp()gql"sv, R"md()cpp";

					if (!_options.noIntrospection)
					{
						sourceFile << inputField.description;
					}

					sourceFile << R"cpp()md"sv, )cpp"
							   << getIntrospectionType(inputField.type, inputField.modifiers)
							   << R"cpp(, R"gql()cpp" << inputField.defaultValueString
							   << R"cpp()gql"sv))cpp";
				}

				sourceFile << R"cpp(
	});
)cpp";
			}
		}
	}

	if (!_loader.getUnionTypes().empty())
	{
		sourceFile << std::endl;

		for (const auto& unionType : _loader.getUnionTypes())
		{
			if (!unionType.options.empty())
			{
				bool firstValue = true;

				sourceFile << R"cpp(	type)cpp" << unionType.cppType
						   << R"cpp(->AddPossibleTypes({
)cpp";

				for (const auto& unionOption : unionType.options)
				{
					if (!firstValue)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstValue = false;
					sourceFile << R"cpp(		schema->LookupType(R"gql()cpp" << unionOption
							   << R"cpp()gql"sv))cpp";
				}

				sourceFile << R"cpp(
	});
)cpp";
			}
		}
	}

	if (!_loader.getInterfaceTypes().empty())
	{
		sourceFile << std::endl;

		for (const auto& interfaceType : _loader.getInterfaceTypes())
		{
			if (!interfaceType.fields.empty())
			{
				bool firstValue = true;

				sourceFile << R"cpp(	type)cpp" << interfaceType.cppType << R"cpp(->AddFields({
)cpp";

				for (const auto& interfaceField : interfaceType.fields)
				{
					if (!firstValue)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstValue = false;
					sourceFile << R"cpp(		schema::Field::Make(R"gql()cpp"
							   << interfaceField.name << R"cpp()gql"sv, R"md()cpp";

					if (!_options.noIntrospection)
					{
						sourceFile << interfaceField.description;
					}

					sourceFile << R"cpp()md"sv, )cpp";

					if (interfaceField.deprecationReason)
					{
						sourceFile << R"cpp(std::make_optional(R"md()cpp"
								   << *interfaceField.deprecationReason << R"cpp()md"sv))cpp";
					}
					else
					{
						sourceFile << R"cpp(std::nullopt)cpp";
					}

					sourceFile << R"cpp(, )cpp"
							   << getIntrospectionType(interfaceField.type,
									  interfaceField.modifiers);

					if (!interfaceField.arguments.empty())
					{
						bool firstArgument = true;

						sourceFile << R"cpp(, {
)cpp";

						for (const auto& argument : interfaceField.arguments)
						{
							if (!firstArgument)
							{
								sourceFile << R"cpp(,
)cpp";
							}

							firstArgument = false;
							sourceFile << R"cpp(			schema::InputValue::Make(R"gql()cpp"
									   << argument.name << R"cpp()gql"sv, R"md()cpp";

							if (!_options.noIntrospection)
							{
								sourceFile << argument.description;
							}

							sourceFile << R"cpp()md"sv, )cpp"
									   << getIntrospectionType(argument.type, argument.modifiers)
									   << R"cpp(, R"gql()cpp" << argument.defaultValueString
									   << R"cpp()gql"sv))cpp";
						}

						sourceFile << R"cpp(
		})cpp";
					}

					sourceFile << R"cpp())cpp";
				}

				sourceFile << R"cpp(
	});
)cpp";
			}
		}
	}

	if (!_loader.getObjectTypes().empty())
	{
		sourceFile << std::endl;

		for (const auto& objectType : _loader.getObjectTypes())
		{
			if (_options.separateFiles)
			{
				sourceFile << R"cpp(	Add)cpp" << objectType.cppType << R"cpp(Details(type)cpp"
						   << objectType.cppType << R"cpp(, schema);
)cpp";
			}
			else
			{
				outputObjectIntrospection(sourceFile, objectType);
			}
		}
	}

	if (!_loader.getDirectives().empty())
	{
		sourceFile << std::endl;

		for (const auto& directive : _loader.getDirectives())
		{
			sourceFile << R"cpp(	schema->AddDirective(schema::Directive::Make(R"gql()cpp"
					   << directive.name << R"cpp()gql"sv, R"md()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << directive.description;
			}

			sourceFile << R"cpp()md"sv, {)cpp";

			if (!directive.locations.empty())
			{
				bool firstLocation = true;

				sourceFile << R"cpp(
)cpp";

				for (const auto& location : directive.locations)
				{
					if (!firstLocation)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstLocation = false;
					sourceFile << R"cpp(		)cpp" << SchemaLoader::getIntrospectionNamespace()
							   << R"cpp(::DirectiveLocation::)cpp" << location;
				}

				sourceFile << R"cpp(
	)cpp";
			}

			sourceFile << R"cpp(})cpp";

			if (!directive.arguments.empty())
			{
				bool firstArgument = true;

				sourceFile << R"cpp(, {
)cpp";

				for (const auto& argument : directive.arguments)
				{
					if (!firstArgument)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstArgument = false;
					sourceFile << R"cpp(		schema::InputValue::Make(R"gql()cpp"
							   << argument.name << R"cpp()gql"sv, R"md()cpp";

					if (!_options.noIntrospection)
					{
						sourceFile << argument.description;
					}

					sourceFile << R"cpp()md"sv, )cpp"
							   << getIntrospectionType(argument.type, argument.modifiers)
							   << R"cpp(, R"gql()cpp" << argument.defaultValueString
							   << R"cpp()gql"sv))cpp";
				}

				sourceFile << R"cpp(
	})cpp";
			}
			sourceFile << R"cpp());
)cpp";
		}
	}

	if (!_loader.isIntrospection())
	{
		sourceFile << std::endl;

		for (const auto& operationType : _loader.getOperationTypes())
		{
			std::string operation(operationType.operation);

			operation[0] =
				static_cast<char>(std::toupper(static_cast<unsigned char>(operation[0])));
			sourceFile << R"cpp(	schema->Add)cpp" << operation << R"cpp(Type(type)cpp"
					   << operationType.cppType << R"cpp();
)cpp";
		}
	}

	sourceFile << R"cpp(}

)cpp";

	if (!_loader.isIntrospection())
	{
		sourceFile << R"cpp(std::shared_ptr<schema::Schema> GetSchema()
{
	static std::weak_ptr<schema::Schema> s_wpSchema;
	auto schema = s_wpSchema.lock();

	if (!schema)
	{
		schema = std::make_shared<schema::Schema>()cpp"
				   << (_options.noIntrospection ? "true" : "false") << R"cpp();
		)cpp" << SchemaLoader::getIntrospectionNamespace()
				   << R"cpp(::AddTypesToSchema(schema);
		AddTypesToSchema(schema);
		s_wpSchema = schema;
	}

	return schema;
}

)cpp";
	}

	return true;
}

void Generator::outputObjectImplementation(
	std::ostream& sourceFile, const ObjectType& objectType, bool isQueryType) const
{
	using namespace std::literals;

	// Output the protected constructor which calls through to the service::Object constructor
	// with arguments that declare the set of types it implements and bind the fields to the
	// resolver methods.
	sourceFile << objectType.cppType << R"cpp(::)cpp" << objectType.cppType << R"cpp(()
	: service::Object({
)cpp";

	for (const auto& interfaceName : objectType.interfaces)
	{
		sourceFile << R"cpp(		")cpp" << interfaceName << R"cpp(",
)cpp";
	}

	for (const auto& unionName : objectType.unions)
	{
		sourceFile << R"cpp(		")cpp" << unionName << R"cpp(",
)cpp";
	}

	sourceFile << R"cpp(		")cpp" << objectType.type << R"cpp("
	}, {
)cpp";

	std::map<std::string_view, std::string, internal::shorter_or_less> resolvers;

	std::transform(objectType.fields.cbegin(),
		objectType.fields.cend(),
		std::inserter(resolvers, resolvers.begin()),
		[](const OutputField& outputField) noexcept {
			std::string fieldName(outputField.cppName);

			fieldName[0] =
				static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));

			std::ostringstream output;

			output << R"cpp(		{ R"gql()cpp" << outputField.name
				   << R"cpp()gql"sv, [this](service::ResolverParams&& params) { return resolve)cpp"
				   << fieldName << R"cpp((std::move(params)); } })cpp";

			return std::make_pair(std::string_view { outputField.name }, output.str());
		});

	resolvers["__typename"sv] =
		R"cpp(		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } })cpp"s;

	if (!_options.noIntrospection && isQueryType)
	{
		resolvers["__schema"sv] =
			R"cpp(		{ R"gql(__schema)gql"sv, [this](service::ResolverParams&& params) { return resolve_schema(std::move(params)); } })cpp"s;
		resolvers["__type"sv] =
			R"cpp(		{ R"gql(__type)gql"sv, [this](service::ResolverParams&& params) { return resolve_type(std::move(params)); } })cpp"s;
	}

	bool firstField = true;

	for (const auto& resolver : resolvers)
	{
		if (!firstField)
		{
			sourceFile << R"cpp(,
)cpp";
		}

		firstField = false;
		sourceFile << resolver.second;
	}

	sourceFile << R"cpp(
	}))cpp";

	if (!_options.noIntrospection && isQueryType)
	{
		sourceFile << R"cpp(
	, _schema(GetSchema()))cpp";
	}

	sourceFile << R"cpp(
{
}
)cpp";

	// Output each of the resolver implementations, which call the virtual property
	// getters that the implementer must define.
	for (const auto& outputField : objectType.fields)
	{
		std::string fieldName(outputField.cppName);

		fieldName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
		if (!_loader.isIntrospection() && !_options.noStubs)
		{
			sourceFile << R"cpp(
service::FieldResult<)cpp"
					   << _loader.getOutputCppType(outputField) << R"cpp(> )cpp"
					   << objectType.cppType << R"cpp(::)cpp" << outputField.accessor << fieldName
					   << R"cpp((service::FieldParams&&)cpp";
			for (const auto& argument : outputField.arguments)
			{
				sourceFile << R"cpp(, )cpp" << _loader.getInputCppType(argument) << R"cpp(&&)cpp";
			}

			sourceFile << R"cpp() const
{
	throw std::runtime_error(R"ex()cpp"
					   << objectType.cppType << R"cpp(::)cpp" << outputField.accessor << fieldName
					   << R"cpp( is not implemented)ex");
}
)cpp";
		}

		sourceFile << R"cpp(
std::future<service::ResolverResult> )cpp"
				   << objectType.cppType << R"cpp(::resolve)cpp" << fieldName
				   << R"cpp((service::ResolverParams&& params)
{
)cpp";

		// Output a preamble to retrieve all of the arguments from the resolver parameters.
		if (!outputField.arguments.empty())
		{
			bool firstArgument = true;

			for (const auto& argument : outputField.arguments)
			{
				if (argument.defaultValue.type() != response::Type::Null)
				{
					if (firstArgument)
					{
						firstArgument = false;
						sourceFile << R"cpp(	const auto defaultArguments = []()
	{
		response::Value values(response::Type::Map);
		response::Value entry;

)cpp";
					}

					sourceFile << getArgumentDefaultValue(0, argument.defaultValue)
							   << R"cpp(		values.emplace_back(")cpp" << argument.name
							   << R"cpp(", std::move(entry));
)cpp";
				}
			}

			if (!firstArgument)
			{
				sourceFile << R"cpp(
		return values;
	}();

)cpp";
			}

			for (const auto& argument : outputField.arguments)
			{
				sourceFile << getArgumentDeclaration(argument,
					"arg",
					"params.arguments",
					"defaultArguments");
			}
		}

		sourceFile << R"cpp(	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = )cpp"
				   << outputField.accessor << fieldName
				   << R"cpp((service::FieldParams(std::move(params), std::move(directives)))cpp";

		if (!outputField.arguments.empty())
		{
			for (const auto& argument : outputField.arguments)
			{
				std::string argumentName(argument.cppName);

				argumentName[0] =
					static_cast<char>(std::toupper(static_cast<unsigned char>(argumentName[0])));
				sourceFile << R"cpp(, std::move(arg)cpp" << argumentName << R"cpp())cpp";
			}
		}

		sourceFile << R"cpp();
	resolverLock.unlock();

	return )cpp" << getResultAccessType(outputField)
				   << R"cpp(::convert)cpp" << getTypeModifiers(outputField.modifiers)
				   << R"cpp((std::move(result), std::move(params));
}
)cpp";
	}

	sourceFile << R"cpp(
std::future<service::ResolverResult> )cpp"
			   << objectType.cppType << R"cpp(::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql()cpp"
			   << objectType.type << R"cpp()gql" }, std::move(params));
}
)cpp";

	if (!_options.noIntrospection && isQueryType)
	{
		sourceFile
			<< R"cpp(
std::future<service::ResolverResult> )cpp"
			<< objectType.cppType << R"cpp(::resolve_schema(service::ResolverParams&& params)
{
	return service::ModifiedResult<service::Object>::convert(std::static_pointer_cast<service::Object>(std::make_shared<)cpp"
			<< SchemaLoader::getIntrospectionNamespace()
			<< R"cpp(::Schema>(_schema)), std::move(params));
}

std::future<service::ResolverResult> )cpp"
			<< objectType.cppType << R"cpp(::resolve_type(service::ResolverParams&& params)
{
	auto argName = service::ModifiedArgument<response::StringType>::require("name", params.arguments);
	const auto& baseType = _schema->LookupType(argName);
	std::shared_ptr<)cpp"
			<< SchemaLoader::getIntrospectionNamespace()
			<< R"cpp(::object::Type> result { baseType ? std::make_shared<)cpp"
			<< SchemaLoader::getIntrospectionNamespace() << R"cpp(::Type>(baseType) : nullptr };

	return service::ModifiedResult<)cpp"
			<< SchemaLoader::getIntrospectionNamespace()
			<< R"cpp(::object::Type>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}
)cpp";
	}
}

void Generator::outputObjectIntrospection(
	std::ostream& sourceFile, const ObjectType& objectType) const
{
	if (!objectType.interfaces.empty())
	{
		bool firstInterface = true;

		sourceFile << R"cpp(	type)cpp" << objectType.cppType << R"cpp(->AddInterfaces({
)cpp";

		for (const auto& interfaceName : objectType.interfaces)
		{
			if (!firstInterface)
			{
				sourceFile << R"cpp(,
)cpp";
			}

			firstInterface = false;

			if (_options.separateFiles)
			{
				sourceFile
					<< R"cpp(		std::static_pointer_cast<const schema::InterfaceType>(schema->LookupType(R"gql()cpp"
					<< interfaceName << R"cpp()gql"sv)))cpp";
			}
			else
			{
				sourceFile << R"cpp(		type)cpp" << _loader.getSafeCppName(interfaceName);
			}
		}

		sourceFile << R"cpp(
	});
)cpp";
	}

	if (!objectType.fields.empty())
	{
		bool firstValue = true;

		sourceFile << R"cpp(	type)cpp" << objectType.cppType << R"cpp(->AddFields({
)cpp";

		for (const auto& objectField : objectType.fields)
		{
			if (!firstValue)
			{
				sourceFile << R"cpp(,
)cpp";
			}

			firstValue = false;
			sourceFile << R"cpp(		schema::Field::Make(R"gql()cpp" << objectField.name
					   << R"cpp()gql"sv, R"md()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << objectField.description;
			}

			sourceFile << R"cpp()md"sv, )cpp";

			if (objectField.deprecationReason)
			{
				sourceFile << R"cpp(std::make_optional(R"md()cpp" << *objectField.deprecationReason
						   << R"cpp()md"sv))cpp";
			}
			else
			{
				sourceFile << R"cpp(std::nullopt)cpp";
			}

			sourceFile << R"cpp(, )cpp"
					   << getIntrospectionType(objectField.type, objectField.modifiers);

			if (!objectField.arguments.empty())
			{
				bool firstArgument = true;

				sourceFile << R"cpp(, {
)cpp";

				for (const auto& argument : objectField.arguments)
				{
					if (!firstArgument)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstArgument = false;
					sourceFile << R"cpp(			schema::InputValue::Make(R"gql()cpp"
							   << argument.name << R"cpp()gql"sv, R"md()cpp";

					if (!_options.noIntrospection)
					{
						sourceFile << argument.description;
					}

					sourceFile << R"cpp()md"sv, )cpp"
							   << getIntrospectionType(argument.type, argument.modifiers)
							   << R"cpp(, R"gql()cpp" << argument.defaultValueString
							   << R"cpp()gql"sv))cpp";
				}

				sourceFile << R"cpp(
		})cpp";
			}

			sourceFile << R"cpp())cpp";
		}

		sourceFile << R"cpp(
	});
)cpp";
	}
}

std::string Generator::getArgumentDefaultValue(
	size_t level, const response::Value& defaultValue) const noexcept
{
	const std::string padding(level, '\t');
	std::ostringstream argumentDefaultValue;

	switch (defaultValue.type())
	{
		case response::Type::Map:
		{
			const auto& members = defaultValue.get<response::MapType>();

			argumentDefaultValue << padding << R"cpp(		entry = []()
)cpp" << padding << R"cpp(		{
)cpp" << padding << R"cpp(			response::Value members(response::Type::Map);
)cpp" << padding << R"cpp(			response::Value entry;

)cpp";

			for (const auto& entry : members)
			{
				argumentDefaultValue << getArgumentDefaultValue(level + 1, entry.second) << padding
									 << R"cpp(			members.emplace_back(")cpp" << entry.first
									 << R"cpp(", std::move(entry));
)cpp";
			}

			argumentDefaultValue << padding << R"cpp(			return members;
)cpp" << padding << R"cpp(		}();
)cpp";
			break;
		}

		case response::Type::List:
		{
			const auto& elements = defaultValue.get<response::ListType>();

			argumentDefaultValue << padding << R"cpp(		entry = []()
)cpp" << padding << R"cpp(		{
)cpp" << padding << R"cpp(			response::Value elements(response::Type::List);
)cpp" << padding << R"cpp(			response::Value entry;

)cpp";

			for (const auto& entry : elements)
			{
				argumentDefaultValue << getArgumentDefaultValue(level + 1, entry) << padding
									 << R"cpp(			elements.emplace_back(std::move(entry));
)cpp";
			}

			argumentDefaultValue << padding << R"cpp(			return elements;
)cpp" << padding << R"cpp(		}();
)cpp";
			break;
		}

		case response::Type::String:
		{
			argumentDefaultValue << padding
								 << R"cpp(		entry = response::Value(std::string(R"gql()cpp"
								 << defaultValue.get<response::StringType>() << R"cpp()gql"));
)cpp";
			break;
		}

		case response::Type::Null:
		{
			argumentDefaultValue << padding << R"cpp(		entry = {};
)cpp";
			break;
		}

		case response::Type::Boolean:
		{
			argumentDefaultValue << padding << R"cpp(		entry = response::Value()cpp"
								 << (defaultValue.get<response::BooleanType>() ? R"cpp(true)cpp"
																			   : R"cpp(false)cpp")
								 << R"cpp();
)cpp";
			break;
		}

		case response::Type::Int:
		{
			argumentDefaultValue
				<< padding
				<< R"cpp(		entry = response::Value(static_cast<response::IntType>()cpp"
				<< defaultValue.get<response::IntType>() << R"cpp());
)cpp";
			break;
		}

		case response::Type::Float:
		{
			argumentDefaultValue
				<< padding
				<< R"cpp(		entry = response::Value(static_cast<response::FloatType>()cpp"
				<< defaultValue.get<response::FloatType>() << R"cpp());
)cpp";
			break;
		}

		case response::Type::EnumValue:
		{
			argumentDefaultValue
				<< padding << R"cpp(		entry = response::Value(response::Type::EnumValue);
		entry.set<response::StringType>(R"gql()cpp"
				<< defaultValue.get<response::StringType>() << R"cpp()gql");
)cpp";
			break;
		}

		case response::Type::Scalar:
		{
			argumentDefaultValue << padding << R"cpp(		entry = []()
)cpp" << padding << R"cpp(		{
)cpp" << padding << R"cpp(			response::Value scalar(response::Type::Scalar);
)cpp" << padding << R"cpp(			response::Value entry;

)cpp";
			argumentDefaultValue
				<< padding << R"cpp(	)cpp"
				<< getArgumentDefaultValue(level + 1, defaultValue.get<response::ScalarType>())
				<< padding << R"cpp(			scalar.set<response::ScalarType>(std::move(entry));

)cpp" << padding << R"cpp(			return scalar;
)cpp" << padding << R"cpp(		}();
)cpp";
			break;
		}
	}

	return argumentDefaultValue.str();
}

std::string Generator::getArgumentDeclaration(const InputField& argument, const char* prefixToken,
	const char* argumentsToken, const char* defaultToken) const noexcept
{
	std::ostringstream argumentDeclaration;
	std::string argumentName(argument.cppName);

	argumentName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(argumentName[0])));
	if (argument.defaultValue.type() == response::Type::Null)
	{
		argumentDeclaration << R"cpp(	auto )cpp" << prefixToken << argumentName << R"cpp( = )cpp"
							<< getArgumentAccessType(argument) << R"cpp(::require)cpp"
							<< getTypeModifiers(argument.modifiers) << R"cpp((")cpp"
							<< argument.name << R"cpp(", )cpp" << argumentsToken << R"cpp();
)cpp";
	}
	else
	{
		argumentDeclaration << R"cpp(	auto pair)cpp" << argumentName << R"cpp( = )cpp"
							<< getArgumentAccessType(argument) << R"cpp(::find)cpp"
							<< getTypeModifiers(argument.modifiers) << R"cpp((")cpp"
							<< argument.name << R"cpp(", )cpp" << argumentsToken << R"cpp();
	auto )cpp" << prefixToken
							<< argumentName << R"cpp( = (pair)cpp" << argumentName << R"cpp(.second
		? std::move(pair)cpp"
							<< argumentName << R"cpp(.first)
		: )cpp" << getArgumentAccessType(argument)
							<< R"cpp(::require)cpp" << getTypeModifiers(argument.modifiers)
							<< R"cpp((")cpp" << argument.name << R"cpp(", )cpp" << defaultToken
							<< R"cpp());
)cpp";
	}

	return argumentDeclaration.str();
}

std::string Generator::getArgumentAccessType(const InputField& argument) const noexcept
{
	std::ostringstream argumentType;

	argumentType << R"cpp(service::ModifiedArgument<)cpp";

	switch (argument.fieldType)
	{
		case InputFieldType::Builtin:
			argumentType << _loader.getCppType(argument.type);
			break;

		case InputFieldType::Enum:
		case InputFieldType::Input:
			argumentType << _loader.getSchemaNamespace() << R"cpp(::)cpp"
						 << _loader.getCppType(argument.type);
			break;

		case InputFieldType::Scalar:
			argumentType << R"cpp(response::Value)cpp";
			break;
	}

	argumentType << R"cpp(>)cpp";

	return argumentType.str();
}

std::string Generator::getResultAccessType(const OutputField& result) const noexcept
{
	std::ostringstream resultType;

	resultType << R"cpp(service::ModifiedResult<)cpp";

	switch (result.fieldType)
	{
		case OutputFieldType::Builtin:
		case OutputFieldType::Enum:
		case OutputFieldType::Object:
			resultType << _loader.getCppType(result.type);
			break;

		case OutputFieldType::Scalar:
			resultType << R"cpp(response::Value)cpp";
			break;

		case OutputFieldType::Union:
		case OutputFieldType::Interface:
			resultType << R"cpp(service::Object)cpp";
			break;
	}

	resultType << R"cpp(>)cpp";

	return resultType.str();
}

std::string Generator::getTypeModifiers(const TypeModifierStack& modifiers) const noexcept
{
	bool firstValue = true;
	std::ostringstream typeModifiers;

	for (auto modifier : modifiers)
	{
		if (firstValue)
		{
			typeModifiers << R"cpp(<)cpp";
			firstValue = false;
		}
		else
		{
			typeModifiers << R"cpp(, )cpp";
		}

		switch (modifier)
		{
			case service::TypeModifier::Nullable:
				typeModifiers << R"cpp(service::TypeModifier::Nullable)cpp";
				break;

			case service::TypeModifier::List:
				typeModifiers << R"cpp(service::TypeModifier::List)cpp";
				break;

			case service::TypeModifier::None:
				break;
		}
	}

	if (!firstValue)
	{
		typeModifiers << R"cpp(>)cpp";
	}

	return typeModifiers.str();
}

std::string Generator::getIntrospectionType(
	std::string_view type, const TypeModifierStack& modifiers) const noexcept
{
	size_t wrapperCount = 0;
	bool nonNull = true;
	std::ostringstream introspectionType;

	for (auto modifier : modifiers)
	{
		if (nonNull)
		{
			switch (modifier)
			{
				case service::TypeModifier::None:
				case service::TypeModifier::List:
				{
					introspectionType << R"cpp(schema->WrapType()cpp"
									  << SchemaLoader::getIntrospectionNamespace()
									  << R"cpp(::TypeKind::NON_NULL, )cpp";
					++wrapperCount;
					break;
				}

				case service::TypeModifier::Nullable:
				{
					// If the next modifier is Nullable that cancels the non-nullable state.
					nonNull = false;
					break;
				}
			}
		}

		switch (modifier)
		{
			case service::TypeModifier::None:
			{
				nonNull = true;
				break;
			}

			case service::TypeModifier::List:
			{
				nonNull = true;
				introspectionType << R"cpp(schema->WrapType()cpp"
								  << SchemaLoader::getIntrospectionNamespace()
								  << R"cpp(::TypeKind::LIST, )cpp";
				++wrapperCount;
				break;
			}

			case service::TypeModifier::Nullable:
				break;
		}
	}

	if (nonNull)
	{
		introspectionType << R"cpp(schema->WrapType()cpp"
						  << SchemaLoader::getIntrospectionNamespace()
						  << R"cpp(::TypeKind::NON_NULL, )cpp";
		++wrapperCount;
	}

	introspectionType << R"cpp(schema->LookupType(")cpp" << type << R"cpp("))cpp";

	for (size_t i = 0; i < wrapperCount; ++i)
	{
		introspectionType << R"cpp())cpp";
	}

	return introspectionType.str();
}

std::vector<std::string> Generator::outputSeparateFiles() const noexcept
{
	std::vector<std::string> files;
	const fs::path headerDir(_headerDir);
	const fs::path sourceDir(_sourceDir);
	std::string_view queryType;

	for (const auto& operation : _loader.getOperationTypes())
	{
		if (operation.operation == service::strQuery)
		{
			queryType = operation.type;
			break;
		}
	}

	// Output a convenience header
	std::ofstream objectHeaderFile(_objectHeaderPath, std::ios_base::trunc);
	IncludeGuardScope includeGuard { objectHeaderFile,
		fs::path(_objectHeaderPath).filename().string() };

	objectHeaderFile << R"cpp(#include ")cpp" << fs::path(_headerPath).filename().string()
					 << R"cpp("

)cpp";

	for (const auto& objectType : _loader.getObjectTypes())
	{
		const auto headerFilename = std::string(objectType.cppType) + "Object.h";

		objectHeaderFile << R"cpp(#include ")cpp" << headerFilename << R"cpp("
)cpp";
	}

	if (_options.verbose)
	{
		files.push_back({ _objectHeaderPath });
	}

	for (const auto& objectType : _loader.getObjectTypes())
	{
		std::ostringstream ossNamespace;

		ossNamespace << R"cpp(graphql::)cpp" << _loader.getSchemaNamespace();

		const auto schemaNamespace = ossNamespace.str();

		ossNamespace << R"cpp(::object)cpp";

		const auto objectNamespace = ossNamespace.str();
		const bool isQueryType = objectType.type == queryType;
		const auto headerFilename = std::string(objectType.cppType) + "Object.h";
		auto headerPath = (headerDir / headerFilename).string();
		std::ofstream headerFile(headerPath, std::ios_base::trunc);
		IncludeGuardScope includeGuard { headerFile, headerFilename };

		headerFile << R"cpp(#include ")cpp" << fs::path(_headerPath).filename().string() << R"cpp("

)cpp";

		NamespaceScope headerNamespace { headerFile, objectNamespace };

		// Output the full declaration
		headerFile << std::endl;
		outputObjectDeclaration(headerFile, objectType, isQueryType);
		headerFile << std::endl;

		if (_options.verbose)
		{
			files.push_back(std::move(headerPath));
		}

		const auto sourceFilename = std::string(objectType.cppType) + "Object.cpp";
		auto sourcePath = (sourceDir / sourceFilename).string();
		std::ofstream sourceFile(sourcePath, std::ios_base::trunc);

		sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include ")cpp" << fs::path(_objectHeaderPath).filename().string()
				   << R"cpp("

#include "graphqlservice/introspection/Introspection.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

)cpp";

		NamespaceScope sourceSchemaNamespace { sourceFile, schemaNamespace };
		NamespaceScope sourceObjectNamespace { sourceFile, "object" };

		sourceFile << std::endl;
		outputObjectImplementation(sourceFile, objectType, isQueryType);
		sourceFile << std::endl;

		sourceObjectNamespace.exit();
		sourceFile << std::endl;

		sourceFile << R"cpp(void Add)cpp" << objectType.cppType
				   << R"cpp(Details(std::shared_ptr<schema::ObjectType> type)cpp"
				   << objectType.cppType << R"cpp(, const std::shared_ptr<schema::Schema>& schema)
{
)cpp";
		outputObjectIntrospection(sourceFile, objectType);
		sourceFile << R"cpp(}

)cpp";

		files.push_back(std::move(sourcePath));
	}

	return files;
}

} // namespace graphql::generator::schema

namespace po = boost::program_options;

void outputVersion(std::ostream& ostm)
{
	ostm << graphql::internal::FullVersion << std::endl;
}

void outputUsage(std::ostream& ostm, const po::options_description& options)
{
	ostm << "Usage:\tschemagen [options] <schema file> <output filename prefix> <output namespace>"
		 << std::endl;
	ostm << options;
}

int main(int argc, char** argv)
{
	po::options_description options("Command line options");
	po::positional_options_description positional;
	po::options_description internalOptions("Internal options");
	po::variables_map variables;
	bool showUsage = false;
	bool showVersion = false;
	bool buildIntrospection = false;
	bool buildCustom = false;
	bool noStubs = false;
	bool verbose = false;
	bool separateFiles = false;
	bool noIntrospection = false;
	std::string schemaFileName;
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
		"Schema definition file path")("prefix,p",
		po::value(&filenamePrefix),
		"Prefix to use for the generated C++ filenames")("namespace,n",
		po::value(&schemaNamespace),
		"C++ sub-namespace for the generated types")("source-dir",
		po::value(&sourceDir),
		"Target path for the <prefix>Schema.cpp source file")("header-dir",
		po::value(&headerDir),
		"Target path for the <prefix>Schema.h header file")("no-stubs",
		po::bool_switch(&noStubs),
		"Generate abstract classes without stub implementations")("separate-files",
		po::bool_switch(&separateFiles),
		"Generate separate files for each of the types")("no-introspection",
		po::bool_switch(&noIntrospection),
		"Do not generate support for Introspection");
	positional.add("schema", 1).add("prefix", 1).add("namespace", 1);
	internalOptions.add_options()("introspection",
		po::bool_switch(&buildIntrospection),
		"Generate IntrospectionSchema.*");
	internalOptions.add(options);

	try
	{
		po::store(po::command_line_parser(argc, argv)
					  .options(internalOptions)
					  .positional(positional)
					  .run(),
			variables);
		po::notify(variables);

		// If you specify any of these parameters, you must specify all three.
		buildCustom =
			!schemaFileName.empty() || !filenamePrefix.empty() || !schemaNamespace.empty();

		if (buildCustom)
		{
			if (schemaFileName.empty())
			{
				throw po::required_option("schema");
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
	else if (showUsage || (!buildIntrospection && !buildCustom))
	{
		outputUsage(std::cout, options);
		return 0;
	}

	try
	{
		if (buildIntrospection)
		{
			const auto files =
				graphql::generator::schema::Generator(std::nullopt, { std::nullopt, verbose })
					.Build();

			for (const auto& file : files)
			{
				std::cout << file << std::endl;
			}
		}

		if (buildCustom)
		{
			const auto files = graphql::generator::schema::Generator(
				std::make_optional(graphql::generator::SchemaOptions { std::move(schemaFileName),
					std::move(filenamePrefix),
					std::move(schemaNamespace) }),
				{
					graphql::generator::schema::GeneratorPaths { std::move(headerDir),
						std::move(sourceDir) },
					verbose,
					separateFiles,
					noStubs,
					noIntrospection,
				})
								   .Build();

			for (const auto& file : files)
			{
				std::cout << file << std::endl;
			}
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
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
