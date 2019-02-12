// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphQLGrammar.h"

#include <graphqlservice/GraphQLService.h>

#include <iostream>
#include <algorithm>
#include <array>
#include <stack>

namespace facebook {
namespace graphql {
namespace service {

schema_exception::schema_exception(std::vector<std::string>&& messages)
	: _errors(response::Type::List)
{
	for (auto& message : messages)
	{
		response::Value error(response::Type::Map);

		error.emplace_back(strMessage, response::Value(std::move(message)));
		_errors.emplace_back(std::move(error));
	}

	messages.clear();
}

const char* schema_exception::what() const noexcept
{
	return (_errors.size() < 1 || _errors[0].type() != response::Type::String)
		? "Unknown schema error"
		: _errors[0].get<const response::StringType&>().c_str();
}

const response::Value& schema_exception::getErrors() const noexcept
{
	return _errors;
}

response::Value schema_exception::getErrors() noexcept
{
	auto errors = std::move(_errors);

	return errors;
}

FieldParams::FieldParams(const SelectionSetParams & selectionSetParams, response::Value && directives)
	: SelectionSetParams(selectionSetParams)
	, fieldDirectives(std::move(directives))
{
}

// ValueVisitor visits the AST and builds a response::Value representation of any value
// hardcoded or referencing a variable in an operation.
class ValueVisitor
{
public:
	ValueVisitor(const response::Value& variables);

	void visit(const peg::ast_node& value);

	response::Value getValue();

private:
	void visitVariable(const peg::ast_node& variable);
	void visitIntValue(const peg::ast_node& intValue);
	void visitFloatValue(const peg::ast_node& floatValue);
	void visitStringValue(const peg::ast_node& stringValue);
	void visitBooleanValue(const peg::ast_node& booleanValue);
	void visitNullValue(const peg::ast_node& nullValue);
	void visitEnumValue(const peg::ast_node& enumValue);
	void visitListValue(const peg::ast_node& listValue);
	void visitObjectValue(const peg::ast_node& objectValue);

	const response::Value& _variables;
	response::Value _value;
};

ValueVisitor::ValueVisitor(const response::Value & variables)
	: _variables(variables)
{
}

response::Value ValueVisitor::getValue()
{
	auto result = std::move(_value);

	return result;
}

void ValueVisitor::visit(const peg::ast_node & value)
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

void ValueVisitor::visitVariable(const peg::ast_node & variable)
{
	const std::string name(variable.content().c_str() + 1);
	auto itr = _variables.find(name);

	if (itr == _variables.get<const response::MapType&>().cend())
	{
		auto position = variable.begin();
		std::ostringstream error;

		error << "Unknown variable name: " << name
			<< " line: " << position.line
			<< " column: " << position.byte_in_line;

		throw schema_exception({ error.str() });
	}

	_value = response::Value(itr->second);
}

void ValueVisitor::visitIntValue(const peg::ast_node & intValue)
{
	_value = response::Value(std::atoi(intValue.content().c_str()));
}

void ValueVisitor::visitFloatValue(const peg::ast_node & floatValue)
{
	_value = response::Value(std::atof(floatValue.content().c_str()));
}

void ValueVisitor::visitStringValue(const peg::ast_node & stringValue)
{
	_value = response::Value(std::string(stringValue.unescaped));
}

void ValueVisitor::visitBooleanValue(const peg::ast_node & booleanValue)
{
	_value = response::Value(booleanValue.is<peg::true_keyword>());
}

void ValueVisitor::visitNullValue(const peg::ast_node& /*nullValue*/)
{
	_value = {};
}

void ValueVisitor::visitEnumValue(const peg::ast_node & enumValue)
{
	_value = response::Value(response::Type::EnumValue);
	_value.set<response::StringType>(enumValue.content());
}

void ValueVisitor::visitListValue(const peg::ast_node & listValue)
{
	_value = response::Value(response::Type::List);
	_value.reserve(listValue.children.size());

	ValueVisitor visitor(_variables);

	for (const auto& child : listValue.children)
	{
		visitor.visit(*child);
		_value.emplace_back(visitor.getValue());
	}
}

void ValueVisitor::visitObjectValue(const peg::ast_node & objectValue)
{
	_value = response::Value(response::Type::Map);
	_value.reserve(objectValue.children.size());

	ValueVisitor visitor(_variables);

	for (const auto& field : objectValue.children)
	{
		visitor.visit(*field->children.back());
		_value.emplace_back(field->children.front()->content(), visitor.getValue());
	}
}

// DirectiveVisitor visits the AST and builds a 2-level map of directive names to argument
// name/value pairs.
class DirectiveVisitor
{
public:
	explicit DirectiveVisitor(const response::Value& variables);

	void visit(const peg::ast_node& directives);

	bool shouldSkip() const;
	response::Value getDirectives();

private:
	const response::Value& _variables;

