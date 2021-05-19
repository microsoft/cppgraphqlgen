// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ClientGenerator.h"
#include "GeneratorUtil.h"
#include "Validation.h"

#include "graphqlservice/internal/Version.h"
#include "graphqlservice/introspection/Introspection.h"

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

Generator::Generator(SchemaOptions&& schemaOptions, GeneratorOptions&& options)
	: _loader(std::make_optional(std::move(schemaOptions)))
	, _options(std::move(options))
	, _headerDir(getHeaderDir())
	, _sourceDir(getSourceDir())
	, _headerPath(getHeaderPath())
	, _sourcePath(getSourcePath())
{
	_request = peg::parseFile(_options.requestFilename);

	if (!_request.root)
	{
		throw std::logic_error("Unable to parse the request document, but there was no error "
							   "message from the parser!");
	}
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

	fullPath /= (std::string { _loader.getFilenamePrefix() } + "Client.h");

	return fullPath.string();
}

std::string Generator::getSourcePath() const noexcept
{
	fs::path fullPath { _sourceDir };

	fullPath /= (std::string { _loader.getFilenamePrefix() } + "Client.cpp");

	return fullPath.string();
}

void Generator::validateQuery() const
{
	auto schema = buildSchema();
	service::ValidateExecutableVisitor validation { schema };

	validation.visit(*_request.root);

	auto errors = validation.getStructuredErrors();

	if (!errors.empty())
	{
		throw service::schema_exception { std::move(errors) };
	}
}

std::shared_ptr<schema::Schema> Generator::buildSchema() const
{
	auto schema = std::make_shared<schema::Schema>(_options.noIntrospection);

	introspection::AddTypesToSchema(schema);
	addTypesToSchema(schema);

	return schema;
}

