// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "SchemaGenerator.h"
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

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <ranges>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

using namespace std::literals;

namespace graphql::generator::schema {

Generator::Generator(SchemaOptions&& schemaOptions, GeneratorOptions&& options)
	: _loader(std::move(schemaOptions))
	, _options(std::move(options))
	, _headerDir(getHeaderDir())
	, _sourceDir(getSourceDir())
	, _schemaHeaderPath(getSchemaHeaderPath())
	, _schemaModulePath(getSchemaModulePath())
	, _schemaSourcePath(getSchemaSourcePath())
	, _sharedTypesHeaderPath(getSharedTypesHeaderPath())
	, _sharedTypesModulePath(getSharedTypesModulePath())
	, _sharedTypesSourcePath(getSharedTypesSourcePath())
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

std::string Generator::getSchemaHeaderPath() const noexcept
{
	std::filesystem::path fullPath { _headerDir };

	fullPath /= (std::string { _loader.getFilenamePrefix() } + "Schema.h");
	return fullPath.string();
}

std::string Generator::getSchemaModulePath() const noexcept
{
	std::filesystem::path fullPath { _headerDir };

	fullPath /= (std::string { _loader.getFilenamePrefix() } + "Schema.ixx");
	return fullPath.string();
}

std::string Generator::getSchemaSourcePath() const noexcept
{
	std::filesystem::path fullPath { _sourceDir };

	fullPath /= (std::string { _loader.getFilenamePrefix() } + "Schema.cpp");
	return fullPath.string();
}

std::string Generator::getSharedTypesHeaderPath() const noexcept
{
	std::filesystem::path fullPath { _headerDir };

	fullPath /= (std::string { _loader.getFilenamePrefix() } + "SharedTypes.h");
	return fullPath.string();
}

std::string Generator::getSharedTypesModulePath() const noexcept
{
	std::filesystem::path fullPath { _headerDir };

	fullPath /= (std::string { _loader.getFilenamePrefix() } + "SharedTypes.ixx");
	return fullPath.string();
}

std::string Generator::getSharedTypesSourcePath() const noexcept
{
	std::filesystem::path fullPath { _sourceDir };

	fullPath /= (std::string { _loader.getFilenamePrefix() } + "SharedTypes.cpp");
	return fullPath.string();
}

std::vector<std::string> Generator::Build() const noexcept
{
	std::vector<std::string> builtFiles;

	if (outputSharedTypesHeader() && _options.verbose)
	{
		builtFiles.push_back(_sharedTypesHeaderPath);
	}

	if (outputSharedTypesModule() && _options.verbose)
	{
		builtFiles.push_back(_sharedTypesModulePath);
	}

	if (outputSchemaHeader() && _options.verbose)
	{
		builtFiles.push_back(_schemaHeaderPath);
	}

	if (outputSchemaModule() && _options.verbose)
	{
		builtFiles.push_back(_schemaModulePath);
	}

	if (outputSharedTypesSource())
	{
		builtFiles.push_back(_sharedTypesSourcePath);
	}

	if (outputSchemaSource())
	{
		builtFiles.push_back(_schemaSourcePath);
	}

	auto separateFiles = outputSeparateFiles();

	for (auto& file : separateFiles)
	{
		builtFiles.push_back(std::move(file));
	}

	return builtFiles;
}

bool Generator::outputSchemaHeader() const noexcept
{
	std::ofstream headerFile(_schemaHeaderPath, std::ios_base::trunc);
	IncludeGuardScope includeGuard { headerFile,
		std::filesystem::path(_schemaHeaderPath).filename().string() };

	headerFile << R"cpp(#include "graphqlservice/GraphQLResponse.h"
#include "graphqlservice/GraphQLService.h"

)cpp";

	if (_loader.isIntrospection())
	{
		headerFile << R"cpp(#include "graphqlservice/internal/DllExports.h"
)cpp";
	}

	headerFile << R"cpp(#include "graphqlservice/internal/Version.h"
#include "graphqlservice/internal/Schema.h"
)cpp";

	if (!_loader.getEnumTypes().empty() || !_loader.getInputTypes().empty())
	{
		headerFile << R"cpp(
#include ")cpp" << _loader.getFilenamePrefix()
				   << R"cpp(SharedTypes.h"
)cpp";
	}

	headerFile << R"cpp(
#include <array>
#include <memory>
#include <string>
#include <string_view>

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

	const auto schemaNamespace = std::format(R"cpp(graphql::{})cpp", _loader.getSchemaNamespace());
	NamespaceScope schemaNamespaceScope { headerFile, schemaNamespace };
	NamespaceScope objectNamespace { headerFile, "object", true };
	PendingBlankLine pendingSeparator { headerFile };

	if (!_loader.getInterfaceTypes().empty())
	{
		objectNamespace.enter();
		headerFile << std::endl;

		// Forward declare all of the interface types
		for (const auto& interfaceType : _loader.getInterfaceTypes())
		{
			headerFile << R"cpp(class )cpp" << interfaceType.cppType << R"cpp(;
)cpp";
		}

		headerFile << std::endl;
	}

	if (!_loader.getUnionTypes().empty())
	{
		if (objectNamespace.enter())
		{
			headerFile << std::endl;
		}

		// Forward declare all of the union types
		for (const auto& unionType : _loader.getUnionTypes())
		{
			headerFile << R"cpp(class )cpp" << unionType.cppType << R"cpp(;
)cpp";
		}

		headerFile << std::endl;
	}

	if (!_loader.getObjectTypes().empty())
	{
		if (_loader.isIntrospection())
		{
			if (objectNamespace.exit())
			{
				headerFile << std::endl;
			}

			// Forward declare all of the concrete types for the Introspection schema
			for (const auto& objectType : _loader.getObjectTypes())
			{
				headerFile << R"cpp(class )cpp" << objectType.cppType << R"cpp(;
)cpp";
			}

			headerFile << std::endl;
		}

		if (objectNamespace.enter())
		{
			headerFile << std::endl;
		}

		// Forward declare all of the object types
		for (const auto& objectType : _loader.getObjectTypes())
		{
			headerFile << R"cpp(class )cpp" << objectType.cppType << R"cpp(;
)cpp";
		}

		headerFile << std::endl;
	}

	if (objectNamespace.exit())
	{
		headerFile << std::endl;
	}

	if (!_loader.isIntrospection())
	{
		bool hasSubscription = false;
		bool firstOperation = true;

		headerFile << R"cpp(class [[nodiscard("unnecessary construction")]] Operations final
	: public service::Request
{
public:
	explicit Operations()cpp";

		for (const auto& operation : _loader.getOperationTypes())
		{
			hasSubscription = hasSubscription || operation.operation == service::strSubscription;

			if (!firstOperation)
			{
				headerFile << R"cpp(, )cpp";
			}

			firstOperation = false;
			headerFile << R"cpp(std::shared_ptr<object::)cpp" << operation.cppType << R"cpp(> )cpp"
					   << operation.operation;
		}

		headerFile << R"cpp();
)cpp";

		if (!_loader.getOperationTypes().empty())
		{
			firstOperation = true;

			headerFile << R"cpp(
	template <)cpp";
			for (const auto& operation : _loader.getOperationTypes())
			{
				if (!firstOperation)
				{
					headerFile << R"cpp(, )cpp";
				}

				firstOperation = false;
				headerFile << R"cpp(class T)cpp" << operation.cppType;

				if (hasSubscription && operation.operation == service::strSubscription)
				{
					headerFile << R"cpp( = service::SubscriptionPlaceholder)cpp";
				}
			}

			headerFile << R"cpp(>
	explicit Operations()cpp";

			firstOperation = true;

			for (const auto& operation : _loader.getOperationTypes())
			{
				if (!firstOperation)
				{
					headerFile << R"cpp(, )cpp";
				}

				firstOperation = false;
				headerFile << R"cpp(std::shared_ptr<T)cpp" << operation.cppType << R"cpp(> )cpp"
						   << operation.operation;

				if (hasSubscription && operation.operation == service::strSubscription)
				{
					headerFile << R"cpp( = {})cpp";
				}
			}

			headerFile << R"cpp()
		: Operations {)cpp";

			firstOperation = true;

			for (const auto& operation : _loader.getOperationTypes())
			{
				if (!firstOperation)
				{
					headerFile << R"cpp(,)cpp";
				}

				firstOperation = false;

				if (hasSubscription && operation.operation == service::strSubscription)
				{
					headerFile << R"cpp(
			)cpp" << operation.operation
							   << R"cpp( ? std::make_shared<object::)cpp" << operation.cppType
							   << R"cpp(>(std::move()cpp" << operation.operation
							   << R"cpp()) : std::shared_ptr<object::)cpp" << operation.cppType
							   << R"cpp(> {})cpp";
				}
				else
				{
					headerFile << R"cpp(
			std::make_shared<object::)cpp"
							   << operation.cppType << R"cpp(>(std::move()cpp"
							   << operation.operation << R"cpp()))cpp";
				}
			}

			headerFile << R"cpp(
		}
	{
	}
)cpp";
		}

		headerFile << R"cpp(
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

	if (!_loader.getInterfaceTypes().empty())
	{
		for (const auto& interfaceType : _loader.getInterfaceTypes())
		{
			headerFile << R"cpp(void Add)cpp" << interfaceType.cppType
					   << R"cpp(Details(const std::shared_ptr<schema::InterfaceType>& type)cpp"
					   << interfaceType.cppType
					   << R"cpp(, const std::shared_ptr<schema::Schema>& schema);
)cpp";
		}

		headerFile << std::endl;
	}

	if (!_loader.getUnionTypes().empty())
	{
		for (const auto& unionType : _loader.getUnionTypes())
		{
			headerFile << R"cpp(void Add)cpp" << unionType.cppType
					   << R"cpp(Details(const std::shared_ptr<schema::UnionType>& type)cpp"
					   << unionType.cppType
					   << R"cpp(, const std::shared_ptr<schema::Schema>& schema);
)cpp";
		}

		headerFile << std::endl;
	}

	if (!_loader.getObjectTypes().empty())
	{
		for (const auto& objectType : _loader.getObjectTypes())
		{
			headerFile << R"cpp(void Add)cpp" << objectType.cppType
					   << R"cpp(Details(const std::shared_ptr<schema::ObjectType>& type)cpp"
					   << objectType.cppType
					   << R"cpp(, const std::shared_ptr<schema::Schema>& schema);
)cpp";
		}

		headerFile << std::endl;
	}

	if (_loader.isIntrospection())
	{
		headerFile
			<< R"cpp(GRAPHQLSERVICE_EXPORT void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema);

)cpp";
	}
	else
	{
		headerFile << R"cpp(std::shared_ptr<schema::Schema> GetSchema();

)cpp";
	}

	return true;
}

