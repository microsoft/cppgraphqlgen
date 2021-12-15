// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "SchemaLoader.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace std::literals;

namespace graphql::generator {

const std::string_view SchemaLoader::s_introspectionNamespace = "introspection"sv;

const BuiltinTypeMap SchemaLoader::s_builtinTypes = {
	{ "Int"sv, BuiltinType::Int },
	{ "Float"sv, BuiltinType::Float },
	{ "String"sv, BuiltinType::String },
	{ "Boolean"sv, BuiltinType::Boolean },
	{ "ID"sv, BuiltinType::ID },
};

const CppTypeMap SchemaLoader::s_builtinCppTypes = {
	"int"sv,
	"double"sv,
	"std::string"sv,
	"bool"sv,
	"response::IdType"sv,
};

const std::string_view SchemaLoader::s_scalarCppType = R"cpp(response::Value)cpp"sv;

SchemaLoader::SchemaLoader(SchemaOptions&& schemaOptions)
	: _schemaOptions(std::move(schemaOptions))
	, _isIntrospection(_schemaOptions.isIntrospection)
	, _schemaNamespace(_schemaOptions.schemaNamespace)
{
	_ast = peg::parseSchemaFile(_schemaOptions.schemaFilename);

	if (!_ast.root)
	{
		throw std::logic_error("Unable to parse the service schema, but there was no error "
							   "message from the parser!");
	}

	for (const auto& child : _ast.root->children)
	{
		visitDefinition(*child);
	}

	validateSchema();
}

void SchemaLoader::validateSchema()
{
	// Verify that none of the custom types conflict with a built-in type.
	for (const auto& entry : _schemaTypes)
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

	// Handle nested input types by fully declaring the dependencies first.
	reorderInputTypeDependencies();

	// Validate the interface dependencies and that all of the interface fields are implemented.
	validateImplementedInterfaces();

	for (auto& entry : _interfaceTypes)
	{
		fixupOutputFieldList(entry.fields, std::nullopt, std::nullopt);
	}

	if (!_isIntrospection)
	{
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
				_operationTypes.push_back({ strDefaultMutation,
					getSafeCppName(strDefaultMutation),
					service::strMutation });
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
	}
	else if (!_operationTypes.empty())
	{
		throw std::runtime_error("Introspection should not define any operation types");
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

void SchemaLoader::fixupOutputFieldList(OutputFieldList& fields,
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

		auto itr = _schemaTypes.find(entry.type);

		if (itr == _schemaTypes.cend())
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

void SchemaLoader::fixupInputFieldList(InputFieldList& fields)
{
	for (auto& entry : fields)
	{
		if (s_builtinTypes.find(entry.type) != s_builtinTypes.cend())
		{
			continue;
		}

		auto itr = _schemaTypes.find(entry.type);

		if (itr == _schemaTypes.cend())
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

void SchemaLoader::reorderInputTypeDependencies()
{
	// Build the dependency list for each input type.
	std::for_each(_inputTypes.begin(), _inputTypes.end(), [](InputType& entry) noexcept {
		std::for_each(entry.fields.cbegin(),
			entry.fields.cend(),
			[&entry](const InputField& field) noexcept {
				if (field.fieldType == InputFieldType::Input)
				{
					entry.dependencies.insert(field.type);
				}
			});
	});

	std::unordered_set<std::string_view> handled;
	auto itr = _inputTypes.begin();

	while (itr != _inputTypes.end())
	{
		// Put all of the input types without unhandled dependencies at the front.
		const auto itrDependent = std::stable_partition(itr,
			_inputTypes.end(),
			[&handled](const InputType& entry) noexcept {
				return std::find_if(entry.dependencies.cbegin(),
						   entry.dependencies.cend(),
						   [&handled](std::string_view dependency) noexcept {
							   return handled.find(dependency) == handled.cend();
						   })
					== entry.dependencies.cend();
			});

		// Check to make sure we made progress.
		if (itrDependent == itr)
		{
			std::ostringstream error;

			error << "Input object cycle type: " << itr->type;

			throw std::runtime_error(error.str());
		}

		if (itrDependent != _inputTypes.end())
		{
			std::for_each(itr, itrDependent, [&handled](const InputType& entry) noexcept {
				handled.insert(entry.type);
			});
		}

		itr = itrDependent;
	}
}

void SchemaLoader::validateImplementedInterfaces() const
{
	for (const auto& interfaceType : _interfaceTypes)
	{
		validateTransitiveInterfaces(interfaceType.type, interfaceType.interfaces);

		for (auto interfaceName : interfaceType.interfaces)
		{
			validateInterfaceFields(interfaceType.type, interfaceName, interfaceType.fields);
		}
	}

	for (const auto& objectType : _objectTypes)
	{
		validateTransitiveInterfaces(objectType.type, objectType.interfaces);

		for (auto interfaceName : objectType.interfaces)
		{
			validateInterfaceFields(objectType.type, interfaceName, objectType.fields);
		}
	}
}

void SchemaLoader::visitDefinition(const peg::ast_node& definition)
{
	if (definition.is_type<peg::schema_definition>())
	{
		visitSchemaDefinition(definition);
	}
	else if (definition.is_type<peg::schema_extension>())
	{
		visitSchemaExtension(definition);
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

void SchemaLoader::visitSchemaDefinition(const peg::ast_node& schemaDefinition)
{
	std::string_view description;

	peg::on_first_child<peg::description>(schemaDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	if (_schemaDescription.empty())
	{
		_schemaDescription = description;
	}

	peg::for_each_child<peg::root_operation_definition>(schemaDefinition,
		[this](const peg::ast_node& child) {
			const auto operation(child.children.front()->string_view());
			const auto name(child.children.back()->string_view());
			const auto cppName(getSafeCppName(name));

			_operationTypes.push_back({ name, cppName, operation });
		});
}

void SchemaLoader::visitSchemaExtension(const peg::ast_node& schemaExtension)
{
	peg::for_each_child<peg::operation_type_definition>(schemaExtension,
		[this](const peg::ast_node& child) {
			const auto operation(child.children.front()->string_view());
			const auto name(child.children.back()->string_view());
			const auto cppName(getSafeCppName(name));

			_operationTypes.push_back({ name, cppName, operation });
		});
}

void SchemaLoader::visitObjectTypeDefinition(const peg::ast_node& objectTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::object_name>(objectTypeDefinition,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
		});

	peg::on_first_child<peg::description>(objectTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_schemaTypes[name] = SchemaType::Object;
	_typePositions.emplace(name, objectTypeDefinition.begin());
	_objectNames[name] = _objectTypes.size();

	auto cppName = getSafeCppName(name);

	_objectTypes.push_back({ name, cppName, {}, {}, {}, description });

	visitObjectTypeExtension(objectTypeDefinition);
}

void SchemaLoader::visitObjectTypeExtension(const peg::ast_node& objectTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::object_name>(objectTypeExtension,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
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

		if (!_isIntrospection)
		{
			for (const auto& field : objectType.fields)
			{
				blockReservedName(field.name, field.position);
				for (const auto& argument : field.arguments)
				{
					blockReservedName(argument.name, argument.position);
				}
			}
		}
	}
}

void SchemaLoader::visitInterfaceTypeDefinition(const peg::ast_node& interfaceTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::interface_name>(interfaceTypeDefinition,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
		});

	peg::on_first_child<peg::description>(interfaceTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_schemaTypes[name] = SchemaType::Interface;
	_typePositions.emplace(name, interfaceTypeDefinition.begin());
	_interfaceNames[name] = _interfaceTypes.size();

	auto cppName = getSafeCppName(name);

	_interfaceTypes.push_back({ name, cppName, {}, {}, description });

	visitInterfaceTypeExtension(interfaceTypeDefinition);
}

void SchemaLoader::visitInterfaceTypeExtension(const peg::ast_node& interfaceTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::interface_name>(interfaceTypeExtension,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
		});

	const auto itrType = _interfaceNames.find(name);

	if (itrType != _interfaceNames.cend())
	{
		auto& interfaceType = _interfaceTypes[itrType->second];

		peg::for_each_child<peg::interface_type>(interfaceTypeExtension,
			[&interfaceType](const peg::ast_node& child) {
				interfaceType.interfaces.push_back(child.string_view());
			});

		peg::on_first_child<peg::fields_definition>(interfaceTypeExtension,
			[&interfaceType](const peg::ast_node& child) {
				auto fields = getOutputFields(child.children);

				interfaceType.fields.reserve(interfaceType.fields.size() + fields.size());
				for (auto& field : fields)
				{
					interfaceType.fields.push_back(std::move(field));
				}
			});

		if (!_isIntrospection)
		{
			for (const auto& field : interfaceType.fields)
			{
				blockReservedName(field.name, field.position);
				for (const auto& argument : field.arguments)
				{
					blockReservedName(argument.name, argument.position);
				}
			}
		}
	}
}

void SchemaLoader::visitInputObjectTypeDefinition(const peg::ast_node& inputObjectTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::object_name>(inputObjectTypeDefinition,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
		});

	peg::on_first_child<peg::description>(inputObjectTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_schemaTypes[name] = SchemaType::Input;
	_typePositions.emplace(name, inputObjectTypeDefinition.begin());
	_inputNames[name] = _inputTypes.size();

	auto cppName = getSafeCppName(name);

	_inputTypes.push_back({ name, cppName, {}, description });

	visitInputObjectTypeExtension(inputObjectTypeDefinition);
}

void SchemaLoader::visitInputObjectTypeExtension(const peg::ast_node& inputObjectTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::object_name>(inputObjectTypeExtension,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
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

		if (!_isIntrospection)
		{
			for (const auto& field : inputType.fields)
			{
				blockReservedName(field.name, field.position);
			}
		}
	}
}

void SchemaLoader::visitEnumTypeDefinition(const peg::ast_node& enumTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::enum_name>(enumTypeDefinition,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
		});

	peg::on_first_child<peg::description>(enumTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_schemaTypes[name] = SchemaType::Enum;
	_typePositions.emplace(name, enumTypeDefinition.begin());
	_enumNames[name] = _enumTypes.size();

	auto cppName = getSafeCppName(name);

	_enumTypes.push_back({ name, cppName, {}, description });

	visitEnumTypeExtension(enumTypeDefinition);
}

void SchemaLoader::visitEnumTypeExtension(const peg::ast_node& enumTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::enum_name>(enumTypeExtension,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
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

						if (directiveName == "deprecated"sv)
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

											if (argumentName == "reason"sv)
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

void SchemaLoader::visitScalarTypeDefinition(const peg::ast_node& scalarTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::scalar_name>(scalarTypeDefinition,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
		});

	peg::on_first_child<peg::description>(scalarTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_schemaTypes[name] = SchemaType::Scalar;
	_typePositions.emplace(name, scalarTypeDefinition.begin());
	_scalarNames[name] = _scalarTypes.size();
	_scalarTypes.push_back({ name, description });

	visitScalarTypeExtension(scalarTypeDefinition);
}

void SchemaLoader::visitScalarTypeExtension(const peg::ast_node& scalarTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::scalar_name>(scalarTypeExtension,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
		});

	const auto itrType = _scalarNames.find(name);

	if (itrType != _scalarNames.cend())
	{
		auto& scalarType = _scalarTypes[itrType->second];

		peg::on_first_child<peg::directives>(scalarTypeExtension,
			[&scalarType](const peg::ast_node& directives) {
				peg::for_each_child<peg::directive>(directives,
					[&scalarType](const peg::ast_node& directive) {
						std::string_view directiveName;

						peg::on_first_child<peg::directive_name>(directive,
							[&directiveName](const peg::ast_node& name) {
								directiveName = name.string_view();
							});

						if (directiveName == "specifiedBy"sv)
						{
							std::string_view specifiedByURL;

							peg::on_first_child<peg::arguments>(directive,
								[&specifiedByURL](const peg::ast_node& arguments) {
									peg::on_first_child<peg::argument>(arguments,
										[&specifiedByURL](const peg::ast_node& argument) {
											std::string_view argumentName;

											peg::on_first_child<peg::argument_name>(argument,
												[&argumentName](const peg::ast_node& name) {
													argumentName = name.string_view();
												});

											if (argumentName == "url"sv)
											{
												peg::on_first_child<peg::string_value>(argument,
													[&specifiedByURL](const peg::ast_node& url) {
														specifiedByURL = url.unescaped_view();
													});
											}
										});
								});

							scalarType.specifiedByURL = std::move(specifiedByURL);
						}
					});
			});
	}
}

void SchemaLoader::visitUnionTypeDefinition(const peg::ast_node& unionTypeDefinition)
{
	std::string_view name;
	std::string_view description;

	peg::on_first_child<peg::union_name>(unionTypeDefinition,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
		});

	peg::on_first_child<peg::description>(unionTypeDefinition,
		[&description](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				description = child.children.front()->unescaped_view();
			}
		});

	_schemaTypes[name] = SchemaType::Union;
	_typePositions.emplace(name, unionTypeDefinition.begin());
	_unionNames[name] = _unionTypes.size();

	auto cppName = getSafeCppName(name);

	_unionTypes.push_back({ name, cppName, {}, description });

	visitUnionTypeExtension(unionTypeDefinition);
}

void SchemaLoader::visitUnionTypeExtension(const peg::ast_node& unionTypeExtension)
{
	std::string_view name;

	peg::on_first_child<peg::union_name>(unionTypeExtension,
		[isIntrospection = _isIntrospection, &name](const peg::ast_node& child) {
			name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(name, child.begin());
			}
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

void SchemaLoader::visitDirectiveDefinition(const peg::ast_node& directiveDefinition)
{
	Directive directive;

	peg::on_first_child<peg::directive_name>(directiveDefinition,
		[isIntrospection = _isIntrospection, &directive](const peg::ast_node& child) {
			directive.name = child.string_view();
			if (!isIntrospection)
			{
				blockReservedName(directive.name, child.begin());
			}
		});

	peg::on_first_child<peg::description>(directiveDefinition,
		[&directive](const peg::ast_node& child) {
			if (!child.children.empty())
			{
				directive.description = child.children.front()->unescaped_view();
			}
		});

	peg::on_first_child<peg::repeatable_keyword>(directiveDefinition,
		[&directive](const peg::ast_node& child) {
			directive.isRepeatable = true;
		});

	peg::for_each_child<peg::directive_location>(directiveDefinition,
		[&directive](const peg::ast_node& child) {
			directive.locations.push_back(child.string_view());
		});

	peg::on_first_child<peg::arguments_definition>(directiveDefinition,
		[isIntrospection = _isIntrospection, &directive](const peg::ast_node& child) {
			auto fields = getInputFields(child.children);

			directive.arguments.reserve(directive.arguments.size() + fields.size());
			for (auto& field : fields)
			{
				if (!isIntrospection)
				{
					blockReservedName(field.name, field.position);
				}
				directive.arguments.push_back(std::move(field));
			}
		});

	_directivePositions.emplace(directive.name, directiveDefinition.begin());
	_directives.push_back(std::move(directive));
}

std::string_view SchemaLoader::getSafeCppName(std::string_view type) noexcept
{
	// The C++ standard reserves all names starting with '_' followed by a capital letter,
	// and all names that contain a double '_'. So we need to strip those from the types used
	// in GraphQL when declaring C++ types.
	static const std::regex multiple_(R"re(_{2,})re",
		std::regex::optimize | std::regex::ECMAScript);
	static const std::regex leading_Capital(R"re(^_([A-Z]))re",
		std::regex::optimize | std::regex::ECMAScript);

	// Cache the substitutions so we don't need to repeat a replacement.
	using entry_allocation = std::pair<std::string, std::string>;
	static std::unordered_map<std::string_view, std::unique_ptr<entry_allocation>> safeNames;
	auto itr = safeNames.find(type);

	if (safeNames.cend() == itr)
	{
		std::string typeName { type };

		if (std::regex_search(typeName, multiple_) || std::regex_search(typeName, leading_Capital))
		{
			auto cppName = std::regex_replace(std::regex_replace(typeName, multiple_, R"re(_)re"),
				leading_Capital,
				R"re($1)re");

			auto entry = std::make_unique<entry_allocation>(
				entry_allocation { std::move(typeName), std::move(cppName) });

			std::tie(itr, std::ignore) = safeNames.emplace(entry->first, std::move(entry));
		}
	}

	return (safeNames.cend() == itr) ? type : itr->second->second;
}

void SchemaLoader::blockReservedName(
	std::string_view name, std::optional<tao::graphqlpeg::position> position)
{
	// https://spec.graphql.org/October2021/#sec-Names.Reserved-Names
	if (name.size() > 1 && name.substr(0, 2) == R"gql(__)gql"sv)
	{
		std::ostringstream error;

		error << "Names starting with __ are reserved: " << name;

		if (position)
		{
			error << " line: " << position->line << " column: " << position->column;
		}

		throw std::runtime_error(error.str());
	}
}

const InterfaceType& SchemaLoader::findInterfaceType(
	std::string_view typeName, std::string_view interfaceName) const
{
	const auto itrType = _interfaceNames.find(interfaceName);

	if (itrType == _interfaceNames.cend())
	{
		std::ostringstream error;
		const auto itrPosition = _typePositions.find(typeName);

		error << "Unknown interface: " << interfaceName << " implemented by: " << typeName;

		if (itrPosition != _typePositions.cend())
		{
			error << " line: " << itrPosition->second.line
				  << " column: " << itrPosition->second.column;
		}

		throw std::runtime_error(error.str());
	}

	return _interfaceTypes[itrType->second];
}

void SchemaLoader::validateInterfaceFields(std::string_view typeName,
	std::string_view interfaceName, const OutputFieldList& typeFields) const
{
	const auto& interfaceType = findInterfaceType(typeName, interfaceName);
	std::set<std::string_view> unimplemented;

	for (const auto& entry : interfaceType.fields)
	{
		unimplemented.insert(entry.name);
	}

	for (const auto& entry : typeFields)
	{
		unimplemented.erase(entry.name);
	}

	if (!unimplemented.empty())
	{
		std::ostringstream error;
		const auto itrPosition = _typePositions.find(typeName);

		error << "Missing interface fields type: " << typeName
			  << " interface: " << interfaceType.type;

		if (itrPosition != _typePositions.cend())
		{
			error << " line: " << itrPosition->second.line
				  << " column: " << itrPosition->second.column;
		}

		for (auto fieldName : unimplemented)
		{
			error << " field: " << fieldName;
		}

		throw std::runtime_error(error.str());
	}
}

void SchemaLoader::validateTransitiveInterfaces(
	std::string_view typeName, const std::vector<std::string_view>& interfaces) const
{
	std::set<std::string_view> unimplemented;

	for (auto entry : interfaces)
	{
		const auto& interfaceType = findInterfaceType(typeName, entry);

		unimplemented.insert(entry);

		for (auto interfaceName : interfaceType.interfaces)
		{
			unimplemented.insert(interfaceName);
		}
	}

	if (unimplemented.find(typeName) != unimplemented.cend())
	{
		std::ostringstream error;
		const auto itrPosition = _typePositions.find(typeName);

		error << "Interface cycle interface: " << typeName;

		if (itrPosition != _typePositions.cend())
		{
			error << " line: " << itrPosition->second.line
				  << " column: " << itrPosition->second.column;
		}

		throw std::runtime_error(error.str());
	}

	for (auto entry : interfaces)
	{
		unimplemented.erase(entry);
	}

	if (!unimplemented.empty())
	{
		std::ostringstream error;
		const auto itrPosition = _typePositions.find(typeName);

		error << "Missing transitive interface type: " << typeName;

		if (itrPosition != _typePositions.cend())
		{
			error << " line: " << itrPosition->second.line
				  << " column: " << itrPosition->second.column;
		}

		for (auto interfaceName : unimplemented)
		{
			error << " interface: " << interfaceName;
		}

		throw std::runtime_error(error.str());
	}
}

OutputFieldList SchemaLoader::getOutputFields(const peg::ast_node::children_t& fields)
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

						if (directiveName == "deprecated"sv)
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

											if (argumentName == "reason"sv)
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

InputFieldList SchemaLoader::getInputFields(const peg::ast_node::children_t& fields)
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

bool SchemaLoader::isIntrospection() const noexcept
{
	return _isIntrospection;
}

std::string_view SchemaLoader::getSchemaDescription() const noexcept
{
	return _schemaDescription;
}

std::string_view SchemaLoader::getFilenamePrefix() const noexcept
{
	return _schemaOptions.filenamePrefix;
}

std::string_view SchemaLoader::getSchemaNamespace() const noexcept
{
	return _schemaNamespace;
}

std::string_view SchemaLoader::getIntrospectionNamespace() noexcept
{
	return s_introspectionNamespace;
}

const BuiltinTypeMap& SchemaLoader::getBuiltinTypes() noexcept
{
	return s_builtinTypes;
}

const CppTypeMap& SchemaLoader::getBuiltinCppTypes() noexcept
{
	return s_builtinCppTypes;
}

std::string_view SchemaLoader::getScalarCppType() noexcept
{
	return s_scalarCppType;
}

SchemaType SchemaLoader::getSchemaType(std::string_view type) const
{
	return _schemaTypes.at(type);
}

const tao::graphqlpeg::position& SchemaLoader::getTypePosition(std::string_view type) const
{
	return _typePositions.at(type);
}

size_t SchemaLoader::getScalarIndex(std::string_view type) const
{
	return _scalarNames.at(type);
}

const ScalarTypeList& SchemaLoader::getScalarTypes() const noexcept
{
	return _scalarTypes;
}

size_t SchemaLoader::getEnumIndex(std::string_view type) const
{
	return _enumNames.at(type);
}

const EnumTypeList& SchemaLoader::getEnumTypes() const noexcept
{
	return _enumTypes;
}

size_t SchemaLoader::getInputIndex(std::string_view type) const
{
	return _inputNames.at(type);
}

const InputTypeList& SchemaLoader::getInputTypes() const noexcept
{
	return _inputTypes;
}

size_t SchemaLoader::getUnionIndex(std::string_view type) const
{
	return _unionNames.at(type);
}

const UnionTypeList& SchemaLoader::getUnionTypes() const noexcept
{
	return _unionTypes;
}

size_t SchemaLoader::getInterfaceIndex(std::string_view type) const
{
	return _interfaceNames.at(type);
}

const InterfaceTypeList& SchemaLoader::getInterfaceTypes() const noexcept
{
	return _interfaceTypes;
}

size_t SchemaLoader::getObjectIndex(std::string_view type) const
{
	return _objectNames.at(type);
}

const ObjectTypeList& SchemaLoader::getObjectTypes() const noexcept
{
	return _objectTypes;
}

const DirectiveList& SchemaLoader::getDirectives() const noexcept
{
	return _directives;
}

const tao::graphqlpeg::position& SchemaLoader::getDirectivePosition(std::string_view type) const
{
	return _directivePositions.at(type);
}

const OperationTypeList& SchemaLoader::getOperationTypes() const noexcept
{
	return _operationTypes;
}

std::string_view SchemaLoader::getCppType(std::string_view type) const noexcept
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

std::string SchemaLoader::getInputCppType(const InputField& field) const noexcept
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

std::string SchemaLoader::getOutputCppType(const OutputField& field) const noexcept
{
	bool nonNull = true;
	size_t templateCount = 0;
	std::ostringstream outputType;

	switch (field.fieldType)
	{
		case OutputFieldType::Object:
		case OutputFieldType::Union:
		case OutputFieldType::Interface:
			// Even if it's non-nullable, we still want to return a shared_ptr for complex types
			outputType << R"cpp(service::AwaitableObject<)cpp";
			++templateCount;
			break;

		default:
			outputType << R"cpp(service::AwaitableScalar<)cpp";
			++templateCount;
			break;
	}

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

		case OutputFieldType::Interface:
		case OutputFieldType::Union:
		case OutputFieldType::Object:
			outputType << getSafeCppName(field.type);
			break;
	}

	for (size_t i = 0; i < templateCount; ++i)
	{
		outputType << R"cpp(>)cpp";
	}

	return outputType.str();
}

} // namespace graphql::generator
