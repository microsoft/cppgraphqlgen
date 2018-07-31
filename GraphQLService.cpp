// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphQLService.h"

#include <iostream>
#include <algorithm>

namespace facebook {
namespace graphql {
namespace service {

schema_exception::schema_exception(const std::vector<std::string>& messages)
	: _errors(web::json::value::array(messages.size()))
{
	std::transform(messages.cbegin(), messages.cend(), _errors.as_array().begin(),
		[](const std::string& message)
	{
		return web::json::value::object({
			{ _XPLATSTR("message"), web::json::value(utility::conversions::to_string_t(message)) }
			}, true);
	});
}

const web::json::value& schema_exception::getErrors() const noexcept
{
	return _errors;
}

Fragment::Fragment(const ast::FragmentDefinition& fragmentDefinition)
	: _type(fragmentDefinition.getTypeCondition().getName().getValue())
	, _selection(fragmentDefinition.getSelectionSet())
{
}

const std::string& Fragment::getType() const
{
	return _type;
}

const ast::SelectionSet& Fragment::getSelection() const
{
	return _selection;
}

template <>
int ModifiedArgument<int>::convert(const web::json::value& value)
{
	if (!value.is_integer())
	{
		throw web::json::json_exception(_XPLATSTR("not an integer"));
	}

	return value.as_integer();
}

template <>
double ModifiedArgument<double>::convert(const web::json::value& value)
{
	if (!value.is_double())
	{
		throw web::json::json_exception(_XPLATSTR("not a float"));
	}

	return value.as_double();
}

template <>
std::string ModifiedArgument<std::string>::convert(const web::json::value& value)
{
	if (!value.is_string())
	{
		throw web::json::json_exception(_XPLATSTR("not a string"));
	}

	return utility::conversions::to_utf8string(value.as_string());
}

template <>
bool ModifiedArgument<bool>::convert(const web::json::value& value)
{
	if (!value.is_boolean())
	{
		throw web::json::json_exception(_XPLATSTR("not a boolean"));
	}

	return value.as_bool();
}

template <>
web::json::value ModifiedArgument<web::json::value>::convert(const web::json::value& value)
{
	if (!value.is_object())
	{
		throw web::json::json_exception(_XPLATSTR("not an object"));
	}

	return value;
}

template <>
std::vector<unsigned char> ModifiedArgument<std::vector<unsigned char>>::convert(const web::json::value& value)
{
	if (!value.is_string())
	{
		throw web::json::json_exception(_XPLATSTR("not a string"));
	}

	try
	{
		return utility::conversions::from_base64(value.as_string());
	}
	catch (const std::runtime_error& ex)
	{
		std::ostringstream error;

		error << "Error decoding base64 ID: "
			<< ex.what();

		throw schema_exception({ error.str() });
	}
}

template <>
web::json::value ModifiedResult<int>::convert(const int& result, ResolverParams)
{
	return web::json::value::number(result);
}

template <>
web::json::value ModifiedResult<double>::convert(const double& result, ResolverParams)
{
	return web::json::value::number(result);
}

template <>
web::json::value ModifiedResult<std::string>::convert(const std::string& result, ResolverParams)
{
	return web::json::value::string(utility::conversions::to_string_t(result));
}

template <>
web::json::value ModifiedResult<bool>::convert(const bool& result, ResolverParams)
{
	return web::json::value::boolean(result);
}

template <>
web::json::value ModifiedResult<web::json::value>::convert(const web::json::value& result, ResolverParams)
{
	return result;
}

template <>
web::json::value ModifiedResult<std::vector<unsigned char>>::convert(const std::vector<unsigned char>& result, ResolverParams)
{
	try
	{
		return web::json::value::string(utility::conversions::to_base64(result));
	}
	catch (const std::runtime_error& ex)
	{
		std::ostringstream error;

		error << "Error encoding base64 ID: "
			<< ex.what();

		throw schema_exception({ error.str() });
	}
}

template <>
web::json::value ModifiedResult<Object>::convert(const std::shared_ptr<Object>& result, ResolverParams params)
{
	if (!result)
	{
		return web::json::value::null();
	}

	if (!params.selection)
	{
		return web::json::value::object();
	}

	return result->resolve(*params.selection, params.fragments, params.variables);
}

Object::Object(TypeNames typeNames, ResolverMap resolvers)
	: _typeNames(std::move(typeNames))
	, _resolvers(std::move(resolvers))
{
}

web::json::value Object::resolve(const ast::SelectionSet& selection, const FragmentMap& fragments, const web::json::object& variables) const
{
	auto result = web::json::value::object(selection.getSelections().size());

	for (const auto& entry : selection.getSelections())
	{
		SelectionVisitor visitor(fragments, variables, _typeNames, _resolvers);

		entry->accept(&visitor);

		auto values = visitor.getValues();

		if (values.is_object())
		{
			for (auto& value : values.as_object())
			{
				result[value.first] = std::move(value.second);
			}
		}
	}

	return result;
}

Request::Request(TypeMap operationTypes)
	: _operations(std::move(operationTypes))
{
}

web::json::value Request::resolve(const ast::Node& document, const std::string& operationName, const web::json::object& variables) const
{
	FragmentDefinitionVisitor fragmentVisitor;

	document.accept(&fragmentVisitor);

	auto fragments = fragmentVisitor.getFragments();
	OperationDefinitionVisitor operationVisitor(_operations, operationName, variables, fragments);

	document.accept(&operationVisitor);

	return operationVisitor.getValue();
}

SelectionVisitor::SelectionVisitor(const FragmentMap& fragments, const web::json::object& variables, const TypeNames& typeNames, const ResolverMap& resolvers)
	: _fragments(fragments)
	, _variables(variables)
	, _typeNames(typeNames)
	, _resolvers(resolvers)
	, _values(web::json::value::object(true))
{
}

web::json::value SelectionVisitor::getValues()
{
	web::json::value result(std::move(_values));
	return result;
}

bool SelectionVisitor::visitField(const ast::Field& field)
{
	const std::string name(field.getName().getValue());
	const std::string alias((field.getAlias() != nullptr) ? field.getAlias()->getValue() : name);
	auto itr = _resolvers.find(name);

	if (itr == _resolvers.cend())
	{
		std::ostringstream error;

		error << "Unknown field name: " << name
			<< " line: " << field.getLocation().begin.line
			<< " column: " << field.getLocation().begin.column;

		throw schema_exception({ error.str() });
	}

	if (shouldSkip(field.getDirectives()))
	{
		return false;
	}

	auto arguments = web::json::value::object(true);

	if (field.getArguments() != nullptr)
	{
		ValueVisitor visitor(_variables);

		for (const auto& argument : *field.getArguments())
		{
			argument->getValue().accept(&visitor);
			arguments[utility::conversions::to_string_t(argument->getName().getValue())] = visitor.getValue();
		}
	}

	_values[utility::conversions::to_string_t(alias)] = itr->second({ arguments.as_object(), field.getSelectionSet(), _fragments, _variables });

	return false;
}

bool SelectionVisitor::visitFragmentSpread(const ast::FragmentSpread &fragmentSpread)
{
	const std::string name(fragmentSpread.getName().getValue());
	auto itr = _fragments.find(name);

	if (itr == _fragments.cend())
	{
		std::ostringstream error;

		error << "Unknown fragment name: " << name
			<< " line: " << fragmentSpread.getLocation().begin.line
			<< " column: " << fragmentSpread.getLocation().begin.column;

		throw schema_exception({ error.str() });
	}

	if (!shouldSkip(fragmentSpread.getDirectives())
		&& _typeNames.count(itr->second.getType()) > 0)
	{
		itr->second.getSelection().accept(this);
	}

	return false;
}

bool SelectionVisitor::visitInlineFragment(const ast::InlineFragment &inlineFragment)
{
	if (!shouldSkip(inlineFragment.getDirectives())
		&& (inlineFragment.getTypeCondition() == nullptr
			|| _typeNames.count(inlineFragment.getTypeCondition()->getName().getValue()) > 0))
	{
		inlineFragment.getSelectionSet().accept(this);
	}

	return false;
}

bool SelectionVisitor::shouldSkip(const std::vector<std::unique_ptr<ast::Directive>>* directives) const
{
	if (directives == nullptr)
	{
		return false;
	}

	for (const auto& directive : *directives)
	{
		const std::string name(directive->getName().getValue());
		const bool include = (name == "include");
		const bool skip = (!include && (name == "skip"));

		if (!include && !skip)
		{
			continue;
		}

		const auto argument = (directive->getArguments() != nullptr && directive->getArguments()->size() == 1)
			? directive->getArguments()->front().get()
			: nullptr;
		const std::string argumentName((argument != nullptr) ? argument->getName().getValue() : "");

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
				error << " line: " << argument->getLocation().begin.line
					<< " column: " << argument->getLocation().begin.column;
			}

			throw schema_exception({ error.str() });
		}