bool Generator::outputSchemaModule() const noexcept
{
	std::ofstream moduleFile(_schemaModulePath, std::ios_base::trunc);
	const auto schemaNamespace = std::format(R"cpp(graphql::{})cpp", _loader.getSchemaNamespace());

	moduleFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

module;

#include ")cpp" << _loader.getFilenamePrefix()
			   <<
		R"cpp(Schema.h"

export module GraphQL.)cpp"
			   << _loader.getFilenamePrefix() << R"cpp(.)cpp" << _loader.getFilenamePrefix() <<
		R"cpp(Schema;
)cpp";

	if (!_loader.getEnumTypes().empty() || !_loader.getInputTypes().empty())
	{
		moduleFile << R"cpp(
export import GraphQL.)cpp"
				   << _loader.getFilenamePrefix() << R"cpp(.)cpp" << _loader.getFilenamePrefix() <<
			R"cpp(SharedTypes;
)cpp";
	}

	PendingBlankLine pendingSeparator { moduleFile };

	if (!_loader.getInterfaceTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& interfaceType : _loader.getInterfaceTypes())
		{
			moduleFile << R"cpp(export import GraphQL.)cpp" << _loader.getFilenamePrefix()
					   << R"cpp(.)cpp" << interfaceType.cppType << R"cpp(Object;
)cpp";
		}
	}

	if (!_loader.getUnionTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& unionType : _loader.getUnionTypes())
		{
			moduleFile << R"cpp(export import GraphQL.)cpp" << _loader.getFilenamePrefix()
					   << R"cpp(.)cpp" << unionType.cppType << R"cpp(Object;
)cpp";
		}
	}

	if (!_loader.getObjectTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& objectType : _loader.getObjectTypes())
		{
			moduleFile << R"cpp(export import GraphQL.)cpp" << _loader.getFilenamePrefix()
					   << R"cpp(.)cpp" << objectType.cppType << R"cpp(Object;
)cpp";
		}
	}

	moduleFile << R"cpp(
export )cpp";

	NamespaceScope graphqlNamespace { moduleFile, schemaNamespace };

	pendingSeparator.add();

	if (!_loader.isIntrospection())
	{
		pendingSeparator.reset();

		moduleFile << R"cpp(using )cpp" << _loader.getSchemaNamespace() << R"cpp(::Operations;

)cpp";
	}

	if (!_loader.getInterfaceTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& interfaceType : _loader.getInterfaceTypes())
		{
			moduleFile << R"cpp(using )cpp" << _loader.getSchemaNamespace() << R"cpp(::Add)cpp"
					   << interfaceType.cppType << R"cpp(Details;
)cpp";
		}
	}

	if (!_loader.getUnionTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& unionType : _loader.getUnionTypes())
		{
			moduleFile << R"cpp(using )cpp" << _loader.getSchemaNamespace() << R"cpp(::Add)cpp"
					   << unionType.cppType << R"cpp(Details;
)cpp";
		}
	}

	if (!_loader.getObjectTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& objectType : _loader.getObjectTypes())
		{
			moduleFile << R"cpp(using )cpp" << _loader.getSchemaNamespace() << R"cpp(::Add)cpp"
					   << objectType.cppType << R"cpp(Details;
)cpp";
		}
	}

	if (_loader.isIntrospection())
	{
		moduleFile << R"cpp(
using )cpp" << _loader.getSchemaNamespace()
				   << R"cpp(::AddTypesToSchema;

)cpp";
	}
	else
	{
		moduleFile << R"cpp(
using )cpp" << _loader.getSchemaNamespace()
				   << R"cpp(::GetSchema;

)cpp";
	}

	return true;
}

bool Generator::outputSharedTypesHeader() const noexcept
{
	if (_loader.getEnumTypes().empty() && _loader.getInputTypes().empty())
	{
		return false;
	}

	std::ofstream headerFile(_sharedTypesHeaderPath, std::ios_base::trunc);
	IncludeGuardScope includeGuard { headerFile,
		std::filesystem::path(_sharedTypesHeaderPath).filename().string() };

	headerFile << R"cpp(#include "graphqlservice/GraphQLResponse.h"

)cpp";

	if (_loader.isIntrospection())
	{
		headerFile << R"cpp(#include "graphqlservice/internal/DllExports.h"
)cpp";
	}

	headerFile << R"cpp(#include "graphqlservice/internal/Version.h"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

	NamespaceScope graphqlNamespace { headerFile, "graphql" };
	NamespaceScope schemaNamespace { headerFile, _loader.getSchemaNamespace() };
	PendingBlankLine pendingSeparator { headerFile };

	if (!_loader.getEnumTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& enumType : _loader.getEnumTypes())
		{
			headerFile << R"cpp(enum class )cpp";

			if (!_loader.isIntrospection())
			{
				headerFile << R"cpp([[nodiscard("unnecessary conversion")]] )cpp";
			}

			headerFile << enumType.cppType << R"cpp(
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
				headerFile << R"cpp(	)cpp" << value.cppValue;
			}
			headerFile << R"cpp(
};

)cpp";

			headerFile << R"cpp([[nodiscard("unnecessary call")]] constexpr auto get)cpp"
					   << enumType.cppType << R"cpp(Names() noexcept
{
	using namespace std::literals;

	return std::array<std::string_view, )cpp"
					   << enumType.values.size() << R"cpp(> {
)cpp";

			firstValue = true;

			for (const auto& value : enumType.values)
			{
				if (!firstValue)
				{
					headerFile << R"cpp(,
)cpp";
				}

				firstValue = false;
				headerFile << R"cpp(		R"gql()cpp" << value.value << R"cpp()gql"sv)cpp";
			}

			headerFile << R"cpp(
	};
}

[[nodiscard("unnecessary call")]] constexpr auto get)cpp"
					   << enumType.cppType << R"cpp(Values() noexcept
{
	using namespace std::literals;

	return std::array<std::pair<std::string_view, )cpp"
					   << enumType.cppType << R"cpp(>, )cpp" << enumType.values.size() << R"cpp(> {
)cpp";

			std::vector<std::pair<std::string_view, std::string_view>> sortedValues(
				enumType.values.size());

			std::ranges::transform(enumType.values,
				sortedValues.begin(),
				[](const auto& value) noexcept {
					return std::make_pair(value.value, value.cppValue);
				});
			std::ranges::sort(sortedValues, [](const auto& lhs, const auto& rhs) noexcept {
				return internal::shorter_or_less {}(lhs.first, rhs.first);
			});

			firstValue = true;

			for (const auto& [enumName, enumValue] : sortedValues)
			{
				if (!firstValue)
				{
					headerFile << R"cpp(,
)cpp";
				}

				firstValue = false;
				headerFile << R"cpp(		std::make_pair(R"gql()cpp" << enumName
						   << R"cpp()gql"sv, )cpp" << enumType.cppType << R"cpp(::)cpp" << enumValue
						   << R"cpp())cpp";
			}

			headerFile << R"cpp(
	};
}

)cpp";
		}
	}

	if (!_loader.getInputTypes().empty())
	{
		pendingSeparator.reset();

		std::unordered_set<std::string_view> forwardDeclared;
		const auto introspectionExport =
			(_loader.isIntrospection() ? "GRAPHQLSERVICE_EXPORT "sv : ""sv);

		// Output the full declarations
		for (const auto& inputType : _loader.getInputTypes())
		{
			forwardDeclared.insert(inputType.cppType);

			if (!inputType.declarations.empty())
			{
				// Forward declare nullable dependencies
				for (auto declaration : inputType.declarations)
				{
					if (forwardDeclared.insert(declaration).second)
					{
						headerFile << R"cpp(struct )cpp" << declaration << R"cpp(;
)cpp";
						pendingSeparator.add();
					}
				}

				pendingSeparator.reset();
			}

			headerFile << R"cpp(struct [[nodiscard("unnecessary construction")]] )cpp"
					   << inputType.cppType << R"cpp(
{
	)cpp" << introspectionExport
					   << R"cpp(explicit )cpp" << inputType.cppType << R"cpp(()cpp";

			bool firstField = true;

			for (const auto& inputField : inputType.fields)
			{
				if (firstField)
				{
					headerFile << R"cpp() noexcept;
	explicit )cpp" << inputType.cppType
							   << R"cpp(()cpp";
				}
				else
				{
					headerFile << R"cpp(,)cpp";
				}

				firstField = false;

				const auto inputCppType = _loader.getInputCppType(inputField);

				headerFile << R"cpp(
		)cpp" << inputCppType
						   << R"cpp( )cpp" << inputField.cppName << R"cpp(Arg)cpp";
			}

			headerFile << R"cpp() noexcept;
	)cpp" << introspectionExport
					   << inputType.cppType << R"cpp((const )cpp" << inputType.cppType
					   << R"cpp(& other);
	)cpp" << introspectionExport
					   << inputType.cppType << R"cpp(()cpp" << inputType.cppType
					   << R"cpp(&& other) noexcept;
	~)cpp" << inputType.cppType
					   << R"cpp(();

	)cpp" << introspectionExport
					   << inputType.cppType << R"cpp(& operator=(const )cpp" << inputType.cppType
					   << R"cpp(& other);
	)cpp" << introspectionExport
					   << inputType.cppType << R"cpp(& operator=()cpp" << inputType.cppType
					   << R"cpp(&& other) noexcept;
)cpp";

			firstField = true;

			for (const auto& inputField : inputType.fields)
			{
				if (firstField)
				{
					headerFile << std::endl;
				}

				firstField = false;

				headerFile << getFieldDeclaration(inputField);
			}
			headerFile << R"cpp(};

)cpp";
		}
	}

	if (_loader.isIntrospection())
	{
		if (schemaNamespace.exit())
		{
			pendingSeparator.add();
		}

		pendingSeparator.reset();

		NamespaceScope serviceNamespace { headerFile, "service" };

		headerFile << R"cpp(
#ifdef GRAPHQL_DLLEXPORTS
// Export all of the built-in converters
)cpp";

		for (const auto& enumType : _loader.getEnumTypes())
		{
			headerFile << R"cpp(template <>
GRAPHQLSERVICE_EXPORT )cpp"
					   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp( Argument<)cpp" << _loader.getSchemaNamespace() << R"cpp(::)cpp"
					   << enumType.cppType << R"cpp(>::convert(
	const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT AwaitableResolver Result<)cpp"
					   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>::convert(
	AwaitableScalar<)cpp"
					   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(> result, ResolverParams&& params);