	response::Value _directives;
};

DirectiveVisitor::DirectiveVisitor(const response::Value & variables)
	: _variables(variables)
	, _directives(response::Type::Map)
{
}

void DirectiveVisitor::visit(const peg::ast_node & directives)
{
	response::Value result(response::Type::Map);

	for (const auto& directive : directives.children)
	{
		std::string directiveName;

		peg::on_first_child<peg::directive_name>(*directive,
			[&directiveName](const peg::ast_node & child)
			{
				directiveName = child.content();
			});

		if (directiveName.empty())
		{
			continue;
		}

		response::Value directiveArguments(response::Type::Map);

		peg::on_first_child<peg::arguments>(*directive,
			[this, &directiveArguments](const peg::ast_node & child)
			{
				ValueVisitor visitor(_variables);

				for (auto& argument : child.children)
				{
					visitor.visit(*argument->children.back());

					directiveArguments.emplace_back(argument->children.front()->content(), visitor.getValue());
				}
			});

		result.emplace_back(std::move(directiveName), std::move(directiveArguments));
	}

	_directives = std::move(result);
}

response::Value DirectiveVisitor::getDirectives()
{
	auto result = std::move(_directives);

	return result;
}

bool DirectiveVisitor::shouldSkip() const
{
	static const std::array<std::pair<bool, std::string>, 2> skippedNames = {
		std::make_pair<bool, std::string>(true, "skip"),
		std::make_pair<bool, std::string>(false, "include"),
	};

	for (const auto& entry : skippedNames)
	{
		const bool skip = entry.first;
		auto itrDirective = _directives.find(entry.second);

		if (itrDirective == _directives.end())
		{
			continue;
		}

		auto& arguments = itrDirective->second;

		if (arguments.type() != response::Type::Map)
		{
			std::ostringstream error;

			error << "Invalid arguments to directive: " << entry.second;

			throw schema_exception({ error.str() });
		}

		bool argumentTrue = false;
		bool argumentFalse = false;

		for (auto& argument : arguments)
		{
			if (argumentTrue
				|| argumentFalse
				|| argument.second.type() != response::Type::Boolean
				|| argument.first != "if")
			{
				std::ostringstream error;

				error << "Invalid argument to directive: " << entry.second
					<< " name: " << argument.first;

				throw schema_exception({ error.str() });
			}

			argumentTrue = argument.second.get<response::BooleanType>();
			argumentFalse = !argumentTrue;
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
			std::ostringstream error;

			error << "Missing argument to directive: " << entry.second
				<< " name: if";

			throw schema_exception({ error.str() });
		}
	}

	return false;
}

Fragment::Fragment(const peg::ast_node & fragmentDefinition, const response::Value & variables)
	: _type(fragmentDefinition.children[1]->children.front()->content())
	, _directives(response::Type::Map)
	, _selection(*(fragmentDefinition.children.back()))
{
	peg::on_first_child<peg::directives>(fragmentDefinition,
		[this, &variables](const peg::ast_node & child)
		{
			DirectiveVisitor directiveVisitor(variables);

			directiveVisitor.visit(child);
			_directives = directiveVisitor.getDirectives();
		});
}

const std::string& Fragment::getType() const
{
	return _type;
}

const peg::ast_node& Fragment::getSelection() const
{
	return _selection;
}

const response::Value& Fragment::getDirectives() const
{
	return _directives;
}

ResolverParams::ResolverParams(const SelectionSetParams & selectionSetParams, std::string && fieldName, response::Value && arguments, response::Value && fieldDirectives,
	const peg::ast_node * selection, const FragmentMap & fragments, const response::Value & variables)
	: SelectionSetParams(selectionSetParams)
	, fieldName(std::move(fieldName))
	, arguments(std::move(arguments))
	, fieldDirectives(std::move(fieldDirectives))
	, selection(selection)
	, fragments(fragments)
	, variables(variables)
{
}

uint8_t Base64::verifyFromBase64(char ch)
{
	uint8_t result = fromBase64(ch);

	if (result > 63)
	{
		throw schema_exception({ "invalid character in base64 encoded string" });
	}

	return result;
}

std::vector<uint8_t> Base64::fromBase64(const char* encoded, size_t count)
{
	std::vector<uint8_t> result;

	if (!count)
	{
		return result;
	}

	result.reserve((count + (count % 4)) * 3 / 4);

	// First decode all of the full unpadded segments 24 bits at a time
	while (count >= 4
		&& encoded[3] != padding)
	{
		const uint32_t segment = (static_cast<uint32_t>(verifyFromBase64(encoded[0])) << 18)
			| (static_cast<uint32_t>(verifyFromBase64(encoded[1])) << 12)
			| (static_cast<uint32_t>(verifyFromBase64(encoded[2])) << 6)
			| static_cast<uint32_t>(verifyFromBase64(encoded[3]));

		result.emplace_back(static_cast<uint8_t>((segment & 0xFF0000) >> 16));
		result.emplace_back(static_cast<uint8_t>((segment & 0xFF00) >> 8));
		result.emplace_back(static_cast<uint8_t>(segment & 0xFF));

		encoded += 4;
		count -= 4;
	}

	// Get any leftover partial segment with 2 or 3 non-padding characters
	if (count > 1)
	{
		const bool triplet = (count > 2 && padding != encoded[2]);
		const uint8_t tail = (triplet ? verifyFromBase64(encoded[2]) : 0);
		const uint16_t segment = (static_cast<uint16_t>(verifyFromBase64(encoded[0])) << 10)
			| (static_cast<uint16_t>(verifyFromBase64(encoded[1])) << 4)
			| (static_cast<uint16_t>(tail) >> 2);

		if (triplet)
		{
			if (tail & 0x3)
			{
				throw schema_exception({ "invalid padding at the end of a base64 encoded string" });
			}

			result.emplace_back(static_cast<uint8_t>((segment & 0xFF00) >> 8));
			result.emplace_back(static_cast<uint8_t>(segment & 0xFF));

			encoded += 3;
			count -= 3;
		}
		else
		{
			if (segment & 0xFF)
			{
				throw schema_exception({ "invalid padding at the end of a base64 encoded string" });
			}

			result.emplace_back(static_cast<uint8_t>((segment & 0xFF00) >> 8));

			encoded += 2;
			count -= 2;
		}
	}

	// Make sure anything that's left is 0 - 2 characters of padding
	if ((count > 0 && padding != encoded[0])
		|| (count > 1 && padding != encoded[1])
		|| count > 2)
	{
		throw schema_exception({ "invalid padding at the end of a base64 encoded string" });
	}

	return result;
}

char Base64::verifyToBase64(uint8_t i)
{
	unsigned char result = toBase64(i);

	if (result == padding)
	{
		throw schema_exception({ "invalid 6-bit value" });
	}

	return result;
}

std::string Base64::toBase64(const std::vector<uint8_t> & bytes)
{
	std::string result;

	if (bytes.empty())
	{
		return result;
	}

	size_t count = bytes.size();
	const uint8_t* data = bytes.data();

	result.reserve((count + (count % 3)) * 4 / 3);

	// First encode all of the full unpadded segments 24 bits at a time
	while (count >= 3)
	{
		const uint32_t segment = (static_cast<uint32_t>(data[0]) << 16)
			| (static_cast<uint32_t>(data[1]) << 8)
			| static_cast<uint32_t>(data[2]);

		result.append({
			verifyToBase64(static_cast<uint8_t>((segment & 0xFC0000) >> 18)),
			verifyToBase64(static_cast<uint8_t>((segment & 0x3F000) >> 12)),
			verifyToBase64(static_cast<uint8_t>((segment & 0xFC0) >> 6)),
			verifyToBase64(static_cast<uint8_t>(segment & 0x3F))
			});

		data += 3;
		count -= 3;
	}

	// Get any leftover partial segment with 1 or 2 bytes
	if (count > 0)
	{
		const bool pair = (count > 1);
		const uint16_t segment = (static_cast<uint16_t>(data[0]) << 8)
			| (pair ? static_cast<uint16_t>(data[1]) : 0);
		const std::array<char, 4> remainder {
			verifyToBase64(static_cast<uint8_t>((segment & 0xFC00) >> 10)),
			verifyToBase64(static_cast<uint8_t>((segment & 0x3F0) >> 4)),
			(pair ? verifyToBase64(static_cast<uint8_t>((segment & 0xF) << 2)) : padding),
			padding
		};

		result.append(remainder.data(), remainder.size());
	}

	return result;
}

template <>
response::IntType ModifiedArgument<response::IntType>::convert(const response::Value & value)
{
	if (value.type() != response::Type::Int)
	{
		throw schema_exception({ "not an integer" });
	}

	return value.get<response::IntType>();
}

template <>
response::FloatType ModifiedArgument<response::FloatType>::convert(const response::Value & value)
{
	if (value.type() != response::Type::Float)
	{
		throw schema_exception({ "not a float" });
	}

	return value.get<response::FloatType>();
}

template <>
response::StringType ModifiedArgument<response::StringType>::convert(const response::Value & value)
{
	if (value.type() != response::Type::String)
	{
		throw schema_exception({ "not a string" });
	}

	return value.get<const response::StringType&>();
}

template <>
response::BooleanType ModifiedArgument<response::BooleanType>::convert(const response::Value & value)
{
	if (value.type() != response::Type::Boolean)
	{
		throw schema_exception({ "not a boolean" });
	}

	return value.get<response::BooleanType>();
}

template <>
response::Value ModifiedArgument<response::Value>::convert(const response::Value & value)
{
	if (value.type() != response::Type::Map)
	{
		throw schema_exception({ "not an object" });
	}

	return response::Value(value);
}

template <>
std::vector<uint8_t> ModifiedArgument<std::vector<uint8_t>>::convert(const response::Value & value)
{
	if (value.type() != response::Type::String)
	{
		throw schema_exception({ "not a string" });
	}

	const auto& encoded = value.get<const response::StringType&>();

	return Base64::fromBase64(encoded.c_str(), encoded.size());
}

template <>
std::future<response::Value> ModifiedResult<response::IntType>::convert(std::future<response::IntType> && result, ResolverParams && params)
{
	return resolve(std::move(result), std::move(params),
		[](response::IntType && value, const ResolverParams&)
		{
			return response::Value(value);
		});
}

template <>
std::future<response::Value> ModifiedResult<response::FloatType>::convert(std::future<response::FloatType> && result, ResolverParams && params)
{
	return resolve(std::move(result), std::move(params),
		[](response::FloatType && value, const ResolverParams&)
		{
			return response::Value(value);
		});
}

template <>
std::future<response::Value> ModifiedResult<response::StringType>::convert(std::future<response::StringType> && result, ResolverParams && params)
{
	return resolve(std::move(result), std::move(params),
		[](response::StringType && value, const ResolverParams&)
		{
			return response::Value(std::move(value));
		});
}

template <>
std::future<response::Value> ModifiedResult<response::BooleanType>::convert(std::future<response::BooleanType> && result, ResolverParams && params)
{
	return resolve(std::move(result), std::move(params),
		[](response::BooleanType && value, const ResolverParams&)
		{
			return response::Value(value);
		});
}

template <>
std::future<response::Value> ModifiedResult<response::Value>::convert(std::future<response::Value> && result, ResolverParams && params)
{
	return resolve(std::move(result), std::move(params),
		[](response::Value && value, const ResolverParams&)
		{
			return response::Value(std::move(value));
		});
}

template <>
std::future<response::Value> ModifiedResult<std::vector<uint8_t>>::convert(std::future<std::vector<uint8_t>> && result, ResolverParams && params)
{
	return resolve(std::move(result), std::move(params),
		[](std::vector<uint8_t> && value, const ResolverParams&)
		{
			return response::Value(Base64::toBase64(value));
		});
}

template <>
std::future<response::Value> ModifiedResult<Object>::convert(std::future<std::shared_ptr<Object>> result, ResolverParams && params)
{
	return std::async(std::launch::deferred,
		[](std::future<std::shared_ptr<Object>> && resultFuture, ResolverParams && paramsFuture)
		{
			auto wrappedResult = resultFuture.get();

			if (!wrappedResult || !paramsFuture.selection)
			{
				response::Value document(response::Type::Map);

				document.emplace_back(strData, response::Value(!wrappedResult
					? response::Type::Null
					: response::Type::Map));

				return document;
			}

			return wrappedResult->resolve(paramsFuture, *paramsFuture.selection, paramsFuture.fragments, paramsFuture.variables).get();
		}, std::move(result), std::move(params));
}

// As we recursively expand fragment spreads and inline fragments, we want to accumulate the directives
// at each location and merge them with any directives included in outer fragments to build the complete
// set of directives for nested fragments. Directives with the same name at the same location will be
// overwritten by the innermost fragment.
struct FragmentDirectives
{
	response::Value fragmentDefinitionDirectives;
	response::Value fragmentSpreadDirectives;
	response::Value inlineFragmentDirectives;
};

// SelectionVisitor visits the AST and resolves a field or fragment, unless it's skipped by
// a directive or type condition.
class SelectionVisitor
{
public:
	explicit SelectionVisitor(const SelectionSetParams& selectionSetParams, const FragmentMap& fragments, const response::Value& variables,
		const TypeNames& typeNames, const ResolverMap& resolvers);

