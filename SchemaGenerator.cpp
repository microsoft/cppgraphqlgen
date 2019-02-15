// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "SchemaGenerator.h"
#include "GraphQLGrammar.h"

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

namespace facebook {
namespace graphql {
namespace schema {

const std::string Generator::s_introspectionNamespace = "introspection";

const BuiltinTypeMap Generator::s_builtinTypes = {
		{ "Int", BuiltinType::Int },
		{ "Float", BuiltinType::Float },
		{ "String", BuiltinType::String },
		{ "Boolean", BuiltinType::Boolean },
		{ "ID", BuiltinType::ID },
};

const CppTypeMap Generator::s_builtinCppTypes = {
		"response::IntType",
		"response::FloatType",
		"response::StringType",
		"response::BooleanType",
		"std::vector<uint8_t>",
};

const std::string Generator::s_scalarCppType = R"cpp(response::Value)cpp";

Generator::Generator()
	: _isIntrospection(true)
	, _filenamePrefix("Introspection")
	, _schemaNamespace(s_introspectionNamespace)
{
	// Introspection Schema: https://facebook.github.io/graphql/June2018/#sec-Schema-Introspection
	auto ast = R"(
		type __Schema {
		  types: [__Type!]!
		  queryType: __Type!
		  mutationType: __Type
		  subscriptionType: __Type
		  directives: [__Directive!]!
		}

		type __Type {
		  kind: __TypeKind!
		  name: String
		  description: String

		  # OBJECT and INTERFACE only
		  fields(includeDeprecated: Boolean = false): [__Field!]

		  # OBJECT only
		  interfaces: [__Type!]

		  # INTERFACE and UNION only
		  possibleTypes: [__Type!]

		  # ENUM only
		  enumValues(includeDeprecated: Boolean = false): [__EnumValue!]

		  # INPUT_OBJECT only
		  inputFields: [__InputValue!]

		  # NON_NULL and LIST only
		  ofType: __Type
		}

		type __Field {
		  name: String!
		  description: String
		  args: [__InputValue!]!
		  type: __Type!
		  isDeprecated: Boolean!
		  deprecationReason: String
		}

		type __InputValue {
		  name: String!
		  description: String
		  type: __Type!
		  defaultValue: String
		}

		type __EnumValue {
		  name: String!
		  description: String
		  isDeprecated: Boolean!
		  deprecationReason: String
		}

		enum __TypeKind {
		  SCALAR
		  OBJECT
		  INTERFACE
		  UNION
		  ENUM
		  INPUT_OBJECT
		  LIST
		  NON_NULL
		}

		type __Directive {
		  name: String!
		  description: String
		  locations: [__DirectiveLocation!]!
		  args: [__InputValue!]!
		}

		enum __DirectiveLocation {
		  QUERY
		  MUTATION
		  SUBSCRIPTION
		  FIELD
		  FRAGMENT_DEFINITION
		  FRAGMENT_SPREAD
		  INLINE_FRAGMENT
		  SCHEMA
		  SCALAR
		  OBJECT
		  FIELD_DEFINITION
		  ARGUMENT_DEFINITION
		  INTERFACE
		  UNION
		  ENUM
		  ENUM_VALUE
		  INPUT_OBJECT
		  INPUT_FIELD_DEFINITION
		})"_graphql;

	if (!ast.root)
	{
		throw std::logic_error("Unable to parse the introspection schema, but there was no error message from the parser!");
	}

	for (const auto& child : ast.root->children)
	{
		visitDefinition(*child);
	}

	if (!validateSchema())
	{
		throw std::runtime_error("Invalid introspection schema!");
	}
}

Generator::Generator(std::string&& schemaFileName, std::string&& filenamePrefix, std::string&& schemaNamespace)
	: _isIntrospection(false)
	, _filenamePrefix(std::move(filenamePrefix))
	, _schemaNamespace(std::move(schemaNamespace))
{
	auto ast = peg::parseFile(schemaFileName.c_str());

	if (!ast.root)
	{
		throw std::logic_error("Unable to parse the service schema, but there was no error message from the parser!");
	}

	for (const auto& child : ast.root->children)
	{
		visitDefinition(*child);
	}

	if (!validateSchema())
	{
		throw std::runtime_error("Invalid service schema!");
	}
}

bool Generator::validateSchema()
{
	// Verify that none of the custom types conflict with a built-in type.
	for (const auto& entry : _schemaTypes)
	{
		if (s_builtinTypes.find(entry.first) != s_builtinTypes.cend())
		{
			return false;
		}
	}

	// Fixup all of the fieldType members.
	for (auto& entry : _inputTypes)
	{
		if (!fixupInputFieldList(entry.fields))
		{
			return false;
		}
	}

	for (auto& entry : _interfaceTypes)
	{
		if (!fixupOutputFieldList(entry.fields))
		{
			return false;
		}
	}

	for (auto& entry : _objectTypes)
	{
		if (!fixupOutputFieldList(entry.fields))
		{
			return false;
		}
	}

	// Validate the interfaces implemented by the object types.
	for (const auto& entry : _objectTypes)
	{
		for (const auto& interfaceName : entry.interfaces)
		{
			if (_interfaceNames.find(interfaceName) == _interfaceNames.cend())
			{
				return false;
			}
		}
	}

	return true;
}

bool Generator::fixupOutputFieldList(OutputFieldList& fields)
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
			return false;
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
				return false;
		}

		if (!fixupInputFieldList(entry.arguments))
		{
			return false;
		}
	}

	return true;
}

bool Generator::fixupInputFieldList(InputFieldList& fields)
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
			return false;
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
				return false;
		}
	}

	return true;
}

void Generator::visitDefinition(const peg::ast_node& definition)
{
	if (definition.is<peg::schema_definition>()
		|| definition.is<peg::schema_extension>())
	{
		visitSchemaDefinition(definition);
	}
	else if (definition.is<peg::scalar_type_definition>())
	{
		visitScalarTypeDefinition(definition);
	}
	else if (definition.is<peg::enum_type_definition>())
	{
		visitEnumTypeDefinition(definition);
	}
	else if (definition.is<peg::enum_type_extension>())
	{
		visitEnumTypeExtension(definition);
	}
	else if (definition.is<peg::input_object_type_definition>())
	{
		visitInputObjectTypeDefinition(definition);
	}
	else if (definition.is<peg::input_object_type_extension>())
	{
		visitInputObjectTypeExtension(definition);
	}
	else if (definition.is<peg::union_type_definition>())
	{
		visitUnionTypeDefinition(definition);
	}
	else if (definition.is<peg::union_type_extension>())
	{
		visitUnionTypeExtension(definition);
	}
	else if (definition.is<peg::interface_type_definition>())
	{
		visitInterfaceTypeDefinition(definition);
	}
	else if (definition.is<peg::interface_type_extension>())
	{
		visitInterfaceTypeExtension(definition);
	}
	else if (definition.is<peg::object_type_definition>())
	{
		visitObjectTypeDefinition(definition);
	}
	else if (definition.is<peg::object_type_extension>())
	{
		visitObjectTypeExtension(definition);
	}
	else if (definition.is<peg::directive_definition>())
	{
		visitDirectiveDefinition(definition);
	}
}

void Generator::visitSchemaDefinition(const peg::ast_node& schemaDefinition)
{
	peg::for_each_child<peg::root_operation_definition>(schemaDefinition,
		[this](const peg::ast_node & child)
		{
			std::string operation(child.children.front()->string());
			std::string name(child.children.back()->string());

			_operationTypes.push_back({ std::move(name), std::move(operation) });
		});
}