template <>
GRAPHQLSERVICE_EXPORT void Result<)cpp"
					   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>::validateScalar(
	const response::Value& value);
)cpp";
		}

		for (const auto& inputType : _loader.getInputTypes())
		{
			headerFile << R"cpp(template <>
GRAPHQLSERVICE_EXPORT )cpp"
					   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << inputType.cppType
					   << R"cpp( Argument<)cpp" << inputType.cppType << R"cpp(>::convert(
	const response::Value& value);
)cpp";
		}

		headerFile << R"cpp(#endif // GRAPHQL_DLLEXPORTS

)cpp";
	}

	return true;
}

bool Generator::outputSharedTypesModule() const noexcept
{
	if (_loader.getEnumTypes().empty() && _loader.getInputTypes().empty())
	{
		return false;
	}

	std::ofstream moduleFile(_sharedTypesModulePath, std::ios_base::trunc);
	const auto schemaNamespace = std::format(R"cpp(graphql::{})cpp", _loader.getSchemaNamespace());

	moduleFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

module;

#include ")cpp" << _loader.getFilenamePrefix()
			   <<
		R"cpp(SharedTypes.h"

export module GraphQL.)cpp"
			   << _loader.getFilenamePrefix() << R"cpp(.)cpp" << _loader.getFilenamePrefix() <<
		R"cpp(SharedTypes;
)cpp";

	PendingBlankLine pendingSeparator { moduleFile };

	moduleFile << R"cpp(
export )cpp";

	NamespaceScope graphqlNamespace { moduleFile, schemaNamespace };

	pendingSeparator.add();

	if (!_loader.getEnumTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& enumType : _loader.getEnumTypes())
		{
			moduleFile << R"cpp(using )cpp" << _loader.getSchemaNamespace() << R"cpp(::)cpp"
					   << enumType.cppType << R"cpp(;
using )cpp" << _loader.getSchemaNamespace()
					   << R"cpp(::get)cpp" << enumType.cppType << R"cpp(Names;
using )cpp" << _loader.getSchemaNamespace()
					   << R"cpp(::get)cpp" << enumType.cppType << R"cpp(Values;

)cpp";
		}
	}

	if (!_loader.getInputTypes().empty())
	{
		pendingSeparator.reset();

		for (const auto& inputType : _loader.getInputTypes())
		{
			moduleFile << R"cpp(using )cpp" << _loader.getSchemaNamespace() << R"cpp(::)cpp"
					   << inputType.cppType << R"cpp(;
)cpp";
		}

		moduleFile << std::endl;
	}

	return true;
}

void Generator::outputInterfaceDeclaration(std::ostream& headerFile, std::string_view cppType) const
{
	headerFile
		<< R"cpp(class [[nodiscard("unnecessary construction")]] )cpp" << cppType << R"cpp( final
	: public service::Object
{
private:
	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		[[nodiscard("unnecessary call")]] virtual service::TypeNames getTypeNames() const noexcept = 0;
		[[nodiscard("unnecessary call")]] virtual service::ResolverMap getResolvers() const noexcept = 0;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;
	};

	template <class T>
	struct [[nodiscard("unnecessary construction")]] Model final
		: Concept
	{
		explicit Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept override
		{
			return _pimpl->getTypeNames();
		}

		[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept override
		{
			return _pimpl->getResolvers();
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			_pimpl->beginSelectionSet(params);
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			_pimpl->endSelectionSet(params);
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit )cpp"
		<< cppType << R"cpp((std::unique_ptr<const Concept> pimpl) noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit )cpp"
		<< cppType << R"cpp((std::shared_ptr<T> pimpl) noexcept
		: )cpp"
		<< cppType
		<< R"cpp( { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
		static_assert(T::template implements<)cpp"
		<< cppType << R"cpp(>(), ")cpp" << cppType << R"cpp( is not implemented");
	}
};
)cpp";
}

void Generator::outputObjectModule(
	std::ostream& moduleFile, std::string_view objectNamespace, std::string_view cppType) const
{
	moduleFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

module;

#include ")cpp";

	if (_options.prefixedHeaders)
	{
		moduleFile << _loader.getFilenamePrefix();
	}

	moduleFile << cppType << R"cpp(Object.h"

export module GraphQL.)cpp"
			   << _loader.getFilenamePrefix() << R"cpp(.)cpp" << cppType << R"cpp(Object;

export namespace )cpp"
			   << objectNamespace << R"cpp( {

using object::)cpp"
			   << cppType << R"cpp(;

} // namespace )cpp"
			   << objectNamespace << R"cpp(
)cpp";
}

void Generator::outputObjectImplements(std::ostream& headerFile, const ObjectType& objectType) const
{
	headerFile << R"cpp(template <class I>
concept )cpp" << objectType.cppType
			   << R"cpp(Is = )cpp";

	bool firstInterface = true;

	for (auto interfaceName : objectType.interfaces)
	{
		if (!firstInterface)
		{
			headerFile << R"cpp( || )cpp";
		}

		headerFile << R"cpp(std::is_same_v<I, )cpp" << SchemaLoader::getSafeCppName(interfaceName)
				   << R"cpp(>)cpp";
		firstInterface = false;
	}

	for (auto unionName : objectType.unions)
	{
		if (!firstInterface)
		{
			headerFile << R"cpp( || )cpp";
		}

		headerFile << R"cpp(std::is_same_v<I, )cpp" << SchemaLoader::getSafeCppName(unionName)
				   << R"cpp(>)cpp";
		firstInterface = false;
	}

	headerFile << R"cpp(;

)cpp";
}

void Generator::outputObjectStubs(std::ostream& headerFile, const ObjectType& objectType) const
{
	for (const auto& outputField : objectType.fields)
	{
		const auto accessorName = SchemaLoader::getOutputCppAccessor(outputField);
		std::ostringstream ossPassedArguments;
		bool firstArgument = true;

		for (const auto& argument : outputField.arguments)
		{
			if (!firstArgument)
			{
				ossPassedArguments << R"cpp(, )cpp";
			}

			ossPassedArguments << R"cpp(std::move()cpp" << argument.cppName << R"cpp(Arg))cpp";
			firstArgument = false;
		}

		const auto passedArguments = ossPassedArguments.str();

		headerFile << R"cpp(
template <class TImpl>
concept )cpp" << accessorName
				   << R"cpp(WithParams = requires (TImpl impl, service::FieldParams params)cpp";
		for (const auto& argument : outputField.arguments)
		{
			headerFile << R"cpp(, )cpp" << _loader.getInputCppType(argument) << R"cpp( )cpp"
					   << argument.cppName << R"cpp(Arg)cpp";
		}

		headerFile << R"cpp()
{
	{ )cpp" << _loader.getOutputCppType(outputField)
				   << R"cpp( { impl.)cpp" << accessorName << R"cpp((std::move(params))cpp";

		if (!passedArguments.empty())
		{
			headerFile << R"cpp(, )cpp" << passedArguments;
		}

		headerFile << R"cpp() } };
};

template <class TImpl>
concept )cpp" << accessorName
				   << R"cpp( = requires (TImpl impl)cpp";
		for (const auto& argument : outputField.arguments)
		{
			headerFile << R"cpp(, )cpp" << _loader.getInputCppType(argument) << R"cpp( )cpp"
					   << argument.cppName << R"cpp(Arg)cpp";
		}

		headerFile << R"cpp()
{
	{ )cpp" << _loader.getOutputCppType(outputField)
				   << R"cpp( { impl.)cpp" << accessorName << R"cpp(()cpp";

		if (!passedArguments.empty())
		{
			headerFile << passedArguments;
		}

		headerFile << R"cpp() } };
};
)cpp";
	}

	headerFile << R"cpp(
