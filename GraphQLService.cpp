// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphQLService.h"
#include "GraphQLGrammar.h"

#include <iostream>
#include <algorithm>
#include <array>

namespace facebook {
namespace graphql {
namespace service {

schema_exception::schema_exception(std::vector<std::string>&& messages)
	: _errors(response::Value::Type::List)
{
	for (auto& message : messages)
	{
		response::Value error(response::Value::Type::Map);

		error.emplace_back("message", response::Value(std::move(message)));
		_errors.emplace_back(std::move(error));
	}

	messages.clear();
}

const response::Value& schema_exception::getErrors() const noexcept
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

std::string Base64::toBase64(const std::vector<uint8_t>& bytes)
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
			verifyToBase64((segment & 0xFC0000) >> 18),
			verifyToBase64((segment & 0x3F000) >> 12),
			verifyToBase64((segment & 0xFC0) >> 6),
			verifyToBase64(segment & 0x3F)
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
			verifyToBase64((segment & 0xFC00) >> 10),
			verifyToBase64((segment & 0x3F0) >> 4),
			(pair ? verifyToBase64((segment & 0xF) << 2) : padding),
			padding
		};

		result.append(remainder.data(), remainder.size());
	}

	return result;
}

template <>
response::Value::IntType ModifiedArgument<response::Value::IntType>::convert(const response::Value& value)
{
	if (value.type() != response::Value::Type::Int)
	{
		throw schema_exception({ "not an integer" });
	}

	return value.get<response::Value::IntType>();
}

template <>
response::Value::FloatType ModifiedArgument<response::Value::FloatType>::convert(const response::Value& value)
{
	if (value.type() != response::Value::Type::Float)
	{
		throw schema_exception({ "not a float" });
	}

	return value.get<response::Value::FloatType>();
}

template <>
response::Value::StringType ModifiedArgument<response::Value::StringType>::convert(const response::Value& value)
{
	if (value.type() != response::Value::Type::String)
	{
		throw schema_exception({ "not a string" });
	}

	return value.get<const response::Value::StringType&>();
}

template <>
response::Value::BooleanType ModifiedArgument<response::Value::BooleanType>::convert(const response::Value& value)
{
	if (value.type() != response::Value::Type::Boolean)
	{
		throw schema_exception({ "not a boolean" });
	}

	return value.get<response::Value::BooleanType>();
}

template <>
response::Value ModifiedArgument<response::Value>::convert(const response::Value& value)
{
	if (value.type() != response::Value::Type::Map)
	{
		throw schema_exception({ "not an object" });
	}

	return response::Value(value);
}

template <>
std::vector<uint8_t> ModifiedArgument<std::vector<uint8_t>>::convert(const response::Value& value)
{
	if (value.type() != response::Value::Type::String)
	{
		throw schema_exception({ "not a string" });
	}

	const auto& encoded = value.get<const response::Value::StringType&>();

	return Base64::fromBase64(encoded.c_str(), encoded.size());
}

template <>
std::future<response::Value> ModifiedResult<response::Value::IntType>::convert(std::future<response::Value::IntType>&& result, ResolverParams&&)
{
	return std::async(std::launch::deferred,
		[](std::future<response::Value::IntType>&& resultFuture)
	{
		return response::Value(resultFuture.get());
	}, std::move(result));
}

template <>
std::future<response::Value> ModifiedResult<response::Value::FloatType>::convert(std::future<response::Value::FloatType>&& result, ResolverParams&&)
{
	return std::async(std::launch::deferred,
		[](std::future<response::Value::FloatType>&& resultFuture)
	{
		return response::Value(resultFuture.get());
	}, std::move(result));
}

template <>
std::future<response::Value> ModifiedResult<response::Value::StringType>::convert(std::future<response::Value::StringType>&& result, ResolverParams&& params)
{
	return std::async(std::launch::deferred,
		[&](std::future<response::Value::StringType>&& resultFuture, ResolverParams&& paramsFuture)
	{
		return response::Value(resultFuture.get());
	}, std::move(result), std::move(params));
}

template <>
std::future<response::Value> ModifiedResult<response::Value::BooleanType>::convert(std::future<response::Value::BooleanType>&& result, ResolverParams&&)
{
	return std::async(std::launch::deferred,
		[](std::future<response::Value::BooleanType>&& resultFuture)
	{
		return response::Value(resultFuture.get());
	}, std::move(result));
}