void Generator::visitObjectTypeDefinition(const peg::ast_node& objectTypeDefinition)
{
	std::string name;
	std::string description;

	peg::on_first_child<peg::object_name>(objectTypeDefinition,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	peg::on_first_child<peg::description>(objectTypeDefinition,
		[&description](const peg::ast_node & child)
		{
			description = child.children.front()->unescaped;
		});

	_schemaTypes[name] = SchemaType::Object;
	_objectNames[name] = _objectTypes.size();
	_objectTypes.push_back({ std::move(name), {}, {}, std::move(description) });

	visitObjectTypeExtension(objectTypeDefinition);
}

void Generator::visitObjectTypeExtension(const peg::ast_node& objectTypeExtension)
{
	std::string name;

	peg::on_first_child<peg::object_name>(objectTypeExtension,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	const auto itrType = _objectNames.find(name);

	if (itrType != _objectNames.cend())
	{
		auto& objectType = _objectTypes[itrType->second];

		peg::for_each_child<peg::interface_type>(objectTypeExtension,
			[&objectType](const peg::ast_node & child)
			{
				objectType.interfaces.push_back(child.string());
			});

		peg::on_first_child<peg::fields_definition>(objectTypeExtension,
			[&objectType](const peg::ast_node & child)
			{
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
	std::string name;
	std::string description;

	peg::on_first_child<peg::interface_name>(interfaceTypeDefinition,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	peg::on_first_child<peg::description>(interfaceTypeDefinition,
		[&description](const peg::ast_node & child)
		{
			description = child.children.front()->unescaped;
		});

	_schemaTypes[name] = SchemaType::Interface;
	_interfaceNames[name] = _interfaceTypes.size();
	_interfaceTypes.push_back({ std::move(name), {}, std::move(description) });

	visitInterfaceTypeExtension(interfaceTypeDefinition);
}

void Generator::visitInterfaceTypeExtension(const peg::ast_node& interfaceTypeExtension)
{
	std::string name;

	peg::on_first_child<peg::interface_name>(interfaceTypeExtension,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	const auto itrType = _interfaceNames.find(name);

	if (itrType != _interfaceNames.cend())
	{
		auto& interfaceType = _interfaceTypes[itrType->second];

		peg::on_first_child<peg::fields_definition>(interfaceTypeExtension,
			[&interfaceType](const peg::ast_node & child)
			{
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
	std::string name;
	std::string description;

	peg::on_first_child<peg::object_name>(inputObjectTypeDefinition,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	peg::on_first_child<peg::description>(inputObjectTypeDefinition,
		[&description](const peg::ast_node & child)
		{
			description = child.children.front()->unescaped;
		});

	_schemaTypes[name] = SchemaType::Input;
	_inputNames[name] = _inputTypes.size();
	_inputTypes.push_back({ std::move(name), {}, std::move(description) });

	visitInputObjectTypeExtension(inputObjectTypeDefinition);
}

void Generator::visitInputObjectTypeExtension(const peg::ast_node& inputObjectTypeExtension)
{
	std::string name;

	peg::on_first_child<peg::object_name>(inputObjectTypeExtension,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	const auto itrType = _inputNames.find(name);

	if (itrType != _inputNames.cend())
	{
		auto& inputType = _inputTypes[itrType->second];

		peg::on_first_child<peg::input_fields_definition>(inputObjectTypeExtension,
			[&inputType](const peg::ast_node & child)
			{
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
	std::string name;
	std::string description;

	peg::on_first_child<peg::enum_name>(enumTypeDefinition,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	peg::on_first_child<peg::description>(enumTypeDefinition,
		[&description](const peg::ast_node & child)
		{
			description = child.children.front()->unescaped;
		});

	_schemaTypes[name] = SchemaType::Enum;
	_enumNames[name] = _enumTypes.size();
	_enumTypes.push_back({ std::move(name), {}, std::move(description) });

	visitEnumTypeExtension(enumTypeDefinition);
}

void Generator::visitEnumTypeExtension(const peg::ast_node& enumTypeExtension)
{
	std::string name;

	peg::on_first_child<peg::enum_name>(enumTypeExtension,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	const auto itrType = _enumNames.find(name);

	if (itrType != _enumNames.cend())
	{
		auto& enumType = _enumTypes[itrType->second];

		peg::for_each_child<peg::enum_value_definition>(enumTypeExtension,
			[&enumType](const peg::ast_node & child)
			{
				std::string value;
				std::string valueDescription;
				std::unique_ptr<std::string> deprecationReason;

				peg::on_first_child<peg::enum_value>(child,
					[&value](const peg::ast_node & enumValue)
					{
						value = enumValue.string();
					});

				peg::on_first_child<peg::description>(child,
					[&valueDescription](const peg::ast_node & enumValue)
					{
						valueDescription = enumValue.children.front()->unescaped;
					});

				peg::on_first_child<peg::directives>(child,
					[&deprecationReason](const peg::ast_node & directives)
					{
						peg::for_each_child<peg::directive>(directives,
							[&deprecationReason](const peg::ast_node & directive)
							{
								std::string directiveName;

								peg::on_first_child<peg::directive_name>(directive,
									[&directiveName](const peg::ast_node & name)
									{
										directiveName = name.string();
									});

								if (directiveName == "deprecated")
								{
									std::string reason;

									peg::on_first_child<peg::arguments>(directive,
										[&reason](const peg::ast_node & arguments)
										{
											peg::on_first_child<peg::argument>(arguments,
												[&reason](const peg::ast_node & argument)
												{
													std::string argumentName;

													peg::on_first_child<peg::argument_name>(argument,
														[&argumentName](const peg::ast_node & name)
														{
															argumentName = name.string();
														});

													if (argumentName == "reason")
													{
														peg::on_first_child<peg::string_value>(argument,
															[&reason](const peg::ast_node & argumentValue)
															{
																reason = argumentValue.unescaped;
															});
													}
												});
										});

									deprecationReason.reset(new std::string(std::move(reason)));
								}
							});
					});

				enumType.values.push_back({ std::move(value), std::move(valueDescription), std::move(deprecationReason) });
			});
	}
}

void Generator::visitScalarTypeDefinition(const peg::ast_node& scalarTypeDefinition)
{
	std::string name;
	std::string description;

	peg::on_first_child<peg::scalar_name>(scalarTypeDefinition,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	peg::on_first_child<peg::description>(scalarTypeDefinition,
		[&description](const peg::ast_node & child)
		{
			description = child.children.front()->unescaped;
		});

	_schemaTypes[name] = SchemaType::Scalar;
	_scalarNames[name] = _scalarTypes.size();
	_scalarTypes.push_back({ std::move(name), std::move(description) });
}

void Generator::visitUnionTypeDefinition(const peg::ast_node& unionTypeDefinition)
{
	std::string name;
	std::string description;

	peg::on_first_child<peg::union_name>(unionTypeDefinition,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	peg::on_first_child<peg::description>(unionTypeDefinition,
		[&description](const peg::ast_node & child)
		{
			description = child.children.front()->unescaped;
		});

	_schemaTypes[name] = SchemaType::Union;
	_unionNames[name] = _unionTypes.size();
	_unionTypes.push_back({ std::move(name), {}, std::move(description) });

	visitUnionTypeExtension(unionTypeDefinition);
}

void Generator::visitUnionTypeExtension(const peg::ast_node& unionTypeExtension)
{
	std::string name;

	peg::on_first_child<peg::union_name>(unionTypeExtension,
		[&name](const peg::ast_node & child)
		{
			name = child.string();
		});

	const auto itrType = _unionNames.find(name);

	if (itrType != _unionNames.cend())
	{
		auto& unionType = _unionTypes[itrType->second];

		peg::for_each_child<peg::union_type>(unionTypeExtension,
			[&unionType](const peg::ast_node & child)
			{
				unionType.options.push_back(child.string());
			});
	}
}

void Generator::visitDirectiveDefinition(const peg::ast_node& directiveDefinition)
{
	Directive directive;

	peg::on_first_child<peg::directive_name>(directiveDefinition,
		[&directive](const peg::ast_node & child)
		{
			directive.name = child.string();
		});

	peg::on_first_child<peg::description>(directiveDefinition,
		[&directive](const peg::ast_node & child)
		{
			directive.description = child.children.front()->unescaped;
		});

	peg::for_each_child<peg::directive_location>(directiveDefinition,
		[&directive](const peg::ast_node & child)
		{
			directive.locations.push_back(child.string());
		});

	peg::on_first_child<peg::arguments_definition>(directiveDefinition,
		[&directive](const peg::ast_node & child)
		{
			auto fields = getInputFields(child.children);

			directive.arguments.reserve(directive.arguments.size() + fields.size());
			for (auto& field : fields)
			{
				directive.arguments.push_back(std::move(field));
			}
		});

	_directives.push_back(std::move(directive));
}

OutputFieldList Generator::getOutputFields(const std::vector<std::unique_ptr<peg::ast_node>>& fields)
{
	OutputFieldList outputFields;

	for (const auto& fieldDefinition : fields)
	{
		OutputField field;
		TypeVisitor fieldType;

		for (const auto& child : fieldDefinition->children)
		{
			if (child->is<peg::field_name>())
			{
				field.name = child->string();
			}
			else if (child->is<peg::arguments_definition>())
			{
				field.arguments = getInputFields(child->children);
			}
			else if (child->is<peg::named_type>()
				|| child->is<peg::list_type>()
				|| child->is<peg::nonnull_type>())
			{
				fieldType.visit(*child);
			}
			else if (child->is<peg::description>())
			{
				field.description = child->children.front()->unescaped;
			}
			else if (child->is<peg::directives>())
			{
				peg::for_each_child<peg::directive>(*child,
					[&field](const peg::ast_node & directive)
					{
						std::string directiveName;

						peg::on_first_child<peg::directive_name>(directive,
							[&directiveName](const peg::ast_node & name)
							{
								directiveName = name.string();
							});

						if (directiveName == "deprecated")
						{
							std::string deprecationReason;

							peg::on_first_child<peg::arguments>(directive,
								[&deprecationReason](const peg::ast_node & arguments)
								{
									peg::on_first_child<peg::argument>(arguments,
										[&deprecationReason](const peg::ast_node & argument)
										{
											std::string argumentName;

											peg::on_first_child<peg::argument_name>(argument,
												[&argumentName](const peg::ast_node & name)
												{
													argumentName = name.string();
												});

											if (argumentName == "reason")
											{
												peg::on_first_child<peg::string_value>(argument,
													[&deprecationReason](const peg::ast_node & reason)
													{
														deprecationReason = reason.unescaped;
													});
											}
										});
								});

							field.deprecationReason.reset(new std::string(std::move(deprecationReason)));
						}
					});
			}
		}

		std::tie(field.type, field.modifiers) = fieldType.getType();
		outputFields.push_back(std::move(field));
	}

	return outputFields;
}

InputFieldList Generator::getInputFields(const std::vector<std::unique_ptr<peg::ast_node>>& fields)
{
	InputFieldList inputFields;

	for (const auto& fieldDefinition : fields)
	{
		InputField field;
		TypeVisitor fieldType;

		for (const auto& child : fieldDefinition->children)
		{
			if (child->is<peg::argument_name>())
			{
				field.name = child->string();
			}
			else if (child->is<peg::named_type>()
				|| child->is<peg::list_type>()
				|| child->is<peg::nonnull_type>())
			{
				fieldType.visit(*child);
			}
			else if (child->is<peg::default_value>())
			{
				DefaultValueVisitor defaultValue;

				defaultValue.visit(*child->children.back());
				field.defaultValue = defaultValue.getValue();
				field.defaultValueString = child->children.back()->string();
			}
			else if (child->is<peg::description>())
			{
				field.description = child->children.front()->unescaped;
			}
		}

		std::tie(field.type, field.modifiers) = fieldType.getType();
		inputFields.push_back(std::move(field));
	}

	return inputFields;
}

void Generator::TypeVisitor::visit(const peg::ast_node& typeName)
{
	if (typeName.is<peg::nonnull_type>())
	{
		visitNonNullType(typeName);
	}
	else if (typeName.is<peg::list_type>())
	{
		visitListType(typeName);
	}
	else if (typeName.is<peg::named_type>())
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

	_type = namedType.string();
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

std::pair<std::string, TypeModifierStack> Generator::TypeVisitor::getType()
{
	return { std::move(_type), std::move(_modifiers) };
}

void Generator::DefaultValueVisitor::visit(const peg::ast_node& value)
{
	if (value.is<peg::integer_value>())
	{
		visitIntValue(value);
	}
	else if (value.is<peg::float_value>())
	{
		visitFloatValue(value);
	}
	else if (value.is<peg::string_value>())
	{
		visitStringValue(value);
	}
	else if (value.is<peg::true_keyword>()
		|| value.is<peg::false_keyword>())
	{
		visitBooleanValue(value);
	}
	else if (value.is<peg::null_keyword>())
	{
		visitNullValue(value);
	}
	else if (value.is<peg::enum_value>())
	{
		visitEnumValue(value);
	}
	else if (value.is<peg::list_value>())
	{
		visitListValue(value);
	}
	else if (value.is<peg::object_value>())
	{
		visitObjectValue(value);
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
	_value = response::Value(std::string(stringValue.unescaped));
}

void Generator::DefaultValueVisitor::visitBooleanValue(const peg::ast_node& booleanValue)
{
	_value = response::Value(booleanValue.is<peg::true_keyword>());
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

	if (outputHeader())
	{
		builtFiles.push_back(_filenamePrefix + "Schema.h");
	}

	if (outputSource())
	{
		builtFiles.push_back(_filenamePrefix + "Schema.cpp");
	}

	return builtFiles;
}

const std::string& Generator::getCppType(const std::string & type) const noexcept
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

	return type;
}

std::string Generator::getInputCppType(const InputField & field) const noexcept
{
	size_t templateCount = 0;
	std::ostringstream inputType;

	for (auto modifier : field.modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::Nullable:
				inputType << R"cpp(std::unique_ptr<)cpp";
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

std::string Generator::getOutputCppType(const OutputField & field, bool interfaceField) const noexcept
{
	bool nonNull = true;
	size_t templateCount = 0;
	std::ostringstream outputType;

	for (auto modifier : field.modifiers)
	{
		if (!nonNull)
		{
			outputType << R"cpp(std::unique_ptr<)cpp";
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
				outputType << R"cpp(std::unique_ptr<)cpp";
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
			if (interfaceField)
			{
				outputType << R"cpp(object::)cpp";
			}

			outputType << field.type;
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
	std::ofstream headerFile(_filenamePrefix + "Schema.h", std::ios_base::trunc);

	headerFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <graphqlservice/GraphQLService.h>

#include <memory>
#include <string>
#include <vector>

namespace facebook {
namespace graphql {
namespace introspection {

class Schema;
)cpp";

	std::string queryType;

	if (!_isIntrospection)
	{
		for (const auto& operation : _operationTypes)
		{
			if (operation.operation == "query")
			{
				queryType = operation.type;
				break;
			}
		}

		headerFile << R"cpp(
} /* namespace introspection */

namespace )cpp" << _schemaNamespace << R"cpp( {
)cpp";
	}

	if (!_enumTypes.empty())
	{
		for (const auto& enumType : _enumTypes)
		{
			headerFile << R"cpp(
enum class )cpp" << enumType.type << R"cpp(
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
		// Forward declare all of the input types
		if (_inputTypes.size() > 1)
		{
			for (const auto& inputType : _inputTypes)
			{
				headerFile << R"cpp(
struct )cpp" << inputType.type << R"cpp(;)cpp";
			}

			headerFile << R"cpp(
)cpp";
		}

		// Output the full declarations
		for (const auto& inputType : _inputTypes)
		{
			headerFile << R"cpp(
struct )cpp" << inputType.type << R"cpp(
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

	if (!_interfaceTypes.empty())
	{
		if (!_objectTypes.empty())
		{
			headerFile << R"cpp(
namespace object {
)cpp";

			// Forward declare all of the object types so the interface types can reference them
			for (const auto& objectType : _objectTypes)
			{
				headerFile << R"cpp(
class )cpp" << objectType.type << R"cpp(;)cpp";
			}

			headerFile << R"cpp(

} /* namespace object */
)cpp";
		}

		// Forward declare all of the interface types
		if (_interfaceTypes.size() > 1)
		{
			for (const auto& interfaceType : _interfaceTypes)
			{
				headerFile << R"cpp(
struct )cpp" << interfaceType.type << R"cpp(;)cpp";
			}

			headerFile << R"cpp(
)cpp";
		}

		// Output the full declarations
		for (const auto& interfaceType : _interfaceTypes)
		{
			headerFile << R"cpp(
struct )cpp" << interfaceType.type << R"cpp(
{
)cpp";
			for (const auto& outputField : interfaceType.fields)
			{
				headerFile << getFieldDeclaration(outputField, true);
			}
			headerFile << R"cpp(};
)cpp";
		}
	}

	headerFile << R"cpp(
)cpp";

	if (!_objectTypes.empty())
	{
		headerFile << R"cpp(namespace object {
)cpp";

		if (_interfaceTypes.empty()
			&& _objectTypes.size() > 1)
		{
			// Forward declare all of the object types if there were no interfaces so the
			// object types can reference one another
			for (const auto& objectType : _objectTypes)
			{
				headerFile << R"cpp(
class )cpp" << objectType.type << R"cpp(;)cpp";
			}

			headerFile << R"cpp(
)cpp";
		}

		// Output the full declarations
		for (const auto& objectType : _objectTypes)
		{
			std::unordered_set<std::string> interfaceFields;

			headerFile << R"cpp(
class )cpp" << objectType.type << R"cpp(
	: public service::Object)cpp";

			for (const auto& interfaceName : objectType.interfaces)
			{
				headerFile << R"cpp(
	, public )cpp" << interfaceName;

				auto itr = _interfaceNames.find(interfaceName);

				if (itr != _interfaceNames.cend())
				{
					for (const auto& field : _interfaceTypes[itr->second].fields)
					{
						interfaceFields.insert(field.name);
					}
				}
			}

			headerFile << R"cpp(
{
protected:
	)cpp" << objectType.type << R"cpp(();
)cpp";

			if (!objectType.fields.empty())
			{
				bool firstField = true;

				for (const auto& outputField : objectType.fields)
				{
					if (interfaceFields.find(outputField.name) != interfaceFields.cend())
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

					headerFile << getFieldDeclaration(outputField, false);
				}

				headerFile << R"cpp(
private:
)cpp";

				for (const auto& outputField : objectType.fields)
				{
					headerFile << getResolverDeclaration(outputField);
				}

				headerFile << R"cpp(
	std::future<response::Value> resolve__typename(service::ResolverParams&& params);
)cpp";

				if (objectType.type == queryType)
				{
					headerFile << R"cpp(	std::future<response::Value> resolve__schema(service::ResolverParams&& params);
	std::future<response::Value> resolve__type(service::ResolverParams&& params);

	std::shared_ptr<)cpp" << s_introspectionNamespace << R"cpp(::Schema> _schema;
)cpp";
				}
			}

			headerFile << R"cpp(};
)cpp";
		}
		headerFile << R"cpp(
} /* namespace object */
)cpp";
	}

	if (!_isIntrospection)
	{
		if (!_operationTypes.empty())
		{
			bool firstOperation = true;

			headerFile << R"cpp(
class Operations
	: public service::Request
{
public:
	Operations()cpp";

			for (const auto& operation : _operationTypes)
			{
				if (!firstOperation)
				{
					headerFile << R"cpp(, )cpp";
				}

				firstOperation = false;
				headerFile << R"cpp(std::shared_ptr<object::)cpp" << operation.type << R"cpp(> )cpp"
					<< operation.operation;
			}

			headerFile << R"cpp();

private:
)cpp";

			for (const auto& operation : _operationTypes)
			{
				headerFile << R"cpp(	std::shared_ptr<object::)cpp" << operation.type << R"cpp(> _)cpp"
					<< operation.operation << R"cpp(;
)cpp";
			}

			headerFile << R"cpp(};
)cpp";
		}
	}

	headerFile << R"cpp(
void AddTypesToSchema(std::shared_ptr<)cpp" << s_introspectionNamespace << R"cpp(::Schema> schema);

} /* namespace )cpp" << _schemaNamespace << R"cpp( */
} /* namespace graphql */
} /* namespace facebook */)cpp";

	return true;
}

std::string Generator::getFieldDeclaration(const InputField & inputField) const noexcept
{
	std::ostringstream output;

	output << getInputCppType(inputField) << R"cpp( )cpp"
		<< inputField.name;

	return output.str();
}

std::string Generator::getFieldDeclaration(const OutputField & outputField, bool interfaceField) const noexcept
{
	std::ostringstream output;
	std::string fieldName(outputField.name);

	fieldName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
	output << R"cpp(	virtual std::future<)cpp" << getOutputCppType(outputField, interfaceField)
		<< R"cpp(> get)cpp" << fieldName << R"cpp((service::FieldParams&& params)cpp";

	for (const auto& argument : outputField.arguments)
	{
		output << R"cpp(, )cpp" << getInputCppType(argument)
			<< R"cpp(&& )cpp" << argument.name << "Arg";
	}

	output << R"cpp() const = 0;
)cpp";

	return output.str();
}

std::string Generator::getResolverDeclaration(const OutputField & outputField) const noexcept
{
	std::ostringstream output;
	std::string fieldName(outputField.name);

	fieldName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
	output << R"cpp(	std::future<response::Value> resolve)cpp" << fieldName
		<< R"cpp((service::ResolverParams&& params);
)cpp";

	return output.str();
}