template <class TImpl>
concept beginSelectionSet = requires (TImpl impl, const service::SelectionSetParams params)
{
	{ impl.beginSelectionSet(params) };
};

template <class TImpl>
concept endSelectionSet = requires (TImpl impl, const service::SelectionSetParams params)
{
	{ impl.endSelectionSet(params) };
};

)cpp";
}

void Generator::outputObjectDeclaration(std::ostream& headerFile, const ObjectType& objectType,
	bool isQueryType, bool isSubscriptionType) const
{
	headerFile << R"cpp(class [[nodiscard("unnecessary construction")]] )cpp" << objectType.cppType
			   << R"cpp( final
	: public service::Object
{
private:
)cpp";

	for (const auto& outputField : objectType.fields)
	{
		headerFile << getResolverDeclaration(outputField);
	}

	headerFile << R"cpp(
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;
)cpp";

	if (!_options.noIntrospection && isQueryType)
	{
		headerFile
			<< R"cpp(	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_schema(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_type(service::ResolverParams&& params) const;

	std::shared_ptr<schema::Schema> _schema;
)cpp";
	}

	headerFile << R"cpp(
	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

)cpp";

	if (!_loader.isIntrospection())
	{
		headerFile
			<< R"cpp(		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

)cpp";
	}

	for (const auto& outputField : objectType.fields)
	{
		headerFile << getFieldDeclaration(outputField);
	}

	headerFile << R"cpp(	};

	template <class T>
	struct [[nodiscard("unnecessary construction")]] Model final
		: Concept
	{
		explicit Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
)cpp";

	if (isSubscriptionType && !_options.stubs)
	{
		headerFile << R"cpp(			static_assert()cpp";

		bool firstField = true;

		for (const auto& outputField : objectType.fields)
		{
			const auto accessorName = SchemaLoader::getOutputCppAccessor(outputField);

			if (!firstField)
			{
				headerFile << R"cpp(
				|| )cpp";
			}

			firstField = false;
			headerFile << R"cpp(methods::)cpp" << objectType.cppType << R"cpp(Has::)cpp"
					   << accessorName << R"cpp(WithParams<T>
				|| methods::)cpp"
					   << objectType.cppType << R"cpp(Has::)cpp" << accessorName << R"cpp(<T>)cpp";
		}

		headerFile << R"cpp(, R"msg()cpp" << objectType.cppType
				   << R"cpp( fields are not implemented)msg");
)cpp";
	}

	headerFile << R"cpp(		}
)cpp";

	for (const auto& outputField : objectType.fields)
	{
		const auto accessorName = SchemaLoader::getOutputCppAccessor(outputField);

		headerFile << R"cpp(
		[[nodiscard("unnecessary call")]] )cpp"
				   << _loader.getOutputCppType(outputField) << R"cpp( )cpp" << accessorName
				   << R"cpp(()cpp";

		bool firstArgument = _loader.isIntrospection();

		if (!firstArgument)
		{
			headerFile << R"cpp(service::FieldParams&& params)cpp";
		}

		for (const auto& argument : outputField.arguments)
		{
			if (!firstArgument)
			{
				headerFile << R"cpp(, )cpp";
			}

			headerFile << _loader.getInputCppType(argument) << R"cpp(&& )cpp" << argument.cppName
					   << R"cpp(Arg)cpp";
			firstArgument = false;
		}

		headerFile << R"cpp() const override
		{
			)cpp";

		std::ostringstream ossPassedArguments;
		firstArgument = true;

		for (const auto& argument : outputField.arguments)
		{
			if (!firstArgument)
			{
				ossPassedArguments << R"cpp(, )cpp";
			}

			ossPassedArguments << R"cpp(std::move()cpp" << argument.cppName << R"cpp(Arg))cpp";
			firstArgument = false;
		}

		const auto passedArguments = ossPassedArguments.str();

		if (_loader.isIntrospection())
		{
			headerFile << R"cpp(return { _pimpl->)cpp" << accessorName << R"cpp(()cpp";

			if (!passedArguments.empty())
			{
				headerFile << passedArguments;
			}

			headerFile << R"cpp() };)cpp";
		}
		else
		{
			headerFile << R"cpp(if constexpr (methods::)cpp" << objectType.cppType
					   << R"cpp(Has::)cpp" << accessorName << R"cpp(WithParams<T>)
			{
				return { _pimpl->)cpp"
					   << accessorName << R"cpp((std::move(params))cpp";

			if (!passedArguments.empty())
			{
				headerFile << R"cpp(, )cpp" << passedArguments;
			}

			headerFile << R"cpp() };
			}
			else)cpp";

			if (!isSubscriptionType && !_options.stubs)
			{
				headerFile << R"cpp(
			{
				static_assert(methods::)cpp"
						   << objectType.cppType << R"cpp(Has::)cpp" << accessorName
						   << R"cpp(<T>, R"msg()cpp" << objectType.cppType << R"cpp(::)cpp"
						   << accessorName << R"cpp( is not implemented)msg");)cpp";
			}
			else
			{
				headerFile << R"cpp( if constexpr (methods::)cpp" << objectType.cppType
						   << R"cpp(Has::)cpp" << accessorName << R"cpp(<T>)
			{)cpp";
			}

			headerFile << R"cpp(
				return { _pimpl->)cpp"
					   << accessorName << R"cpp(()cpp";

			if (!passedArguments.empty())
			{
				headerFile << passedArguments;
			}

			headerFile << R"cpp() };
			})cpp";

			if (isSubscriptionType || _options.stubs)
			{
				headerFile << R"cpp(
			else
			{
				throw service::unimplemented_method(R"ex()cpp"
						   << objectType.cppType << R"cpp(::)cpp" << accessorName << R"cpp()ex");
			})cpp";
			}
		}

		headerFile << R"cpp(
		}
)cpp";
	}

	if (!_loader.isIntrospection())
	{
		headerFile << R"cpp(
		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::)cpp"
				   << objectType.cppType << R"cpp(Has::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::)cpp"
				   << objectType.cppType << R"cpp(Has::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}
)cpp";
	}

	headerFile << R"cpp(
	private:
		const std::shared_ptr<T> _pimpl;
	};

)cpp";

	if (_loader.isIntrospection())
	{
		headerFile << R"cpp(	const std::unique_ptr<const Concept> _pimpl;

	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

public:
	GRAPHQLSERVICE_EXPORT explicit )cpp"
				   << objectType.cppType << R"cpp((std::shared_ptr<)cpp"
				   << SchemaLoader::getIntrospectionNamespace() << R"cpp(::)cpp"
				   << objectType.cppType << R"cpp(> pimpl) noexcept;
	GRAPHQLSERVICE_EXPORT ~)cpp"
				   << objectType.cppType << R"cpp(();
};
)cpp";
	}
	else
	{
		headerFile << R"cpp(	explicit )cpp" << objectType.cppType
				   << R"cpp((std::unique_ptr<const Concept> pimpl) noexcept;

)cpp";

		if (!objectType.interfaces.empty())
		{
			headerFile << R"cpp(	// Interfaces which this type implements
)cpp";

			for (auto interfaceName : objectType.interfaces)
			{
				headerFile << R"cpp(	friend )cpp" << SchemaLoader::getSafeCppName(interfaceName)
						   << R"cpp(;
)cpp";
			}

			headerFile << std::endl;
		}

		if (!objectType.unions.empty())
		{
			headerFile << R"cpp(	// Unions which include this type
)cpp";

			for (auto unionName : objectType.unions)
			{
				headerFile << R"cpp(	friend )cpp" << SchemaLoader::getSafeCppName(unionName)
						   << R"cpp(;
)cpp";
			}

			headerFile << std::endl;
		}

		if (!objectType.interfaces.empty() || !objectType.unions.empty())
		{

			headerFile << R"cpp(	template <class I>
	[[nodiscard("unnecessary call")]] static constexpr bool implements() noexcept
	{
		return implements::)cpp"
					   << objectType.cppType << R"cpp(Is<I>;
	}

)cpp";
		}

		headerFile
			<< R"cpp(	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit )cpp"
			<< objectType.cppType << R"cpp((std::shared_ptr<T> pimpl) noexcept
		: )cpp"
			<< objectType.cppType
			<< R"cpp( { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard("unnecessary call")]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql()cpp"
			<< objectType.type << R"cpp()gql" };
	}
};
)cpp";
	}
}

std::string Generator::getFieldDeclaration(const InputField& inputField) const noexcept
{
	return std::format(R"cpp(	{} {};
)cpp",
		_loader.getInputCppType(inputField),
		inputField.cppName);
}

std::string Generator::getFieldDeclaration(const OutputField& outputField) const noexcept
{
	std::ostringstream output;
	const auto accessorName = SchemaLoader::getOutputCppAccessor(outputField);

	output << R"cpp(		[[nodiscard("unnecessary call")]] virtual )cpp"
		   << _loader.getOutputCppType(outputField) << R"cpp( )cpp" << accessorName << R"cpp(()cpp";

	bool firstArgument = _loader.isIntrospection();

	if (!firstArgument)
	{
		output << R"cpp(service::FieldParams&& params)cpp";
	}

	for (const auto& argument : outputField.arguments)
	{
		if (!firstArgument)
		{
			output << R"cpp(, )cpp";
		}

		output << _loader.getInputCppType(argument) << R"cpp(&& )cpp" << argument.cppName << "Arg";
		firstArgument = false;
	}

	output << R"cpp() const = 0;
)cpp";

	return output.str();
}

