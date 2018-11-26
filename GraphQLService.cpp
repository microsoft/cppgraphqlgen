// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphQLService.h"
#include "GraphQLTree.h"
#include "GraphQLGrammar.h"

#include <iostream>
#include <algorithm>
#include <stdexcept>

namespace facebook {
namespace graphql {
namespace service {

schema_exception::schema_exception(const std::vector<std::string>& messages)
	: _errors(rapidjson::Type::kArrayType)
{
	auto& allocator = _errors.GetAllocator();

	for (const auto& message : messages)
	{
		rapidjson::Value error(rapidjson::Type::kObjectType);

		error.AddMember(rapidjson::StringRef("message"), rapidjson::Value(message.c_str(), allocator), allocator);
		_errors.PushBack(error, allocator);
	}
}

const rapidjson::Document& schema_exception::getErrors() const noexcept
{
	return _errors;
}

Fragment::Fragment(const peg::ast_node& fragmentDefinition)
	: _type(fragmentDefinition.children[1]->children.front()->content())
	, _selection(*(fragmentDefinition.children.back()))
{
}

const std::string& Fragment::getType() const
{
	return _type;
}

const peg::ast_node& Fragment::getSelection() const
{
	return _selection;
}

uint8_t Base64::verifyFromBase64(unsigned char ch)
{
	uint8_t result = fromBase64(ch);

	if (result > 63)
	{
		throw schema_exception({ "invalid character in base64 encoded string" });
	}

	return result;
}

std::vector<unsigned char> Base64::fromBase64(const char* encoded, size_t count)
{
	std::vector<unsigned char> result;

	if (!count)
	{
		return result;
	}

	result.reserve(count * 3 / 4);
	while (encoded[0] && encoded[1])
	{
		uint16_t buffer = static_cast<uint16_t>(verifyFromBase64(*encoded++)) << 10;

		buffer |= static_cast<uint16_t>(verifyFromBase64(*encoded++)) << 4;
		result.push_back(static_cast<unsigned char>((buffer & 0xFF00) >> 8));
		buffer = (buffer & 0xFF) << 8;

		if (!*encoded || '=' == *encoded)
		{
			if (0 != buffer
				|| (*encoded && (*++encoded != '=' || *++encoded)))
			{
				throw schema_exception({ "invalid padding at the end of a base64 encoded string" });
			}

			break;
		}

		buffer |= static_cast<uint16_t>(verifyFromBase64(*encoded++)) << 6;
		result.push_back(static_cast<unsigned char>((buffer & 0xFF00) >> 8));
		buffer &= 0xFF;

		if (!*encoded || '=' == *encoded)
		{
			if (0 != buffer
				|| (*encoded && *++encoded))
			{
				throw schema_exception({ "invalid padding at the end of a base64 encoded string" });
			}

			break;
		}

		buffer |= static_cast<uint16_t>(verifyFromBase64(*encoded++));
		result.push_back(static_cast<unsigned char>(buffer & 0xFF));
	}

	return result;
}

unsigned char Base64::verifyToBase64(uint8_t i)
{
	unsigned char result = toBase64(i);

	if (result == '=')
	{
		throw std::logic_error("invalid 6-bit value");
	}

	return result;
}

std::string Base64::toBase64(const std::vector<unsigned char>& bytes)
{
	std::string result;

	if (bytes.empty())
	{
		return result;
	}

	auto itr = bytes.cbegin();
	const auto itrEnd = bytes.cend();
	const size_t count = bytes.size();

	result.reserve((count + (count % 3)) * 4 / 3);
	while (itr != itrEnd)
	{
		uint16_t buffer = static_cast<uint8_t>(*itr++) << 8;

		result.push_back(verifyToBase64((buffer & 0xFC00) >> 10));

		if (itr == itrEnd)
		{
			result.push_back(verifyToBase64((buffer & 0x03F0) >> 4));
			result.append("==");
			break;
		}

		buffer |= static_cast<uint8_t>(*itr++);
		result.push_back(verifyToBase64((buffer & 0x03F0) >> 4));
		buffer = buffer << 8;

		if (itr == itrEnd)
		{
			result.push_back(verifyToBase64((buffer & 0x0FC0) >> 6));
			result.push_back('=');
			break;
		}

		buffer |= static_cast<uint8_t>(*itr++);
		result.push_back(verifyToBase64((buffer & 0x0FC0) >> 6));
		result.push_back(verifyToBase64(buffer & 0x3F));
	}

	return result;
}

template <>
int ModifiedArgument<int>::convert(const rapidjson::Value& value)
{
	if (!value.IsInt())
	{
		throw schema_exception({ "not an integer" });
	}

	return value.GetInt();
}

template <>
double ModifiedArgument<double>::convert(const rapidjson::Value& value)
{
	if (!value.IsDouble())
	{
		throw schema_exception({ "not a float" });
	}

	return value.GetDouble();
}

template <>
std::string ModifiedArgument<std::string>::convert(const rapidjson::Value& value)
{
	if (!value.IsString())
	{
		throw schema_exception({ "not a string" });
	}

	return value.GetString();
}

template <>
bool ModifiedArgument<bool>::convert(const rapidjson::Value& value)
{
	if (!value.IsBool())
	{
		throw schema_exception({ "not a boolean" });
	}

	return value.GetBool();
}

template <>
rapidjson::Document ModifiedArgument<rapidjson::Document>::convert(const rapidjson::Value& value)
{
	if (!value.IsObject())
	{
		throw schema_exception({ "not an object" });
	}

	rapidjson::Document document(rapidjson::Type::kObjectType);

	document.CopyFrom(value, document.GetAllocator());

	return document;
}

template <>
std::vector<unsigned char> ModifiedArgument<std::vector<unsigned char>>::convert(const rapidjson::Value& value)
{
	if (!value.IsString())
	{
		throw schema_exception({ "not a string" });
	}

	return Base64::fromBase64(value.GetString(), value.GetStringLength());
}

template <>
rapidjson::Document ModifiedResult<int>::convert(int&& result, ResolverParams&&)
{
	rapidjson::Document document(rapidjson::Type::kNumberType);

	document.SetInt(result);

	return document;
}

template <>
rapidjson::Document ModifiedResult<double>::convert(double&& result, ResolverParams&&)
{
	rapidjson::Document document(rapidjson::Type::kNumberType);

	document.SetDouble(result);

	return document;
}

template <>
rapidjson::Document ModifiedResult<std::string>::convert(std::string&& result, ResolverParams&&)
{
	rapidjson::Document document(rapidjson::Type::kStringType);

	document.SetString(result.c_str(), document.GetAllocator());
	
	return document;
}

template <>
rapidjson::Document ModifiedResult<bool>::convert(bool&& result, ResolverParams&&)
{
	return rapidjson::Document(result ? rapidjson::Type::kTrueType : rapidjson::Type::kFalseType);
}

template <>
rapidjson::Document ModifiedResult<rapidjson::Document>::convert(rapidjson::Document&& result, ResolverParams&&)
{
	return rapidjson::Document(std::move(result));
}

template <>
rapidjson::Document ModifiedResult<std::vector<unsigned char>>::convert(std::vector<unsigned char>&& result, ResolverParams&&)
{
	rapidjson::Document document(rapidjson::Type::kStringType);

	document.SetString(Base64::toBase64(result).c_str(), document.GetAllocator());

	return document;
}

template <>
rapidjson::Document ModifiedResult<Object>::convert(std::shared_ptr<Object> result, ResolverParams&& params)
{
	if (!result)
	{
		return rapidjson::Document(rapidjson::Type::kNullType);
	}

	if (!params.selection)
	{
		return rapidjson::Document(rapidjson::Type::kObjectType);
	}

	return result->resolve(*params.selection, params.fragments, params.variables);
}

Object::Object(TypeNames&& typeNames, ResolverMap&& resolvers)
	: _typeNames(std::move(typeNames))
	, _resolvers(std::move(resolvers))
{
}

rapidjson::Document Object::resolve(const peg::ast_node& selection, const FragmentMap& fragments, const rapidjson::Value::ConstObject& variables) const
{
	rapidjson::Document result(rapidjson::Type::kObjectType);
	auto& allocator = result.GetAllocator();

	for (const auto& child : selection.children)
	{
		SelectionVisitor visitor(fragments, variables, _typeNames, _resolvers);

		visitor.visit(*child);

		auto values = visitor.getValues();

		if (values.IsObject())
		{
			for (const auto& entry : values.GetObject())
			{
				rapidjson::Value name;
				rapidjson::Value value;

				name.CopyFrom(entry.name, allocator);
				value.CopyFrom(entry.value, allocator);

				result.AddMember(name, value, allocator);
			}
		}
	}

	return result;
}

Request::Request(TypeMap&& operationTypes)
	: _operations(std::move(operationTypes))
{
}

rapidjson::Document Request::resolve(const peg::ast_node& root, const std::string& operationName, const rapidjson::Document::ConstObject& variables) const
{
	const auto& document = *root.children.front();
	FragmentDefinitionVisitor fragmentVisitor;

	peg::for_each_child<peg::fragment_definition>(document,
		[&fragmentVisitor](const peg::ast_node& child)
	{
		fragmentVisitor.visit(child);
	});

	auto fragments = fragmentVisitor.getFragments();
	OperationDefinitionVisitor operationVisitor(_operations, operationName, variables, fragments);

	peg::for_each_child<peg::operation_definition>(document,
		[&operationVisitor](const peg::ast_node& child)
	{
		operationVisitor.visit(child);
	});

	return operationVisitor.getValue();
}

SelectionVisitor::SelectionVisitor(const FragmentMap& fragments, const rapidjson::Document::ConstObject& variables, const TypeNames& typeNames, const ResolverMap& resolvers)
	: _fragments(fragments)
	, _variables(variables)
	, _typeNames(typeNames)
	, _resolvers(resolvers)
	, _values(rapidjson::Type::kObjectType)
{
}

rapidjson::Document SelectionVisitor::getValues()
{
	return rapidjson::Document(std::move(_values));
}

void SelectionVisitor::visit(const peg::ast_node& selection)
{
	if (selection.is<peg::field>())
	{
		visitField(selection);
	}
	else if (selection.is<peg::fragment_spread>())
	{
		visitFragmentSpread(selection);
	}
	else if (selection.is<peg::inline_fragment>())
	{
		visitInlineFragment(selection);
	}
}

void SelectionVisitor::visitField(const peg::ast_node& field)
{
	std::string name;

	peg::on_first_child<peg::field_name>(field,
		[&name](const peg::ast_node& child)
	{
		name = child.content();
	});

	std::string alias;

	peg::on_first_child<peg::alias_name>(field,
		[&alias](const peg::ast_node& child)
	{
		alias = child.content();
	});

	if (alias.empty())
	{
		alias = name;
	}

	const auto itr = _resolvers.find(name);

	if (itr == _resolvers.cend())
	{
		auto position = field.begin();
		std::ostringstream error;

		error << "Unknown field name: " << name
			<< " line: " << position.line
			<< " column: " << position.byte_in_line;

		throw schema_exception({ error.str() });
	}

	bool skip = false;

	peg::on_first_child<peg::directives>(field,
		[this, &skip](const peg::ast_node& child)
	{
		skip = shouldSkip(&child.children);
	});

	if (skip)
	{
		return;
	}

	rapidjson::Document arguments(rapidjson::Type::kObjectType);

	peg::on_first_child<peg::arguments>(field,
		[this, &arguments](const peg::ast_node& child)
	{
		ValueVisitor visitor(_variables);
		auto& argumentAllocator = arguments.GetAllocator();

		for (const auto& argument : child.children)
		{
			rapidjson::Value argumentName(argument->children.front()->content().c_str(), argumentAllocator);
			rapidjson::Value argumentValue;

			visitor.visit(*argument->children.back());
			argumentValue.CopyFrom(visitor.getValue(), argumentAllocator);
			arguments.AddMember(argumentName, argumentValue, argumentAllocator);
		}
	});

	const peg::ast_node* selection = nullptr;

	peg::on_first_child<peg::selection_set>(field,
		[&selection](const peg::ast_node& child)
	{
		selection = &child;
	});

	auto& selectionAllocator = _values.GetAllocator();
	rapidjson::Value selectionName(alias.c_str(), selectionAllocator);
	rapidjson::Value selectionValue;

	selectionValue.CopyFrom(itr->second({ const_cast<const rapidjson::Document&>(arguments).GetObject(), selection, _fragments, _variables }), selectionAllocator);

	_values.AddMember(selectionName, selectionValue, selectionAllocator);
}

void SelectionVisitor::visitFragmentSpread(const peg::ast_node& fragmentSpread)
{
	const std::string name(fragmentSpread.children.front()->content());
	auto itr = _fragments.find(name);

	if (itr == _fragments.cend())
	{
		auto position = fragmentSpread.begin();
		std::ostringstream error;

		error << "Unknown fragment name: " << name
			<< " line: " << position.line
			<< " column: " << position.byte_in_line;

		throw schema_exception({ error.str() });
	}

	bool skip = (_typeNames.count(itr->second.getType()) == 0);

	if (!skip)
	{
		peg::on_first_child<peg::directives>(fragmentSpread,
			[this, &skip](const peg::ast_node& child)
		{
			skip = shouldSkip(&child.children);
		});
	}

	if (skip)
	{
		return;
	}

	peg::on_first_child<peg::selection_set>(fragmentSpread,
		[this](const peg::ast_node& child)
	{
		for (const auto& selection : child.children)
		{
			visit(*selection);
		}
	});
}

void SelectionVisitor::visitInlineFragment(const peg::ast_node& inlineFragment)
{
	bool skip = false;

	peg::on_first_child<peg::directives>(inlineFragment,
		[this, &skip](const peg::ast_node& child)
	{
		skip = shouldSkip(&child.children);
	});

	if (skip)
	{
		return;
	}

	const peg::ast_node* typeCondition = nullptr;

	peg::on_first_child<peg::type_condition>(inlineFragment,
		[&typeCondition](const peg::ast_node& child)
	{
		typeCondition = &child;
	});

	if (typeCondition == nullptr
		|| _typeNames.count(typeCondition->children.front()->content()) > 0)
	{
		peg::on_first_child<peg::selection_set>(inlineFragment,
			[this](const peg::ast_node& child)
		{
			for (const auto& selection : child.children)
			{
				visit(*selection);
			}
		});
	}
}

bool SelectionVisitor::shouldSkip(const std::vector<std::unique_ptr<peg::ast_node>>* directives) const
{
	if (directives == nullptr)
	{
		return false;
	}

	for (const auto& directive : *directives)
	{
		std::string name;

		peg::on_first_child<peg::directive_name>(*directive,
			[&name](const peg::ast_node& child)
		{
			name = child.content();
		});

		const bool include = (name == "include");
		const bool skip = (!include && (name == "skip"));

		if (!include && !skip)
		{
			continue;
		}

		peg::ast_node* argument = nullptr;
		std::string argumentName;
		bool argumentTrue = false;
		bool argumentFalse = false;

		peg::on_first_child<peg::arguments>(*directive,
			[&argument, &argumentName, &argumentTrue, &argumentFalse](const peg::ast_node& child)
		{
			if (child.children.size() == 1)
			{
				argument = child.children.front().get();

				peg::on_first_child<peg::argument_name>(*argument,
					[&argumentName](const peg::ast_node& nameArg)
				{
					argumentName = nameArg.content();
				});

				peg::on_first_child<peg::true_keyword>(*argument,
					[&argumentTrue](const peg::ast_node& nameArg)
				{
					argumentTrue = true;
				});

				if (!argumentTrue)
				{
					peg::on_first_child<peg::false_keyword>(*argument,
						[&argumentFalse](const peg::ast_node& nameArg)
					{
						argumentFalse = true;
					});
				}
			}
		});

		if (argumentName != "if")
		{
			std::ostringstream error;

			error << "Unknown argument to directive: " << name;

			if (!argumentName.empty())
			{
				error << " name: " << argumentName;
			}

			if (argument != nullptr)
			{
				auto position = argument->begin();

				error << " line: " << position.line
					<< " column: " << position.byte_in_line;
			}

			throw schema_exception({ error.str() });
		}

		if (argumentTrue)
		{
			return skip;
		}
		else if (argumentFalse)
		{
			return !skip;
		}
		else
		{
			auto position = argument->begin();
			std::ostringstream error;

			error << "Unknown argument to directive: " << name
				<< " line: " << position.line
				<< " column: " << position.byte_in_line;

			throw schema_exception({ error.str() });
		}
	}

	return false;
}

ValueVisitor::ValueVisitor(const rapidjson::Document::ConstObject& variables)
	: _variables(variables)
{
}

rapidjson::Document ValueVisitor::getValue()
{
	return rapidjson::Document(std::move(_value));
}

void ValueVisitor::visit(const peg::ast_node& value)
{
	if (value.is<peg::variable_value>())
	{
		visitVariable(value);
	}
	else if (value.is<peg::integer_value>())
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

void ValueVisitor::visitVariable(const peg::ast_node& variable)
{
	const std::string name(variable.content().c_str() + 1);
	auto itr = _variables.FindMember(name.c_str());

	if (itr == _variables.MemberEnd())
	{
		auto position = variable.begin();
		std::ostringstream error;

		error << "Unknown variable name: " << name
			<< " line: " << position.line
			<< " column: " << position.byte_in_line;

		throw schema_exception({ error.str() });
	}

	_value.CopyFrom(itr->value, _value.GetAllocator());
}

void ValueVisitor::visitIntValue(const peg::ast_node& intValue)
{
	_value.SetInt(std::atoi(intValue.content().c_str()));
}

void ValueVisitor::visitFloatValue(const peg::ast_node& floatValue)
{
	_value.SetDouble(std::atof(floatValue.content().c_str()));
}

void ValueVisitor::visitStringValue(const peg::ast_node& stringValue)
{
	_value.SetString(stringValue.unescaped.c_str(), _value.GetAllocator());
}

void ValueVisitor::visitBooleanValue(const peg::ast_node& booleanValue)
{
	_value.SetBool(booleanValue.is<peg::true_keyword>());
}

void ValueVisitor::visitNullValue(const peg::ast_node& /*nullValue*/)
{
	_value.SetNull();
}

void ValueVisitor::visitEnumValue(const peg::ast_node& enumValue)
{
	_value.SetString(enumValue.content().c_str(), _value.GetAllocator());
}

void ValueVisitor::visitListValue(const peg::ast_node& listValue)
{
	_value = rapidjson::Document(rapidjson::Type::kArrayType);

	auto& allocator = _value.GetAllocator();
	
	_value.Reserve(listValue.children.size(), allocator);
	for (const auto& child : listValue.children)
	{
		rapidjson::Value value;
		ValueVisitor visitor(_variables);

		visitor.visit(*child->children.back());
		value.CopyFrom(visitor.getValue(), allocator);
		_value.PushBack(value, allocator);
	}
}

void ValueVisitor::visitObjectValue(const peg::ast_node& objectValue)
{
	_value = rapidjson::Document(rapidjson::Type::kObjectType);

	auto& allocator = _value.GetAllocator();

	for (const auto& field : objectValue.children)
	{
		rapidjson::Value name(field->children.front()->content().c_str(), allocator);
		rapidjson::Value value;
		ValueVisitor visitor(_variables);

		visitor.visit(*field->children.back());
		value.CopyFrom(visitor.getValue(), allocator);
		_value.AddMember(name, value, allocator);
	}
}

FragmentDefinitionVisitor::FragmentDefinitionVisitor()
{
}

FragmentMap FragmentDefinitionVisitor::getFragments()
{
	FragmentMap result(std::move(_fragments));
	return result;
}

void FragmentDefinitionVisitor::visit(const peg::ast_node& fragmentDefinition)
{
	_fragments.insert({ fragmentDefinition.children.front()->content(), Fragment(fragmentDefinition) });
}

OperationDefinitionVisitor::OperationDefinitionVisitor(const TypeMap& operations, const std::string& operationName, const rapidjson::Document::ConstObject& variables, const FragmentMap& fragments)
	: _operations(operations)
	, _operationName(operationName)
	, _variables(variables)
	, _fragments(fragments)
{
}

rapidjson::Document OperationDefinitionVisitor::getValue()
{
	rapidjson::Document result(std::move(_result));

	try
	{
		if (result.IsNull())
		{
			std::ostringstream error;

			error << "Missing operation";

			if (!_operationName.empty())
			{
				error << " name: " << _operationName;
			}

			throw schema_exception({ error.str() });
		}
	}
	catch (const schema_exception& ex)
	{
		result = rapidjson::Document(rapidjson::Type::kObjectType);
		
		auto& allocator = result.GetAllocator();
		rapidjson::Value errors;

		errors.CopyFrom(ex.getErrors(), allocator);
		result.AddMember(rapidjson::StringRef("data"), rapidjson::Value(rapidjson::Type::kNullType), allocator);
		result.AddMember(rapidjson::StringRef("errors"), errors, allocator);
	}

	return result;
}

void OperationDefinitionVisitor::visit(const peg::ast_node& operationDefinition)
{
	std::string operation;

	peg::on_first_child<peg::operation_type>(operationDefinition,
		[&operation](const peg::ast_node& child)
	{
		operation = child.content();
	});

	if (operation.empty())
	{
		operation = "query";
	}

	auto position = operationDefinition.begin();
	std::string name;

	peg::on_first_child<peg::operation_name>(operationDefinition,
		[&name](const peg::ast_node& child)
	{
		name = child.content();
	});

	if (!_operationName.empty()
		&& name != _operationName)
	{
		// Skip the operations that don't match the name
		return;
	}

	try
	{
		if (!_result.IsNull())
		{
			std::ostringstream error;

			if (_operationName.empty())
			{
				error << "No operationName specified with extra operation";
			}
			else
			{
				error << "Duplicate operation";
			}

			if (!name.empty())
			{
				error << " name: " << name;
			}

			error << " line: " << position.line
				<< " column: " << position.byte_in_line;

			throw schema_exception({ error.str() });
		}

		auto itr = _operations.find(operation);

		if (itr == _operations.cend())
		{
			std::ostringstream error;

			error << "Unknown operation type: " << operation;

			if (!name.empty())
			{
				error << " name: " << name;
			}

			error << " line: " << position.line
				<< " column: " << position.byte_in_line;

			throw schema_exception({ error.str() });
		}

		auto operationVariables = rapidjson::Document(rapidjson::Type::kObjectType);

		peg::on_first_child<peg::variable_definitions>(operationDefinition,
			[this, &operationVariables](const peg::ast_node& child)
		{
			peg::for_each_child<peg::variable>(child,
				[this, &operationVariables](const peg::ast_node& variable)
			{
				std::string variableName;

				peg::on_first_child<peg::variable_name>(variable,
					[&variableName](const peg::ast_node& name)
				{
					// Skip the $ prefix
					variableName = name.content().c_str() + 1;
				});

				rapidjson::Value nameVar(rapidjson::StringRef(variableName.c_str()));
				auto itrVar = _variables.FindMember(nameVar);
				auto& variableAllocator = operationVariables.GetAllocator();
				rapidjson::Value valueVar;

				if (itrVar != _variables.MemberEnd())
				{
					valueVar.CopyFrom(itrVar->value, variableAllocator);
				}
				else
				{
					peg::on_first_child<peg::default_value>(variable,
						[this, &variableAllocator, &valueVar](const peg::ast_node& defaultValue)
					{
						ValueVisitor visitor(_variables);

						visitor.visit(*defaultValue.children.front());
						valueVar.CopyFrom(visitor.getValue(), variableAllocator);
					});
				}

				operationVariables.AddMember(nameVar, valueVar, variableAllocator);
			});
		});

		_result = rapidjson::Document(rapidjson::Type::kObjectType);

		auto& allocator = _result.GetAllocator();
		rapidjson::Value data;

		data.CopyFrom(itr->second->resolve(*operationDefinition.children.back(), _fragments, const_cast<const rapidjson::Document&>(operationVariables).GetObject()), allocator);
		_result.AddMember(rapidjson::StringRef("data"), data, allocator);
	}
	catch (const schema_exception& ex)
	{
		_result = rapidjson::Document(rapidjson::Type::kObjectType);

		auto& allocator = _result.GetAllocator();
		rapidjson::Value errors;

		errors.CopyFrom(ex.getErrors(), allocator);
		_result.AddMember(rapidjson::StringRef("data"), rapidjson::Value(rapidjson::Type::kNullType), allocator);
		_result.AddMember(rapidjson::StringRef("errors"), errors, allocator);
	}
}

} /* namespace service */
} /* namespace graphql */
} /* namespace facebook */