bool Generator::outputSource() const noexcept
{
	std::ofstream sourceFile(_filenamePrefix + "Schema.cpp", std::ios_base::trunc);

	sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

)cpp";
	if (!_isIntrospection)
	{
		sourceFile << R"cpp(#include ")cpp" << _filenamePrefix << R"cpp(Schema.h"

)cpp";
	}

	sourceFile << R"cpp(#include <graphqlservice/Introspection.h>

#include <algorithm>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <exception>

namespace facebook {
namespace graphql {)cpp";

	if (!_enumTypes.empty() || !_inputTypes.empty())
	{
		sourceFile << R"cpp(
namespace service {
)cpp";

		for (const auto& enumType : _enumTypes)
		{
			bool firstValue = true;

			sourceFile << R"cpp(
template <>
)cpp" << _schemaNamespace << R"cpp(::)cpp" << enumType.type
<< R"cpp( ModifiedArgument<)cpp" << _schemaNamespace << R"cpp(::)cpp" << enumType.type
<< R"cpp(>::convert(const response::Value& value)
{
	static const std::unordered_map<std::string, )cpp"
				<< _schemaNamespace << R"cpp(::)cpp" << enumType.type << R"cpp(> s_names = {
)cpp";

			for (const auto& value : enumType.values)
			{
				if (!firstValue)
				{
					sourceFile << R"cpp(,
)cpp";
				}