std::string Generator::getResolverDeclaration(const OutputField& outputField) const noexcept
{
	const auto resolverName = SchemaLoader::getOutputCppResolver(outputField);

	return std::format(
		R"cpp(	[[nodiscard("unnecessary call")]] service::AwaitableResolver {}(service::ResolverParams&& params) const;
)cpp",
		resolverName);
}

bool Generator::outputSharedTypesSource() const noexcept
{
	if (_loader.getEnumTypes().empty() && _loader.getInputTypes().empty())
	{
		return false;
	}

	std::ofstream sourceFile(_sharedTypesSourcePath, std::ios_base::trunc);

	sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "graphqlservice/GraphQLService.h"

#include ")cpp" << getSharedTypesHeaderPath()
			   << R"cpp("

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

)cpp";

	NamespaceScope graphqlNamespace { sourceFile, "graphql" };
	NamespaceScope serviceNamespace { sourceFile, "service" };
	PendingBlankLine pendingSeparator { sourceFile };

	pendingSeparator.reset();

	for (const auto& enumType : _loader.getEnumTypes())
	{
		sourceFile << R"cpp(static const auto s_names)cpp" << enumType.cppType << R"cpp( = )cpp"
				   << _loader.getSchemaNamespace() << R"cpp(::get)cpp" << enumType.cppType
				   << R"cpp(Names();
static const auto s_values)cpp"
				   << enumType.cppType << R"cpp( = )cpp" << _loader.getSchemaNamespace()
				   << R"cpp(::get)cpp" << enumType.cppType << R"cpp(Values();

template <>
)cpp" << _loader.getSchemaNamespace()
				   << R"cpp(::)cpp" << enumType.cppType << R"cpp( Argument<)cpp"
				   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
				   << R"cpp(>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid )cpp"
				   << enumType.type << R"cpp( value)ex" } };
	}

	const auto result = internal::sorted_map_lookup<internal::shorter_or_less>(
		s_values)cpp"
				   << enumType.cppType << R"cpp(,
		std::string_view { value.get<std::string>() });

	if (!result)
	{
		throw service::schema_exception { { R"ex(not a valid )cpp"
				   << enumType.type << R"cpp( value)ex" } };
	}

	return *result;
}

template <>
service::AwaitableResolver Result<)cpp"
				   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
				   << R"cpp(>::convert(service::AwaitableScalar<)cpp"
				   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
				   << R"cpp(> result, ResolverParams&& params)
{
	return ModifiedResult<)cpp"
				   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
				   << R"cpp(>::resolve(std::move(result), std::move(params),
		[]()cpp" << _loader.getSchemaNamespace()
				   << R"cpp(::)cpp" << enumType.cppType << R"cpp( value, const ResolverParams&)
		{
			const size_t idx = static_cast<size_t>(value);
			if (idx >= s_names)cpp"
					   << enumType.cppType << R"cpp(.size())
			{
				throw service::schema_exception { { R"ex(Enum value out of range for )cpp"
					   << enumType.type << R"cpp()ex" } };
			}
			return ResolverResult { { response::ValueToken::EnumValue { std::string { s_names)cpp"
				   << enumType.cppType << R"cpp([idx] } } } };
		});
}

template <>
void Result<)cpp" << _loader.getSchemaNamespace()
				   << R"cpp(::)cpp" << enumType.cppType
				   << R"cpp(>::validateScalar(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { R"ex(not a valid )cpp"
				   << enumType.type << R"cpp( value)ex" } };
	}

	const auto [itr, itrEnd] = internal::sorted_map_equal_range<internal::shorter_or_less>(
		s_values)cpp"
				   << enumType.cppType << R"cpp(.begin(),
		s_values)cpp"
				   << enumType.cppType << R"cpp(.end(),
		std::string_view { value.get<std::string>() });

	if (itr == itrEnd)
	{
		throw service::schema_exception { { R"ex(not a valid )cpp"
				   << enumType.type << R"cpp( value)ex" } };
	}
}

)cpp";
	}

	for (const auto& inputType : _loader.getInputTypes())
	{
		bool firstField = true;

		sourceFile << R"cpp(template <>
)cpp" << _loader.getSchemaNamespace()
				   << R"cpp(::)cpp" << inputType.cppType << R"cpp( Argument<)cpp"
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

		sourceFile << R"cpp(	return )cpp" << _loader.getSchemaNamespace() << R"cpp(::)cpp"
				   << inputType.cppType << R"cpp( {
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

			const bool shouldMove = SchemaLoader::shouldMoveInputField(inputField);

			firstField = false;
			fieldName[0] =
				static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));

			sourceFile << R"cpp(		)cpp";

			if (shouldMove)
			{
				sourceFile << R"cpp(std::move()cpp";
			}

			sourceFile << R"cpp(value)cpp" << fieldName;

			if (shouldMove)
			{
				sourceFile << R"cpp())cpp";
			}
		}

		sourceFile << R"cpp(
	};
}

)cpp";
	}

	serviceNamespace.exit();
	pendingSeparator.add();

	if (!_loader.getInputTypes().empty())
	{
		pendingSeparator.reset();

		NamespaceScope schemaNamespace { sourceFile, _loader.getSchemaNamespace() };

		for (const auto& inputType : _loader.getInputTypes())
		{
			sourceFile << std::endl
					   << inputType.cppType << R"cpp(::)cpp" << inputType.cppType
					   << R"cpp(() noexcept)cpp";

			bool firstField = true;

			for (const auto& inputField : inputType.fields)
			{
				sourceFile << R"cpp(
	)cpp" << (firstField ? R"cpp(:)cpp" : R"cpp(,)cpp")
						   << R"cpp( )cpp" << inputField.cppName << R"cpp( {})cpp";
				firstField = false;
			}

			sourceFile << R"cpp(
{
	// Explicit definition to prevent ODR violations when LTO is enabled.
}

)cpp" << inputType.cppType
					   << R"cpp(::)cpp" << inputType.cppType << R"cpp(()cpp";

			firstField = true;

			for (const auto& inputField : inputType.fields)
			{
				if (!firstField)
				{
					sourceFile << R"cpp(,)cpp";
				}

				firstField = false;
				sourceFile << R"cpp(
		)cpp" << _loader.getInputCppType(inputField)
						   << R"cpp( )cpp" << inputField.cppName << R"cpp(Arg)cpp";
			}

			sourceFile << R"cpp() noexcept
)cpp";

			firstField = true;

			for (const auto& inputField : inputType.fields)
			{
				sourceFile << (firstField ? R"cpp(	: )cpp" : R"cpp(	, )cpp");
				firstField = false;

				sourceFile << inputField.cppName << R"cpp( { std::move()cpp" << inputField.cppName
						   << R"cpp(Arg) }
)cpp";
			}

			sourceFile << R"cpp({
}

)cpp" << inputType.cppType
					   << R"cpp(::)cpp" << inputType.cppType << R"cpp((const )cpp"
					   << inputType.cppType << R"cpp(& other)
)cpp";

			firstField = true;

			for (const auto& inputField : inputType.fields)
			{
				sourceFile << (firstField ? R"cpp(	: )cpp" : R"cpp(	, )cpp");
				firstField = false;

				sourceFile << inputField.cppName << R"cpp( { service::ModifiedArgument<)cpp"
						   << _loader.getCppType(inputField.type) << R"cpp(>::duplicate)cpp";

				if (!inputField.modifiers.empty())
				{
					bool firstModifier = true;

					for (const auto modifier : inputField.modifiers)
					{
						sourceFile << (firstModifier ? R"cpp(<)cpp" : R"cpp(, )cpp");
						firstModifier = false;

						switch (modifier)
						{
							case service::TypeModifier::None:
								sourceFile << R"cpp(service::TypeModifier::None)cpp";
								break;

							case service::TypeModifier::Nullable:
								sourceFile << R"cpp(service::TypeModifier::Nullable)cpp";
								break;

							case service::TypeModifier::List:
								sourceFile << R"cpp(service::TypeModifier::List)cpp";
								break;
						}
					}

					sourceFile << R"cpp(>)cpp";
				}

				sourceFile << R"cpp((other.)cpp" << inputField.cppName << R"cpp() }
)cpp";
			}

			sourceFile << R"cpp({
}

)cpp" << inputType.cppType
					   << R"cpp(::)cpp" << inputType.cppType << R"cpp(()cpp" << inputType.cppType
					   << R"cpp(&& other) noexcept
)cpp";

			firstField = true;

			for (const auto& inputField : inputType.fields)
			{
				sourceFile << (firstField ? R"cpp(	: )cpp" : R"cpp(	, )cpp");
				firstField = false;

				sourceFile << inputField.cppName << R"cpp( { std::move(other.)cpp"
						   << inputField.cppName << R"cpp() }
)cpp";
			}

			sourceFile << R"cpp({
}

)cpp" << inputType.cppType
					   << R"cpp(::~)cpp" << inputType.cppType << R"cpp(()
{
	// Explicit definition to prevent ODR violations when LTO is enabled.
}

)cpp" << inputType.cppType
					   << R"cpp(& )cpp" << inputType.cppType << R"cpp(::operator=(const )cpp"
					   << inputType.cppType << R"cpp(& other)
{
	)cpp" << inputType.cppType
					   << R"cpp( value { other };

	std::swap(*this, value);

	return *this;
}

)cpp" << inputType.cppType
					   << R"cpp(& )cpp" << inputType.cppType << R"cpp(::operator=()cpp"
					   << inputType.cppType << R"cpp(&& other) noexcept
{
)cpp";

			for (const auto& inputField : inputType.fields)
			{
				sourceFile << R"cpp(	)cpp" << inputField.cppName
						   << R"cpp( = std::move(other.)cpp" << inputField.cppName << R"cpp();
)cpp";
			}

			sourceFile << R"cpp(
	return *this;
}

)cpp";
		}
	}

	return true;
}

