// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "SchemaGenerator.h"
#include "GraphQLService.h"
#include "GraphQLGrammar.h"

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

#include <graphqlparser/GraphQLParser.h>

namespace facebook {
namespace graphql {
namespace schema {

const std::string Generator::s_introspectionNamespace = "introspection";

const BuiltinTypeMap Generator::s_builtinTypes= {
		{ "Int", BuiltinType::Int },
		{ "Float", BuiltinType::Float },
		{ "String", BuiltinType::String },
		{ "Boolean", BuiltinType::Boolean },
		{ "ID", BuiltinType::ID },
	};

const CppTypeMap Generator::s_builtinCppTypes= {
		"int",
		"double",
		"std::string",
		"bool",
		"std::vector<unsigned char>",
	};

const std::string Generator::s_scalarCppType = R"cpp(web::json::value)cpp";

Generator::Generator()
	: _isIntrospection(true)
	, _filenamePrefix("Introspection")
	, _schemaNamespace(s_introspectionNamespace)
{
	// TODO: Double check that this still matches the spec
	auto ast = grammar::parseString(R"gql(
		# Introspection Schema

		type __Schema {
			types: [__Type!]!
			queryType: __Type!
			mutationType: __Type
			subscriptionType: __Type
			directives: [__Directive!]!
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
		}

		type __Type {
			kind: __TypeKind!
			name: String
			description: String
			fields(includeDeprecated: Boolean = false): [__Field!]
			interfaces: [__Type!]
			possibleTypes: [__Type!]
			enumValues(includeDeprecated: Boolean = false): [__EnumValue!]
			inputFields: [__InputValue!]
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
		})gql");

	if (!ast)
	{
		throw std::logic_error("Unable to parse the introspection schema, but there was no error message from the parser!");
	}

	for (const auto& child : ast->children.front()->children)
	{
		if (child->is<grammar::schema_definition>())
		{
			visitSchemaDefinition(*child);
		}
		else if (child->is<grammar::scalar_type_definition>())
		{
			visitScalarTypeDefinition(*child);
		}
		else if (child->is<grammar::enum_type_definition>())
		{
			visitEnumTypeDefinition(*child);
		}
		else if (child->is<grammar::input_object_type_definition>())
		{
			visitInputObjectTypeDefinition(*child);
		}
		else if (child->is<grammar::input_object_type_definition>())
		{
			visitUnionTypeDefinition(*child);
		}
		else if (child->is<grammar::interface_type_definition>())
		{
			visitInterfaceTypeDefinition(*child);
		}
		else if (child->is<grammar::object_type_definition>())
		{
			visitObjectTypeDefinition(*child);
		}
	}

	if (!validateSchema())
	{
		throw std::runtime_error("Invalid introspection schema!");
	}
}

Generator::Generator(std::string schemaFileName, std::string filenamePrefix, std::string schemaNamespace)
	: _isIntrospection(false)
	, _filenamePrefix(std::move(filenamePrefix))
	, _schemaNamespace(std::move(schemaNamespace))
{
	tao::pegtl::file_input<> in(schemaFileName.c_str());
	auto ast = grammar::parseFile(std::move(in));

	if (!ast)
	{
		throw std::logic_error("Unable to parse the service schema, but there was no error message from the parser!");
	}

	for (const auto& child : ast->children.front()->children)
	{
		if (child->is<grammar::schema_definition>())
		{
			visitSchemaDefinition(*child);
		}
		else if (child->is<grammar::scalar_type_definition>())
		{
			visitScalarTypeDefinition(*child);
		}
		else if (child->is<grammar::enum_type_definition>())
		{
			visitEnumTypeDefinition(*child);
		}
		else if (child->is<grammar::input_object_type_definition>())
		{
			visitInputObjectTypeDefinition(*child);
		}
		else if (child->is<grammar::input_object_type_definition>())
		{
			visitUnionTypeDefinition(*child);
		}
		else if (child->is<grammar::interface_type_definition>())
		{
			visitInterfaceTypeDefinition(*child);
		}
		else if (child->is<grammar::object_type_definition>())
		{
			visitObjectTypeDefinition(*child);
		}
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

bool Generator::visitSchemaDefinition(const grammar::ast_node& schemaDefinition)
{
	for (const auto& operationTypeDefinition : schemaDefinition.children)
	{
		if (operationTypeDefinition->is<grammar::root_operation_definition>())
		{
			std::string operation(operationTypeDefinition->children.front()->content());
			std::string name(operationTypeDefinition->children.back()->content());

			_operationTypes.push_back({ std::move(name), std::move(operation) });
		}
	}

	return false;
}

bool Generator::visitObjectTypeDefinition(const grammar::ast_node& objectTypeDefinition)
{
	std::string name;
	std::vector<std::string> interfaces;
	OutputFieldList fields;

	for (const auto& child : objectTypeDefinition.children)
	{
		if (child->is<grammar::object_name>())
		{
			name = child->content();
		}
		else if (child->is<grammar::implements_interfaces>())
		{
			for (const auto& namedType : child->children)
			{
				interfaces.push_back(namedType->content());
			}
		}
		else if (child->is<grammar::fields_definition>())
		{
			fields = getOutputFields(child->children);
		}
	}

	_schemaTypes[name] = SchemaType::Object;
	_objectNames[name] = _objectTypes.size();
	_objectTypes.push_back({ std::move(name), std::move(interfaces), std::move(fields) });

	return false;
}

bool Generator::visitInterfaceTypeDefinition(const grammar::ast_node& interfaceTypeDefinition)
{
	std::string name;
	OutputFieldList fields;

	for (const auto& child : interfaceTypeDefinition.children)
	{
		if (child->is<grammar::interface_name>())
		{
			name = child->content();
		}
		else if (child->is<grammar::fields_definition>())
		{
			fields = getOutputFields(child->children);
		}
	}

	_schemaTypes[name] = SchemaType::Interface;
	_interfaceNames[name] = _interfaceTypes.size();
	_interfaceTypes.push_back({ std::move(name), std::move(fields) });

	return false;
}

bool Generator::visitInputObjectTypeDefinition(const grammar::ast_node& inputObjectTypeDefinition)
{
	std::string name;
	InputFieldList fields;

	for (const auto& child : inputObjectTypeDefinition.children)
	{
		if (child->is<grammar::object_name>())
		{
			name = child->content();
		}
		else if (child->is<grammar::input_fields_definition>())
		{
			fields = getInputFields(child->children);
		}
	}

	_schemaTypes[name] = SchemaType::Input;
	_inputNames[name] = _inputTypes.size();
	_inputTypes.push_back({ std::move(name), std::move(fields) });

	return false;
}

bool Generator::visitEnumTypeDefinition(const grammar::ast_node& enumTypeDefinition)
{
	std::string name;
	std::vector<std::string> values;

	for (const auto& child : enumTypeDefinition.children)
	{
		if (child->is<grammar::enum_name>())
		{
			name = child->content();
		}
		else if (child->is<grammar::enum_value_definition>())
		{
			for (const auto& enumValue : child->children)
			{
				if (enumValue->is<grammar::enum_value>())
				{
					values.push_back(enumValue->content());
					break;
				}
			}
		}
	}

	_schemaTypes[name] = SchemaType::Enum;
	_enumNames[name] = _enumTypes.size();
	_enumTypes.push_back({ std::move(name), std::move(values) });

	return false;
}

bool Generator::visitScalarTypeDefinition(const grammar::ast_node& scalarTypeDefinition)
{
	std::string name;

	for (const auto& child : scalarTypeDefinition.children)
	{
		if (child->is<grammar::scalar_name>())
		{
			name = child->content();
			break;
		}
	}

	_schemaTypes[name] = SchemaType::Scalar;
	_scalarNames[name] = _scalarTypes.size();
	_scalarTypes.push_back(std::move(name));

	return false;
}

bool Generator::visitUnionTypeDefinition(const grammar::ast_node& unionTypeDefinition)
{
	std::string name;
	std::vector<std::string> options;

	for (const auto& child : unionTypeDefinition.children)
	{
		if (child->is<grammar::union_name>())
		{
			name = child->content();
		}
		else if (child->is<grammar::union_type>())
		{
			options.push_back(child->content());
		}
	}

	_schemaTypes[name] = SchemaType::Union;
	_unionNames[name] = _unionTypes.size();
	_unionTypes.push_back({ std::move(name), std::move(options) });

	return false;
}

OutputFieldList Generator::getOutputFields(const std::vector<std::unique_ptr<grammar::ast_node>>& fields)
{
	OutputFieldList outputFields;

	for (const auto& fieldDefinition : fields)
	{
		OutputField field;
		TypeVisitor fieldType;

		for (const auto& child : fieldDefinition->children)
		{
			if (child->is<grammar::field_name>())
			{
				field.name = child->content();
			}
			else if (child->is<grammar::arguments_definition>())
			{
				field.arguments = getInputFields(child->children);
			}
			else if (child->is<grammar::named_type>())
			{
				fieldType.visitNamedType(*child);
			}
			else if (child->is<grammar::list_type>())
			{
				fieldType.visitListType(*child);
			}
			else if (child->is<grammar::nonnull_type>())
			{
				fieldType.visitNonNullType(*child);
			}
		}

		std::tie(field.type, field.modifiers) = fieldType.getType();
		outputFields.push_back(std::move(field));
	}

	return outputFields;
}

InputFieldList Generator::getInputFields(const std::vector<std::unique_ptr<grammar::ast_node>>& fields)
{
	InputFieldList inputFields;

	for (const auto& fieldDefinition : fields)
	{
		InputField field;
		TypeVisitor fieldType;

		for (const auto& child : fieldDefinition->children)
		{
			if (child->is<grammar::argument_name>())
			{
				field.name = child->content();
			}
			else if (child->is<grammar::named_type>())
			{
				fieldType.visitNamedType(*child);
			}
			else if (child->is<grammar::list_type>())
			{
				fieldType.visitListType(*child);
			}
			else if (child->is<grammar::nonnull_type>())
			{
				fieldType.visitNonNullType(*child);
			}
			else if (child->is<grammar::default_value>())
			{
				DefaultValueVisitor defaultValue;

				if (child->children.back()->is<grammar::integer_value>())
				{
					defaultValue.visitIntValue(*child->children.back());
				}
				else if (child->children.back()->is<grammar::float_value>())
				{
					defaultValue.visitFloatValue(*child->children.back());
				}
				else if (child->children.back()->is<grammar::string_value>())
				{
					defaultValue.visitStringValue(*child->children.back());
				}
				else if (child->children.back()->is<grammar::true_keyword>()
					|| child->children.back()->is<grammar::false_keyword>())
				{
					defaultValue.visitBooleanValue(*child->children.back());
				}
				else if (child->children.back()->is<grammar::null_keyword>())
				{
					defaultValue.visitNullValue(*child->children.back());
				}
				else if (child->children.back()->is<grammar::enum_value>())
				{
					defaultValue.visitEnumValue(*child->children.back());
				}
				else if (child->children.back()->is<grammar::list_value>())
				{
					defaultValue.visitListValue(*child->children.back());
				}
				else if (child->children.back()->is<grammar::object_value>())
				{
					defaultValue.visitObjectValue(*child->children.back());
				}

				field.defaultValue = defaultValue.getValue();
			}
		}

		std::tie(field.type, field.modifiers) = fieldType.getType();
		inputFields.push_back(std::move(field));
	}

	return inputFields;
}

bool Generator::TypeVisitor::visitNamedType(const grammar::ast_node& namedType)
{
	if (!_nonNull)
	{
		_modifiers.push_back(service::TypeModifier::Nullable);
	}

	_type = namedType.content();
	return false;
}

bool Generator::TypeVisitor::visitListType(const grammar::ast_node& listType)
{
	if (!_nonNull)
	{
		_modifiers.push_back(service::TypeModifier::Nullable);
	}
	_nonNull = false;

	_modifiers.push_back(service::TypeModifier::List);
	
	if (listType.children.front()->is<grammar::nonnull_type>())
	{
		visitNonNullType(*listType.children.front());
	}
	else if (listType.children.front()->is<grammar::list_type>())
	{
		visitListType(*listType.children.front());
	}
	else if (listType.children.front()->is<grammar::named_type>())
	{
		visitNamedType(*listType.children.front());
	}

	return true;
}

bool Generator::TypeVisitor::visitNonNullType(const grammar::ast_node& nonNullType)
{
	_nonNull = true;

	if (nonNullType.children.front()->is<grammar::list_type>())
	{
		visitListType(*nonNullType.children.front());
	}
	else if (nonNullType.children.front()->is<grammar::named_type>())
	{
		visitNamedType(*nonNullType.children.front());
	}

	return true;
}

std::pair<std::string, TypeModifierStack> Generator::TypeVisitor::getType()
{
	return { std::move(_type), std::move(_modifiers) };
}

bool Generator::DefaultValueVisitor::visitIntValue(const grammar::ast_node& intValue)
{
	_value = web::json::value::number(std::atoi(intValue.content().c_str()));
	return false;
}

bool Generator::DefaultValueVisitor::visitFloatValue(const grammar::ast_node& floatValue)
{
	_value = web::json::value::number(std::atof(floatValue.content().c_str()));
	return false;
}

bool Generator::DefaultValueVisitor::visitStringValue(const grammar::ast_node& stringValue)
{
	_value = web::json::value::string(utility::conversions::to_string_t(stringValue.unescaped));
	return false;
}

bool Generator::DefaultValueVisitor::visitBooleanValue(const grammar::ast_node& booleanValue)
{
	_value = web::json::value::boolean(booleanValue.content().c_str());
	return false;
}

bool Generator::DefaultValueVisitor::visitNullValue(const grammar::ast_node& nullValue)
{
	_value = web::json::value::null();
	return false;
}

bool Generator::DefaultValueVisitor::visitEnumValue(const grammar::ast_node& enumValue)
{
	_value = web::json::value::string(utility::conversions::to_string_t(enumValue.content()));
	return false;
}

bool Generator::DefaultValueVisitor::visitListValue(const grammar::ast_node& listValue)
{
	_value = web::json::value::array(listValue.children.size());

	std::transform(listValue.children.cbegin(), listValue.children.cend(), _value.as_array().begin(),
		[this](const std::unique_ptr<grammar::ast_node>& value)
	{
		DefaultValueVisitor visitor;

		if (value->children.back()->is<grammar::integer_value>())
		{
			visitor.visitIntValue(*value->children.back());
		}
		else if (value->children.back()->is<grammar::float_value>())
		{
			visitor.visitFloatValue(*value->children.back());
		}
		else if (value->children.back()->is<grammar::string_value>())
		{
			visitor.visitStringValue(*value->children.back());
		}
		else if (value->children.back()->is<grammar::true_keyword>()
			|| value->children.back()->is<grammar::false_keyword>())
		{
			visitor.visitBooleanValue(*value->children.back());
		}
		else if (value->children.back()->is<grammar::null_keyword>())
		{
			visitor.visitNullValue(*value->children.back());
		}
		else if (value->children.back()->is<grammar::enum_value>())
		{
			visitor.visitEnumValue(*value->children.back());
		}
		else if (value->children.back()->is<grammar::list_value>())
		{
			visitor.visitListValue(*value->children.back());
		}
		else if (value->children.back()->is<grammar::object_value>())
		{
			visitor.visitObjectValue(*value->children.back());
		}

		return visitor.getValue();
	});

	return false;
}

bool Generator::DefaultValueVisitor::visitObjectValue(const grammar::ast_node& objectValue)
{
	_value = web::json::value::object(true);

	for (const auto& field : objectValue.children)
	{
		const std::string name(field->children.front()->content());
		DefaultValueVisitor visitor;

		if (field->children.back()->children.back()->is<grammar::integer_value>())
		{
			visitor.visitIntValue(*field->children.back()->children.back());
		}
		else if (field->children.back()->children.back()->is<grammar::float_value>())
		{
			visitor.visitFloatValue(*field->children.back()->children.back());
		}
		else if (field->children.back()->children.back()->is<grammar::string_value>())
		{
			visitor.visitStringValue(*field->children.back()->children.back());
		}
		else if (field->children.back()->children.back()->is<grammar::true_keyword>()
			|| field->children.back()->children.back()->is<grammar::false_keyword>())
		{
			visitor.visitBooleanValue(*field->children.back()->children.back());
		}
		else if (field->children.back()->children.back()->is<grammar::null_keyword>())
		{
			visitor.visitNullValue(*field->children.back()->children.back());
		}
		else if (field->children.back()->children.back()->is<grammar::enum_value>())
		{
			visitor.visitEnumValue(*field->children.back()->children.back());
		}
		else if (field->children.back()->children.back()->is<grammar::list_value>())
		{
			visitor.visitListValue(*field->children.back()->children.back());
		}
		else if (field->children.back()->children.back()->is<grammar::object_value>())
		{
			visitor.visitObjectValue(*field->children.back()->children.back());
		}

		_value[utility::conversions::to_string_t(name)] = visitor.getValue();
	}

	return false;
}

web::json::value Generator::DefaultValueVisitor::getValue()
{
	web::json::value result(std::move(_value));
	return result;
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

const std::string& Generator::getCppType(const std::string& type) const noexcept
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

std::string Generator::getInputCppType(const InputField& field) const noexcept
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

std::string Generator::getOutputCppType(const OutputField& field) const noexcept
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
		case OutputFieldType::Object:
			outputType << getCppType(field.type);
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

#include <memory>
#include <string>
#include <vector>

#include <cpprest/json.h>

#include "GraphQLService.h"

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
				headerFile << R"cpp(	)cpp" << value;
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
				headerFile << getFieldDeclaration(outputField);
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

		if (_objectTypes.size() > 1)
		{
			// Forward declare all of the object types
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
	web::json::value resolve__typename(service::ResolverParams&& params);
)cpp";

				if (objectType.type == queryType)
				{
					headerFile << R"cpp(	web::json::value resolve__schema(service::ResolverParams&& params);
	web::json::value resolve__type(service::ResolverParams&& params);

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

std::string Generator::getFieldDeclaration(const InputField& inputField) const noexcept
{
	std::ostringstream output;

	output << getInputCppType(inputField) << R"cpp( )cpp"
		<< inputField.name;

	return output.str();
}

std::string Generator::getFieldDeclaration(const OutputField& outputField) const noexcept
{
	std::ostringstream output;
	bool firstArgument = true;
	std::string fieldName(outputField.name);

	fieldName[0] = std::toupper(fieldName[0]);
	output << R"cpp(	virtual )cpp" << getOutputCppType(outputField)
		<< R"cpp( get)cpp" << fieldName << R"cpp(()cpp";

	for (const auto& argument : outputField.arguments)
	{
		if (!firstArgument)
		{
			output << R"cpp(, )cpp";
		}

		firstArgument = false;
		output << getInputCppType(argument) << R"cpp(&& )cpp"
			<< argument.name;
	}

	output << R"cpp() const = 0;
)cpp";

	return output.str();
}

std::string Generator::getResolverDeclaration(const OutputField& outputField) const noexcept
{
	std::ostringstream output;
	std::string fieldName(outputField.name);

	fieldName[0] = std::toupper(fieldName[0]);
	output << R"cpp(	web::json::value resolve)cpp" << fieldName
		<< R"cpp((service::ResolverParams&& params);
)cpp";

	return output.str();
}

bool Generator::outputSource() const noexcept
{
	std::ofstream sourceFile(_filenamePrefix + "Schema.cpp", std::ios_base::trunc);

	sourceFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include ")cpp" << _filenamePrefix << R"cpp(Schema.h"
#include "Introspection.h"

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
<< R"cpp(>::convert(const web::json::value& value)
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
				sourceFile << R"cpp(		{ ")cpp" << value << R"cpp(", )cpp"
					<< _schemaNamespace << R"cpp(::)cpp" << enumType.type
					<< R"cpp(::)cpp" << value << R"cpp( })cpp";
			}

			sourceFile << R"cpp(
	};

	auto itr = s_names.find(utility::conversions::to_utf8string(value.as_string()));

	if (itr == s_names.cend())
	{
		throw web::json::json_exception(_XPLATSTR("not a valid )cpp" << enumType.type << R"cpp( value"));
	}

	return itr->second;
}

template <>
web::json::value service::ModifiedResult<)cpp" << _schemaNamespace << R"cpp(::)cpp" << enumType.type
<< R"cpp(>::convert(const )cpp" << _schemaNamespace << R"cpp(::)cpp" << enumType.type
<< R"cpp(& value, ResolverParams&&)
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
				sourceFile << R"cpp(		")cpp" << value << R"cpp(")cpp";
			}