				firstValue = false;
				sourceFile << R"cpp(		{ ")cpp" << value.value << R"cpp(", )cpp"
					<< _schemaNamespace << R"cpp(::)cpp" << enumType.type
					<< R"cpp(::)cpp" << value.value << R"cpp( })cpp";
			}

			sourceFile << R"cpp(
	};

	if (!value.maybe_enum())
	{
		throw service::schema_exception({ "not a valid )cpp" << enumType.type << R"cpp( value" });
	}

	auto itr = s_names.find(value.get<const response::StringType&>());

	if (itr == s_names.cend())
	{
		throw service::schema_exception({ "not a valid )cpp" << enumType.type << R"cpp( value" });
	}

	return itr->second;
}

template <>
std::future<response::Value> ModifiedResult<)cpp" << _schemaNamespace << R"cpp(::)cpp" << enumType.type
<< R"cpp(>::convert(std::future<)cpp" << _schemaNamespace << R"cpp(::)cpp" << enumType.type
<< R"cpp(>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[]()cpp" << _schemaNamespace << R"cpp(::)cpp" << enumType.type
				<< R"cpp( && value, const ResolverParams&)
		{
			static const std::string s_names[] = {
		)cpp";

			firstValue = true;

			for (const auto& value : enumType.values)
			{
				if (!firstValue)
				{
					sourceFile << R"cpp(,
		)cpp";
				}

				firstValue = false;
				sourceFile << R"cpp(		")cpp" << value.value << R"cpp(")cpp";
			}

			sourceFile << R"cpp(
			};

			response::Value result(response::Type::EnumValue);

			result.set<response::StringType>(std::string(s_names[static_cast<size_t>(value)]));

			return result;
		});
}
)cpp";
		}

		for (const auto& inputType : _inputTypes)
		{
			bool firstField = true;

			sourceFile << R"cpp(
template <>
)cpp" << _schemaNamespace << R"cpp(::)cpp" << inputType.type
<< R"cpp( ModifiedArgument<)cpp" << _schemaNamespace << R"cpp(::)cpp" << inputType.type
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
				sourceFile << R"cpp(
)cpp";
			}

			sourceFile << R"cpp(	return {
)cpp";

			firstField = true;

			for (const auto& inputField : inputType.fields)
			{
				std::string fieldName(inputField.name);

				if (!firstField)
				{
					sourceFile << R"cpp(,
)cpp";
				}

				firstField = false;
				fieldName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
				sourceFile << R"cpp(		std::move(value)cpp" << fieldName << R"cpp())cpp";
			}

			sourceFile << R"cpp(
	};
}
)cpp";
		}

		sourceFile << R"cpp(
} /* namespace service */
)cpp";
	}

	sourceFile << R"cpp(
