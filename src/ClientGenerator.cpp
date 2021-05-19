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

namespace graphql::client {

using namespace generator;

const BuiltinTypeMap Generator::s_builtinTypes = {
	{ "Int"sv, BuiltinType::Int },
	{ "Float"sv, BuiltinType::Float },
	{ "String"sv, BuiltinType::String },
	{ "Boolean"sv, BuiltinType::Boolean },
	{ "ID"sv, BuiltinType::ID },
};

const CppTypeMap Generator::s_builtinCppTypes = {
	"response::IntType"sv,
	"response::FloatType"sv,
	"response::StringType"sv,
	"response::BooleanType"sv,
	"response::IdType"sv,
};

const std::string_view Generator::s_scalarCppType = R"cpp(response::Value)cpp"sv;

Generator::Generator(GeneratorOptions&& options)
	: _options(std::move(options))
	, _clientNamespace(_options.client.clientNamespace)
	, _headerDir(getHeaderDir())
	, _sourceDir(getSourceDir())
	, _headerPath(getHeaderPath())
	, _sourcePath(getSourcePath())
{

	_schema = peg::parseSchemaFile(_options.client.schemaFilename);

	if (!_schema.root)
	{
		throw std::logic_error("Unable to parse the service schema, but there was no error "
							   "message from the parser!");
	}

	for (const auto& child : _schema.root->children)
	{
		visitDefinition(*child);
	}

	validateSchema();

	_request = peg::parseFile(_options.client.requestFilename);

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

	fullPath /= (_options.client.filenamePrefix + "Client.h");

	return fullPath.string();
}

std::string Generator::getSourcePath() const noexcept
{
	fs::path fullPath { _sourceDir };

	fullPath /= (_options.client.filenamePrefix + "Client.cpp");

	return fullPath.string();
}

void Generator::validateSchema()
{
	// Verify that none of the custom types conflict with a built-in type.
	for (const auto& entry : _clientTypes)
	{
		if (s_builtinTypes.find(entry.first) != s_builtinTypes.cend())
		{
			std::ostringstream error;
			auto itrPosition = _typePositions.find(entry.first);

			error << "Builtin type overridden: " << entry.first;

			if (itrPosition != _typePositions.cend())
			{
				error << " line: " << itrPosition->second.line
					  << " column: " << itrPosition->second.column;
			}

			throw std::runtime_error(error.str());
		}
	}

	// Fixup all of the fieldType members.
	for (auto& entry : _inputTypes)
	{
		fixupInputFieldList(entry.fields);
	}

	for (auto& entry : _interfaceTypes)
	{
		fixupOutputFieldList(entry.fields, std::nullopt, std::nullopt);
	}

	bool queryDefined = false;

	if (_operationTypes.empty())
	{
		// Fill in the operations with default type names if present.
		constexpr auto strDefaultQuery = "Query"sv;
		constexpr auto strDefaultMutation = "Mutation"sv;
		constexpr auto strDefaultSubscription = "Subscription"sv;

		if (_objectNames.find(strDefaultQuery) != _objectNames.cend())
		{
			_operationTypes.push_back(
				{ strDefaultQuery, getSafeCppName(strDefaultQuery), service::strQuery });
			queryDefined = true;
		}

		if (_objectNames.find(strDefaultMutation) != _objectNames.cend())
		{
			_operationTypes.push_back(
				{ strDefaultMutation, getSafeCppName(strDefaultMutation), service::strMutation });
		}

		if (_objectNames.find(strDefaultSubscription) != _objectNames.cend())
		{
			_operationTypes.push_back({ strDefaultSubscription,
				getSafeCppName(strDefaultSubscription),
				service::strSubscription });
		}
	}
	else
	{
		// Validate that all of the operation types exist and that query is defined.
		for (const auto& operation : _operationTypes)
		{
			if (_objectNames.find(operation.type) == _objectNames.cend())
			{
				std::ostringstream error;

				error << "Unknown operation type: " << operation.type
					  << " operation: " << operation.operation;

				throw std::runtime_error(error.str());
			}

			queryDefined = queryDefined || (operation.operation == service::strQuery);
		}
	}

	if (!queryDefined)
	{
		throw std::runtime_error("Query operation type undefined");
	}

	std::string_view mutationType;

	for (const auto& operation : _operationTypes)
	{
		if (operation.operation == service::strMutation)
		{
			mutationType = operation.type;
			break;
		}
	}

	for (auto& entry : _objectTypes)
	{
		auto interfaceFields = std::make_optional<std::unordered_set<std::string_view>>();
		auto accessor = (mutationType == entry.type)
			? std::make_optional<std::string_view>(strApply)
			: std::nullopt;

		for (const auto& interfaceName : entry.interfaces)
		{
			auto itr = _interfaceNames.find(interfaceName);

			if (itr != _interfaceNames.cend())
			{
				for (const auto& field : _interfaceTypes[itr->second].fields)
				{
					interfaceFields->insert(field.name);
				}
			}
		}

		fixupOutputFieldList(entry.fields, interfaceFields, accessor);
	}

	// Validate the interfaces implemented by the object types.
	for (const auto& entry : _objectTypes)
	{
		for (const auto& interfaceName : entry.interfaces)
		{
			if (_interfaceNames.find(interfaceName) == _interfaceNames.cend())
			{
				std::ostringstream error;
				auto itrPosition = _typePositions.find(entry.type);

				error << "Unknown interface: " << interfaceName
					  << " implemented by: " << entry.type;

				if (itrPosition != _typePositions.cend())
				{
					error << " line: " << itrPosition->second.line
						  << " column: " << itrPosition->second.column;
				}

				throw std::runtime_error(error.str());
			}
		}
	}

	// Validate the objects that are possible types for unions and add the unions to
	// the list of matching types for the objects.
	for (const auto& entry : _unionTypes)
	{
		for (const auto& objectName : entry.options)
		{
			auto itr = _objectNames.find(objectName);

			if (itr == _objectNames.cend())
			{
				std::ostringstream error;
				auto itrPosition = _typePositions.find(entry.type);

				error << "Unknown type: " << objectName << " included by: " << entry.type;

				if (itrPosition != _typePositions.cend())
				{
					error << " line: " << itrPosition->second.line
						  << " column: " << itrPosition->second.column;
				}

				throw std::runtime_error(error.str());
			}

			_objectTypes[itr->second].unions.push_back(entry.type);
		}
	}
}

void Generator::fixupOutputFieldList(OutputFieldList& fields,
	const std::optional<std::unordered_set<std::string_view>>& interfaceFields,
	const std::optional<std::string_view>& accessor)
{
	for (auto& entry : fields)
	{
		if (interfaceFields)
		{
			entry.interfaceField = false;
			entry.inheritedField = interfaceFields->find(entry.name) != interfaceFields->cend();
		}
		else
		{
			entry.interfaceField = true;
			entry.inheritedField = false;
		}

		if (accessor)
		{
			entry.accessor = *accessor;
		}

		if (s_builtinTypes.find(entry.type) != s_builtinTypes.cend())
		{
			continue;
		}

		auto itr = _clientTypes.find(entry.type);

		if (itr == _clientTypes.cend())
		{
			std::ostringstream error;

			error << "Unknown field type: " << entry.type;

			if (entry.position)
			{
				error << " line: " << entry.position->line << " column: " << entry.position->column;
			}

			throw std::runtime_error(error.str());
		}

		switch (itr->second)
		{
			case SchemaType::Scalar:
				entry.fieldType = OutputFieldType::Scalar;
				break;

			case SchemaType::Enum:
				entry.fieldType = OutputFieldType::Enum;
				break;

			case SchemaType::Union:
				entry.fieldType = OutputFieldType::Union;
				break;

			case SchemaType::Interface:
				entry.fieldType = OutputFieldType::Interface;
				break;

			case SchemaType::Object:
				entry.fieldType = OutputFieldType::Object;
				break;

			default:
			{
				std::ostringstream error;

				error << "Invalid field type: " << entry.type;

				if (entry.position)
				{
					error << " line: " << entry.position->line
						  << " column: " << entry.position->column;
				}

				throw std::runtime_error(error.str());
			}
		}

		fixupInputFieldList(entry.arguments);
	}
}

void Generator::fixupInputFieldList(InputFieldList& fields)
{
	for (auto& entry : fields)
	{
		if (s_builtinTypes.find(entry.type) != s_builtinTypes.cend())
		{
			continue;
		}

		auto itr = _clientTypes.find(entry.type);

		if (itr == _clientTypes.cend())
		{
			std::ostringstream error;

			error << "Unknown argument type: " << entry.type;

			if (entry.position)
			{
				error << " line: " << entry.position->line << " column: " << entry.position->column;
			}

			throw std::runtime_error(error.str());
		}

		switch (itr->second)
		{
			case SchemaType::Scalar:
				entry.fieldType = InputFieldType::Scalar;
				break;

			case SchemaType::Enum:
				entry.fieldType = InputFieldType::Enum;
				break;

			case SchemaType::Input:
				entry.fieldType = InputFieldType::Input;
				break;

			default:
			{
				std::ostringstream error;

				error << "Invalid argument type: " << entry.type;

				if (entry.position)
				{
					error << " line: " << entry.position->line
						  << " column: " << entry.position->column;
				}

				throw std::runtime_error(error.str());
			}
		}
	}
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
	if (!_scalarTypes.empty())
	{
		for (const auto& scalarType : _scalarTypes)
		{
			schema->AddType(scalarType.type,
				schema::ScalarType::Make(scalarType.type, scalarType.description));
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::EnumType>> enumTypes;

	if (!_enumTypes.empty())
	{
		for (const auto& enumType : _enumTypes)
		{
			const auto itr = enumTypes
								 .emplace(std::make_pair(enumType.type,
									 schema::EnumType::Make(enumType.type, enumType.description)))
								 .first;

			schema->AddType(enumType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::InputObjectType>> inputTypes;

	if (!_inputTypes.empty())
	{
		for (const auto& inputType : _inputTypes)
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

	if (!_unionTypes.empty())
	{
		for (const auto& unionType : _unionTypes)
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

	if (!_interfaceTypes.empty())
	{
		for (const auto& interfaceType : _interfaceTypes)
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

	if (!_objectTypes.empty())
	{
		for (const auto& objectType : _objectTypes)
		{
			const auto itr =
				objectTypes
					.emplace(std::make_pair(objectType.type,
						schema::ObjectType::Make(objectType.type, objectType.description)))
					.first;

			schema->AddType(objectType.type, itr->second);
		}
	}

	for (const auto& enumType : _enumTypes)
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

	for (const auto& inputType : _inputTypes)
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

	for (const auto& unionType : _unionTypes)
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

	for (const auto& interfaceType : _interfaceTypes)
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

	for (const auto& objectType : _objectTypes)
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

			itr->second->AddInterfaces(
				std::initializer_list<std::shared_ptr<const schema::InterfaceType>>(
					interfaces.data(),
					interfaces.data() + interfaces.size()));
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

	for (const auto& directive : _directives)
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

	for (const auto& operationType : _operationTypes)
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

void Generator::visitDefinition(const peg::ast_node& definition)
{
	if (definition.is_type<peg::schema_definition>())
	{
		visitClientDefinition(definition);
	}
	else if (definition.is_type<peg::schema_extension>())
	{
		visitClientExtension(definition);
	}
	else if (definition.is_type<peg::scalar_type_definition>())
	{
		visitScalarTypeDefinition(definition);
	}
	else if (definition.is_type<peg::enum_type_definition>())
	{
		visitEnumTypeDefinition(definition);
	}
	else if (definition.is_type<peg::enum_type_extension>())
	{
		visitEnumTypeExtension(definition);
	}
	else if (definition.is_type<peg::input_object_type_definition>())
	{
		visitInputObjectTypeDefinition(definition);
	}
	else if (definition.is_type<peg::input_object_type_extension>())
	{
		visitInputObjectTypeExtension(definition);
	}
	else if (definition.is_type<peg::union_type_definition>())
	{
		visitUnionTypeDefinition(definition);
	}
	else if (definition.is_type<peg::union_type_extension>())
	{
		visitUnionTypeExtension(definition);
	}
	else if (definition.is_type<peg::interface_type_definition>())
	{
		visitInterfaceTypeDefinition(definition);
	}
	else if (definition.is_type<peg::interface_type_extension>())
	{
		visitInterfaceTypeExtension(definition);
	}
	else if (definition.is_type<peg::object_type_definition>())
	{
		visitObjectTypeDefinition(definition);
	}
	else if (definition.is_type<peg::object_type_extension>())
	{
		visitObjectTypeExtension(definition);
	}
	else if (definition.is_type<peg::directive_definition>())
	{
		visitDirectiveDefinition(definition);
	}
	else
	{
		const auto position = definition.begin();
		std::ostringstream error;

		error << "Unexpected executable definition line: " << position.line
			  << " column: " << position.column;

		throw std::runtime_error(error.str());
	}
}

void Generator::visitClientDefinition(const peg::ast_node& clientDefinition)
{
	peg::for_each_child<peg::root_operation_definition>(clientDefinition,
		[this](const peg::ast_node& child) {
			const auto operation(child.children.front()->string_view());
			const auto name(child.children.back()->string_view());
			const auto cppName(getSafeCppName(name));

			_operationTypes.push_back({ name, cppName, operation });
		});
}

void Generator::visitClientExtension(const peg::ast_node& clientExtension)
{
	peg::for_each_child<peg::operation_type_definition>(clientExtension,
		[this](const peg::ast_node& child) {
			const auto operation(child.children.front()->string_view());
			const auto name(child.children.back()->string_view());
			const auto cppName(getSafeCppName(name));

			_operationTypes.push_back({ name, cppName, operation });
		});
}

void Generator::visitObjectTypeDefinition(const peg::ast_node& objectTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::object_name>(objectTypeDefinition,
		[&name](const peg::ast_node& child) {
			name = child.string_view();
		});

	peg::on_first_child<peg::description>(objectTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_clientTypes[name] = SchemaType::Object;
	_typePositions.emplace(name, objectTypeDefinition.begin());
	_objectNames[name] = _objectTypes.size();

	auto cppName = getSafeCppName(name);

	_objectTypes.push_back({ name, cppName, {}, {}, {}, description });

	visitObjectTypeExtension(objectTypeDefinition);
}

void Generator::visitObjectTypeExtension(const peg::ast_node& objectTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::object_name>(objectTypeExtension, [&name](const peg::ast_node& child) {
		name = child.string_view();
	});

	const auto itrType = _objectNames.find(name);

	if (itrType != _objectNames.cend())
	{
		auto& objectType = _objectTypes[itrType->second];

		peg::for_each_child<peg::interface_type>(objectTypeExtension,
			[&objectType](const peg::ast_node& child) {
				objectType.interfaces.push_back(child.string_view());
			});

		peg::on_first_child<peg::fields_definition>(objectTypeExtension,
			[&objectType](const peg::ast_node& child) {
				auto fields = getOutputFields(child.children);

				objectType.fields.reserve(objectType.fields.size() + fields.size());
				for (auto& field : fields)
				{
					objectType.fields.push_back(std::move(field));
				}
			});
	}
}

void Generator::visitInterfaceTypeDefinition(const peg::ast_node& interfaceTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::interface_name>(interfaceTypeDefinition,
		[&name](const peg::ast_node& child) {
			name = child.string_view();
		});

	peg::on_first_child<peg::description>(interfaceTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_clientTypes[name] = SchemaType::Interface;
	_typePositions.emplace(name, interfaceTypeDefinition.begin());
	_interfaceNames[name] = _interfaceTypes.size();

	auto cppName = getSafeCppName(name);

	_interfaceTypes.push_back({ name, cppName, {}, description });

	visitInterfaceTypeExtension(interfaceTypeDefinition);
}

void Generator::visitInterfaceTypeExtension(const peg::ast_node& interfaceTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::interface_name>(interfaceTypeExtension,
		[&name](const peg::ast_node& child) {
			name = child.string_view();
		});

	const auto itrType = _interfaceNames.find(name);

	if (itrType != _interfaceNames.cend())
	{
		auto& interfaceType = _interfaceTypes[itrType->second];

		peg::on_first_child<peg::fields_definition>(interfaceTypeExtension,
			[&interfaceType](const peg::ast_node& child) {
				auto fields = getOutputFields(child.children);

				interfaceType.fields.reserve(interfaceType.fields.size() + fields.size());
				for (auto& field : fields)
				{
					interfaceType.fields.push_back(std::move(field));
				}
			});
	}
}

void Generator::visitInputObjectTypeDefinition(const peg::ast_node& inputObjectTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::object_name>(inputObjectTypeDefinition,
		[&name](const peg::ast_node& child) {
			name = child.string_view();
		});

	peg::on_first_child<peg::description>(inputObjectTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_clientTypes[name] = SchemaType::Input;
	_typePositions.emplace(name, inputObjectTypeDefinition.begin());
	_inputNames[name] = _inputTypes.size();

	auto cppName = getSafeCppName(name);

	_inputTypes.push_back({ name, cppName, {}, description });

	visitInputObjectTypeExtension(inputObjectTypeDefinition);
}

void Generator::visitInputObjectTypeExtension(const peg::ast_node& inputObjectTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::object_name>(inputObjectTypeExtension,
		[&name](const peg::ast_node& child) {
			name = child.string_view();
		});

	const auto itrType = _inputNames.find(name);

	if (itrType != _inputNames.cend())
	{
		auto& inputType = _inputTypes[itrType->second];

		peg::on_first_child<peg::input_fields_definition>(inputObjectTypeExtension,
			[&inputType](const peg::ast_node& child) {
				auto fields = getInputFields(child.children);

				inputType.fields.reserve(inputType.fields.size() + fields.size());
				for (auto& field : fields)
				{
					inputType.fields.push_back(std::move(field));
				}
			});
	}
}

void Generator::visitEnumTypeDefinition(const peg::ast_node& enumTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::enum_name>(enumTypeDefinition, [&name](const peg::ast_node& child) {
		name = child.string_view();
	});

	peg::on_first_child<peg::description>(enumTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_clientTypes[name] = SchemaType::Enum;
	_typePositions.emplace(name, enumTypeDefinition.begin());
	_enumNames[name] = _enumTypes.size();

	auto cppName = getSafeCppName(name);

	_enumTypes.push_back({ name, cppName, {}, description });

	visitEnumTypeExtension(enumTypeDefinition);
}

void Generator::visitEnumTypeExtension(const peg::ast_node& enumTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::enum_name>(enumTypeExtension, [&name](const peg::ast_node& child) {
		name = child.string_view();
	});

	const auto itrType = _enumNames.find(name);

	if (itrType != _enumNames.cend())
	{
		auto& enumType = _enumTypes[itrType->second];

		peg::for_each_child<
			peg::enum_value_definition>(enumTypeExtension, [&enumType](const peg::ast_node& child) {
			EnumValueType value;

			peg::on_first_child<peg::enum_value>(child, [&value](const peg::ast_node& enumValue) {
				value.value = enumValue.string_view();
				value.cppValue = getSafeCppName(value.value);
			});

			peg::on_first_child<peg::description>(child,
				[&value](const peg::ast_node& description) {
					if (!description.children.empty())
					{
						value.description = description.children.front()->unescaped_view();
					}
				});

			peg::on_first_child<peg::directives>(child, [&value](const peg::ast_node& directives) {
				peg::for_each_child<peg::directive>(directives,
					[&value](const peg::ast_node& directive) {
						std::string_view directiveName;

						peg::on_first_child<peg::directive_name>(directive,
							[&directiveName](const peg::ast_node& name) {
								directiveName = name.string_view();
							});

						if (directiveName == "deprecated")
						{
							std::string_view reason;

							peg::on_first_child<peg::arguments>(directive,
								[&value](const peg::ast_node& arguments) {
									peg::on_first_child<peg::argument>(arguments,
										[&value](const peg::ast_node& argument) {
											std::string_view argumentName;

											peg::on_first_child<peg::argument_name>(argument,
												[&argumentName](const peg::ast_node& name) {
													argumentName = name.string_view();
												});

											if (argumentName == "reason")
											{
												peg::on_first_child<peg::string_value>(argument,
													[&value](const peg::ast_node& argumentValue) {
														value.deprecationReason =
															argumentValue.unescaped_view();
													});
											}
										});
								});
						}
					});
			});

			value.position = child.begin();
			enumType.values.push_back(std::move(value));
		});
	}
}

void Generator::visitScalarTypeDefinition(const peg::ast_node& scalarTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::scalar_name>(scalarTypeDefinition,
		[&name](const peg::ast_node& child) {
			name = child.string_view();
		});

	peg::on_first_child<peg::description>(scalarTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_clientTypes[name] = SchemaType::Scalar;
	_typePositions.emplace(name, scalarTypeDefinition.begin());
	_scalarNames[name] = _scalarTypes.size();
	_scalarTypes.push_back({ name, description });
}

void Generator::visitUnionTypeDefinition(const peg::ast_node& unionTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::union_name>(unionTypeDefinition, [&name](const peg::ast_node& child) {
		name = child.string_view();
	});

	peg::on_first_child<peg::description>(unionTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_clientTypes[name] = SchemaType::Union;
	_typePositions.emplace(name, unionTypeDefinition.begin());
	_unionNames[name] = _unionTypes.size();

	auto cppName = getSafeCppName(name);

	_unionTypes.push_back({ name, cppName, {}, description });

	visitUnionTypeExtension(unionTypeDefinition);
}

void Generator::visitUnionTypeExtension(const peg::ast_node& unionTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::union_name>(unionTypeExtension, [&name](const peg::ast_node& child) {
		name = child.string_view();
	});

	const auto itrType = _unionNames.find(name);

	if (itrType != _unionNames.cend())
	{
		auto& unionType = _unionTypes[itrType->second];

		peg::for_each_child<peg::union_type>(unionTypeExtension,
			[&unionType](const peg::ast_node& child) {
				unionType.options.push_back(child.string_view());
			});
	}
}

void Generator::visitDirectiveDefinition(const peg::ast_node& directiveDefinition)
{
	Directive directive;

	peg::on_first_child<peg::directive_name>(directiveDefinition,
		[&directive](const peg::ast_node& child) {
			directive.name = child.string_view();
		});

	peg::on_first_child<peg::description>(directiveDefinition,
		[&directive](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				directive.description = child.children.front()->unescaped_view();
			}
		});

	peg::for_each_child<peg::directive_location>(directiveDefinition,
		[&directive](const peg::ast_node& child) {
			directive.locations.push_back(child.string_view());
		});

	peg::on_first_child<peg::arguments_definition>(directiveDefinition,
		[&directive](const peg::ast_node& child) {
			auto fields = getInputFields(child.children);

			directive.arguments.reserve(directive.arguments.size() + fields.size());
			for (auto& field : fields)
			{
				directive.arguments.push_back(std::move(field));
			}
		});

	_directivePositions.emplace(directive.name, directiveDefinition.begin());
	_directives.push_back(std::move(directive));
}

std::string_view Generator::getSafeCppName(std::string_view type) noexcept
{
	// The C++ standard reserves all names starting with '_' followed by a capital letter,
	// and all names that contain a double '_'. So we need to strip those from the types used
	// in GraphQL when declaring C++ types.
	static const std::regex multiple_(R"re(_{2,})re",
		std::regex::optimize | std::regex::ECMAScript);
	static const std::regex leading_Capital(R"re(^_([A-Z]))re",
		std::regex::optimize | std::regex::ECMAScript);

	// Cache the substitutions so we don't need to repeat a replacement.
	static std::unordered_map<std::string_view, std::string> safeNames;
	auto itr = safeNames.find(type);
	std::string cppName { type };

	if (safeNames.cend() == itr
		&& (std::regex_search(cppName, leading_Capital) || std::regex_search(cppName, multiple_)))
	{
		std::tie(itr, std::ignore) = safeNames.emplace(type,
			std::regex_replace(std::regex_replace(cppName, multiple_, R"re(_)re"),
				leading_Capital,
				R"re($1)re"));
	}

	return (safeNames.cend() == itr) ? type : itr->second;
}

OutputFieldList Generator::getOutputFields(const peg::ast_node::children_t& fields)
{
	OutputFieldList outputFields;

	for (const auto& fieldDefinition : fields)
	{
		OutputField field;
		TypeVisitor fieldType;

		for (const auto& child : fieldDefinition->children)
		{
			if (child->is_type<peg::field_name>())
			{
				field.name = child->string_view();
				field.cppName = getSafeCppName(field.name);
			}
			else if (child->is_type<peg::arguments_definition>())
			{
				field.arguments = getInputFields(child->children);
			}
			else if (child->is_type<peg::named_type>() || child->is_type<peg::list_type>()
				|| child->is_type<peg::nonnull_type>())
			{
				fieldType.visit(*child);
			}
			else if (child->is_type<peg::description>() && !child->children.empty())
			{
				field.description = child->children.front()->unescaped_view();
			}
			else if (child->is_type<peg::directives>())
			{
				peg::for_each_child<peg::directive>(*child,
					[&field](const peg::ast_node& directive) {
						std::string_view directiveName;

						peg::on_first_child<peg::directive_name>(directive,
							[&directiveName](const peg::ast_node& name) {
								directiveName = name.string_view();
							});

						if (directiveName == "deprecated")
						{
							std::string_view deprecationReason;

							peg::on_first_child<peg::arguments>(directive,
								[&deprecationReason](const peg::ast_node& arguments) {
									peg::on_first_child<peg::argument>(arguments,
										[&deprecationReason](const peg::ast_node& argument) {
											std::string_view argumentName;

											peg::on_first_child<peg::argument_name>(argument,
												[&argumentName](const peg::ast_node& name) {
													argumentName = name.string_view();
												});

											if (argumentName == "reason")
											{
												peg::on_first_child<peg::string_value>(argument,
													[&deprecationReason](
														const peg::ast_node& reason) {
														deprecationReason = reason.unescaped_view();
													});
											}
										});
								});

							field.deprecationReason = std::move(deprecationReason);
						}
					});
			}
		}

		std::tie(field.type, field.modifiers) = fieldType.getType();
		field.position = fieldDefinition->begin();
		outputFields.push_back(std::move(field));
	}

	return outputFields;
}

InputFieldList Generator::getInputFields(const peg::ast_node::children_t& fields)
{
	InputFieldList inputFields;

	for (const auto& fieldDefinition : fields)
	{
		InputField field;
		TypeVisitor fieldType;
		service::schema_location defaultValueLocation;

		for (const auto& child : fieldDefinition->children)
		{
			if (child->is_type<peg::argument_name>())
			{
				field.name = child->string_view();
				field.cppName = getSafeCppName(field.name);
			}
			else if (child->is_type<peg::named_type>() || child->is_type<peg::list_type>()
				|| child->is_type<peg::nonnull_type>())
			{
				fieldType.visit(*child);
			}
			else if (child->is_type<peg::default_value>())
			{
				const auto position = child->begin();
				DefaultValueVisitor defaultValue;

				defaultValue.visit(*child->children.back());
				field.defaultValue = defaultValue.getValue();
				field.defaultValueString = child->children.back()->string_view();

				defaultValueLocation = { position.line, position.column };
			}
			else if (child->is_type<peg::description>() && !child->children.empty())
			{
				field.description = child->children.front()->unescaped_view();
			}
		}

		std::tie(field.type, field.modifiers) = fieldType.getType();
		field.position = fieldDefinition->begin();

		if (!field.defaultValueString.empty() && field.defaultValue.type() == response::Type::Null
			&& (field.modifiers.empty()
				|| field.modifiers.front() != service::TypeModifier::Nullable))
		{
			std::ostringstream error;

			error << "Expected Non-Null default value for field name: " << field.name
				  << " line: " << defaultValueLocation.line
				  << " column: " << defaultValueLocation.column;

			throw std::runtime_error(error.str());
		}

		inputFields.push_back(std::move(field));
	}

	return inputFields;
}

void Generator::TypeVisitor::visit(const peg::ast_node& typeName)
{
	if (typeName.is_type<peg::nonnull_type>())
	{
		visitNonNullType(typeName);
	}
	else if (typeName.is_type<peg::list_type>())
	{
		visitListType(typeName);
	}
	else if (typeName.is_type<peg::named_type>())
	{
		visitNamedType(typeName);
	}
}

void Generator::TypeVisitor::visitNamedType(const peg::ast_node& namedType)
{
	if (!_nonNull)
	{
		_modifiers.push_back(service::TypeModifier::Nullable);
	}

	_type = namedType.string_view();
}

void Generator::TypeVisitor::visitListType(const peg::ast_node& listType)
{
	if (!_nonNull)
	{
		_modifiers.push_back(service::TypeModifier::Nullable);
	}
	_nonNull = false;

	_modifiers.push_back(service::TypeModifier::List);

	visit(*listType.children.front());
}

void Generator::TypeVisitor::visitNonNullType(const peg::ast_node& nonNullType)
{
	_nonNull = true;

	visit(*nonNullType.children.front());
}

std::pair<std::string_view, TypeModifierStack> Generator::TypeVisitor::getType()
{
	return { std::move(_type), std::move(_modifiers) };
}

void Generator::DefaultValueVisitor::visit(const peg::ast_node& value)
{
	if (value.is_type<peg::integer_value>())
	{
		visitIntValue(value);
	}
	else if (value.is_type<peg::float_value>())
	{
		visitFloatValue(value);
	}
	else if (value.is_type<peg::string_value>())
	{
		visitStringValue(value);
	}
	else if (value.is_type<peg::true_keyword>() || value.is_type<peg::false_keyword>())
	{
		visitBooleanValue(value);
	}
	else if (value.is_type<peg::null_keyword>())
	{
		visitNullValue(value);
	}
	else if (value.is_type<peg::enum_value>())
	{
		visitEnumValue(value);
	}
	else if (value.is_type<peg::list_value>())
	{
		visitListValue(value);
	}
	else if (value.is_type<peg::object_value>())
	{
		visitObjectValue(value);
	}
	else if (value.is_type<peg::variable>())
	{
		std::ostringstream error;
		const auto position = value.begin();

		error << "Unexpected variable in default value line: " << position.line
			  << " column: " << position.column;

		throw std::runtime_error(error.str());
	}
}

void Generator::DefaultValueVisitor::visitIntValue(const peg::ast_node& intValue)
{
	_value = response::Value(std::atoi(intValue.string().c_str()));
}

void Generator::DefaultValueVisitor::visitFloatValue(const peg::ast_node& floatValue)
{
	_value = response::Value(std::atof(floatValue.string().c_str()));
}

void Generator::DefaultValueVisitor::visitStringValue(const peg::ast_node& stringValue)
{
	_value = response::Value(std::string { stringValue.unescaped_view() });
}

void Generator::DefaultValueVisitor::visitBooleanValue(const peg::ast_node& booleanValue)
{
	_value = response::Value(booleanValue.is_type<peg::true_keyword>());
}

void Generator::DefaultValueVisitor::visitNullValue(const peg::ast_node& /*nullValue*/)
{
	_value = {};
}

void Generator::DefaultValueVisitor::visitEnumValue(const peg::ast_node& enumValue)
{
	_value = response::Value(response::Type::EnumValue);
	_value.set<response::StringType>(enumValue.string());
}

void Generator::DefaultValueVisitor::visitListValue(const peg::ast_node& listValue)
{
	_value = response::Value(response::Type::List);
	_value.reserve(listValue.children.size());

	for (const auto& child : listValue.children)
	{
		DefaultValueVisitor visitor;

		visitor.visit(*child);
		_value.emplace_back(visitor.getValue());
	}
}

void Generator::DefaultValueVisitor::visitObjectValue(const peg::ast_node& objectValue)
{
	_value = response::Value(response::Type::Map);
	_value.reserve(objectValue.children.size());

	for (const auto& field : objectValue.children)
	{
		DefaultValueVisitor visitor;

		visitor.visit(*field->children.back());
		_value.emplace_back(field->children.front()->string(), visitor.getValue());
	}
}

response::Value Generator::DefaultValueVisitor::getValue()
{
	return response::Value(std::move(_value));
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

std::string_view Generator::getCppType(std::string_view type) const noexcept
{
	auto itrBuiltin = s_builtinTypes.find(type);

	if (itrBuiltin != s_builtinTypes.cend())
	{
		if (static_cast<size_t>(itrBuiltin->second) < s_builtinCppTypes.size())
		{
			return s_builtinCppTypes[static_cast<size_t>(itrBuiltin->second)];
		}
	}
	else
	{
		auto itrScalar = _scalarNames.find(type);

		if (itrScalar != _scalarNames.cend())
		{
			return s_scalarCppType;
		}
	}

	return getSafeCppName(type);
}

std::string Generator::getInputCppType(const InputField& field) const noexcept
{
	size_t templateCount = 0;
	std::ostringstream inputType;

	for (auto modifier : field.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::Nullable:
				inputType << R"cpp(std::optional<)cpp";
				++templateCount;
				break;

			case service::TypeModifier::List:
				inputType << R"cpp(std::vector<)cpp";
				++templateCount;
				break;

			default:
				break;
		}
	}

	inputType << getCppType(field.type);

	for (size_t i = 0; i < templateCount; ++i)
	{
		inputType << R"cpp(>)cpp";
	}

	return inputType.str();
}

std::string Generator::getOutputCppType(const OutputField& field) const noexcept
{
	bool nonNull = true;
	size_t templateCount = 0;
	std::ostringstream outputType;

	for (auto modifier : field.modifiers)
	{
		if (!nonNull)
		{
			outputType << R"cpp(std::optional<)cpp";
			++templateCount;
		}

		switch (modifier)
		{
			case service::TypeModifier::None:
				nonNull = true;
				break;

			case service::TypeModifier::Nullable:
				nonNull = false;
				break;

			case service::TypeModifier::List:
				nonNull = true;
				outputType << R"cpp(std::vector<)cpp";
				++templateCount;
				break;
		}
	}

	switch (field.fieldType)
	{
		case OutputFieldType::Object:
		case OutputFieldType::Union:
		case OutputFieldType::Interface:
			// Even if it's non-nullable, we still want to return a shared_ptr for complex types
			outputType << R"cpp(std::shared_ptr<)cpp";
			++templateCount;
			break;

		default:
			if (!nonNull)
			{
				outputType << R"cpp(std::optional<)cpp";
				++templateCount;
			}
			break;
	}

	switch (field.fieldType)
	{
		case OutputFieldType::Builtin:
		case OutputFieldType::Scalar:
		case OutputFieldType::Enum:
			outputType << getCppType(field.type);
			break;

		case OutputFieldType::Object:
			if (field.interfaceField)
			{
				outputType << R"cpp(object::)cpp";
			}

			outputType << getSafeCppName(field.type);
			break;

		case OutputFieldType::Union:
		case OutputFieldType::Interface:
			outputType << R"cpp(service::Object)cpp";
			break;
	}

	for (size_t i = 0; i < templateCount; ++i)
	{
		outputType << R"cpp(>)cpp";
	}

	return outputType.str();
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
	NamespaceScope clientNamespace { headerFile, _clientNamespace };
	NamespaceScope objectNamespace { headerFile, "object", true };
	PendingBlankLine pendingSeparator { headerFile };

	std::string_view queryType;

	for (const auto& operation : _operationTypes)
	{
		if (operation.operation == service::strQuery)
		{
			queryType = operation.type;
			break;
		}
	}

	if (!_enumTypes.empty())
	{
		pendingSeparator.reset();

		for (const auto& enumType : _enumTypes)
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

	if (!_inputTypes.empty())
	{
		pendingSeparator.reset();

		// Forward declare all of the input types
		if (_inputTypes.size() > 1)
		{
			for (const auto& inputType : _inputTypes)
			{
				headerFile << R"cpp(struct )cpp" << inputType.cppType << R"cpp(;
)cpp";
			}

			headerFile << std::endl;
		}

		// Output the full declarations
		for (const auto& inputType : _inputTypes)
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

	if (!_objectTypes.empty())
	{
		objectNamespace.enter();
		headerFile << std::endl;

		// Forward declare all of the object types
		for (const auto& objectType : _objectTypes)
		{
			headerFile << R"cpp(class )cpp" << objectType.cppType << R"cpp(;
)cpp";
		}

		headerFile << std::endl;
	}

	if (!_interfaceTypes.empty())
	{
		if (objectNamespace.exit())
		{
			headerFile << std::endl;
		}

		// Forward declare all of the interface types
		if (_interfaceTypes.size() > 1)
		{
			for (const auto& interfaceType : _interfaceTypes)
			{
				headerFile << R"cpp(struct )cpp" << interfaceType.cppType << R"cpp(;
)cpp";
			}

			headerFile << std::endl;
		}

		// Output the full declarations
		for (const auto& interfaceType : _interfaceTypes)
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

	if (!_objectTypes.empty())
	{
		if (objectNamespace.enter())
		{
			headerFile << std::endl;
		}

		// Output the full declarations
		for (const auto& objectType : _objectTypes)
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

	for (const auto& operation : _operationTypes)
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

	for (const auto& operation : _operationTypes)
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
	, public )cpp" << getSafeCppName(interfaceName);
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

	output << getInputCppType(inputField) << R"cpp( )cpp" << inputField.cppName;

	return output.str();
}

std::string Generator::getFieldDeclaration(const OutputField& outputField) const noexcept
{
	std::ostringstream output;
	std::string fieldName { outputField.cppName };

	fieldName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
	output << R"cpp(	virtual service::FieldResult<)cpp" << getOutputCppType(outputField)
		   << R"cpp(> )cpp" << outputField.accessor << fieldName
		   << R"cpp((service::FieldParams&& params)cpp";

	for (const auto& argument : outputField.arguments)
	{
		output << R"cpp(, )cpp" << getInputCppType(argument) << R"cpp(&& )cpp" << argument.cppName
			   << "Arg";
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

	if (!_enumTypes.empty() || !_inputTypes.empty())
	{
		NamespaceScope serviceNamespace { sourceFile, "service" };

		sourceFile << std::endl;

		for (const auto& enumType : _enumTypes)
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
)cpp" << _clientNamespace
					   << R"cpp(::)cpp" << enumType.cppType << R"cpp( ModifiedArgument<)cpp"
					   << _clientNamespace << R"cpp(::)cpp" << enumType.cppType
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
					   << _clientNamespace << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>(itr - s_names)cpp" << enumType.cppType << R"cpp(.cbegin());
}

template <>
std::future<service::ResolverResult> ModifiedResult<)cpp"
					   << _clientNamespace << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>::convert(service::FieldResult<)cpp" << _clientNamespace
					   << R"cpp(::)cpp" << enumType.cppType
					   << R"cpp(>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[]()cpp" << _clientNamespace
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

		for (const auto& inputType : _inputTypes)
		{
			bool firstField = true;

			sourceFile << R"cpp(template <>
)cpp" << _clientNamespace
					   << R"cpp(::)cpp" << inputType.cppType << R"cpp( ModifiedArgument<)cpp"
					   << _clientNamespace << R"cpp(::)cpp" << inputType.cppType
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

	NamespaceScope clientNamespace { sourceFile, _clientNamespace };
	std::string_view queryType;

	for (const auto& operation : _operationTypes)
	{
		if (operation.operation == service::strQuery)
		{
			queryType = operation.type;
			break;
		}
	}

	if (!_objectTypes.empty())
	{
		NamespaceScope objectNamespace { sourceFile, "object" };

		sourceFile << std::endl;

		for (const auto& objectType : _objectTypes)
		{
			outputObjectImplementation(sourceFile, objectType, objectType.type == queryType);
			sourceFile << std::endl;
		}
	}

	bool firstOperation = true;

	sourceFile << R"cpp(
Operations::Operations()cpp";

	for (const auto& operation : _operationTypes)
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

	for (const auto& operation : _operationTypes)
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

	for (const auto& operation : _operationTypes)
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
				   << getOutputCppType(outputField) << R"cpp(> )cpp" << objectType.cppType
				   << R"cpp(::)cpp" << outputField.accessor << fieldName
				   << R"cpp((service::FieldParams&&)cpp";
		for (const auto& argument : outputField.arguments)
		{
			sourceFile << R"cpp(, )cpp" << getInputCppType(argument) << R"cpp(&&)cpp";
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
			argumentType << getCppType(argument.type);
			break;

		case InputFieldType::Enum:
		case InputFieldType::Input:
			argumentType << _clientNamespace << R"cpp(::)cpp" << getCppType(argument.type);
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
			resultType << getCppType(result.type);
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

} /* namespace graphql::client */

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
	std::string clientNamespace;
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
		po::value(&clientNamespace),
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
			|| !clientNamespace.empty();

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
			else if (clientNamespace.empty())
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
		const auto files = graphql::client::Generator(
			{
				graphql::client::GeneratorClient { std::move(schemaFileName),
					std::move(requestFileName),
					std::move(filenamePrefix),
					std::move(clientNamespace) },
				graphql::client::GeneratorPaths { std::move(headerDir), std::move(sourceDir) },
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