	void visit(const peg::ast_node& selection);

	std::queue<std::pair<std::string, std::future<response::Value>>> getValues();

private:
	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	const std::shared_ptr<RequestState>& _state;
	const response::Value& _operationDirectives;
	const FragmentMap& _fragments;
	const response::Value& _variables;
	const TypeNames& _typeNames;
	const ResolverMap& _resolvers;

	std::stack<FragmentDirectives> _fragmentDirectives;
	std::queue<std::pair<std::string, std::future<response::Value>>> _values;
};

SelectionVisitor::SelectionVisitor(const SelectionSetParams & selectionSetParams, const FragmentMap & fragments, const response::Value & variables,
	const TypeNames & typeNames, const ResolverMap & resolvers)
	: _state(selectionSetParams.state)
	, _operationDirectives(selectionSetParams.operationDirectives)
	, _fragments(fragments)
	, _variables(variables)
	, _typeNames(typeNames)
	, _resolvers(resolvers)
{
	_fragmentDirectives.push({
		response::Value(response::Type::Map),
		response::Value(response::Type::Map),
		response::Value(response::Type::Map)
		});
}

std::queue<std::pair<std::string, std::future<response::Value>>> SelectionVisitor::getValues()
{
	auto values = std::move(_values);

	return values;
}

void SelectionVisitor::visit(const peg::ast_node & selection)
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

void SelectionVisitor::visitField(const peg::ast_node & field)
{
	std::string name;

	peg::on_first_child<peg::field_name>(field,
		[&name](const peg::ast_node & child)
		{
			name = child.content();
		});

	std::string alias;

	peg::on_first_child<peg::alias_name>(field,
		[&alias](const peg::ast_node & child)
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

	DirectiveVisitor directiveVisitor(_variables);

	peg::on_first_child<peg::directives>(field,
		[&directiveVisitor](const peg::ast_node & child)
		{
			directiveVisitor.visit(child);
		});

	if (directiveVisitor.shouldSkip())
	{
		return;
	}

	response::Value arguments(response::Type::Map);

	peg::on_first_child<peg::arguments>(field,
		[this, &arguments](const peg::ast_node & child)
		{
			ValueVisitor visitor(_variables);

			for (auto& argument : child.children)
			{
				visitor.visit(*argument->children.back());

				arguments.emplace_back(argument->children.front()->content(), visitor.getValue());
			}
		});

	const peg::ast_node* selection = nullptr;

	peg::on_first_child<peg::selection_set>(field,
		[&selection](const peg::ast_node & child)
		{
			selection = &child;
		});

	SelectionSetParams selectionSetParams {
		_state,
		_operationDirectives,
		_fragmentDirectives.top().fragmentDefinitionDirectives,
		_fragmentDirectives.top().fragmentSpreadDirectives,
		_fragmentDirectives.top().inlineFragmentDirectives
	};

	auto result = itr->second(ResolverParams(selectionSetParams, std::string(alias), std::move(arguments), directiveVisitor.getDirectives(), selection, _fragments, _variables));

	_values.push({
		std::move(alias),
		std::move(result)
		});
}

void SelectionVisitor::visitFragmentSpread(const peg::ast_node & fragmentSpread)
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
	DirectiveVisitor directiveVisitor(_variables);

	if (!skip)
	{
		peg::on_first_child<peg::directives>(fragmentSpread,
			[&directiveVisitor](const peg::ast_node & child)
			{

				directiveVisitor.visit(child);
			});

		skip = directiveVisitor.shouldSkip();
	}

	if (skip)
	{
		return;
	}

	auto fragmentSpreadDirectives = directiveVisitor.getDirectives();

	// Merge outer fragment spread directives as long as they don't conflict.
	for (const auto& entry : _fragmentDirectives.top().fragmentSpreadDirectives)
	{
		if (fragmentSpreadDirectives.find(entry.first) == fragmentSpreadDirectives.end())
		{
			fragmentSpreadDirectives.emplace_back(std::string(entry.first), response::Value(entry.second));
		}
	}

	response::Value fragmentDefinitionDirectives(itr->second.getDirectives());

	// Merge outer fragment definition directives as long as they don't conflict.
	for (const auto& entry : _fragmentDirectives.top().fragmentDefinitionDirectives)
	{
		if (fragmentDefinitionDirectives.find(entry.first) == fragmentDefinitionDirectives.end())
		{
			fragmentDefinitionDirectives.emplace_back(std::string(entry.first), response::Value(entry.second));
		}
	}

	_fragmentDirectives.push({
		std::move(fragmentDefinitionDirectives),
		std::move(fragmentSpreadDirectives),
		response::Value(_fragmentDirectives.top().inlineFragmentDirectives)
		});

	for (const auto& selection : itr->second.getSelection().children)
	{
		visit(*selection);
	}

	_fragmentDirectives.pop();
}

void SelectionVisitor::visitInlineFragment(const peg::ast_node & inlineFragment)
{
	DirectiveVisitor directiveVisitor(_variables);

	peg::on_first_child<peg::directives>(inlineFragment,
		[&directiveVisitor](const peg::ast_node & child)
		{
			directiveVisitor.visit(child);
		});

	if (directiveVisitor.shouldSkip())
	{
		return;
	}

	const peg::ast_node* typeCondition = nullptr;

	peg::on_first_child<peg::type_condition>(inlineFragment,
		[&typeCondition](const peg::ast_node & child)
		{
			typeCondition = &child;
		});

	if (typeCondition == nullptr
		|| _typeNames.count(typeCondition->children.front()->content()) > 0)
	{
		peg::on_first_child<peg::selection_set>(inlineFragment,
			[this, &directiveVisitor](const peg::ast_node & child)
			{
				auto inlineFragmentDirectives = directiveVisitor.getDirectives();

				// Merge outer inline fragment directives as long as they don't conflict.
				for (const auto& entry : _fragmentDirectives.top().inlineFragmentDirectives)
				{
					if (inlineFragmentDirectives.find(entry.first) == inlineFragmentDirectives.end())
					{
						inlineFragmentDirectives.emplace_back(std::string(entry.first), response::Value(entry.second));
					}
				}

				_fragmentDirectives.push({
					response::Value(_fragmentDirectives.top().fragmentDefinitionDirectives),
					response::Value(_fragmentDirectives.top().fragmentSpreadDirectives),
					std::move(inlineFragmentDirectives)
					});

				for (const auto& selection : child.children)
				{
					visit(*selection);
				}

				_fragmentDirectives.pop();
			});
	}
}

Object::Object(TypeNames && typeNames, ResolverMap && resolvers)
	: _typeNames(std::move(typeNames))
	, _resolvers(std::move(resolvers))
{
}

std::future<response::Value> Object::resolve(const SelectionSetParams & selectionSetParams, const peg::ast_node & selection, const FragmentMap & fragments, const response::Value & variables) const
{
	std::queue<std::pair<std::string, std::future<response::Value>>> selections;

	beginSelectionSet(selectionSetParams);

	for (const auto& child : selection.children)
	{
		SelectionVisitor visitor(selectionSetParams, fragments, variables, _typeNames, _resolvers);

		visitor.visit(*child);

		auto values = visitor.getValues();

		while (!values.empty())
		{
			selections.push(std::move(values.front()));
			values.pop();
		}
	}

	endSelectionSet(selectionSetParams);

	return std::async(std::launch::deferred,
		[](std::queue<std::pair<std::string, std::future<response::Value>>> && children)
		{
			response::Value data(response::Type::Map);
			response::Value errors(response::Type::List);

			while (!children.empty())
			{
				auto name = std::move(children.front().first);

				try
				{
					auto value = children.front().second.get();
					auto members = value.release<response::MapType>();

					for (auto& entry : members)
					{
						if (entry.second.type() == response::Type::List
							&& entry.first == strErrors)
						{
							auto errorEntries = entry.second.release<response::ListType>();

							for (auto& errorEntry : errorEntries)
							{
								errors.emplace_back(std::move(errorEntry));
							}
						}
						else if (entry.first == strData)
						{
							if (data.find(name) != data.end())
							{
								std::ostringstream message;

								message << "Field error name: " << name
									<< " error: duplicate field";

								errors.emplace_back(response::Value(message.str()));
							}
							else
							{
								data.emplace_back(std::move(name), std::move(entry.second));
							}
						}
					}
				}
				catch (const std::exception & ex)
				{
					std::ostringstream message;

					message << "Field error name: " << name
						<< " unknown error: " << ex.what();

					errors.emplace_back(response::Value(message.str()));
				}

				children.pop();
			}

			response::Value result(response::Type::Map);

			result.emplace_back(strData, std::move(data));

			if (errors.size() > 0)
			{
				result.emplace_back(strErrors, std::move(errors));
			}

			return result;
		}, std::move(selections));
}

bool Object::matchesType(const std::string & typeName) const
{
	return _typeNames.find(typeName) != _typeNames.cend();
}

void Object::beginSelectionSet(const SelectionSetParams &) const
{
}

void Object::endSelectionSet(const SelectionSetParams &) const
{
}

OperationData::OperationData(std::shared_ptr<RequestState> && state, response::Value && variables,
	response::Value && directives, FragmentMap && fragments)
	: state(std::move(state))
	, variables(std::move(variables))
	, directives(std::move(directives))
	, fragments(std::move(fragments))
{
}

// FragmentDefinitionVisitor visits the AST and collects all of the fragment
// definitions in the document.
class FragmentDefinitionVisitor
{
public:
	FragmentDefinitionVisitor(const response::Value& variables);