namespace )cpp" << _schemaNamespace << R"cpp( {)cpp";

	std::string queryType;

	if (!_isIntrospection)
	{
		for (const auto& operation : _operationTypes)
		{
			if (operation.operation == "query")
			{
				queryType = operation.type;
				break;
			}
		}
	}

	if (!_objectTypes.empty())
	{
		sourceFile << R"cpp(
namespace object {
)cpp";

		for (const auto& objectType : _objectTypes)
		{
			// Output the protected constructor which calls through to the service::Object constructor
			// with arguments that declare the set of types it implements and bind the fields to the
			// resolver methods.
			sourceFile << R"cpp(
)cpp" << objectType.type << R"cpp(::)cpp" << objectType.type << R"cpp(()
	: service::Object({
)cpp";

			for (const auto& interfaceName : objectType.interfaces)
			{
				sourceFile << R"cpp(		")cpp" << interfaceName << R"cpp(",
)cpp";
			}

			sourceFile << R"cpp(		")cpp" << objectType.type << R"cpp("
	}, {
)cpp";

			bool firstField = true;

			for (const auto& outputField : objectType.fields)
			{
				if (!firstField)
				{
					sourceFile << R"cpp(,
)cpp";
				}

				firstField = false;

				std::string fieldName(outputField.name);

				fieldName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
				sourceFile << R"cpp(		{ ")cpp" << outputField.name
					<< R"cpp(", [this](service::ResolverParams&& params) { return resolve)cpp" << fieldName
					<< R"cpp((std::move(params)); } })cpp";
			}

			if (!firstField)
			{
				sourceFile << R"cpp(,
)cpp";
			}

			sourceFile << R"cpp(		{ "__typename", [this](service::ResolverParams&& params) { return resolve__typename(std::move(params)); } })cpp";


			if (objectType.type == queryType)
			{
				sourceFile << R"cpp(,
		{ "__schema", [this](service::ResolverParams&& params) { return resolve__schema(std::move(params)); } },
		{ "__type", [this](service::ResolverParams&& params) { return resolve__type(std::move(params)); } })cpp";
			}

			sourceFile << R"cpp(
	}))cpp";

			if (objectType.type == queryType)
			{
				sourceFile << R"cpp(
	, _schema(std::make_shared<)cpp" << s_introspectionNamespace
					<< R"cpp(::Schema>()))cpp";
			}

			sourceFile << R"cpp(
{
)cpp";

			if (objectType.type == queryType)
			{
				sourceFile << R"cpp(	)cpp" << s_introspectionNamespace
					<< R"cpp(::AddTypesToSchema(_schema);
	)cpp" << _schemaNamespace
					<< R"cpp(::AddTypesToSchema(_schema);
)cpp";
			}

			sourceFile << R"cpp(}
)cpp";

			// Output each of the resolver implementations, which call the virtual property
			// getters that the implementer must define.
			for (const auto& outputField : objectType.fields)
			{
				std::string fieldName(outputField.name);

				fieldName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
				sourceFile << R"cpp(
std::future<response::Value> )cpp" << objectType.type
<< R"cpp(::resolve)cpp" << fieldName
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
						sourceFile << getArgumentDeclaration(argument, "arg", "params.arguments", "defaultArguments");
					}
				}

				sourceFile << R"cpp(	auto result = get)cpp" << fieldName << R"cpp((service::FieldParams(params, std::move(params.fieldDirectives)))cpp";

				if (!outputField.arguments.empty())
				{
					for (const auto& argument : outputField.arguments)
					{
						std::string argumentName(argument.name);

						argumentName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(argumentName[0])));
						sourceFile << R"cpp(, std::move(arg)cpp" << argumentName << R"cpp())cpp";
					}
				}

				sourceFile << R"cpp();

	return )cpp" << getResultAccessType(outputField)
					<< R"cpp(::convert)cpp" << getTypeModifiers(outputField.modifiers)
					<< R"cpp((std::move(result), std::move(params));
}
)cpp";
			}

			sourceFile << R"cpp(
std::future<response::Value> )cpp" << objectType.type
<< R"cpp(::resolve__typename(service::ResolverParams&& params)
{
	std::promise<response::StringType> promise;

	promise.set_value(")cpp" << objectType.type << R"cpp(");

	return service::ModifiedResult<response::StringType>::convert(promise.get_future(), std::move(params));
}
)cpp";

			if (objectType.type == queryType)
			{
				sourceFile << R"cpp(
std::future<response::Value> )cpp" << objectType.type
<< R"cpp(::resolve__schema(service::ResolverParams&& params)
{
	std::promise<std::shared_ptr<service::Object>> promise;

	promise.set_value(std::static_pointer_cast<service::Object>(_schema));

	return service::ModifiedResult<service::Object>::convert(promise.get_future(), std::move(params));
}