template <>
std::future<response::Value> ModifiedResult<response::Value>::convert(std::future<response::Value>&& result, ResolverParams&&)
{
	return std::move(result);
}

template <>
std::future<response::Value> ModifiedResult<std::vector<uint8_t>>::convert(std::future<std::vector<uint8_t>>&& result, ResolverParams&& params)
{
	return std::async(std::launch::deferred,
		[](std::future<std::vector<uint8_t>>&& resultFuture, ResolverParams&& paramsFuture)
	{
		return response::Value(Base64::toBase64(resultFuture.get()));
	}, std::move(result), std::move(params));
}

template <>
std::future<response::Value> ModifiedResult<Object>::convert(std::future<std::shared_ptr<Object>> result, ResolverParams&& params)
{
	return std::async(std::launch::deferred,
		[](std::future<std::shared_ptr<Object>>&& resultFuture, ResolverParams&& paramsFuture)
	{
		auto wrappedResult = resultFuture.get();

		if (!wrappedResult || !paramsFuture.selection)
		{
			return response::Value(!wrappedResult
				? response::Value::Type::Null
				: response::Value::Type::Map);
		}

		return wrappedResult->resolve(paramsFuture.requestId, *paramsFuture.selection, paramsFuture.fragments, paramsFuture.variables).get();
	}, std::move(result), std::move(params));
}

Object::Object(TypeNames&& typeNames, ResolverMap&& resolvers)
	: _typeNames(std::move(typeNames))
	, _resolvers(std::move(resolvers))
{
}

std::future<response::Value> Object::resolve(RequestId requestId, const peg::ast_node& selection, const FragmentMap& fragments, const response::Value& variables) const
{
	std::queue<std::future<response::Value>> selections;

	beginSelectionSet(requestId);

	for (const auto& child : selection.children)
	{
		SelectionVisitor visitor(requestId, fragments, variables, _typeNames, _resolvers);

		visitor.visit(*child);
		selections.push(visitor.getValues());
	}

	endSelectionSet(requestId);

	return std::async(std::launch::deferred,
		[](std::queue<std::future<response::Value>>&& promises)
	{
		response::Value result(response::Value::Type::Map);

		while (!promises.empty())
		{
			auto values = promises.front().get();

			promises.pop();

			if (values.type() == response::Value::Type::Map)
			{
				auto members = values.release<response::Value::MapType>();

				for (auto& entry : members)
				{
					result.emplace_back(std::move(entry.first), std::move(entry.second));
				}
			}
		}

		return result;
	}, std::move(selections));
}

void Object::beginSelectionSet(RequestId) const
{
}

void Object::endSelectionSet(RequestId) const
{
}

Request::Request(TypeMap&& operationTypes)
	: _operations(std::move(operationTypes))
{
}

std::future<response::Value> Request::resolve(RequestId requestId, const peg::ast_node& root, const std::string& operationName, const response::Value& variables) const
{
	FragmentDefinitionVisitor fragmentVisitor;

	peg::for_each_child<peg::fragment_definition>(root,
		[&fragmentVisitor](const peg::ast_node& child)
	{
		fragmentVisitor.visit(child);
	});

	auto fragments = fragmentVisitor.getFragments();
	OperationDefinitionVisitor operationVisitor(requestId, _operations, operationName, variables, fragments);

	peg::for_each_child<peg::operation_definition>(root,
		[&operationVisitor](const peg::ast_node& child)
	{
		operationVisitor.visit(child);
	});

	return operationVisitor.getValue();
}

SelectionVisitor::SelectionVisitor(RequestId requestId, const FragmentMap& fragments, const response::Value& variables, const TypeNames& typeNames, const ResolverMap& resolvers)
	: _requestId(requestId)
	, _fragments(fragments)
	, _variables(variables)
	, _typeNames(typeNames)
	, _resolvers(resolvers)
{
}

