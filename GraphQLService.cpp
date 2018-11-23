// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphQLService.h"
#include "GraphQLGrammar.h"

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

Fragment::Fragment(const grammar::ast_node& fragmentDefinition)
	: _type(fragmentDefinition.children[1]->children.front()->content())
	, _selection(*(fragmentDefinition.children.back()))
{
}

const std::string& Fragment::getType() const
{
	return _type;
}

const grammar::ast_node& Fragment::getSelection() const
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
web::json::value ModifiedResult<int>::convert(const int& result, ResolverParams&&)
{
	return web::json::value::number(result);
}

template <>
web::json::value ModifiedResult<double>::convert(const double& result, ResolverParams&&)
{
	return web::json::value::number(result);
}

template <>
web::json::value ModifiedResult<std::string>::convert(const std::string& result, ResolverParams&&)
{
	return web::json::value::string(utility::conversions::to_string_t(result));
}

template <>
web::json::value ModifiedResult<bool>::convert(const bool& result, ResolverParams&&)
{
	return web::json::value::boolean(result);
}

template <>
web::json::value ModifiedResult<web::json::value>::convert(const web::json::value& result, ResolverParams&&)
{
	return result;
}

template <>
web::json::value ModifiedResult<std::vector<unsigned char>>::convert(const std::vector<unsigned char>& result, ResolverParams&&)
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
web::json::value ModifiedResult<Object>::convert(const std::shared_ptr<Object>& result, ResolverParams&& params)
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

Object::Object(TypeNames&& typeNames, ResolverMap&& resolvers)
	: _typeNames(std::move(typeNames))
	, _resolvers(std::move(resolvers))
{
}