std::future<response::Value> )cpp" << objectType.type
<< R"cpp(::resolve__type(service::ResolverParams&& params)
{
	auto argName = service::ModifiedArgument<std::string>::require("name", params.arguments);
	std::promise<std::shared_ptr<)cpp" << s_introspectionNamespace << R"cpp(::object::__Type>> promise;

	promise.set_value(_schema->LookupType(argName));

	return service::ModifiedResult<)cpp" << s_introspectionNamespace << R"cpp(::object::__Type>::convert<service::TypeModifier::Nullable>(promise.get_future(), std::move(params));
}
)cpp";
			}
		}

		sourceFile << R"cpp(
} /* namespace object */)cpp";
	}

	if (!_operationTypes.empty())
	{
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
			sourceFile << R"cpp(std::shared_ptr<object::)cpp" << operation.type << R"cpp(> )cpp"
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
			sourceFile << R"cpp(		{ ")cpp" << operation.operation
				<< R"cpp(", )cpp" << operation.operation
				<< R"cpp( })cpp";
		}

		sourceFile << R"cpp(
	})
)cpp";

		for (const auto& operation : _operationTypes)
		{
			sourceFile << R"cpp(	, _)cpp" << operation.operation
				<< R"cpp((std::move()cpp" << operation.operation
				<< R"cpp())
)cpp";
		}

		sourceFile << R"cpp({
}

)cpp";
	}
	else
	{
		sourceFile << R"cpp(

)cpp";
	}

	sourceFile << R"cpp(void AddTypesToSchema(std::shared_ptr<)cpp" << s_introspectionNamespace
		<< R"cpp(::Schema> schema)
{
)cpp";

	if (_isIntrospection)
	{
		// Add SCALAR types for each of the built-in types
		for (const auto& builtinType : s_builtinTypes)
		{
			sourceFile << R"cpp(	schema->AddType(")cpp" << builtinType.first
				<< R"cpp(", std::make_shared<)cpp" << s_introspectionNamespace
				<< R"cpp(::ScalarType>(")cpp" << builtinType.first
				<< R"cpp(", R"md(Built-in type)md"));
)cpp";
		}
	}

	if (!_scalarTypes.empty())
	{
		for (const auto& scalarType : _scalarTypes)
		{
			sourceFile << R"cpp(	schema->AddType(")cpp" << scalarType.type
				<< R"cpp(", std::make_shared<)cpp" << s_introspectionNamespace
				<< R"cpp(::ScalarType>(")cpp" << scalarType.type
				<< R"cpp(", R"md()cpp" << scalarType.description << R"cpp()md"));
)cpp";
		}
	}

	if (!_enumTypes.empty())
	{
		for (const auto& enumType : _enumTypes)
		{
			sourceFile << R"cpp(	auto type)cpp" << enumType.type
				<< R"cpp(= std::make_shared<)cpp" << s_introspectionNamespace
				<< R"cpp(::EnumType>(")cpp" << enumType.type
				<< R"cpp(", R"md()cpp" << enumType.description << R"cpp()md");
	schema->AddType(")cpp" << enumType.type
				<< R"cpp(", type)cpp" << enumType.type
				<< R"cpp();
)cpp";
		}
	}

	if (!_inputTypes.empty())
	{
		for (const auto& inputType : _inputTypes)
		{
			sourceFile << R"cpp(	auto type)cpp" << inputType.type
				<< R"cpp(= std::make_shared<)cpp" << s_introspectionNamespace
				<< R"cpp(::InputObjectType>(")cpp" << inputType.type
				<< R"cpp(", R"md()cpp" << inputType.description << R"cpp()md");
	schema->AddType(")cpp" << inputType.type
				<< R"cpp(", type)cpp" << inputType.type
				<< R"cpp();
)cpp";
		}
	}

	if (!_unionTypes.empty())
	{
		for (const auto& unionType : _unionTypes)
		{
			sourceFile << R"cpp(	auto type)cpp" << unionType.type
				<< R"cpp(= std::make_shared<)cpp" << s_introspectionNamespace
				<< R"cpp(::UnionType>(")cpp" << unionType.type
				<< R"cpp(", R"md()cpp" << unionType.description << R"cpp()md");
	schema->AddType(")cpp" << unionType.type
				<< R"cpp(", type)cpp" << unionType.type
				<< R"cpp();
)cpp";
		}
	}

	if (!_interfaceTypes.empty())
	{
		for (const auto& interfaceType : _interfaceTypes)
		{
			sourceFile << R"cpp(	auto type)cpp" << interfaceType.type
				<< R"cpp(= std::make_shared<)cpp" << s_introspectionNamespace
				<< R"cpp(::InterfaceType>(")cpp" << interfaceType.type
				<< R"cpp(", R"md()cpp" << interfaceType.description << R"cpp()md");
	schema->AddType(")cpp" << interfaceType.type
				<< R"cpp(", type)cpp" << interfaceType.type
				<< R"cpp();
)cpp";
		}
	}

	if (!_objectTypes.empty())
	{
		for (const auto& objectType : _objectTypes)
		{
			sourceFile << R"cpp(	auto type)cpp" << objectType.type
				<< R"cpp(= std::make_shared<)cpp" << s_introspectionNamespace
				<< R"cpp(::ObjectType>(")cpp" << objectType.type
				<< R"cpp(", R"md()cpp" << objectType.description << R"cpp()md");
	schema->AddType(")cpp" << objectType.type
				<< R"cpp(", type)cpp" << objectType.type
				<< R"cpp();
)cpp";
		}
	}

	if (!_enumTypes.empty())
	{
		sourceFile << R"cpp(
)cpp";

		for (const auto& enumType : _enumTypes)
		{
			if (!enumType.values.empty())
			{
				bool firstValue = true;

				sourceFile << R"cpp(	type)cpp" << enumType.type
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
					sourceFile << R"cpp(		{ ")cpp" << enumValue.value
						<< R"cpp(", R"md()cpp" << enumValue.description << R"cpp()md", )cpp";

					if (enumValue.deprecationReason)
					{
						sourceFile << R"cpp(R"md()cpp" << *enumValue.deprecationReason << R"cpp()md")cpp";
					}
					else
					{
						sourceFile << R"cpp(nullptr)cpp";
					}

					sourceFile << R"cpp( })cpp";
				}

				sourceFile << R"cpp(
	});
)cpp";
			}
		}
	}

	if (!_inputTypes.empty())
	{
		sourceFile << R"cpp(
)cpp";

		for (const auto& inputType : _inputTypes)
		{
			if (!inputType.fields.empty())
			{
				bool firstValue = true;

				sourceFile << R"cpp(	type)cpp" << inputType.type
					<< R"cpp(->AddInputValues({
)cpp";

				for (const auto& inputField : inputType.fields)
				{
					if (!firstValue)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstValue = false;
					sourceFile << R"cpp(		std::make_shared<)cpp" << s_introspectionNamespace
						<< R"cpp(::InputValue>(")cpp" << inputField.name
						<< R"cpp(", R"md()cpp" << inputField.description
						<< R"cpp()md", )cpp" << getIntrospectionType(inputField.type, inputField.modifiers)
						<< R"cpp(, R"gql()cpp" << inputField.defaultValueString << R"cpp()gql"))cpp";
				}

				sourceFile << R"cpp(
	});
)cpp";
			}
		}
	}

	if (!_unionTypes.empty())
	{
		sourceFile << R"cpp(
)cpp";

		for (const auto& unionType : _unionTypes)
		{
			if (!unionType.options.empty())
			{
				bool firstValue = true;

				sourceFile << R"cpp(	type)cpp" << unionType.type
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
					sourceFile << R"cpp(		schema->LookupType(")cpp" << unionOption
						<< R"cpp("))cpp";
				}

				sourceFile << R"cpp(
	});
)cpp";
			}
		}
	}

	if (!_interfaceTypes.empty())
	{
		sourceFile << R"cpp(
)cpp";

		for (const auto& interfaceType : _interfaceTypes)
		{
			if (!interfaceType.fields.empty())
			{
				bool firstValue = true;

				sourceFile << R"cpp(	type)cpp" << interfaceType.type
					<< R"cpp(->AddFields({
)cpp";

				for (const auto& interfaceField : interfaceType.fields)
				{
					if (!firstValue)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstValue = false;
					sourceFile << R"cpp(		std::make_shared<)cpp" << s_introspectionNamespace
						<< R"cpp(::Field>(")cpp" << interfaceField.name
						<< R"cpp(", R"md()cpp" << interfaceField.description
						<< R"cpp()md", std::unique_ptr<std::string>()cpp";

					if (interfaceField.deprecationReason)
					{
						sourceFile << R"cpp(new std::string(R"md()cpp"
							<< *interfaceField.deprecationReason << R"cpp()md"))cpp";
					}
					else
					{
						sourceFile << R"cpp(nullptr)cpp";
					}

					sourceFile << R"cpp(), std::vector<std::shared_ptr<)cpp" << s_introspectionNamespace
						<< R"cpp(::InputValue>>()cpp";

					if (!interfaceField.arguments.empty())
					{
						bool firstArgument = true;

						sourceFile << R"cpp({
)cpp";

						for (const auto& argument : interfaceField.arguments)
						{
							if (!firstArgument)
							{
								sourceFile << R"cpp(,
)cpp";
							}

							firstArgument = false;
							sourceFile << R"cpp(			std::make_shared<)cpp" << s_introspectionNamespace
								<< R"cpp(::InputValue>(")cpp" << argument.name
								<< R"cpp(", R"md()cpp" << argument.description
								<< R"cpp()md", )cpp" << getIntrospectionType(argument.type, argument.modifiers)
								<< R"cpp(, R"gql()cpp" << argument.defaultValueString << R"cpp()gql"))cpp";
						}

						sourceFile << R"cpp(
		})cpp";
					}

					sourceFile << R"cpp(), )cpp" << getIntrospectionType(interfaceField.type, interfaceField.modifiers)
						<< R"cpp())cpp";
				}

				sourceFile << R"cpp(
	});
)cpp";
			}
		}
	}

	if (!_objectTypes.empty())
	{
		sourceFile << R"cpp(
)cpp";

		for (const auto& objectType : _objectTypes)
		{
			if (!objectType.interfaces.empty())
			{
				bool firstInterface = true;

				sourceFile << R"cpp(	type)cpp" << objectType.type
					<< R"cpp(->AddInterfaces({
)cpp";

				for (const auto& interfaceName : objectType.interfaces)
				{
					if (!firstInterface)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstInterface = false;
					sourceFile << R"cpp(		type)cpp" << interfaceName;
				}

				sourceFile << R"cpp(
	});
)cpp";
			}

			if (!objectType.fields.empty())
			{
				bool firstValue = true;

				sourceFile << R"cpp(	type)cpp" << objectType.type
					<< R"cpp(->AddFields({
)cpp";

				for (const auto& objectField : objectType.fields)
				{
					if (!firstValue)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstValue = false;
					sourceFile << R"cpp(		std::make_shared<)cpp" << s_introspectionNamespace
						<< R"cpp(::Field>(")cpp" << objectField.name
						<< R"cpp(", R"md()cpp" << objectField.description
						<< R"cpp()md", std::unique_ptr<std::string>()cpp";

					if (objectField.deprecationReason)
					{
						sourceFile << R"cpp(new std::string(R"md()cpp"
							<< *objectField.deprecationReason << R"cpp()md"))cpp";
					}
					else
					{
						sourceFile << R"cpp(nullptr)cpp";
					}

					sourceFile << R"cpp(), std::vector<std::shared_ptr<)cpp" << s_introspectionNamespace
						<< R"cpp(::InputValue>>()cpp";

					if (!objectField.arguments.empty())
					{
						bool firstArgument = true;

						sourceFile << R"cpp({
)cpp";

						for (const auto& argument : objectField.arguments)
						{
							if (!firstArgument)
							{
								sourceFile << R"cpp(,
)cpp";
							}

							firstArgument = false;
							sourceFile << R"cpp(			std::make_shared<)cpp" << s_introspectionNamespace
								<< R"cpp(::InputValue>(")cpp" << argument.name
								<< R"cpp(", R"md()cpp" << argument.description
								<< R"cpp()md", )cpp" << getIntrospectionType(argument.type, argument.modifiers)
								<< R"cpp(, R"gql()cpp" << argument.defaultValueString << R"cpp()gql"))cpp";
						}

						sourceFile << R"cpp(
		})cpp";
					}

					sourceFile << R"cpp(), )cpp" << getIntrospectionType(objectField.type, objectField.modifiers)
						<< R"cpp())cpp";
				}

				sourceFile << R"cpp(
	});
)cpp";
			}
		}
	}

	if (!_directives.empty())
	{
		sourceFile << R"cpp(
)cpp";

		for (const auto& directive : _directives)
		{
			sourceFile << R"cpp(	schema->AddDirective(std::make_shared<)cpp" << s_introspectionNamespace
				<< R"cpp(::Directive>(")cpp" << directive.name
				<< R"cpp(", R"md()cpp" << directive.description
				<< R"cpp()md", std::vector<response::StringType>()cpp";

			if (!directive.locations.empty())
			{
				bool firstLocation = true;


				sourceFile << R"cpp({
)cpp";

				for (const auto& location : directive.locations)
				{
					if (!firstLocation)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstLocation = false;
					sourceFile << R"cpp(		R"gql()cpp" << location << R"cpp()gql")cpp";
				}


				sourceFile << R"cpp(
	})cpp";
			}

			sourceFile << R"cpp(), std::vector<std::shared_ptr<)cpp" << s_introspectionNamespace
				<< R"cpp(::InputValue>>()cpp";

			if (!directive.arguments.empty())
			{
				bool firstArgument = true;

				sourceFile << R"cpp({
)cpp";

				for (const auto& argument : directive.arguments)
				{
					if (!firstArgument)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstArgument = false;
					sourceFile << R"cpp(		std::make_shared<)cpp" << s_introspectionNamespace
						<< R"cpp(::InputValue>(")cpp" << argument.name
						<< R"cpp(", R"md()cpp" << argument.description
						<< R"cpp()md", )cpp" << getIntrospectionType(argument.type, argument.modifiers)
						<< R"cpp(, R"gql()cpp" << argument.defaultValueString << R"cpp()gql"))cpp";
				}

				sourceFile << R"cpp(
	})cpp";
			}
			sourceFile << R"cpp()));
)cpp";
		}
	}

	if (!_operationTypes.empty())
	{
		sourceFile << R"cpp(
)cpp";

		for (const auto& operationType : _operationTypes)
		{
			std::string operation(operationType.operation);

			operation[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(operation[0])));
			sourceFile << R"cpp(	schema->Add)cpp" << operation
				<< R"cpp(Type(type)cpp" << operationType.type
				<< R"cpp();
)cpp";
		}
	}

	sourceFile << R"cpp(}

} /* namespace )cpp" << _schemaNamespace << R"cpp( */
} /* namespace graphql */
} /* namespace facebook */)cpp";

	return true;
}