std::future<response::Value> SelectionVisitor::getValues()
{
	return std::async(std::launch::deferred,
		[](std::queue<std::pair<std::string, std::future<response::Value>>>&& values)
	{
		response::Value result(response::Value::Type::Map);

		while (!values.empty())
		{
			auto& entry = values.front();

			result.emplace_back(std::move(entry.first), entry.second.get());
			values.pop();
		}

		return result;
	}, std::move(_values));
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

	response::Value arguments(response::Value::Type::Map);

	peg::on_first_child<peg::arguments>(field,
		[this, &arguments](const peg::ast_node& child)
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
		[&selection](const peg::ast_node& child)
	{
		selection = &child;
	});

	_values.push({
		std::move(alias),
		itr->second({ _requestId, arguments, selection, _fragments, _variables })
		});
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

ValueVisitor::ValueVisitor(const response::Value& variables)
	: _variables(variables)
{
}

response::Value ValueVisitor::getValue()
{
	auto result = std::move(_value);

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
	auto itr = _variables.find(name);

	if (itr == _variables.get<const response::Value::MapType&>().cend())
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

void ValueVisitor::visitIntValue(const peg::ast_node& intValue)
{
	_value = response::Value(std::atoi(intValue.content().c_str()));
}

void ValueVisitor::visitFloatValue(const peg::ast_node& floatValue)
{
	_value = response::Value(std::atof(floatValue.content().c_str()));
}

void ValueVisitor::visitStringValue(const peg::ast_node& stringValue)
{
	_value = response::Value(std::string(stringValue.unescaped));
}

void ValueVisitor::visitBooleanValue(const peg::ast_node& booleanValue)
{
	_value = response::Value(booleanValue.is<peg::true_keyword>());
}

void ValueVisitor::visitNullValue(const peg::ast_node& /*nullValue*/)
{
	_value = {};
}

void ValueVisitor::visitEnumValue(const peg::ast_node& enumValue)
{
	_value = response::Value(enumValue.content());
}

void ValueVisitor::visitListValue(const peg::ast_node& listValue)
{
	_value = response::Value(response::Value::Type::List);
	_value.reserve(listValue.children.size());

	ValueVisitor visitor(_variables);

	for (const auto& child : listValue.children)
	{
		visitor.visit(*child->children.back());
		_value.emplace_back(visitor.getValue());
	}
}

void ValueVisitor::visitObjectValue(const peg::ast_node& objectValue)
{
	_value = response::Value(response::Value::Type::Map);
	_value.reserve(objectValue.children.size());

	ValueVisitor visitor(_variables);

	for (const auto& field : objectValue.children)
	{
		visitor.visit(*field->children.back());
		_value.emplace_back(field->children.front()->content(), visitor.getValue());
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

OperationDefinitionVisitor::OperationDefinitionVisitor(RequestId requestId, const TypeMap& operations, const std::string& operationName, const response::Value& variables, const FragmentMap& fragments)
	: _requestId(requestId)
	, _operations(operations)
	, _operationName(operationName)
	, _variables(variables)
	, _fragments(fragments)
{
}

std::future<response::Value> OperationDefinitionVisitor::getValue()
{
	auto result = std::move(_result);

	try
	{
		if (!result.valid())
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
		std::promise<response::Value> promise;
		response::Value document(response::Value::Type::Map);

		document.emplace_back("data", response::Value());
		document.emplace_back("errors", response::Value(ex.getErrors()));
		promise.set_value(std::move(document));

		result = promise.get_future();
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
		if (_result.valid())
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

		auto operationObject = itr->second;

		_result = std::async(std::launch::deferred,
			[this, &operationDefinition, operationObject]()
		{
			response::Value operationVariables(response::Value::Type::Map);

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

					auto itrVar = _variables.find(variableName);
					response::Value valueVar;

					if (itrVar != _variables.get<const response::Value::MapType&>().cend())
					{
						valueVar = response::Value(itrVar->second);
					}
					else
					{
						peg::on_first_child<peg::default_value>(variable,
							[this, &valueVar](const peg::ast_node& defaultValue)
						{
							ValueVisitor visitor(_variables);

							visitor.visit(*defaultValue.children.front());
							valueVar = visitor.getValue();
						});
					}

					operationVariables.emplace_back(std::move(variableName), std::move(valueVar));
				});
			});

			response::Value document(response::Value::Type::Map);
			auto data = operationObject->resolve(_requestId, *operationDefinition.children.back(), _fragments, operationVariables);

			document.emplace_back("data", data.get());

			return document;
		});
	}
	catch (const schema_exception& ex)
	{
		std::promise<response::Value> promise;
		response::Value document(response::Value::Type::Map);

		document.emplace_back("data", response::Value());
		document.emplace_back("errors", response::Value(ex.getErrors()));
		promise.set_value(std::move(document));

		_result = promise.get_future();
	}
}

} /* namespace service */
} /* namespace graphql */
} /* namespace facebook */