web::json::value Object::resolve(const grammar::ast_node& selection, const FragmentMap& fragments, const web::json::object& variables) const
{
	auto result = web::json::value::object(selection.children.size());

	for (const auto& child : selection.children)
	{
		SelectionVisitor visitor(fragments, variables, _typeNames, _resolvers);

		if (child->is<grammar::field>())
		{
			visitor.visitField(*child);
		}
		else if (child->is<grammar::fragment_spread>())
		{
			visitor.visitFragmentSpread(*child);
		}
		else if (child->is<grammar::inline_fragment>())
		{
			visitor.visitInlineFragment(*child);
		}

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

Request::Request(TypeMap&& operationTypes)
	: _operations(std::move(operationTypes))
{
}

web::json::value Request::resolve(const grammar::ast_node& document, const std::string& operationName, const web::json::object& variables) const
{
	FragmentDefinitionVisitor fragmentVisitor;

	for (const auto& child : document.children.front()->children)
	{
		if (child->is<grammar::fragment_definition>())
		{
			fragmentVisitor.visitFragmentDefinition(*child);
		}
	}

	auto fragments = fragmentVisitor.getFragments();
	OperationDefinitionVisitor operationVisitor(_operations, operationName, variables, fragments);

	for (const auto& child : document.children.front()->children)
	{
		if (child->is<grammar::operation_definition>())
		{
			operationVisitor.visitOperationDefinition(*child);
		}
	}

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

bool SelectionVisitor::visitField(const grammar::ast_node& field)
{
	const bool hasAlias = field.children.front()->is<grammar::alias_name>();
	const std::string name(field.children[hasAlias ? 1 : 0]->content());
	const std::string alias(hasAlias ? field.children.front()->content() : name);
	auto itr = _resolvers.find(name);

	if (itr == _resolvers.cend())
	{
		auto position = field.begin();
		std::ostringstream error;

		error << "Unknown field name: " << name
			<< " line: " << position.line
			<< " column: " << position.byte_in_line;

		throw schema_exception({ error.str() });
	}

	for (const auto& child : field.children)
	{
		if (child->is<grammar::directives>())
		{
			if (shouldSkip(&(child->children)))
			{
				return false;
			}
			else
			{
				break;
			}
		}
	}

	auto arguments = web::json::value::object(true);

	for (const auto& child : field.children)
	{
		if (child->is<grammar::arguments>())
		{
			ValueVisitor visitor(_variables);

			for (const auto& argument : child->children)
			{
				if (argument->children.back()->is<grammar::variable_value>())
				{
					visitor.visitVariable(*argument->children.back());
				}
				else if (argument->children.back()->is<grammar::integer_value>())
				{
					visitor.visitIntValue(*argument->children.back());
				}
				else if (argument->children.back()->is<grammar::float_value>())
				{
					visitor.visitFloatValue(*argument->children.back());
				}
				else if (argument->children.back()->is<grammar::string_value>())
				{
					visitor.visitStringValue(*argument->children.back());
				}
				else if (argument->children.back()->is<grammar::true_keyword>()
					|| argument->children.back()->is<grammar::false_keyword>())
				{
					visitor.visitBooleanValue(*argument->children.back());
				}
				else if (argument->children.back()->is<grammar::null_keyword>())
				{
					visitor.visitNullValue(*argument->children.back());
				}
				else if (argument->children.back()->is<grammar::enum_value>())
				{
					visitor.visitEnumValue(*argument->children.back());
				}
				else if (argument->children.back()->is<grammar::list_value>())
				{
					visitor.visitListValue(*argument->children.back());
				}
				else if (argument->children.back()->is<grammar::object_value>())
				{
					visitor.visitObjectValue(*argument->children.back());
				}

				arguments[utility::conversions::to_string_t(argument->children.front()->content())] = visitor.getValue();
			}

			break;
		}
	}

	const grammar::ast_node* selection = nullptr;

	for (const auto& child : field.children)
	{
		if (child->is<grammar::selection_set>())
		{
			selection = child.get();
			break;
		}
	}

	_values[utility::conversions::to_string_t(alias)] = itr->second({ arguments.as_object(), selection, _fragments, _variables });

	return false;
}

bool SelectionVisitor::visitFragmentSpread(const grammar::ast_node& fragmentSpread)
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

	const std::vector<std::unique_ptr<grammar::ast_node>>* directives = nullptr;

	for (const auto& child : fragmentSpread.children)
	{
		if (child->is<grammar::directives>())
		{
			directives = &child->children;
			break;
		}
	}

	if (!shouldSkip(directives)
		&& _typeNames.count(itr->second.getType()) > 0)
	{
		for (const auto& child : fragmentSpread.children)
		{
			if (child->is<grammar::selection_set>())
			{
				for (const auto& selection : child->children)
				{
					if (selection->is<grammar::field>())
					{
						visitField(*selection);
					}
					else if (selection->is<grammar::fragment_spread>())
					{
						visitFragmentSpread(*selection);
					}
					else if (selection->is<grammar::inline_fragment>())
					{
						visitInlineFragment(*selection);
					}
				}

				break;
			}
		}
	}

	return false;
}

bool SelectionVisitor::visitInlineFragment(const grammar::ast_node& inlineFragment)
{
	const std::vector<std::unique_ptr<grammar::ast_node>>* directives = nullptr;

	for (const auto& child : inlineFragment.children)
	{
		if (child->is<grammar::directives>())
		{
			directives = &child->children;
			break;
		}
	}

	if (!shouldSkip(directives))
	{
		const grammar::ast_node* typeCondition = nullptr;

		for (const auto& child : inlineFragment.children)
		{
			if (child->is<grammar::type_condition>())
			{
				typeCondition = child.get();
				break;
			}
		}

		if (typeCondition == nullptr
			|| _typeNames.count(typeCondition->children.front()->content()) > 0)
		{
			for (const auto& child : inlineFragment.children)
			{
				if (child->is<grammar::selection_set>())
				{
					for (const auto& selection : child->children)
					{
						if (selection->is<grammar::field>())
						{
							visitField(*selection);
						}
						else if (selection->is<grammar::fragment_spread>())
						{
							visitFragmentSpread(*selection);
						}
						else if (selection->is<grammar::inline_fragment>())
						{
							visitInlineFragment(*selection);
						}
					}

					break;
				}
			}
		}
	}

	return false;
}

bool SelectionVisitor::shouldSkip(const std::vector<std::unique_ptr<grammar::ast_node>>* directives) const
{
	if (directives == nullptr)
	{
		return false;
	}

	for (const auto& directive : *directives)
	{
		const std::string name(directive->children.front()->content());
		const bool include = (name == "include");
		const bool skip = (!include && (name == "skip"));

		if (!include && !skip)
		{
			continue;
		}

		const auto argument = (directive->children.back()->is<grammar::arguments>() && directive->children.back()->children.size() == 1)
			? directive->children.back()->children.front().get()
			: nullptr;
		const std::string argumentName((argument != nullptr) ? argument->children.front()->content() : "");

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

		if (argument->children.back()->is<grammar::true_keyword>())
		{
			return skip;
		}
		else if (argument->children.back()->is<grammar::false_keyword>())
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

ValueVisitor::ValueVisitor(const web::json::object& variables)
	: _variables(variables)
{
}

web::json::value ValueVisitor::getValue()
{
	web::json::value result(std::move(_value));
	return result;
}

bool ValueVisitor::visitVariable(const grammar::ast_node& variable)
{
	const std::string name(variable.content().c_str() + 1);
	auto itr = _variables.find(utility::conversions::to_string_t(name));

	if (itr == _variables.cend())
	{
		auto position = variable.begin();
		std::ostringstream error;

		error << "Unknown variable name: " << name
			<< " line: " << position.line
			<< " column: " << position.byte_in_line;

		throw schema_exception({ error.str() });
	}

	_value = itr->second;

	return false;
}

bool ValueVisitor::visitIntValue(const grammar::ast_node& intValue)
{
	_value = web::json::value::number(std::atoi(intValue.content().c_str()));
	return false;
}

bool ValueVisitor::visitFloatValue(const grammar::ast_node& floatValue)
{
	_value = web::json::value::number(std::atof(floatValue.content().c_str()));
	return false;
}

bool ValueVisitor::visitStringValue(const grammar::ast_node& stringValue)
{
	_value = web::json::value::string(utility::conversions::to_string_t(stringValue.unescaped));
	return false;
}

bool ValueVisitor::visitBooleanValue(const grammar::ast_node& booleanValue)
{
	_value = web::json::value::boolean(booleanValue.content().c_str());
	return false;
}

bool ValueVisitor::visitNullValue(const grammar::ast_node& nullValue)
{
	_value = web::json::value::null();
	return false;
}

bool ValueVisitor::visitEnumValue(const grammar::ast_node& enumValue)
{
	_value = web::json::value::string(utility::conversions::to_string_t(enumValue.content()));
	return false;
}

bool ValueVisitor::visitListValue(const grammar::ast_node& listValue)
{
	_value = web::json::value::array(listValue.children.size());

	std::transform(listValue.children.cbegin(), listValue.children.cend(), _value.as_array().begin(),
		[this](const std::unique_ptr<grammar::ast_node>& value)
	{
		ValueVisitor visitor(_variables);

		if (value->children.back()->is<grammar::variable_value>())
		{
			visitor.visitVariable(*value->children.back());
		}
		else if (value->children.back()->is<grammar::integer_value>())
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

bool ValueVisitor::visitObjectValue(const grammar::ast_node& objectValue)
{
	_value = web::json::value::object(true);

	for (const auto& field : objectValue.children)
	{
		const std::string name(field->children.front()->content());
		ValueVisitor visitor(_variables);

		if (field->children.back()->is<grammar::variable_value>())
		{
			visitor.visitVariable(*field->children.back());
		}
		else if (field->children.back()->is<grammar::integer_value>())
		{
			visitor.visitIntValue(*field->children.back());
		}
		else if (field->children.back()->is<grammar::float_value>())
		{
			visitor.visitFloatValue(*field->children.back());
		}
		else if (field->children.back()->is<grammar::string_value>())
		{
			visitor.visitStringValue(*field->children.back());
		}
		else if (field->children.back()->is<grammar::true_keyword>()
			|| field->children.back()->is<grammar::false_keyword>())
		{
			visitor.visitBooleanValue(*field->children.back());
		}
		else if (field->children.back()->is<grammar::null_keyword>())
		{
			visitor.visitNullValue(*field->children.back());
		}
		else if (field->children.back()->is<grammar::enum_value>())
		{
			visitor.visitEnumValue(*field->children.back());
		}
		else if (field->children.back()->is<grammar::list_value>())
		{
			visitor.visitListValue(*field->children.back());
		}
		else if (field->children.back()->is<grammar::object_value>())
		{
			visitor.visitObjectValue(*field->children.back());
		}

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

bool FragmentDefinitionVisitor::visitFragmentDefinition(const grammar::ast_node& fragmentDefinition)
{
	_fragments.insert({ fragmentDefinition.children.front()->content(), Fragment(fragmentDefinition) });
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

bool OperationDefinitionVisitor::visitOperationDefinition(const grammar::ast_node& operationDefinition)
{
	auto position = operationDefinition.begin();
	auto operation = operationDefinition.children.front()->is<grammar::operation_type>()
		? operationDefinition.children.front()->content()
		: std::string("query");
	std::string name;

	for (const auto& child : operationDefinition.children)
	{
		if (child->is<grammar::operation_name>())
		{
			name = child->content();
			break;
		}
	}

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

		auto operationVariables = web::json::value::object();

		for (const auto& child : operationDefinition.children)
		{
			if (child->is<grammar::variable_definitions>())
			{
				for (const auto& variable : child->children)
				{
					auto nameVar = utility::conversions::to_string_t(variable->children.front()->content().c_str() + 1);
					auto itrVar = _variables.find(nameVar);

					if (itrVar != _variables.cend())
					{
						operationVariables[itrVar->first] = itrVar->second;
					}
					else if (variable->children.back()->is<grammar::default_value>())
					{
						ValueVisitor visitor(_variables);

						if (variable->children.back()->children.front()->is<grammar::variable_value>())
						{
							visitor.visitVariable(*variable->children.back()->children.front());
						}
						else if (variable->children.back()->children.front()->is<grammar::integer_value>())
						{
							visitor.visitIntValue(*variable->children.back()->children.front());
						}
						else if (variable->children.back()->children.front()->is<grammar::float_value>())
						{
							visitor.visitFloatValue(*variable->children.back()->children.front());
						}
						else if (variable->children.back()->children.front()->is<grammar::string_value>())
						{
							visitor.visitStringValue(*variable->children.back()->children.front());
						}
						else if (variable->children.back()->children.front()->is<grammar::true_keyword>()
							|| variable->children.back()->children.front()->is<grammar::false_keyword>())
						{
							visitor.visitBooleanValue(*variable->children.back()->children.front());
						}
						else if (variable->children.back()->children.front()->is<grammar::null_keyword>())
						{
							visitor.visitNullValue(*variable->children.back()->children.front());
						}
						else if (variable->children.back()->children.front()->is<grammar::enum_value>())
						{
							visitor.visitEnumValue(*variable->children.back()->children.front());
						}
						else if (variable->children.back()->children.front()->is<grammar::list_value>())
						{
							visitor.visitListValue(*variable->children.back()->children.front());
						}
						else if (variable->children.back()->children.front()->is<grammar::object_value>())
						{
							visitor.visitObjectValue(*variable->children.back()->children.front());
						}

						operationVariables[std::move(nameVar)] = visitor.getValue();
					}
				}

				break;
			}
		}

		_result = web::json::value::object({
			{ _XPLATSTR("data"), itr->second->resolve(*operationDefinition.children.back(), _fragments, operationVariables.as_object()) }
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