			sourceFile << R"cpp(
	};

	return web::json::value::string(utility::conversions::to_string_t(s_names[static_cast<size_t>(value)]));
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
<< R"cpp(>::convert(const web::json::value& value)
{
)cpp";

			for (const auto& inputField : inputType.fields)
			{
				if (!inputField.defaultValue.is_null())
				{
					if (firstField)
					{
						firstField = false;
						sourceFile << R"cpp(	static const auto defaultValue = web::json::value::object({
)cpp";
					}
					else
					{
						sourceFile << R"cpp(,
)cpp";
					}

					utility::ostringstream_t defaultValue;

					defaultValue << inputField.defaultValue;
					sourceFile << R"cpp(		{ _XPLATSTR(")cpp"
						<< inputField.name << R"cpp("), web::json::value::parse(_XPLATSTR(R"js()cpp"
						<< utility::conversions::to_utf8string(defaultValue.str()) << R"cpp()js")) })cpp";
				}
			}

			if (!firstField)
			{
				sourceFile << R"cpp(
	});

)cpp";
			}

			for (const auto& inputField : inputType.fields)
			{
				std::string fieldName(inputField.name);

				fieldName[0] = std::toupper(fieldName[0]);
				if (inputField.defaultValue.is_null())
				{
					sourceFile << R"cpp(	auto value)cpp" << fieldName
						<< R"cpp( = )cpp" << getArgumentAccessType(inputField)
						<< R"cpp(::require)cpp" << getTypeModifiers(inputField.modifiers)
						<< R"cpp((")cpp" << inputField.name
						<< R"cpp(", value.as_object());
)cpp";
				}
				else
				{
					sourceFile << R"cpp(	auto pair)cpp" << fieldName
						<< R"cpp( = )cpp" << getArgumentAccessType(inputField)
						<< R"cpp(::find)cpp" << getTypeModifiers(inputField.modifiers)
						<< R"cpp((")cpp" << inputField.name
						<< R"cpp(", value.as_object());
	auto value)cpp" << fieldName << R"cpp( = (pair)cpp" << fieldName << R"cpp(.second
		? std::move(pair)cpp" << fieldName << R"cpp(.first)
		: )cpp" << getArgumentAccessType(inputField)
						<< R"cpp(::require)cpp" << getTypeModifiers(inputField.modifiers)
						<< R"cpp((")cpp" << inputField.name
						<< R"cpp(", defaultValue.as_object()));
)cpp";
				}
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
				fieldName[0] = std::toupper(fieldName[0]);
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

				fieldName[0] = std::toupper(fieldName[0]);
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

				fieldName[0] = std::toupper(fieldName[0]);
				sourceFile << R"cpp(