void Generator::addTypesToSchema(const std::shared_ptr<schema::Schema>& schema) const
{
	if (!_loader.getScalarTypes().empty())
	{
		for (const auto& scalarType : _loader.getScalarTypes())
		{
			schema->AddType(scalarType.type,
				schema::ScalarType::Make(scalarType.type, scalarType.description));
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::EnumType>> enumTypes;

	if (!_loader.getEnumTypes().empty())
	{
		for (const auto& enumType : _loader.getEnumTypes())
		{
			const auto itr = enumTypes
								 .emplace(std::make_pair(enumType.type,
									 schema::EnumType::Make(enumType.type, enumType.description)))
								 .first;

			schema->AddType(enumType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::InputObjectType>> inputTypes;

	if (!_loader.getInputTypes().empty())
	{
		for (const auto& inputType : _loader.getInputTypes())
		{
			const auto itr =
				inputTypes
					.emplace(std::make_pair(inputType.type,
						schema::InputObjectType::Make(inputType.type, inputType.description)))
					.first;

			schema->AddType(inputType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::UnionType>> unionTypes;

	if (!_loader.getUnionTypes().empty())
	{
		for (const auto& unionType : _loader.getUnionTypes())
		{
			const auto itr =
				unionTypes
					.emplace(std::make_pair(unionType.type,
						schema::UnionType::Make(unionType.type, unionType.description)))
					.first;

			schema->AddType(unionType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::InterfaceType>> interfaceTypes;

	if (!_loader.getInterfaceTypes().empty())
	{
		for (const auto& interfaceType : _loader.getInterfaceTypes())
		{
			const auto itr =
				interfaceTypes
					.emplace(std::make_pair(interfaceType.type,
						schema::InterfaceType::Make(interfaceType.type, interfaceType.description)))
					.first;

			schema->AddType(interfaceType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::ObjectType>> objectTypes;

	if (!_loader.getObjectTypes().empty())
	{
		for (const auto& objectType : _loader.getObjectTypes())
		{
			const auto itr =
				objectTypes
					.emplace(std::make_pair(objectType.type,
						schema::ObjectType::Make(objectType.type, objectType.description)))
					.first;

			schema->AddType(objectType.type, itr->second);
		}
	}

	for (const auto& enumType : _loader.getEnumTypes())
	{
		const auto itr = enumTypes.find(enumType.type);

		if (itr != enumTypes.cend() && !enumType.values.empty())
		{
			std::vector<schema::EnumValueType> values(enumType.values.size());

			std::transform(enumType.values.cbegin(),
				enumType.values.cend(),
				values.begin(),
				[](const EnumValueType& value) noexcept {
					return schema::EnumValueType {
						value.value,
						value.description,
						value.deprecationReason,
					};
				});

			itr->second->AddEnumValues(std::move(values));
		}
	}

	for (const auto& inputType : _loader.getInputTypes())
	{
		const auto itr = inputTypes.find(inputType.type);

		if (itr != inputTypes.cend() && !inputType.fields.empty())
		{
			std::vector<std::shared_ptr<const schema::InputValue>> fields(inputType.fields.size());

			std::transform(inputType.fields.cbegin(),
				inputType.fields.cend(),
				fields.begin(),
				[schema](const InputField& field) noexcept {
					return schema::InputValue::Make(field.name,
						field.description,
						getIntrospectionType(schema, field.type, field.modifiers),
						field.defaultValueString);
				});

			itr->second->AddInputValues(std::move(fields));
		}
	}

	for (const auto& unionType : _loader.getUnionTypes())
	{
		const auto itr = unionTypes.find(unionType.type);

		if (!unionType.options.empty())
		{
			std::vector<std::weak_ptr<const schema::BaseType>> options(unionType.options.size());

			std::transform(unionType.options.cbegin(),
				unionType.options.cend(),
				options.begin(),
				[schema](std::string_view option) noexcept {
					return schema->LookupType(option);
				});

			itr->second->AddPossibleTypes(std::move(options));
		}
	}

	for (const auto& interfaceType : _loader.getInterfaceTypes())
	{
		const auto itr = interfaceTypes.find(interfaceType.type);

		if (!interfaceType.fields.empty())
		{
			std::vector<std::shared_ptr<const schema::Field>> fields(interfaceType.fields.size());

			std::transform(interfaceType.fields.cbegin(),
				interfaceType.fields.cend(),
				fields.begin(),
				[schema](const OutputField& field) noexcept {
					std::vector<std::shared_ptr<const schema::InputValue>> arguments(
						field.arguments.size());

					std::transform(field.arguments.cbegin(),
						field.arguments.cend(),
						arguments.begin(),
						[schema](const InputField& argument) noexcept {
							return schema::InputValue::Make(argument.name,
								argument.description,
								getIntrospectionType(schema, argument.type, argument.modifiers),
								argument.defaultValueString);
						});

					return schema::Field::Make(field.name,
						field.description,
						field.deprecationReason,
						getIntrospectionType(schema, field.type, field.modifiers),
						std::move(arguments));
				});

			itr->second->AddFields(std::move(fields));
		}
	}

	for (const auto& objectType : _loader.getObjectTypes())
	{
		const auto itr = objectTypes.find(objectType.type);

		if (!objectType.interfaces.empty())
		{
			std::vector<std::shared_ptr<const schema::InterfaceType>> interfaces(
				objectType.interfaces.size());

			std::transform(objectType.interfaces.cbegin(),
				objectType.interfaces.cend(),
				interfaces.begin(),
				[schema, &interfaceTypes](std::string_view interfaceName) noexcept {
					return interfaceTypes[interfaceName];
				});

			itr->second->AddInterfaces(std::move(interfaces));
		}

		if (!objectType.fields.empty())
		{
			std::vector<std::shared_ptr<const schema::Field>> fields(objectType.fields.size());

			std::transform(objectType.fields.cbegin(),
				objectType.fields.cend(),
				fields.begin(),
				[schema](const OutputField& field) noexcept {
					std::vector<std::shared_ptr<const schema::InputValue>> arguments(
						field.arguments.size());

					std::transform(field.arguments.cbegin(),
						field.arguments.cend(),
						arguments.begin(),
						[schema](const InputField& argument) noexcept {
							return schema::InputValue::Make(argument.name,
								argument.description,
								getIntrospectionType(schema, argument.type, argument.modifiers),
								argument.defaultValueString);
						});

					return schema::Field::Make(field.name,
						field.description,
						field.deprecationReason,
						getIntrospectionType(schema, field.type, field.modifiers),
						std::move(arguments));
				});

			itr->second->AddFields(std::move(fields));
		}
	}

	for (const auto& directive : _loader.getDirectives())
	{
		std::vector<introspection::DirectiveLocation> locations(directive.locations.size());

		std::transform(directive.locations.cbegin(),
			directive.locations.cend(),
			locations.begin(),
			[](std::string_view locationName) noexcept {
				response::Value locationValue(response::Type::EnumValue);

				locationValue.set<response::StringType>(response::StringType { locationName });

				return service::ModifiedArgument<introspection::DirectiveLocation>::convert(
					locationValue);
			});

		std::vector<std::shared_ptr<const schema::InputValue>> arguments(
			directive.arguments.size());

		std::transform(directive.arguments.cbegin(),
			directive.arguments.cend(),
			arguments.begin(),
			[schema](const InputField& argument) noexcept {
				return schema::InputValue::Make(argument.name,
					argument.description,
					getIntrospectionType(schema, argument.type, argument.modifiers),
					argument.defaultValueString);
			});

		schema->AddDirective(schema::Directive::Make(directive.name,
			directive.description,
			std::move(locations),
			std::move(arguments)));
	}

	for (const auto& operationType : _loader.getOperationTypes())
	{
		const auto itr = objectTypes.find(operationType.type);

		if (operationType.operation == service::strQuery)
		{
			schema->AddQueryType(itr->second);
		}
		else if (operationType.operation == service::strMutation)
		{
			schema->AddMutationType(itr->second);
		}
		else if (operationType.operation == service::strSubscription)
		{
			schema->AddSubscriptionType(itr->second);
		}
	}
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
#include <vector>

)cpp";

	NamespaceScope graphqlNamespace { headerFile, "graphql" };
	NamespaceScope schemaNamespace { headerFile, _loader.getSchemaNamespace() };
	NamespaceScope objectNamespace { headerFile, "object", true };
	PendingBlankLine pendingSeparator { headerFile };

	std::string_view queryType;

	for (const auto& operation : _loader.getOperationTypes())
	{
		if (operation.operation == service::strQuery)
		{
			queryType = operation.type;
			break;
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

		// Forward declare all of the input types
		if (_loader.getInputTypes().size() > 1)
		{
			for (const auto& inputType : _loader.getInputTypes())
			{
				headerFile << R"cpp(struct )cpp" << inputType.cppType << R"cpp(;
)cpp";
			}

			headerFile << std::endl;
		}

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

	if (!_loader.getObjectTypes().empty())
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
		headerFile << R"cpp(	std::shared_ptr<object::)cpp" << operation.cppType << R"cpp(> _)cpp"
				   << operation.operation << R"cpp(;
)cpp";
	}

	headerFile << R"cpp(};

std::shared_ptr<client::Client> GetClient();

)cpp";

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

		if (isQueryType)
		{
			headerFile
				<< R"cpp(	std::future<service::ResolverResult> resolve_client(service::ResolverParams&& params);
	std::future<service::ResolverResult> resolve_type(service::ResolverParams&& params);

	std::shared_ptr<client::Client> _client;
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
	if (outputField.interfaceField)
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
				sourceFile << R"cpp(	")cpp" << value.value << R"cpp(")cpp";
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

	for (const auto& operation : _loader.getOperationTypes())
	{
		if (operation.operation == service::strQuery)
		{
			queryType = operation.type;
			break;
		}
	}

	if (!_loader.getObjectTypes().empty())
	{
		NamespaceScope objectNamespace { sourceFile, "object" };

		sourceFile << std::endl;

		for (const auto& objectType : _loader.getObjectTypes())
		{
			outputObjectImplementation(sourceFile, objectType, objectType.type == queryType);
			sourceFile << std::endl;
		}
	}

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
	}, GetClient())
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

	if (isQueryType)
	{
		resolvers["__client"sv] =
			R"cpp(		{ R"gql(__client)gql"sv, [this](service::ResolverParams&& params) { return resolve_client(std::move(params)); } })cpp"s;
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

	if (isQueryType)
	{
		sourceFile << R"cpp(
	, _client(GetClient()))cpp";
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
		sourceFile << R"cpp(
service::FieldResult<)cpp"
				   << _loader.getOutputCppType(outputField) << R"cpp(> )cpp" << objectType.cppType
				   << R"cpp(::)cpp" << outputField.accessor << fieldName
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

	if (isQueryType)
	{
		sourceFile
			<< R"cpp(
std::future<service::ResolverResult> )cpp"
			<< objectType.cppType
			<< R"cpp(::resolve_client(service::ResolverParams&& params)
{
	return service::ModifiedResult<service::Object>::convert(std::static_pointer_cast<service::Object>(std::make_shared<)cpp"
			// << s_introspectionNamespace
			<< R"cpp(::Client>(_client)), std::move(params));
}

std::future<service::ResolverResult> )cpp"
			<< objectType.cppType
			<< R"cpp(::resolve_type(service::ResolverParams&& params)
{
	auto argName = service::ModifiedArgument<response::StringType>::require("name", params.arguments);
	const auto& baseType = _client->LookupType(argName);
	std::shared_ptr<)cpp"
			// << s_introspectionNamespace
			<< R"cpp(::object::Type> result { baseType ? std::make_shared<)cpp"
			// << s_introspectionNamespace
			<< R"cpp(::Type>(baseType) : nullptr };

	return service::ModifiedResult<)cpp"
			// << s_introspectionNamespace
			<< R"cpp(::object::Type>::convert<service::TypeModifier::Nullable>(result, std::move(params));
}
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

std::shared_ptr<const schema::BaseType> Generator::getIntrospectionType(
	const std::shared_ptr<schema::Schema>& schema, std::string_view type,
	TypeModifierStack modifiers, bool nonNull /* = true */) noexcept
{
	std::shared_ptr<const schema::BaseType> introspectionType;

	if (modifiers.empty())
	{
		introspectionType = schema->LookupType(type);
	}
	else
	{
		const auto modifier = modifiers.front();

		modifiers.erase(modifiers.cbegin());
		introspectionType = getIntrospectionType(schema,
			type,
			std::move(modifiers),
			modifier != service::TypeModifier::Nullable);

		if (modifier == service::TypeModifier::List && introspectionType)
		{
			introspectionType =
				schema->WrapType(introspection::TypeKind::LIST, std::move(introspectionType));
		}
	}

	if (nonNull && introspectionType)
	{
		introspectionType =
			schema->WrapType(introspection::TypeKind::NON_NULL, std::move(introspectionType));
	}

	return introspectionType;
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
		"Request document file path")("prefix,p",
		po::value(&filenamePrefix),
		"Prefix to use for the generated C++ filenames")("namespace,n",
		po::value(&schemaNamespace),
		"C++ sub-namespace for the generated types")("source-dir",
		po::value(&sourceDir),
		"Target path for the <prefix>Client.cpp source file")("header-dir",
		po::value(&headerDir),
		"Target path for the <prefix>Client.h header file")("no-introspection",
		po::bool_switch(&noIntrospection),
		"Do not generate support for Introspection");
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
			graphql::generator::client::GeneratorOptions {
				std::move(requestFileName),
				graphql::generator::client::GeneratorPaths { std::move(headerDir),
					std::move(sourceDir) },
				verbose,
				noIntrospection,
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
