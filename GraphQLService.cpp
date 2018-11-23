// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphQLService.h"
#include "GraphQLTree.h"

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

web::json::value Object::resolve(const peg::ast_node& selection, const FragmentMap& fragments, const web::json::object& variables) const
{
	auto result = web::json::value::object(selection.children.size());

	for (const auto& child : selection.children)
	{
		SelectionVisitor visitor(fragments, variables, _typeNames, _resolvers);

		visitor.visit(*child);

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

web::json::value Request::resolve(const peg::ast_node& root, const std::string& operationName, const web::json::object& variables) const
{
	const auto& document = *root.children.front();
	FragmentDefinitionVisitor fragmentVisitor;

	peg::for_each_child<peg::fragment_definition>(document,
		[&fragmentVisitor](const peg::ast_node& child)
	{
		fragmentVisitor.visit(child);
		return true;
	});

	auto fragments = fragmentVisitor.getFragments();
	OperationDefinitionVisitor operationVisitor(_operations, operationName, variables, fragments);

	peg::for_each_child<peg::operation_definition>(document,
		[&operationVisitor](const peg::ast_node& child)
	{
		operationVisitor.visit(child);
		return true;
	});

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
	const bool hasAlias = field.children.front()->is<peg::alias_name>();
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

	bool skip = false;

	peg::for_each_child<peg::directives>(field,
		[this, &skip](const peg::ast_node& child)
	{
		skip = shouldSkip(&child.children);
		return false;
	});

	if (skip)
	{
		return;
	}

	auto arguments = web::json::value::object(true);

	for (const auto& child : field.children)
	{
		if (child->is<peg::arguments>())
		{
			ValueVisitor visitor(_variables);

			for (const auto& argument : child->children)
			{
				visitor.visit(*argument->children.back());
				arguments[utility::conversions::to_string_t(argument->children.front()->content())] = visitor.getValue();
			}

			break;
		}
	}

	const peg::ast_node* selection = nullptr;

	for (const auto& child : field.children)
	{
		if (child->is<peg::selection_set>())
		{
			selection = child.get();
			break;
		}
	}

	_values[utility::conversions::to_string_t(alias)] = itr->second({ arguments.as_object(), selection, _fragments, _variables });
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
		peg::for_each_child<peg::directives>(fragmentSpread,
			[this, &skip](const peg::ast_node& child)
		{
			skip = shouldSkip(&child.children);
			return false;
		});
	}

	if (skip)
	{
		return;
	}

	peg::for_each_child<peg::selection_set>(fragmentSpread,
		[this](const peg::ast_node& child)
	{
		for (const auto& selection : child.children)
		{
			visit(*selection);
		}
		return false;
	});
}

void SelectionVisitor::visitInlineFragment(const peg::ast_node& inlineFragment)
{
	bool skip = false;

	peg::for_each_child<peg::directives>(inlineFragment,
		[this, &skip](const peg::ast_node& child)
	{
		skip = shouldSkip(&child.children);
		return false;
	});

	if (skip)
	{
		return;
	}

	const peg::ast_node* typeCondition = nullptr;

	for (const auto& child : inlineFragment.children)
	{
		if (child->is<peg::type_condition>())
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
			if (child->is<peg::selection_set>())
			{
				for (const auto& selection : child->children)
				{
					if (selection->is<peg::field>())
					{
						visitField(*selection);
					}
					else if (selection->is<peg::fragment_spread>())
					{
						visitFragmentSpread(*selection);
					}
					else if (selection->is<peg::inline_fragment>())
					{
						visitInlineFragment(*selection);
					}
				}

				break;
			}
		}
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
		const std::string name(directive->children.front()->content());
		const bool include = (name == "include");
		const bool skip = (!include && (name == "skip"));

		if (!include && !skip)
		{
			continue;
		}

		const auto argument = (directive->children.back()->is<peg::arguments>() && directive->children.back()->children.size() == 1)
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

		if (argument->children.back()->is<peg::true_keyword>())
		{
			return skip;
		}
		else if (argument->children.back()->is<peg::false_keyword>())
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
}

void ValueVisitor::visitIntValue(const peg::ast_node& intValue)
{
	_value = web::json::value::number(std::atoi(intValue.content().c_str()));
}

void ValueVisitor::visitFloatValue(const peg::ast_node& floatValue)
{
	_value = web::json::value::number(std::atof(floatValue.content().c_str()));
}

void ValueVisitor::visitStringValue(const peg::ast_node& stringValue)
{
	_value = web::json::value::string(utility::conversions::to_string_t(stringValue.unescaped));
}

void ValueVisitor::visitBooleanValue(const peg::ast_node& booleanValue)
{
	_value = web::json::value::boolean(booleanValue.is<peg::true_keyword>());
}

void ValueVisitor::visitNullValue(const peg::ast_node& /*nullValue*/)
{
	_value = web::json::value::null();
}

void ValueVisitor::visitEnumValue(const peg::ast_node& enumValue)
{
	_value = web::json::value::string(utility::conversions::to_string_t(enumValue.content()));
}

void ValueVisitor::visitListValue(const peg::ast_node& listValue)
{
	_value = web::json::value::array(listValue.children.size());

	std::transform(listValue.children.cbegin(), listValue.children.cend(), _value.as_array().begin(),
		[this](const std::unique_ptr<peg::ast_node>& value)
	{
		ValueVisitor visitor(_variables);

		visitor.visit(*value->children.back());

		return visitor.getValue();
	});
}

void ValueVisitor::visitObjectValue(const peg::ast_node& objectValue)
{
	_value = web::json::value::object(true);

	for (const auto& field : objectValue.children)
	{
		const std::string name(field->children.front()->content());
		ValueVisitor visitor(_variables);

		visitor.visit(*field->children.back());

		_value[utility::conversions::to_string_t(name)] = visitor.getValue();
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

void OperationDefinitionVisitor::visit(const peg::ast_node& operationDefinition)
{
	auto position = operationDefinition.begin();
	auto operation = operationDefinition.children.front()->is<peg::operation_type>()
		? operationDefinition.children.front()->content()
		: std::string("query");
	std::string name;

	peg::for_each_child<peg::operation_name>(operationDefinition,
		[&name](const peg::ast_node& child)
	{
		name = child.content();
		return false;
	});

	if (!_operationName.empty()
		&& name != _operationName)
	{
		// Skip the operations that don't match the name
		return;
	}

	try
	{
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

		peg::for_each_child<peg::variable_definitions>(operationDefinition,
			[this, &operationVariables](const peg::ast_node& child)
		{
			for (const auto& variable : child.children)
			{
				const auto& nameNode = *variable->children.front();
				const auto& defaultValueNode = *variable->children.back();
				auto nameVar = utility::conversions::to_string_t(nameNode.content().c_str() + 1);
				auto itrVar = _variables.find(nameVar);

				if (itrVar != _variables.cend())
				{
					operationVariables[itrVar->first] = itrVar->second;
				}
				else if (defaultValueNode.is<peg::default_value>())
				{
					ValueVisitor visitor(_variables);

					visitor.visit(*defaultValueNode.children.front());
					operationVariables[std::move(nameVar)] = visitor.getValue();
				}
			}

			return false;
		});

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
}

} /* namespace service */
} /* namespace graphql */
} /* namespace facebook */