	FragmentMap getFragments();

	void visit(const peg::ast_node& fragmentDefinition);

private:
	const response::Value& _variables;

	FragmentMap _fragments;
};

FragmentDefinitionVisitor::FragmentDefinitionVisitor(const response::Value & variables)
	: _variables(variables)
{
}

FragmentMap FragmentDefinitionVisitor::getFragments()
{
	FragmentMap result(std::move(_fragments));
	return result;
}

void FragmentDefinitionVisitor::visit(const peg::ast_node & fragmentDefinition)
{
	_fragments.insert({ fragmentDefinition.children.front()->content(), Fragment(fragmentDefinition, _variables) });
}

// OperationDefinitionVisitor visits the AST and executes the one with the specified
// operation name.
class OperationDefinitionVisitor
{
public:
	OperationDefinitionVisitor(std::shared_ptr<RequestState> state, const TypeMap& operations, response::Value&& variables, FragmentMap&& fragments);

	std::future<response::Value> getValue();

	void visit(const std::string& operationType, const peg::ast_node& operationDefinition);

private:
	std::shared_ptr<OperationData> _params;
	const TypeMap& _operations;
	std::future<response::Value> _result;
};

OperationDefinitionVisitor::OperationDefinitionVisitor(std::shared_ptr<RequestState> state, const TypeMap & operations, response::Value && variables, FragmentMap && fragments)
	: _params(std::make_shared<OperationData>(
		std::move(state),
		std::move(variables),
		response::Value(),
		std::move(fragments)))
	, _operations(operations)
{
}

std::future<response::Value> OperationDefinitionVisitor::getValue()
{
	auto result = std::move(_result);

	return result;
}

void OperationDefinitionVisitor::visit(const std::string & operationType, const peg::ast_node & operationDefinition)
{
	auto itr = _operations.find(operationType);

	// Filter the variable definitions down to the ones referenced in this operation
	response::Value operationVariables(response::Type::Map);

	peg::for_each_child<peg::variable>(operationDefinition,
		[this, &operationVariables](const peg::ast_node & variable)
		{
			std::string variableName;

			peg::on_first_child<peg::variable_name>(variable,
				[&variableName](const peg::ast_node & name)
				{
					// Skip the $ prefix
					variableName = name.content().c_str() + 1;
				});

			auto itrVar = _params->variables.find(variableName);
			response::Value valueVar;

			if (itrVar != _params->variables.get<const response::MapType&>().cend())
			{
				valueVar = response::Value(itrVar->second);
			}
			else
			{
				peg::on_first_child<peg::default_value>(variable,
					[this, &valueVar](const peg::ast_node & defaultValue)
					{
						ValueVisitor visitor(_params->variables);

						visitor.visit(*defaultValue.children.front());
						valueVar = visitor.getValue();
					});
			}

			operationVariables.emplace_back(std::move(variableName), std::move(valueVar));
		});

	_params->variables = std::move(operationVariables);

	response::Value operationDirectives(response::Type::Map);

	peg::on_first_child<peg::directives>(operationDefinition,
		[this, &operationDirectives](const peg::ast_node & child)
		{
			DirectiveVisitor directiveVisitor(_params->variables);

			directiveVisitor.visit(child);
			operationDirectives = directiveVisitor.getDirectives();
		});

	_params->directives = std::move(operationDirectives);

	// Keep the params alive until the deferred lambda has executed
	auto params = std::move(_params);

	// The top level object doesn't come from inside of a fragment, so all of the fragment directives are empty.
	response::Value emptyFragmentDirectives(response::Type::Map);
	const SelectionSetParams selectionSetParams {
		params->state,
		params->directives,
		emptyFragmentDirectives,
		emptyFragmentDirectives,
		emptyFragmentDirectives
	};

	_result = std::async(std::launch::deferred,
		[params](std::future<response::Value> document)
		{
			return document.get();
		}, itr->second->resolve(selectionSetParams, *operationDefinition.children.back(), params->fragments, params->variables));
}

SubscriptionData::SubscriptionData(std::shared_ptr<OperationData> && data, std::unordered_map<SubscriptionName, std::vector<response::Value>> && fieldNamesAndArgs,
	peg::ast<std::string> && query, std::string && operationName, SubscriptionCallback && callback,
	const peg::ast_node & selection)
	: data(std::move(data))
	, fieldNamesAndArgs(std::move(fieldNamesAndArgs))
	, query(std::move(query))
	, operationName(std::move(operationName))
	, callback(std::move(callback))
	, selection(selection)
{
}

// SubscriptionDefinitionVisitor visits the AST collects the fields referenced in the subscription at the point
// where we create a subscription.
class SubscriptionDefinitionVisitor
{
public:
	SubscriptionDefinitionVisitor(SubscriptionParams&& params, SubscriptionCallback&& callback, FragmentMap&& fragments, const std::shared_ptr<Object>& subscriptionObject);