		ValueVisitor visitor(_variables);

		argument->getValue().accept(&visitor);

		auto value = visitor.getValue();

		if (!value.is_boolean())
		{
			std::ostringstream error;

			error << "Invalid argument to directive: " << name
				<< " name: if line: " << argument->getLocation().begin.line
				<< " column: " << argument->getLocation().begin.column;

			throw schema_exception({ error.str() });
		}

		if (value.as_bool() == skip)
		{
			// Skip this item
			return true;
		}
	}

	return false;
}

ValueVisitor::ValueVisitor(const web::json::object& variables)
	: _variables(variables)
{
}

web::json::value ValueVisitor::getValue()
{
	web::json::value result(std::move(_value));
	return result;
}

bool ValueVisitor::visitVariable(const ast::Variable& variable)
{
	const std::string name(variable.getName().getValue());
	auto itr = _variables.find(utility::conversions::to_string_t(name));

	if (itr == _variables.cend())
	{
		std::ostringstream error;

		error << "Unknown variable name: " << name
			<< " line: " << variable.getLocation().begin.line
			<< " column: " << variable.getLocation().begin.column;

		throw schema_exception({ error.str() });
	}

	_value = itr->second;

	return false;
}

bool ValueVisitor::visitIntValue(const ast::IntValue& intValue)
{
	_value = web::json::value::number(std::atoi(intValue.getValue()));
	return false;
}

bool ValueVisitor::visitFloatValue(const ast::FloatValue& floatValue)
{
	_value = web::json::value::number(std::atof(floatValue.getValue()));
	return false;
}

bool ValueVisitor::visitStringValue(const ast::StringValue& stringValue)
{
	_value = web::json::value::string(utility::conversions::to_string_t(stringValue.getValue()));
	return false;
}

bool ValueVisitor::visitBooleanValue(const ast::BooleanValue& booleanValue)
{
	_value = web::json::value::boolean(booleanValue.getValue());
	return false;
}

bool ValueVisitor::visitNullValue(const ast::NullValue& nullValue)
{
	_value = web::json::value::null();
	return false;
}

bool ValueVisitor::visitEnumValue(const ast::EnumValue& enumValue)
{
	_value = web::json::value::string(utility::conversions::to_string_t(enumValue.getValue()));
	return false;
}

bool ValueVisitor::visitListValue(const ast::ListValue& listValue)
{
	_value = web::json::value::array(listValue.getValues().size());

	std::transform(listValue.getValues().cbegin(), listValue.getValues().cend(), _value.as_array().begin(),
		[this](const std::unique_ptr<ast::Value>& value)
	{
		ValueVisitor visitor(_variables);

		value->accept(&visitor);
		return visitor.getValue();
	});

	return false;
}

bool ValueVisitor::visitObjectValue(const ast::ObjectValue& objectValue)
{
	_value = web::json::value::object(true);

	for (const auto& field : objectValue.getFields())
	{
		const std::string name(field->getName().getValue());
		ValueVisitor visitor(_variables);

		field->getValue().accept(&visitor);
		_value[utility::conversions::to_string_t(name)] = visitor.getValue();
	}

	return false;
}

FragmentDefinitionVisitor::FragmentDefinitionVisitor()
{
}

FragmentMap FragmentDefinitionVisitor::getFragments()
{
	FragmentMap result(std::move(_fragments));
	return result;
}

bool FragmentDefinitionVisitor::visitFragmentDefinition(const ast::FragmentDefinition& fragmentDefinition)
{
	_fragments.insert({ fragmentDefinition.getName().getValue(), Fragment(fragmentDefinition) });
	return false;
}

OperationDefinitionVisitor::OperationDefinitionVisitor(const TypeMap& operations, const std::string& operationName, const web::json::object& variables, const FragmentMap& fragments)
	: _operations(operations)
	, _operationName(operationName)
	, _variables(variables)
	, _fragments(fragments)
{
}

web::json::value OperationDefinitionVisitor::getValue()
{
	web::json::value result(std::move(_result));

	try
	{
		if (result.is_null())
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
		result = web::json::value::object({
			{ _XPLATSTR("data"),  web::json::value::null() },
			{ _XPLATSTR("errors"), ex.getErrors() }
			}, true);
	}

	return result;
}

bool OperationDefinitionVisitor::visitOperationDefinition(const ast::OperationDefinition& operationDefinition)
{
	std::string operation(operationDefinition.getOperation());
	const std::string name((operationDefinition.getName() != nullptr) ? operationDefinition.getName()->getValue() : "");
	const yy::location& location = name.empty() ? operationDefinition.getLocation() : operationDefinition.getName()->getLocation();

	try
	{
		if (!_operationName.empty()
			&& name != _operationName)
		{
			// Skip the operations that don't match the name
			return false;
		}


		if (!_result.is_null())
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

			error << " line: " << location.begin.line
				<< " column: " << location.begin.column;

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

			error << " line: " << location.begin.line
				<< " column: " << location.begin.column;

			throw schema_exception({ error.str() });
		}

		auto operationVariables = web::json::value::object();

		if (operationDefinition.getVariableDefinitions() != nullptr)
		{
			for (const auto& variable : *operationDefinition.getVariableDefinitions())
			{
				auto nameVar = utility::conversions::to_string_t(variable->getVariable().getName().getValue());
				auto itrVar = _variables.find(nameVar);

				if (itrVar != _variables.cend())
				{
					operationVariables[itrVar->first] = itrVar->second;
				}
				else if (variable->getDefaultValue() != nullptr)
				{
					ValueVisitor visitor(_variables);

					variable->getDefaultValue()->accept(&visitor);
					operationVariables[std::move(nameVar)] = visitor.getValue();
				}
			}
		}

		_result = web::json::value::object({
			{ _XPLATSTR("data"), itr->second->resolve(operationDefinition.getSelectionSet(), _fragments, operationVariables.as_object()) }
			}, true);
	}
	catch (const schema_exception& ex)
	{
		_result = web::json::value::object({
			{ _XPLATSTR("data"),  web::json::value::null() },
			{ _XPLATSTR("errors"), ex.getErrors() }
			}, true);
	}

	return false;
}

} /* namespace service */
} /* namespace graphql */
} /* namespace facebook */