std::string Generator::getArgumentDefaultValue(size_t level, const response::Value & defaultValue) const noexcept
{
	const std::string padding(level, '\t');
	std::ostringstream argumentDefaultValue;

	switch (defaultValue.type())
	{
		case response::Type::Map:
		{
			const auto& members = defaultValue.get<const response::MapType&>();

			argumentDefaultValue << padding << R"cpp(		entry = []()
)cpp" << padding << R"cpp(		{
)cpp" << padding << R"cpp(			response::Value members(response::Type::Map);
)cpp" << padding << R"cpp(			response::Value entry;

)cpp";

			for (const auto& entry : members)
			{
				argumentDefaultValue << getArgumentDefaultValue(level + 1, entry.second)
					<< padding << R"cpp(			members.emplace_back(")cpp" << entry.first << R"cpp(", std::move(entry));
)cpp";
			}

			argumentDefaultValue << padding << R"cpp(			return members;
)cpp" << padding << R"cpp(		}();
)cpp";
			break;
		}

		case response::Type::List:
		{
			const auto& elements = defaultValue.get<const response::ListType&>();

			argumentDefaultValue << padding << R"cpp(		entry = []()
)cpp" << padding << R"cpp(		{
)cpp" << padding << R"cpp(			response::Value elements(response::Type::List);
)cpp" << padding << R"cpp(			response::Value entry;

)cpp";

			for (const auto& entry : elements)
			{
				argumentDefaultValue << getArgumentDefaultValue(level + 1, entry)
					<< padding << R"cpp(			elements.emplace_back(std::move(entry));
)cpp";
			}

			argumentDefaultValue << padding << R"cpp(			return elements;
)cpp" << padding << R"cpp(		}();
)cpp";
			break;
		}

		case response::Type::String:
		{
			argumentDefaultValue << padding << R"cpp(		entry = response::Value(std::string(R"gql()cpp"
				<< defaultValue.get<const response::StringType&>() << R"cpp()gql"));
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
				<< (defaultValue.get<response::BooleanType>()
					? R"cpp(true)cpp"
					: R"cpp(false)cpp")
				<< R"cpp();
)cpp";
			break;
		}

		case response::Type::Int:
		{
			argumentDefaultValue << padding << R"cpp(		entry = response::Value(static_cast<response::IntType>()cpp"
				<< defaultValue.get<response::IntType>() << R"cpp());
)cpp";
			break;
		}

		case response::Type::Float:
		{
			argumentDefaultValue << padding << R"cpp(		entry = response::Value(static_cast<response::FloatType>()cpp"
				<< defaultValue.get<response::FloatType>() << R"cpp());
)cpp";
			break;
		}

		case response::Type::EnumValue:
		{
			argumentDefaultValue << padding << R"cpp(		entry = response::Value(response::Type::EnumValue);
		entry.set<response::StringType>(R"gql()cpp" << defaultValue.get<const response::StringType&>() << R"cpp()gql");
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
			argumentDefaultValue << padding << R"cpp(	)cpp" << getArgumentDefaultValue(level + 1, defaultValue.get<const response::ScalarType&>())
				<< padding << R"cpp(			scalar.set<response::ScalarType>(std::move(entry));

)cpp" << padding << R"cpp(			return scalar;
)cpp" << padding << R"cpp(		}();
)cpp";
			break;
		}
	}

	return argumentDefaultValue.str();
}