	const peg::ast_node& getRoot() const;
	std::shared_ptr<SubscriptionData> getRegistration();

	void visit(const peg::ast_node& operationDefinition);

private:
	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	SubscriptionParams _params;
	SubscriptionCallback _callback;
	FragmentMap _fragments;
	const std::shared_ptr<Object>& _subscriptionObject;
	std::unordered_map<SubscriptionName, std::vector<response::Value>> _fieldNamesAndArgs;
	std::shared_ptr<SubscriptionData> _result;
};

SubscriptionDefinitionVisitor::SubscriptionDefinitionVisitor(SubscriptionParams && params, SubscriptionCallback && callback, FragmentMap && fragments, const std::shared_ptr<Object> & subscriptionObject)
	: _params(std::move(params))
	, _callback(std::move(callback))
	, _fragments(std::move(fragments))
	, _subscriptionObject(subscriptionObject)
{
}

const peg::ast_node& SubscriptionDefinitionVisitor::getRoot() const
{
	return *_params.query.root;
}

std::shared_ptr<SubscriptionData> SubscriptionDefinitionVisitor::getRegistration()
{
	auto result = std::move(_result);

	_result.reset();

	return result;
}

void SubscriptionDefinitionVisitor::visit(const peg::ast_node & operationDefinition)
{
	const auto& selection = *operationDefinition.children.back();

	for (const auto& child : selection.children)
	{
		if (child->is<peg::field>())
		{
			visitField(*child);
		}
		else if (child->is<peg::fragment_spread>())
		{
			visitFragmentSpread(*child);
		}
		else if (child->is<peg::inline_fragment>())
		{
			visitInlineFragment(*child);
		}
	}

	response::Value directives(response::Type::Map);

	peg::on_first_child<peg::directives>(operationDefinition,
		[this, &directives](const peg::ast_node & child)
		{
			DirectiveVisitor directiveVisitor(_params.variables);

			directiveVisitor.visit(child);
			directives = directiveVisitor.getDirectives();
		});

	_result = std::make_shared<SubscriptionData>(
		std::make_shared<OperationData>(
			std::move(_params.state),
			std::move(_params.variables),
			std::move(directives),
			std::move(_fragments)),
		std::move(_fieldNamesAndArgs),
		std::move(_params.query),
		std::move(_params.operationName),
		std::move(_callback),
		selection);
}

void SubscriptionDefinitionVisitor::visitField(const peg::ast_node & field)
{
	std::string name;

	peg::on_first_child<peg::field_name>(field,
		[&name](const peg::ast_node & child)
		{
			name = child.content();
		});

	DirectiveVisitor directiveVisitor(_params.variables);

	peg::on_first_child<peg::directives>(field,
		[&directiveVisitor](const peg::ast_node & child)
		{
			directiveVisitor.visit(child);
		});

	if (directiveVisitor.shouldSkip())
	{
		return;
	}

	response::Value arguments(response::Type::Map);

	peg::on_first_child<peg::arguments>(field,
		[this, &arguments](const peg::ast_node & child)
		{
			ValueVisitor visitor(_params.variables);

			for (auto& argument : child.children)
			{
				visitor.visit(*argument->children.back());

				arguments.emplace_back(argument->children.front()->content(), visitor.getValue());
			}
		});

	_fieldNamesAndArgs[std::move(name)].emplace_back(std::move(arguments));
}

void SubscriptionDefinitionVisitor::visitFragmentSpread(const peg::ast_node & fragmentSpread)
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