bool Generator::outputSchemaSource() const noexcept
{
	std::ofstream sourceFile(_schemaSourcePath, std::ios_base::trunc);

	sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

)cpp";

	if (!_loader.isIntrospection())
	{
		if (_loader.getOperationTypes().empty())
		{
			// Normally this would be included by each of the operation object headers.
			sourceFile << R"cpp(#include ")cpp" << getSchemaHeaderPath() << R"cpp("
)cpp";
		}
		else
		{
			for (const auto& operation : _loader.getOperationTypes())
			{
				sourceFile << R"cpp(#include ")cpp";

				if (_options.prefixedHeaders)
				{
					sourceFile << _loader.getFilenamePrefix();
				}

				sourceFile << operation.cppType << R"cpp(Object.h"
)cpp";
			}
		}

		sourceFile << std::endl;
	}

	if (_loader.isIntrospection())
	{
		sourceFile << R"cpp(#include "graphqlservice/internal/Introspection.h"
)cpp";
	}
	else
	{
		sourceFile << R"cpp(#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"
)cpp";
	}

	sourceFile << R"cpp(
#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

)cpp";

	const auto schemaNamespace = std::format(R"cpp(graphql::{})cpp", _loader.getSchemaNamespace());
	NamespaceScope schemaNamespaceScope { sourceFile, schemaNamespace };

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

			std::string operationName(operation.operation);

			operationName[0] =
				static_cast<char>(std::toupper(static_cast<unsigned char>(operationName[0])));
			sourceFile << R"cpp(		{ service::str)cpp" << operationName << R"cpp(, )cpp"
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
		for (const auto& [typeName, builtinType] : SchemaLoader::getBuiltinTypes())
		{
			sourceFile << R"cpp(	schema->AddType(R"gql()cpp" << typeName
					   << R"cpp()gql"sv, schema::ScalarType::Make(R"gql()cpp" << typeName
					   << R"cpp()gql"sv, R"md()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << R"cpp(Built-in type)cpp";
			}

			sourceFile << R"cpp()md"sv, R"url()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << R"cpp(https://spec.graphql.org/October2021/#sec-)cpp" << typeName;
			}

			sourceFile << R"cpp()url"sv));
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

			sourceFile << R"cpp()md", R"url()cpp";

			if (!_options.noIntrospection)
			{
				sourceFile << scalarType.specifiedByURL;
			}

			sourceFile << R"cpp()url"sv));
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

			sourceFile << R"cpp()md"sv);
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

				sourceFile << R"cpp(	static const auto s_names)cpp" << enumType.cppType
						   << R"cpp( = get)cpp" << enumType.cppType << R"cpp(Names();
	type)cpp" << enumType.cppType
						   << R"cpp(->AddEnumValues({
)cpp";

				for (const auto& enumValue : enumType.values)
				{
					if (!firstValue)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstValue = false;
					sourceFile << R"cpp(		{ s_names)cpp" << enumType.cppType
							   << R"cpp([static_cast<std::size_t>()cpp"
							   << _loader.getSchemaNamespace() << R"cpp(::)cpp" << enumType.cppType
							   << R"cpp(::)cpp" << enumValue.cppValue << R"cpp()], R"md()cpp";

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

	if (!_loader.getInterfaceTypes().empty())
	{
		sourceFile << std::endl;

		for (const auto& interfaceType : _loader.getInterfaceTypes())
		{
			sourceFile << R"cpp(	Add)cpp" << interfaceType.cppType << R"cpp(Details(type)cpp"
					   << interfaceType.cppType << R"cpp(, schema);
)cpp";
		}
	}

	if (!_loader.getUnionTypes().empty())
	{
		sourceFile << std::endl;

		for (const auto& unionType : _loader.getUnionTypes())
		{
			sourceFile << R"cpp(	Add)cpp" << unionType.cppType << R"cpp(Details(type)cpp"
					   << unionType.cppType << R"cpp(, schema);
)cpp";
		}
	}

	if (!_loader.getObjectTypes().empty())
	{
		sourceFile << std::endl;

		for (const auto& objectType : _loader.getObjectTypes())
		{
			sourceFile << R"cpp(	Add)cpp" << objectType.cppType << R"cpp(Details(type)cpp"
					   << objectType.cppType << R"cpp(, schema);
)cpp";
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

			sourceFile << R"cpp(}, {)cpp";

			if (!directive.arguments.empty())
			{
				bool firstArgument = true;

				sourceFile << std::endl;

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
	)cpp";
			}
			sourceFile << R"cpp(}, )cpp"
					   << (directive.isRepeatable ? R"cpp(true)cpp" : R"cpp(false)cpp")
					   << R"cpp());
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
				   << (_options.noIntrospection ? R"cpp(true)cpp" : R"cpp(false)cpp")
				   << R"cpp(, R"md()cpp";

		if (!_options.noIntrospection)
		{
			sourceFile << _loader.getSchemaDescription();
		}

		sourceFile << R"cpp()md"sv);
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

void Generator::outputInterfaceImplementation(
	std::ostream& sourceFile, std::string_view cppType) const
{
	// Output the private constructor which calls through to the service::Object constructor
	// with arguments that declare the set of types it implements and bind the fields to the
	// resolver methods.
	sourceFile << cppType << R"cpp(::)cpp" << cppType
			   << R"cpp((std::unique_ptr<const Concept> pimpl) noexcept
	: service::Object { pimpl->getTypeNames(), pimpl->getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}
)cpp";

	sourceFile << R"cpp(
void )cpp" << cppType
			   << R"cpp(::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void )cpp" << cppType
			   << R"cpp(::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}
)cpp";
}

void Generator::outputInterfaceIntrospection(
	std::ostream& sourceFile, const InterfaceType& interfaceType) const
{
	outputIntrospectionInterfaces(sourceFile, interfaceType.cppType, interfaceType.interfaces);
	outputIntrospectionFields(sourceFile, interfaceType.cppType, interfaceType.fields);
}

void Generator::outputUnionIntrospection(std::ostream& sourceFile, const UnionType& unionType) const
{
	if (unionType.options.empty())
	{
		return;
	}

	bool firstValue = true;

	sourceFile << R"cpp(	type)cpp" << unionType.cppType << R"cpp(->AddPossibleTypes({
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

void Generator::outputObjectImplementation(
	std::ostream& sourceFile, const ObjectType& objectType, bool isQueryType) const
{
	using namespace std::literals;

	if (_loader.isIntrospection())
	{
		// Output the public constructor which calls through to the service::Object constructor
		// with arguments that declare the set of types it implements and bind the fields to the
		// resolver methods.
		sourceFile << objectType.cppType << R"cpp(::)cpp" << objectType.cppType
				   << R"cpp((std::shared_ptr<)cpp" << SchemaLoader::getIntrospectionNamespace()
				   << R"cpp(::)cpp" << objectType.cppType << R"cpp(> pimpl))cpp";
	}
	else
	{
		// Output the private constructor which calls through to the service::Object constructor
		// with arguments that declare the set of types it implements and bind the fields to the
		// resolver methods.
		sourceFile << objectType.cppType << R"cpp(::)cpp" << objectType.cppType
				   << R"cpp((std::unique_ptr<const Concept> pimpl))cpp";
	}

	sourceFile << R"cpp( noexcept
	: service::Object{ getTypeNames(), getResolvers() })cpp";

	if (!_options.noIntrospection && isQueryType)
	{
		sourceFile << R"cpp(
	, _schema { GetSchema() })cpp";
	}

	if (_loader.isIntrospection())
	{
		sourceFile << R"cpp(
	, _pimpl { std::make_unique<Model<)cpp"
				   << SchemaLoader::getIntrospectionNamespace() << R"cpp(::)cpp"
				   << objectType.cppType << R"cpp(>>(std::move(pimpl)) })cpp";
	}
	else
	{
		sourceFile << R"cpp(
	, _pimpl { std::move(pimpl) })cpp";
	}
	sourceFile << R"cpp(
{
}
)cpp";

	if (_loader.isIntrospection())
	{
		sourceFile << R"cpp(
)cpp" << objectType.cppType
				   << R"cpp(::~)cpp" << objectType.cppType << R"cpp(()
{
	// This is empty, but explicitly defined here so that it can access the un-exported destructor
	// of the implementation type.
}
)cpp";
	}

	sourceFile << R"cpp(
service::TypeNames )cpp"
			   << objectType.cppType << R"cpp(::getTypeNames() const noexcept
{
	return {
)cpp";

	for (const auto& interfaceName : objectType.interfaces)
	{
		sourceFile << R"cpp(		R"gql()cpp" << interfaceName << R"cpp()gql"sv,
)cpp";
	}

	for (const auto& unionName : objectType.unions)
	{
		sourceFile << R"cpp(		R"gql()cpp" << unionName << R"cpp()gql"sv,
)cpp";
	}

	sourceFile << R"cpp(		R"gql()cpp" << objectType.type << R"cpp()gql"sv
	};
}