web::json::value )cpp" << objectType.type
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
						if (!argument.defaultValue.is_null())
						{
							if (firstArgument)
							{
								firstArgument = false;
								sourceFile << R"cpp(	static const auto defaultArguments = web::json::value::object({
)cpp";
							}
							else
							{
								sourceFile << R"cpp(,
)cpp";
							}


							utility::ostringstream_t defaultArguments;

							defaultArguments << argument.defaultValue;
							sourceFile << R"cpp(		{ _XPLATSTR(")cpp"
								<< argument.name << R"cpp("), web::json::value::parse(_XPLATSTR(R"js()cpp"
								<< utility::conversions::to_utf8string(defaultArguments.str()) << R"cpp()js")) })cpp";
						}
					}

					if (!firstArgument)
					{
						sourceFile << R"cpp(
	});

)cpp";
					}

					for (const auto& argument : outputField.arguments)
					{
						std::string argumentName(argument.name);

						argumentName[0] = std::toupper(argumentName[0]);
						if (argument.defaultValue.is_null())
						{
							sourceFile << R"cpp(	auto arg)cpp" << argumentName
								<< R"cpp( = )cpp" << getArgumentAccessType(argument)
								<< R"cpp(::require)cpp" << getTypeModifiers(argument.modifiers)
								<< R"cpp((")cpp" << argument.name
								<< R"cpp(", params.arguments);
)cpp";
						}
						else
						{
							sourceFile << R"cpp(	auto pair)cpp" << argumentName
								<< R"cpp( = )cpp" << getArgumentAccessType(argument)
								<< R"cpp(::find)cpp" << getTypeModifiers(argument.modifiers)
								<< R"cpp((")cpp" << argument.name
								<< R"cpp(", params.arguments);
	auto arg)cpp" << argumentName << R"cpp( = (pair)cpp" << argumentName << R"cpp(.second
		? std::move(pair)cpp" << argumentName << R"cpp(.first)
		: )cpp" << getArgumentAccessType(argument)
								<< R"cpp(::require)cpp" << getTypeModifiers(argument.modifiers)
								<< R"cpp((")cpp" << argument.name
								<< R"cpp(", defaultArguments.as_object()));
)cpp";
						}
					}
				}

				sourceFile << R"cpp(	auto result = get)cpp" << fieldName << R"cpp(()cpp";

				if (!outputField.arguments.empty())
				{
					bool firstArgument = true;

					for (const auto& argument : outputField.arguments)
					{
						if (!firstArgument)
						{
							sourceFile << R"cpp(, )cpp";
						}

						firstArgument = false;

						std::string argumentName(argument.name);

						argumentName[0] = std::toupper(argumentName[0]);
						sourceFile << R"cpp(std::move(arg)cpp" << argumentName << R"cpp())cpp";
					}
				}

				sourceFile << R"cpp();

	return )cpp" << getResultAccessType(outputField)
					<< R"cpp(::convert)cpp" << getTypeModifiers(outputField.modifiers)
					<< R"cpp((result, std::move(params));
}
)cpp";
			}

			sourceFile << R"cpp(