	bool skip = !_subscriptionObject->matchesType(itr->second.getType());
	DirectiveVisitor directiveVisitor(_params.variables);

	if (!skip)
	{
		peg::on_first_child<peg::directives>(fragmentSpread,
			[&directiveVisitor](const peg::ast_node & child)
			{
				directiveVisitor.visit(child);
			});

		skip = directiveVisitor.shouldSkip();
	}

	if (skip)
	{
		return;
	}

	for (const auto& selection : itr->second.getSelection().children)
	{
		visit(*selection);
	}
}

void SubscriptionDefinitionVisitor::visitInlineFragment(const peg::ast_node & inlineFragment)
{
	DirectiveVisitor directiveVisitor(_params.variables);

	peg::on_first_child<peg::directives>(inlineFragment,
		[&directiveVisitor](const peg::ast_node & child)
		{
			directiveVisitor.visit(child);
		});

	if (directiveVisitor.shouldSkip())
	{
		return;
	}

	const peg::ast_node* typeCondition = nullptr;

	peg::on_first_child<peg::type_condition>(inlineFragment,
		[&typeCondition](const peg::ast_node & child)
		{
			typeCondition = &child;
		});

	if (typeCondition == nullptr
		|| _subscriptionObject->matchesType(typeCondition->children.front()->content()))
	{
		peg::on_first_child<peg::selection_set>(inlineFragment,
			[this](const peg::ast_node & child)
			{
				for (const auto& selection : child.children)
				{
					visit(*selection);
				}
			});
	}
}

Request::Request(TypeMap && operationTypes)
	: _operations(std::move(operationTypes))
{
}

std::pair<std::string, const peg::ast_node*> Request::findOperationDefinition(const peg::ast_node& root, const std::string& operationName) const
{
	std::pair<std::string, const peg::ast_node*> result = { {}, nullptr };

	peg::for_each_child<peg::operation_definition>(root,
		[this, &operationName, &result](const peg::ast_node & operationDefinition)
		{
			std::string operationType(strQuery);

			peg::on_first_child<peg::operation_type>(operationDefinition,
				[&operationType](const peg::ast_node & child)
				{
					operationType = child.content();
				});

			std::string name;

			peg::on_first_child<peg::operation_name>(operationDefinition,
				[&name](const peg::ast_node & child)
				{
					name = child.content();
				});

			if (!operationName.empty()
				&& name != operationName)
			{
				// Skip the operations that don't match the name
				return;
			}

			std::vector<std::string> errors;
			auto position = operationDefinition.begin();

			if (result.second)
			{
				std::ostringstream message;

				message << operationName.empty()
					? "Multiple ambigious operations"
					: "Duplicate named operations";

				if (!name.empty())
				{
					message << " name: " << name;
				}

				message << " line: " << position.line
					<< " column: " << position.byte_in_line;

				errors.push_back(message.str());
			}

			auto itr = _operations.find(operationType);

			if (itr == _operations.cend())
			{
				std::ostringstream message;

				message << "Unsupported operation type: " << operationType;

				if (!name.empty())
				{
					message << " name: " << name;
				}

				message << " line: " << position.line
					<< " column: " << position.byte_in_line;

				errors.push_back(message.str());
			}

			if (!errors.empty())
			{
				throw schema_exception(std::move(errors));
			}

			result = { std::move(operationType), &operationDefinition };
		});

	return result;
}

std::future<response::Value> Request::resolve(const std::shared_ptr<RequestState> & state, const peg::ast_node & root, const std::string & operationName, response::Value && variables) const
{
	FragmentDefinitionVisitor fragmentVisitor(variables);

	peg::for_each_child<peg::fragment_definition>(root,
		[&fragmentVisitor](const peg::ast_node & child)
		{
			fragmentVisitor.visit(child);
		});

	auto fragments = fragmentVisitor.getFragments();

	try
	{
		auto operationDefinition = findOperationDefinition(root, operationName);

		if (!operationDefinition.second)
		{
			std::ostringstream message;

			message << "Missing operation";

			if (!operationName.empty())
			{
				message << " name: " << operationName;
			}

			throw schema_exception({ message.str() });
		}
		else if (operationDefinition.first == strSubscription)
		{
			std::ostringstream message;

			message << "Unexpected subscription";

			if (!operationName.empty())
			{
				message << " name: " << operationName;
			}

			throw schema_exception({ message.str() });
		}

		OperationDefinitionVisitor operationVisitor(state, _operations, std::move(variables), std::move(fragments));

		operationVisitor.visit(operationDefinition.first, *operationDefinition.second);

		return operationVisitor.getValue();
	}
	catch (schema_exception & ex)
	{
		std::promise<response::Value> promise;
		response::Value document(response::Type::Map);

		document.emplace_back(strData, response::Value());
		document.emplace_back(strErrors, ex.getErrors());
		promise.set_value(std::move(document));

		return promise.get_future();
	}
}

SubscriptionKey Request::subscribe(SubscriptionParams && params, SubscriptionCallback && callback)
{
	FragmentDefinitionVisitor fragmentVisitor(params.variables);

	peg::for_each_child<peg::fragment_definition>(*params.query.root,
		[&fragmentVisitor](const peg::ast_node & child)
		{
			fragmentVisitor.visit(child);
		});

	auto fragments = fragmentVisitor.getFragments();
	auto operationDefinition = findOperationDefinition(*params.query.root, params.operationName);

	if (!operationDefinition.second)
	{
		std::ostringstream message;

		message << "Missing subscription";

		if (!params.operationName.empty())
		{
			message << " name: " << params.operationName;
		}

		throw schema_exception({ message.str() });
	}
	else if (operationDefinition.first != strSubscription)
	{
		std::ostringstream message;

		message << "Unexpected operation type: " << operationDefinition.first;

		if (!params.operationName.empty())
		{
			message << " name: " << params.operationName;
		}

		throw schema_exception({ message.str() });
	}

	auto itr = _operations.find(strSubscription);
	SubscriptionDefinitionVisitor subscriptionVisitor(std::move(params), std::move(callback), std::move(fragments), itr->second);

	peg::for_each_child<peg::operation_definition>(subscriptionVisitor.getRoot(),
		[&subscriptionVisitor](const peg::ast_node & child)
		{
			subscriptionVisitor.visit(child);
		});

	auto registration = subscriptionVisitor.getRegistration();
	auto key = _nextKey++;

	for (const auto& entry : registration->fieldNamesAndArgs)
	{
		_listeners[entry.first].insert(key);
	}

	_subscriptions.emplace(key, std::move(registration));

	return key;
}

void Request::unsubscribe(SubscriptionKey key)
{
	auto itrSubscription = _subscriptions.find(key);

	if (itrSubscription == _subscriptions.cend())
	{
		return;
	}

	for (const auto& entry : itrSubscription->second->fieldNamesAndArgs)
	{
		auto itrListener = _listeners.find(entry.first);

		itrListener->second.erase(key);
		if (itrListener->second.empty())
		{
			_listeners.erase(itrListener);
		}
	}

	_subscriptions.erase(itrSubscription);

	if (_subscriptions.empty())
	{
		_nextKey = 0;
	}
	else
	{
		_nextKey = _subscriptions.crbegin()->first + 1;
	}
}

void Request::deliver(const SubscriptionName & name, const std::shared_ptr<Object> & subscriptionObject) const
{
	deliver(name, SubscriptionArguments {}, subscriptionObject);
}

void Request::deliver(const SubscriptionName & name, const SubscriptionArguments & arguments, const std::shared_ptr<Object> & subscriptionObject) const
{
	SubscriptionFilterCallback exactMatch = [&arguments](response::MapType::const_reference required) noexcept -> bool
	{
		auto itrArgument = arguments.find(required.first);

		return (itrArgument != arguments.cend()
			&& itrArgument->second == required.second);
	};

	deliver(name, exactMatch, subscriptionObject);
}

void Request::deliver(const SubscriptionName & name, const SubscriptionFilterCallback & apply, const std::shared_ptr<Object> & subscriptionObject) const
{
	const auto& optionalOrDefaultSubscription = subscriptionObject
		? subscriptionObject
		: _operations.find(strSubscription)->second;

	auto itrListeners = _listeners.find(name);

	if (itrListeners == _listeners.cend())
	{
		return;
	}

	for (const auto& key : itrListeners->second)
	{
		auto itrSubscription = _subscriptions.find(key);
		auto registration = itrSubscription->second;
		const auto& subscriptionArguments = registration->fieldNamesAndArgs[name];
		bool matchedArguments = false;

		// If none of the fields in this subscription had arguments that match what was provided
		// in this event, don't deliver the event to this subscription
		for (const auto& required : subscriptionArguments)
		{
			matchedArguments = true;

			for (auto itrRequired = required.begin(); itrRequired != required.end(); ++itrRequired)
			{
				if (!apply(*itrRequired))
				{
					matchedArguments = false;
					break;
				}
			}

			if (matchedArguments)
			{
				break;
			}
		}

		if (!matchedArguments)
		{
			continue;
		}

		std::future<response::Value> result;
		response::Value emptyFragmentDirectives(response::Type::Map);
		const SelectionSetParams selectionSetParams {
			registration->data->state,
			registration->data->directives,
			emptyFragmentDirectives,
			emptyFragmentDirectives,
			emptyFragmentDirectives
		};

		try
		{
			result = std::async(std::launch::deferred,
				[registration](std::future<response::Value> document)
				{
					return document.get();
				}, optionalOrDefaultSubscription->resolve(selectionSetParams, registration->selection, registration->data->fragments, registration->data->variables));
		}
		catch (schema_exception & ex)
		{
			std::promise<response::Value> promise;
			response::Value document(response::Type::Map);

			document.emplace_back(strData, response::Value());
			document.emplace_back(strErrors, ex.getErrors());

			result = promise.get_future();
		}

		registration->callback(std::move(result));
	}
}

} /* namespace service */
} /* namespace graphql */
} /* namespace facebook */