service::ResolverMap )cpp"
			   << objectType.cppType << R"cpp(::getResolvers() const noexcept
{
	return {
)cpp";

	std::map<std::string_view, std::string, internal::shorter_or_less> resolvers;

	std::ranges::transform(objectType.fields,
		std::inserter(resolvers, resolvers.begin()),
		[](const OutputField& outputField) noexcept {
			const auto resolverName = SchemaLoader::getOutputCppResolver(outputField);
			auto output = std::format(
				R"cpp(		{{ R"gql({})gql"sv, [this](service::ResolverParams&& params) {{ return {}(std::move(params)); }} }})cpp",
				outputField.name,
				resolverName);

			return std::make_pair(std::string_view { outputField.name }, std::move(output));
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

	for (const auto& [fieldName, resolver] : resolvers)
	{
		if (!firstField)
		{
			sourceFile << R"cpp(,
)cpp";
		}

		firstField = false;
		sourceFile << resolver;
	}

	sourceFile << R"cpp(
	};
}
)cpp";

	if (!_loader.isIntrospection())
	{
		sourceFile << R"cpp(
void )cpp" << objectType.cppType
				   << R"cpp(::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void )cpp" << objectType.cppType
				   << R"cpp(::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}
)cpp";
	}

	// Output each of the resolver implementations, which call the virtual property
	// getters that the implementer must define.
	for (const auto& outputField : objectType.fields)
	{
		const auto resolverName = SchemaLoader::getOutputCppResolver(outputField);

		sourceFile << R"cpp(
service::AwaitableResolver )cpp"
				   << objectType.cppType << R"cpp(::)cpp" << resolverName
				   << R"cpp((service::ResolverParams&& params) const
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
						sourceFile << R"cpp(	static const auto defaultArguments = []()
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
)cpp";

		if (!_loader.isIntrospection())
		{
			sourceFile
				<< R"cpp(	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
)cpp";
		}

		const auto accessorName = SchemaLoader::getOutputCppAccessor(outputField);

		sourceFile << R"cpp(	auto result = _pimpl->)cpp" << accessorName << R"cpp(()cpp";

		bool firstArgument = _loader.isIntrospection();

		if (!firstArgument)
		{
			sourceFile
				<< R"cpp(service::FieldParams { std::move(selectionSetParams), std::move(directives) })cpp";
		}

		if (!outputField.arguments.empty())
		{
			for (const auto& argument : outputField.arguments)
			{
				std::string argumentName(argument.cppName);

				argumentName[0] =
					static_cast<char>(std::toupper(static_cast<unsigned char>(argumentName[0])));

				if (!firstArgument)
				{
					sourceFile << R"cpp(, )cpp";
				}

				sourceFile << R"cpp(std::move(arg)cpp" << argumentName << R"cpp())cpp";
				firstArgument = false;
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
service::AwaitableResolver )cpp"
			   << objectType.cppType
			   << R"cpp(::resolve_typename(service::ResolverParams&& params) const
{
	return service::Result<std::string>::convert(std::string{ R"gql()cpp"
			   << objectType.type << R"cpp()gql" }, std::move(params));
}
)cpp";

	if (!_options.noIntrospection && isQueryType)
	{
		sourceFile
			<< R"cpp(
service::AwaitableResolver )cpp"
			<< objectType.cppType << R"cpp(::resolve_schema(service::ResolverParams&& params) const
{
	return service::Result<service::Object>::convert(std::static_pointer_cast<service::Object>(std::make_shared<)cpp"
			<< SchemaLoader::getIntrospectionNamespace()
			<< R"cpp(::object::Schema>(std::make_shared<)cpp"
			<< SchemaLoader::getIntrospectionNamespace()
			<< R"cpp(::Schema>(_schema))), std::move(params));
}

service::AwaitableResolver )cpp"
			<< objectType.cppType << R"cpp(::resolve_type(service::ResolverParams&& params) const
{
	auto argName = service::ModifiedArgument<std::string>::require("name", params.arguments);
	const auto& baseType = _schema->LookupType(argName);
	std::shared_ptr<)cpp"
			<< SchemaLoader::getIntrospectionNamespace()
			<< R"cpp(::object::Type> result { baseType ? std::make_shared<)cpp"
			<< SchemaLoader::getIntrospectionNamespace()
			<< R"cpp(::object::Type>(std::make_shared<)cpp"
			<< SchemaLoader::getIntrospectionNamespace() << R"cpp(::Type>(baseType)) : nullptr };

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
	outputIntrospectionInterfaces(sourceFile, objectType.cppType, objectType.interfaces);
	outputIntrospectionFields(sourceFile, objectType.cppType, objectType.fields);
}

void Generator::outputIntrospectionInterfaces(std::ostream& sourceFile, std::string_view cppType,
	const std::vector<std::string_view>& interfaces) const
{
	if (!interfaces.empty())
	{
		bool firstInterface = true;

		sourceFile << R"cpp(	type)cpp" << cppType << R"cpp(->AddInterfaces({
)cpp";

		for (const auto& interfaceName : interfaces)
		{
			if (!firstInterface)
			{
				sourceFile << R"cpp(,
)cpp";
			}

			firstInterface = false;

			sourceFile
				<< R"cpp(		std::static_pointer_cast<const schema::InterfaceType>(schema->LookupType(R"gql()cpp"
				<< interfaceName << R"cpp()gql"sv)))cpp";
		}

		sourceFile << R"cpp(
	});
)cpp";
	}
}

void Generator::outputIntrospectionFields(
	std::ostream& sourceFile, std::string_view cppType, const OutputFieldList& fields) const
{
	if (fields.empty())
	{
		return;
	}

	bool firstValue = true;

	sourceFile << R"cpp(	type)cpp" << cppType << R"cpp(->AddFields({
)cpp";

	for (const auto& objectField : fields)
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

std::string Generator::getArgumentDefaultValue(
	std::size_t level, const response::Value& defaultValue) const noexcept
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

			for (const auto& [memberName, memberDefault] : members)
			{
				argumentDefaultValue << getArgumentDefaultValue(level + 1, memberDefault) << padding
									 << R"cpp(			members.emplace_back(")cpp" << memberName
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
								 << defaultValue.get<std::string>() << R"cpp()gql"));
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
								 << (defaultValue.get<bool>() ? R"cpp(true)cpp" : R"cpp(false)cpp")
								 << R"cpp();
)cpp";
			break;
		}

		case response::Type::Int:
		{
			argumentDefaultValue << padding
								 << R"cpp(		entry = response::Value(static_cast<int>()cpp"
								 << defaultValue.get<int>() << R"cpp());
)cpp";
			break;
		}

		case response::Type::Float:
		{
			argumentDefaultValue << padding
								 << R"cpp(		entry = response::Value(static_cast<double>()cpp"
								 << defaultValue.get<double>() << R"cpp());
)cpp";
			break;
		}

		case response::Type::EnumValue:
		{
			argumentDefaultValue
				<< padding << R"cpp(		entry = response::Value(response::Type::EnumValue);
		entry.set<std::string>(R"gql()cpp"
				<< defaultValue.get<std::string>() << R"cpp()gql");
)cpp";
			break;
		}

		case response::Type::ID:
			argumentDefaultValue << padding
								 << R"cpp(		entry = response::Value(response::Type::ID);
		entry.set<std::string>(R"gql()cpp"
								 << defaultValue.get<std::string>() << R"cpp()gql");
)cpp";
			break;

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
		? )cpp";

		const bool shouldMove = SchemaLoader::shouldMoveInputField(argument);

		if (shouldMove)
		{
			argumentDeclaration << R"cpp(std::move()cpp";
		}

		argumentDeclaration << R"cpp(pair)cpp" << argumentName << R"cpp(.first)cpp";

		if (shouldMove)
		{
			argumentDeclaration << R"cpp())cpp";
		}

		argumentDeclaration << R"cpp(
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
		case OutputFieldType::Interface:
		case OutputFieldType::Union:
		case OutputFieldType::Object:
			resultType << _loader.getCppType(result.type);
			break;

		case OutputFieldType::Scalar:
			resultType << R"cpp(response::Value)cpp";
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
	std::size_t wrapperCount = 0;
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

	introspectionType << R"cpp(schema->LookupType(R"gql()cpp" << type << R"cpp()gql"sv))cpp";

	for (std::size_t i = 0; i < wrapperCount; ++i)
	{
		introspectionType << R"cpp())cpp";
	}

	return introspectionType.str();
}