std::string Generator::getArgumentDeclaration(const InputField & argument, const char* prefixToken, const char* argumentsToken, const char* defaultToken) const noexcept
{
	std::ostringstream argumentDeclaration;
	std::string argumentName(argument.name);

	argumentName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(argumentName[0])));
	if (argument.defaultValue.type() == response::Type::Null)
	{
		argumentDeclaration << R"cpp(	auto )cpp" << prefixToken << argumentName
			<< R"cpp( = )cpp" << getArgumentAccessType(argument)
			<< R"cpp(::require)cpp" << getTypeModifiers(argument.modifiers)
			<< R"cpp((")cpp" << argument.name
			<< R"cpp(", )cpp" << argumentsToken
			<< R"cpp();
)cpp";
	}
	else
	{
		argumentDeclaration << R"cpp(	auto pair)cpp" << argumentName
			<< R"cpp( = )cpp" << getArgumentAccessType(argument)
			<< R"cpp(::find)cpp" << getTypeModifiers(argument.modifiers)
			<< R"cpp((")cpp" << argument.name
			<< R"cpp(", )cpp" << argumentsToken
			<< R"cpp();
	auto )cpp" << prefixToken << argumentName << R"cpp( = (pair)cpp" << argumentName << R"cpp(.second
		? std::move(pair)cpp" << argumentName << R"cpp(.first)
		: )cpp" << getArgumentAccessType(argument)
			<< R"cpp(::require)cpp" << getTypeModifiers(argument.modifiers)
			<< R"cpp((")cpp" << argument.name
			<< R"cpp(", )cpp" << defaultToken
			<< R"cpp());
)cpp";
	}

	return argumentDeclaration.str();
}

std::string Generator::getArgumentAccessType(const InputField & argument) const noexcept
{
	std::ostringstream argumentType;

	argumentType << R"cpp(service::ModifiedArgument<)cpp";

	switch (argument.fieldType)
	{
		case InputFieldType::Builtin:
		case InputFieldType::Enum:
		case InputFieldType::Input:
			argumentType << getCppType(argument.type);
			break;

		case InputFieldType::Scalar:
			argumentType << R"cpp(response::Value)cpp";
			break;
	}

	argumentType << R"cpp(>)cpp";

	return argumentType.str();
}

std::string Generator::getResultAccessType(const OutputField & result) const noexcept
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

std::string Generator::getTypeModifiers(const TypeModifierStack & modifiers) const noexcept
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
		}
	}

	if (!firstValue)
	{
		typeModifiers << R"cpp(>)cpp";
	}

	return typeModifiers.str();
}

std::string Generator::getIntrospectionType(const std::string & type, const TypeModifierStack & modifiers) const noexcept
{
	size_t wrapperCount = 0;
	bool nonNull = true;
	std::ostringstream introspectionType;

	for (auto modifier : modifiers)
	{
		if (!nonNull)
		{
			switch (modifier)
			{
				case service::TypeModifier::None:
				case service::TypeModifier::List:
					// If the next modifier is None or List we should treat it as non-nullable.
					nonNull = true;
					break;

				case service::TypeModifier::Nullable:
					break;
			}
		}

		if (nonNull)
		{
			switch (modifier)
			{
				case service::TypeModifier::None:
				case service::TypeModifier::List:
				{
					introspectionType << R"cpp(schema->WrapType()cpp" << s_introspectionNamespace
						<< R"cpp(::__TypeKind::NON_NULL, )cpp";
					++wrapperCount;
					break;
				}

				case service::TypeModifier::Nullable:
					// If the next modifier is Nullable that cancels the non-nullable state.
					nonNull = false;
					break;
			}
		}

		switch (modifier)
		{
			case service::TypeModifier::None:
			case service::TypeModifier::Nullable:
				break;

			case service::TypeModifier::List:
			{
				introspectionType << R"cpp(schema->WrapType()cpp" << s_introspectionNamespace
					<< R"cpp(::__TypeKind::LIST, )cpp";
				++wrapperCount;
				break;
			}
		}
	}

	if (nonNull)
	{
		introspectionType << R"cpp(schema->WrapType()cpp" << s_introspectionNamespace
			<< R"cpp(::__TypeKind::NON_NULL, )cpp";
		++wrapperCount;
	}

	introspectionType << R"cpp(schema->LookupType(")cpp" << type
		<< R"cpp("))cpp";

	for (size_t i = 0; i < wrapperCount; ++i)
	{
		introspectionType << R"cpp())cpp";
	}

	return introspectionType.str();
}

} /* namespace schema */
} /* namespace graphql */
} /* namespace facebook */

int main(int argc, char** argv)
{
	std::vector<std::string> files;

	try
	{
		if (argc == 1)
		{
			files = facebook::graphql::schema::Generator().Build();
		}
		else
		{
			if (argc != 4)
			{
				std::cerr << "Usage (to generate a custom schema): " << argv[0]
					<< " <schema file> <output filename prefix> <output namespace>"
					<< std::endl;
				std::cerr << "Usage (to generate IntrospectionSchema): " << argv[0] << std::endl;
				return 1;
			}

			facebook::graphql::schema::Generator generator(argv[1], argv[2], argv[3]);

			files = generator.Build();
		}

		for (const auto& file : files)
		{
			std::cout << file << std::endl;
		}
	}
	catch (const std::runtime_error & ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