web::json::value )cpp" << objectType.type
<< R"cpp(::resolve__typename(service::ResolverParams&&)
{
	return web::json::value::string(_XPLATSTR(")cpp" << objectType.type << R"cpp("));
}
)cpp";

			if (objectType.type == queryType)
			{
				sourceFile << R"cpp(
web::json::value )cpp" << objectType.type
<< R"cpp(::resolve__schema(service::ResolverParams&& params)
{
	auto result = service::ModifiedResult<introspection::Schema>::convert(_schema, std::move(params));

	return result;
}

web::json::value )cpp" << objectType.type
<< R"cpp(::resolve__type(service::ResolverParams&& params)
{
	auto argName = service::ModifiedArgument<std::string>::require("name", params.arguments);
	auto result = service::ModifiedResult<introspection::object::__Type>::convert<service::TypeModifier::Nullable>(_schema->LookupType(argName), std::move(params));

	return result;
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

	if (!_scalarTypes.empty())
	{
		for (const auto& scalarType : _scalarTypes)
		{
			sourceFile << R"cpp(	schema->AddType(")cpp" << scalarType
				<< R"cpp(", std::make_shared<)cpp" << s_introspectionNamespace
				<< R"cpp(::ScalarType>(")cpp" << scalarType << R"cpp("));
)cpp";
		}
	}

	if (!_enumTypes.empty())
	{
		for (const auto& enumType : _enumTypes)
		{
			sourceFile << R"cpp(	auto type)cpp" << enumType.type
				<< R"cpp(= std::make_shared<)cpp" << s_introspectionNamespace
				<< R"cpp(::EnumType>(")cpp" << enumType.type << R"cpp(");
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
				<< R"cpp(::InputObjectType>(")cpp" << inputType.type << R"cpp(");
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
				<< R"cpp(::UnionType>(")cpp" << unionType.type << R"cpp(");
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
				<< R"cpp(::InterfaceType>(")cpp" << interfaceType.type << R"cpp(");
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
				<< R"cpp(::ObjectType>(")cpp" << objectType.type << R"cpp(");
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
					sourceFile << R"cpp(		")cpp" << enumValue << R"cpp(")cpp";
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
					utility::ostringstream_t defaultValue;

					defaultValue << inputField.defaultValue;

					if (!firstValue)
					{
						sourceFile << R"cpp(,
)cpp";
					}

					firstValue = false;
					sourceFile << R"cpp(		std::make_shared<)cpp" << s_introspectionNamespace
						<< R"cpp(::InputValue>(")cpp" << inputField.name
						<< R"cpp(", )cpp" << getIntrospectionType(inputField.type, inputField.modifiers)
						<< R"cpp(, web::json::value::parse(_XPLATSTR(R"js()cpp"
						<< utility::conversions::to_utf8string(defaultValue.str())
						<< R"cpp()js"))))cpp";
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
						<< R"cpp(", std::vector<std::shared_ptr<)cpp" << s_introspectionNamespace
						<< R"cpp(::InputValue>>()cpp";

					if (!interfaceField.arguments.empty())
					{
						bool firstArgument = true;

						sourceFile << R"cpp({
)cpp";

						for (const auto& argument : interfaceField.arguments)
						{
							utility::ostringstream_t defaultValue;

							defaultValue << argument.defaultValue;

							if (!firstArgument)
							{
								sourceFile << R"cpp(,
)cpp";
							}

							firstArgument = false;
							sourceFile << R"cpp(			std::make_shared<)cpp" << s_introspectionNamespace
								<< R"cpp(::InputValue>(")cpp" << argument.name
								<< R"cpp(", )cpp" << getIntrospectionType(argument.type, argument.modifiers)
								<< R"cpp(, web::json::value::parse(_XPLATSTR(R"js()cpp"
								<< utility::conversions::to_utf8string(defaultValue.str())
								<< R"cpp()js"))))cpp";
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
						<< R"cpp(", std::vector<std::shared_ptr<)cpp" << s_introspectionNamespace
						<< R"cpp(::InputValue>>()cpp";

					if (!objectField.arguments.empty())
					{
						bool firstArgument = true;

						sourceFile << R"cpp({
)cpp";

						for (const auto& argument : objectField.arguments)
						{
							utility::ostringstream_t defaultValue;

							defaultValue << argument.defaultValue;

							if (!firstArgument)
							{
								sourceFile << R"cpp(,
)cpp";
							}

							firstArgument = false;
							sourceFile << R"cpp(			std::make_shared<)cpp" << s_introspectionNamespace
								<< R"cpp(::InputValue>(")cpp" << argument.name
								<< R"cpp(", )cpp" << getIntrospectionType(argument.type, argument.modifiers)
								<< R"cpp(, web::json::value::parse(_XPLATSTR(R"js()cpp"
								<< utility::conversions::to_utf8string(defaultValue.str())
								<< R"cpp()js"))))cpp";
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

	if (!_operationTypes.empty())
	{
		sourceFile << R"cpp(
)cpp";

		for (const auto& operationType : _operationTypes)
		{
			std::string operation(operationType.operation);

			operation[0] = std::toupper(operation[0]);
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

std::string Generator::getArgumentAccessType(const InputField& argument) const noexcept
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
			argumentType << R"cpp(web::json::value)cpp";
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
			resultType << R"cpp(web::json::value)cpp";
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
		}
	}

	if (!firstValue)
	{
		typeModifiers << R"cpp(>)cpp";
	}

	return typeModifiers.str();
}

std::string Generator::getIntrospectionType(const std::string& type, const TypeModifierStack& modifiers) const noexcept
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
					introspectionType << R"cpp(std::make_shared<)cpp" << s_introspectionNamespace
						<< R"cpp(::WrapperType>()cpp" << s_introspectionNamespace
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
				introspectionType << R"cpp(std::make_shared<)cpp" << s_introspectionNamespace
					<< R"cpp(::WrapperType>()cpp" << s_introspectionNamespace
					<< R"cpp(::__TypeKind::LIST, )cpp";
				++wrapperCount;
				break;
			}
		}
	}

	if (nonNull)
	{
		introspectionType << R"cpp(std::make_shared<)cpp" << s_introspectionNamespace
			<< R"cpp(::WrapperType>()cpp" << s_introspectionNamespace
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
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