std::vector<std::string> Generator::outputSeparateFiles() const noexcept
{
	const std::filesystem::path headerDir(_headerDir);
	const std::filesystem::path sourceDir(_sourceDir);
	std::vector<std::string> files;

	const auto schemaNamespace = std::format(R"cpp(graphql::{})cpp", _loader.getSchemaNamespace());
	const auto objectNamespace = std::format(R"cpp({}::object)cpp", schemaNamespace);

	for (const auto& interfaceType : _loader.getInterfaceTypes())
	{
		const auto headerFilename = std::format("{}{}Object.h",
			(_options.prefixedHeaders ? _loader.getFilenamePrefix() : std::string_view {}),
			interfaceType.cppType);
		auto headerPath = (headerDir / headerFilename).string();

		{
			std::ofstream headerFile(headerPath, std::ios_base::trunc);
			IncludeGuardScope includeGuard { headerFile,
				std::format("{}_{}", _loader.getFilenamePrefix(), headerFilename) };

			headerFile << R"cpp(#include ")cpp"
					   << std::filesystem::path(_schemaHeaderPath).filename().string() << R"cpp("

)cpp";

			NamespaceScope headerNamespace { headerFile, objectNamespace };

			// Output the full declaration
			headerFile << std::endl;
			outputInterfaceDeclaration(headerFile, interfaceType.cppType);
			headerFile << std::endl;
		}

		const auto moduleFilename = std::string(interfaceType.cppType) + "Object.ixx";
		auto modulePath = (headerDir / moduleFilename).string();

		{
			std::ofstream moduleFile(modulePath, std::ios_base::trunc);

			outputObjectModule(moduleFile, objectNamespace, interfaceType.cppType);
		}

		if (_options.verbose)
		{
			files.push_back(std::move(headerPath));
			files.push_back(std::move(modulePath));
		}

		const auto sourceFilename = std::string(interfaceType.cppType) + "Object.cpp";
		auto sourcePath = (sourceDir / sourceFilename).string();

		{
			std::ofstream sourceFile(sourcePath, std::ios_base::trunc);

			sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include ")cpp" << headerFilename
					   << R"cpp("

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

using namespace std::literals;

)cpp";

			NamespaceScope sourceSchemaNamespace { sourceFile, schemaNamespace };
			NamespaceScope sourceInterfaceNamespace { sourceFile, "object" };

			sourceFile << std::endl;
			outputInterfaceImplementation(sourceFile, interfaceType.cppType);
			sourceFile << std::endl;

			sourceInterfaceNamespace.exit();
			sourceFile << std::endl;

			sourceFile << R"cpp(void Add)cpp" << interfaceType.cppType
					   << R"cpp(Details(const std::shared_ptr<schema::InterfaceType>& type)cpp"
					   << interfaceType.cppType
					   << R"cpp(, const std::shared_ptr<schema::Schema>& schema)
{
)cpp";
			outputInterfaceIntrospection(sourceFile, interfaceType);
			sourceFile << R"cpp(}

)cpp";
		}

		files.push_back(std::move(sourcePath));
	}

	for (const auto& unionType : _loader.getUnionTypes())
	{
		const auto headerFilename = std::format("{}{}Object.h",
			(_options.prefixedHeaders ? _loader.getFilenamePrefix() : std::string_view {}),
			unionType.cppType);
		auto headerPath = (headerDir / headerFilename).string();

		{
			std::ofstream headerFile(headerPath, std::ios_base::trunc);
			IncludeGuardScope includeGuard { headerFile,
				std::format("{}_{}", _loader.getFilenamePrefix(), headerFilename) };

			headerFile << R"cpp(#include ")cpp"
					   << std::filesystem::path(_schemaHeaderPath).filename().string() << R"cpp("

)cpp";

			NamespaceScope headerNamespace { headerFile, objectNamespace };

			// Output the full declaration
			headerFile << std::endl;
			outputInterfaceDeclaration(headerFile, unionType.cppType);
			headerFile << std::endl;
		}

		const auto moduleFilename = std::string(unionType.cppType) + "Object.ixx";
		auto modulePath = (headerDir / moduleFilename).string();

		{
			std::ofstream moduleFile(modulePath, std::ios_base::trunc);

			outputObjectModule(moduleFile, objectNamespace, unionType.cppType);
		}

		if (_options.verbose)
		{
			files.push_back(std::move(headerPath));
			files.push_back(std::move(modulePath));
		}

		const auto sourceFilename = std::string(unionType.cppType) + "Object.cpp";
		auto sourcePath = (sourceDir / sourceFilename).string();

		{
			std::ofstream sourceFile(sourcePath, std::ios_base::trunc);

			sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include ")cpp" << headerFilename
					   << R"cpp("

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

using namespace std::literals;

)cpp";

			NamespaceScope sourceSchemaNamespace { sourceFile, schemaNamespace };
			NamespaceScope sourceUnionNamespace { sourceFile, "object" };

			sourceFile << std::endl;
			outputInterfaceImplementation(sourceFile, unionType.cppType);
			sourceFile << std::endl;

			sourceUnionNamespace.exit();
			sourceFile << std::endl;

			sourceFile << R"cpp(void Add)cpp" << unionType.cppType
					   << R"cpp(Details(const std::shared_ptr<schema::UnionType>& type)cpp"
					   << unionType.cppType
					   << R"cpp(, const std::shared_ptr<schema::Schema>& schema)
{
)cpp";
			outputUnionIntrospection(sourceFile, unionType);
			sourceFile << R"cpp(}

)cpp";
		}

		files.push_back(std::move(sourcePath));
	}

	for (const auto& objectType : _loader.getObjectTypes())
	{
		bool isQueryType = false;
		bool isSubscriptionType = false;

		for (const auto& operation : _loader.getOperationTypes())
		{
			if (objectType.type == operation.type)
			{
				if (operation.operation == service::strQuery)
				{
					isQueryType = true;
				}
				else if (operation.operation == service::strSubscription)
				{
					isSubscriptionType = true;
				}

				break;
			}
		}

		const auto headerFilename = std::format("{}{}Object.h",
			(_options.prefixedHeaders ? _loader.getFilenamePrefix() : std::string_view {}),
			objectType.cppType);
		auto headerPath = (headerDir / headerFilename).string();

		{
			std::ofstream headerFile(headerPath, std::ios_base::trunc);
			IncludeGuardScope includeGuard { headerFile,
				std::format("{}_{}", _loader.getFilenamePrefix(), headerFilename) };

			headerFile << R"cpp(#include ")cpp"
					   << std::filesystem::path(_schemaHeaderPath).filename().string() << R"cpp("

)cpp";

			NamespaceScope headerNamespace { headerFile, objectNamespace };

			if (!_loader.isIntrospection())
			{
				if (!objectType.interfaces.empty() || !objectType.unions.empty())
				{
					NamespaceScope implementsNamespace { headerFile, R"cpp(implements)cpp" };

					headerFile << std::endl;
					outputObjectImplements(headerFile, objectType);

					implementsNamespace.exit();
					headerFile << std::endl;
				}

				// Output the stub concepts
				const auto conceptNamespace =
					std::format(R"cpp(methods::{}Has)cpp", objectType.cppType);
				NamespaceScope stubNamespace { headerFile, conceptNamespace };

				outputObjectStubs(headerFile, objectType);
			}

			// Output the full declaration
			headerFile << std::endl;
			outputObjectDeclaration(headerFile, objectType, isQueryType, isSubscriptionType);
			headerFile << std::endl;
		}

		const auto moduleFilename = std::string(objectType.cppType) + "Object.ixx";
		auto modulePath = (headerDir / moduleFilename).string();

		{
			std::ofstream moduleFile(modulePath, std::ios_base::trunc);

			outputObjectModule(moduleFile, objectNamespace, objectType.cppType);
		}

		if (_options.verbose)
		{
			files.push_back(std::move(headerPath));
			files.push_back(std::move(modulePath));
		}

		const auto sourceFilename = std::string(objectType.cppType) + "Object.cpp";
		auto sourcePath = (sourceDir / sourceFilename).string();

		{
			std::ofstream sourceFile(sourcePath, std::ios_base::trunc);

			sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include ")cpp" << headerFilename
					   << R"cpp("
)cpp";

			std::unordered_set<std::string_view> includedObjects;

			for (const auto& field : objectType.fields)
			{
				switch (field.fieldType)
				{
					case OutputFieldType::Interface:
					case OutputFieldType::Union:
					case OutputFieldType::Object:
						if (includedObjects.insert(field.type).second)
						{
							sourceFile << R"cpp(#include ")cpp";

							if (_options.prefixedHeaders)
							{
								sourceFile << _loader.getFilenamePrefix();
							}

							sourceFile << SchemaLoader::getSafeCppName(field.type)
									   << R"cpp(Object.h"
)cpp";
						}
						break;

					default:
						break;
				}
			}

			if (_loader.isIntrospection() || (isQueryType && !_options.noIntrospection))
			{
				sourceFile << R"cpp(
#include "graphqlservice/internal/Introspection.h"
)cpp";
			}
			else
			{
				sourceFile << R"cpp(
#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"
)cpp";
			}

			if (isQueryType && !_options.noIntrospection)
			{
				sourceFile << R"cpp(
#include "graphqlservice/introspection/SchemaObject.h"
#include "graphqlservice/introspection/TypeObject.h"
)cpp";
			}

			sourceFile << R"cpp(
#include <algorithm>
#include <functional>
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
					   << R"cpp(Details(const std::shared_ptr<schema::ObjectType>& type)cpp"
					   << objectType.cppType
					   << R"cpp(, const std::shared_ptr<schema::Schema>& schema)
{
)cpp";
			outputObjectIntrospection(sourceFile, objectType);
			sourceFile << R"cpp(}

)cpp";
		}

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
	bool verbose = false;
	bool stubs = false;
	bool noIntrospection = false;
	bool prefixedHeaders = false;
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
		"Target path for the <prefix>Schema.h header file")("stubs",
		po::bool_switch(&stubs),
		"Unimplemented fields throw runtime exceptions instead of compiler errors")("no-"
																					"introspection",
		po::bool_switch(&noIntrospection),
		"Do not generate support for Introspection")("prefix-headers",
		po::bool_switch(&prefixedHeaders),
		"Prefix generated object header filenames");
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
	else if (showUsage || !buildCustom)
	{
		outputUsage(std::cout, options);
		return 0;
	}

	try
	{
		const auto files = graphql::generator::schema::Generator({ std::move(schemaFileName),
																	 std::move(filenamePrefix),
																	 std::move(schemaNamespace),
																	 buildIntrospection },
			{
				{ std::move(headerDir), std::move(sourceDir) }, // paths
				verbose,										// verbose
				stubs,											// stubs
				noIntrospection,								// noIntrospection
				prefixedHeaders,								// prefixedHeaders
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
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
