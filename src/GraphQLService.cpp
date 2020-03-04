// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <graphqlservice/GraphQLService.h>
#include <graphqlservice/GraphQLGrammar.h>
#include <graphqlservice/IntrospectionSchema.h>

#include <iostream>
#include <algorithm>
#include <array>
#include <stack>

namespace graphql::service {

void addErrorMessage(std::string&& message, response::Value& error)
{
	error.emplace_back(std::string { strMessage }, response::Value(std::move(message)));
}

void addErrorLocation(const schema_location& location, response::Value& error)
{
	if (location.line == 0)
	{
		return;
	}

	response::Value errorLocation(response::Type::Map);

	errorLocation.reserve(2);
	errorLocation.emplace_back(std::string { strLine }, response::Value(static_cast<response::IntType>(location.line)));
	errorLocation.emplace_back(std::string { strColumn }, response::Value(static_cast<response::IntType>(location.byte_in_line + 1)));

	response::Value errorLocations(response::Type::List);

	errorLocations.reserve(1);
	errorLocations.emplace_back(std::move(errorLocation));

	error.emplace_back(std::string { strLocations }, std::move(errorLocations));
}

void addErrorPath(field_path&& path, response::Value& error)
{
	if (path.empty())
	{
		return;
	}

	response::Value errorPath(response::Type::List);

	errorPath.reserve(path.size());
	while (!path.empty())
	{
		auto& segment = path.front();

		if (std::holds_alternative<std::string>(segment))
		{
			errorPath.emplace_back(response::Value(std::move(std::get<std::string>(segment))));
		}
		else if (std::holds_alternative<size_t>(segment))
		{
			errorPath.emplace_back(response::Value(static_cast<response::IntType>(std::get<size_t>(segment))));
		}

		path.pop();
	}

	error.emplace_back(std::string { strPath }, std::move(errorPath));
}

schema_exception::schema_exception(std::vector<schema_error>&& structuredErrors)
	: _structuredErrors(std::move(structuredErrors))
	, _errors(response::Type::List)
{
	for (auto error : _structuredErrors)
	{
		response::Value entry(response::Type::Map);

		entry.reserve(3);
		addErrorMessage(std::move(error.message), entry);
		addErrorLocation(error.location, entry);
		addErrorPath(std::move(error.path), entry);

		_errors.emplace_back(std::move(entry));
	}
}

schema_exception::schema_exception(std::vector<std::string>&& messages)
	: schema_exception(convertMessages(std::move(messages)))
{
}

std::vector<schema_error> schema_exception::convertMessages(std::vector<std::string>&& messages) noexcept
{
	std::vector<schema_error> errors(messages.size());

	std::transform(messages.begin(), messages.end(), errors.begin(),
		[](std::string& message) noexcept
	{
		return schema_error { std::move(message) };
	});

	return errors;
}

const char* schema_exception::what() const noexcept
{
	const char* message = nullptr;

	if (_errors.size() > 0)
	{
		auto itr = _errors[0].find("message");

		if (itr != _errors[0].end()
			&& itr->second.type() == response::Type::String)
		{
			message = itr->second.get<response::StringType>().c_str();
		}
	}

	return (message == nullptr)
		? "Unknown schema error"
		: message;
}

const std::vector<schema_error>& schema_exception::getStructuredErrors() const noexcept
{
	return _structuredErrors;
}

std::vector<schema_error> schema_exception::getStructuredErrors() noexcept
{
	auto structuredErrors = std::move(_structuredErrors);

	return structuredErrors;
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

FieldParams::FieldParams(const SelectionSetParams& selectionSetParams, response::Value&& directives)
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
	if (value.is_type<peg::variable_value>())
	{
		visitVariable(value);
	}
	else if (value.is_type<peg::integer_value>())
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
	else if (value.is_type<peg::true_keyword>()
		|| value.is_type<peg::false_keyword>())
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
}

void ValueVisitor::visitVariable(const peg::ast_node& variable)
{
	const std::string name(variable.string_view().substr(1));
	auto itr = _variables.find(name);

	if (itr == _variables.get<response::MapType>().cend())
	{
		auto position = variable.begin();
		std::ostringstream error;

		error << "Unknown variable name: " << name;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line } } } };
	}

	_value = response::Value(itr->second);
}

void ValueVisitor::visitIntValue(const peg::ast_node& intValue)
{
	_value = response::Value(std::atoi(intValue.string().c_str()));
}

void ValueVisitor::visitFloatValue(const peg::ast_node& floatValue)
{
	_value = response::Value(std::atof(floatValue.string().c_str()));
}

void ValueVisitor::visitStringValue(const peg::ast_node& stringValue)
{
	_value = response::Value(std::string(stringValue.unescaped));
}

void ValueVisitor::visitBooleanValue(const peg::ast_node& booleanValue)
{
	_value = response::Value(booleanValue.is_type<peg::true_keyword>());
}

void ValueVisitor::visitNullValue(const peg::ast_node& /*nullValue*/)
{
	_value = {};
}

void ValueVisitor::visitEnumValue(const peg::ast_node& enumValue)
{
	_value = response::Value(response::Type::EnumValue);
	_value.set<response::StringType>(enumValue.string());
}

void ValueVisitor::visitListValue(const peg::ast_node& listValue)
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

void ValueVisitor::visitObjectValue(const peg::ast_node& objectValue)
{
	_value = response::Value(response::Type::Map);
	_value.reserve(objectValue.children.size());

	ValueVisitor visitor(_variables);

	for (const auto& field : objectValue.children)
	{
		visitor.visit(*field->children.back());
		_value.emplace_back(field->children.front()->string(), visitor.getValue());
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

DirectiveVisitor::DirectiveVisitor(const response::Value& variables)
	: _variables(variables)
	, _directives(response::Type::Map)
{
}

void DirectiveVisitor::visit(const peg::ast_node& directives)
{
	response::Value result(response::Type::Map);

	for (const auto& directive : directives.children)
	{
		std::string directiveName;

		peg::on_first_child<peg::directive_name>(*directive,
			[&directiveName](const peg::ast_node& child)
		{
			directiveName = child.string_view();
		});

		if (directiveName.empty())
		{
			continue;
		}

		response::Value directiveArguments(response::Type::Map);

		peg::on_first_child<peg::arguments>(*directive,
			[this, &directiveArguments](const peg::ast_node& child)
		{
			ValueVisitor visitor(_variables);

			for (auto& argument : child.children)
			{
				visitor.visit(*argument->children.back());

				directiveArguments.emplace_back(argument->children.front()->string(), visitor.getValue());
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

			throw schema_exception { { error.str() } };
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

				throw schema_exception { { error.str() } };
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

			error << "Missing argument directive: " << entry.second
				<< " name: if";

			throw schema_exception { { error.str() } };
		}
	}

	return false;
}

Fragment::Fragment(const peg::ast_node& fragmentDefinition, const response::Value& variables)
	: _type(fragmentDefinition.children[1]->children.front()->string_view())
	, _directives(response::Type::Map)
	, _selection(*(fragmentDefinition.children.back()))
{
	peg::on_first_child<peg::directives>(fragmentDefinition,
		[this, &variables](const peg::ast_node& child)
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

ResolverParams::ResolverParams(const SelectionSetParams& selectionSetParams, const peg::ast_node& field,
	std::string&& fieldName, response::Value&& arguments, response::Value&& fieldDirectives,
	const peg::ast_node* selection, const FragmentMap& fragments, const response::Value& variables)
	: SelectionSetParams(selectionSetParams)
	, field(field)
	, fieldName(std::move(fieldName))
	, arguments(std::move(arguments))
	, fieldDirectives(std::move(fieldDirectives))
	, selection(selection)
	, fragments(fragments)
	, variables(variables)
{
}

schema_location ResolverParams::getLocation() const
{
	auto position = field.begin();

	return { position.line, position.byte_in_line };
}

uint8_t Base64::verifyFromBase64(char ch)
{
	uint8_t result = fromBase64(ch);

	if (result > 63)
	{
		throw schema_exception { { "invalid character in base64 encoded string" } };
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
				throw schema_exception { { "invalid padding at the end of a base64 encoded string" } };
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
				throw schema_exception { { "invalid padding at the end of a base64 encoded string" } };
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
		throw schema_exception { { "invalid padding at the end of a base64 encoded string" } };
	}

	return result;
}

char Base64::verifyToBase64(uint8_t i)
{
	unsigned char result = toBase64(i);

	if (result == padding)
	{
		throw schema_exception { { "invalid 6-bit value" } };
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
response::IntType ModifiedArgument<response::IntType>::convert(const response::Value& value)
{
	if (value.type() != response::Type::Int)
	{
		throw schema_exception { { "not an integer" } };
	}

	return value.get<response::IntType>();
}

template <>
response::FloatType ModifiedArgument<response::FloatType>::convert(const response::Value& value)
{
	if (value.type() != response::Type::Float)
	{
		throw schema_exception { { "not a float" } };
	}

	return value.get<response::FloatType>();
}

template <>
response::StringType ModifiedArgument<response::StringType>::convert(const response::Value& value)
{
	if (value.type() != response::Type::String)
	{
		throw schema_exception { { "not a string" } };
	}

	return value.get<response::StringType>();
}

template <>
response::BooleanType ModifiedArgument<response::BooleanType>::convert(const response::Value& value)
{
	if (value.type() != response::Type::Boolean)
	{
		throw schema_exception { { "not a boolean" } };
	}

	return value.get<response::BooleanType>();
}

template <>
response::Value ModifiedArgument<response::Value>::convert(const response::Value& value)
{
	return response::Value(value);
}

template <>
response::IdType ModifiedArgument<response::IdType>::convert(const response::Value& value)
{
	if (value.type() != response::Type::String)
	{
		throw schema_exception { { "not a string" } };
	}

	const auto& encoded = value.get<response::StringType>();

	return Base64::fromBase64(encoded.c_str(), encoded.size());
}

template <>
std::future<response::Value> ModifiedResult<response::IntType>::convert(FieldResult<response::IntType>&& result, ResolverParams&& params)
{
	// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
	if (params.selection != nullptr)
	{
		auto position = params.selection->begin();
		std::ostringstream error;

		error << "Field may not have sub-fields name: " << params.fieldName;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line }, { params.errorPath } } } };
	}

	return resolve(std::move(result), std::move(params),
		[](response::IntType&& value, const ResolverParams&)
	{
		return response::Value(value);
	});
}

template <>
std::future<response::Value> ModifiedResult<response::FloatType>::convert(FieldResult<response::FloatType>&& result, ResolverParams&& params)
{
	// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
	if (params.selection != nullptr)
	{
		auto position = params.selection->begin();
		std::ostringstream error;

		error << "Field may not have sub-fields name: " << params.fieldName;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line }, { params.errorPath } } } };
	}

	return resolve(std::move(result), std::move(params),
		[](response::FloatType&& value, const ResolverParams&)
	{
		return response::Value(value);
	});
}

template <>
std::future<response::Value> ModifiedResult<response::StringType>::convert(FieldResult<response::StringType>&& result, ResolverParams&& params)
{
	// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
	if (params.selection != nullptr)
	{
		auto position = params.selection->begin();
		std::ostringstream error;

		error << "Field may not have sub-fields name: " << params.fieldName;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line }, { params.errorPath } } } };
	}

	return resolve(std::move(result), std::move(params),
		[](response::StringType&& value, const ResolverParams&)
	{
		return response::Value(std::move(value));
	});
}

template <>
std::future<response::Value> ModifiedResult<response::BooleanType>::convert(FieldResult<response::BooleanType>&& result, ResolverParams&& params)
{
	// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
	if (params.selection != nullptr)
	{
		auto position = params.selection->begin();
		std::ostringstream error;

		error << "Field may not have sub-fields name: " << params.fieldName;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line }, { params.errorPath } } } };
	}

	return resolve(std::move(result), std::move(params),
		[](response::BooleanType&& value, const ResolverParams&)
	{
		return response::Value(value);
	});
}

template <>
std::future<response::Value> ModifiedResult<response::Value>::convert(FieldResult<response::Value>&& result, ResolverParams&& params)
{
	// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
	if (params.selection != nullptr)
	{
		auto position = params.selection->begin();
		std::ostringstream error;

		error << "Field may not have sub-fields name: " << params.fieldName;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line }, { params.errorPath } } } };
	}

	return resolve(std::move(result), std::move(params),
		[](response::Value&& value, const ResolverParams&)
	{
		return response::Value(std::move(value));
	});
}

template <>
std::future<response::Value> ModifiedResult<response::IdType>::convert(FieldResult<response::IdType>&& result, ResolverParams&& params)
{
	// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
	if (params.selection != nullptr)
	{
		auto position = params.selection->begin();
		std::ostringstream error;

		error << "Field may not have sub-fields name: " << params.fieldName;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line }, { params.errorPath } } } };
	}

	return resolve(std::move(result), std::move(params),
		[](response::IdType&& value, const ResolverParams&)
	{
		return response::Value(Base64::toBase64(value));
	});
}

template <>
std::future<response::Value> ModifiedResult<Object>::convert(FieldResult<std::shared_ptr<Object>>&& result, ResolverParams&& params)
{
	// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
	if (params.selection == nullptr)
	{
		auto position = params.field.begin();
		std::ostringstream error;

		error << "Field must have sub-fields name: " << params.fieldName;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line }, { params.errorPath } } } };
	}

	return std::async(params.launch,
		[](FieldResult<std::shared_ptr<Object>>&& resultFuture, ResolverParams&& paramsFuture)
	{
		auto wrappedResult = resultFuture.get();

		if (!wrappedResult)
		{
			response::Value document(response::Type::Map);

			document.emplace_back(std::string { strData }, response::Value(response::Type::Null));

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
	explicit SelectionVisitor(const SelectionSetParams& selectionSetParams,
		const FragmentMap& fragments, const response::Value& variables,
		const TypeNames& typeNames, const ResolverMap& resolvers);

	void visit(const peg::ast_node& selection);

	std::queue<std::pair<std::string, std::future<response::Value>>> getValues();

private:
	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	const std::shared_ptr<RequestState>& _state;
	const response::Value& _operationDirectives;
	const field_path _path;
	const std::launch _launch;
	const FragmentMap& _fragments;
	const response::Value& _variables;
	const TypeNames& _typeNames;
	const ResolverMap& _resolvers;

	std::stack<FragmentDirectives> _fragmentDirectives;
	std::unordered_set<std::string> _names;
	std::queue<std::pair<std::string, std::future<response::Value>>> _values;
};

SelectionVisitor::SelectionVisitor(const SelectionSetParams& selectionSetParams,
	const FragmentMap& fragments, const response::Value& variables,
	const TypeNames& typeNames, const ResolverMap& resolvers)
	: _state(selectionSetParams.state)
	, _operationDirectives(selectionSetParams.operationDirectives)
	, _path(selectionSetParams.errorPath)
	, _launch(selectionSetParams.launch)
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

void SelectionVisitor::visit(const peg::ast_node& selection)
{
	if (selection.is_type<peg::field>())
	{
		visitField(selection);
	}
	else if (selection.is_type<peg::fragment_spread>())
	{
		visitFragmentSpread(selection);
	}
	else if (selection.is_type<peg::inline_fragment>())
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
		name = child.string_view();
	});

	std::string alias;

	peg::on_first_child<peg::alias_name>(field,
		[&alias](const peg::ast_node& child)
	{
		alias = child.string_view();
	});

	if (alias.empty())
	{
		alias = name;
	}

	if (!_names.insert(alias).second)
	{
		// Skip resolving fields which map to the same response name as a field we've already
		// resolved. Validation should handle merging multiple references to the same field or
		// to compatible fields.
		return;
	}

	const auto itr = _resolvers.find(name);

	if (itr == _resolvers.cend())
	{
		std::promise<response::Value> promise;
		auto position = field.begin();
		std::ostringstream error;

		error << "Unknown field name: " << name;

		promise.set_exception(std::make_exception_ptr(
			schema_exception { { schema_error{
				error.str(),
				{ position.line, position.byte_in_line },
				{ _path }
			} } }));

		_values.push({
			std::move(alias),
			promise.get_future()
			});
		return;
	}

	DirectiveVisitor directiveVisitor(_variables);

	peg::on_first_child<peg::directives>(field,
		[&directiveVisitor](const peg::ast_node& child)
	{
		directiveVisitor.visit(child);
	});

	if (directiveVisitor.shouldSkip())
	{
		return;
	}

	response::Value arguments(response::Type::Map);

	peg::on_first_child<peg::arguments>(field,
		[this, &arguments](const peg::ast_node& child)
	{
		ValueVisitor visitor(_variables);

		for (auto& argument : child.children)
		{
			visitor.visit(*argument->children.back());

			arguments.emplace_back(argument->children.front()->string(), visitor.getValue());
		}
	});

	const peg::ast_node* selection = nullptr;

	peg::on_first_child<peg::selection_set>(field,
		[&selection](const peg::ast_node& child)
	{
		selection = &child;
	});

	auto path = _path;

	path.push({ alias });

	SelectionSetParams selectionSetParams {
		_state,
		_operationDirectives,
		_fragmentDirectives.top().fragmentDefinitionDirectives,
		_fragmentDirectives.top().fragmentSpreadDirectives,
		_fragmentDirectives.top().inlineFragmentDirectives,
		std::move(path),
		_launch,
	};

	try
	{
		auto result = itr->second(ResolverParams(selectionSetParams, field,
			std::string(alias), std::move(arguments), directiveVisitor.getDirectives(),
			selection, _fragments, _variables));

		_values.push({
			std::move(alias),
			std::move(result)
			});
	}
	catch (schema_exception & scx)
	{
		std::promise<response::Value> promise;
		auto position = field.begin();
		auto messages = scx.getStructuredErrors();

		for (auto& message : messages)
		{
			if (message.location.line == 0)
			{
				message.location = { position.line, position.byte_in_line };
			}

			if (message.path.empty())
			{
				message.path = { selectionSetParams.errorPath };
			}
		}

		promise.set_exception(std::make_exception_ptr(schema_exception { std::move(messages) }));

		_values.push({
			std::move(alias),
			promise.get_future()
			});
	}
	catch (const std::exception & ex)
	{
		std::promise<response::Value> promise;
		auto position = field.begin();
		std::ostringstream message;

		message << "Field error name: " << alias
			<< " unknown error: " << ex.what();

		promise.set_exception(std::make_exception_ptr(
			schema_exception { { schema_error{
				message.str(),
				{ position.line, position.byte_in_line },
				std::move(selectionSetParams.errorPath)
			} } }));

		_values.push({
			std::move(alias),
			promise.get_future()
			});
	}
}

void SelectionVisitor::visitFragmentSpread(const peg::ast_node& fragmentSpread)
{
	const std::string name(fragmentSpread.children.front()->string_view());
	auto itr = _fragments.find(name);

	if (itr == _fragments.cend())
	{
		auto position = fragmentSpread.begin();
		std::ostringstream error;

		error << "Unknown fragment name: " << name;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line }, { _path } } } };
	}

	bool skip = (_typeNames.count(itr->second.getType()) == 0);
	DirectiveVisitor directiveVisitor(_variables);

	if (!skip)
	{
		peg::on_first_child<peg::directives>(fragmentSpread,
			[&directiveVisitor](const peg::ast_node& child)
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
			fragmentSpreadDirectives.emplace_back(std::string { entry.first }, response::Value(entry.second));
		}
	}

	response::Value fragmentDefinitionDirectives(itr->second.getDirectives());

	// Merge outer fragment definition directives as long as they don't conflict.
	for (const auto& entry : _fragmentDirectives.top().fragmentDefinitionDirectives)
	{
		if (fragmentDefinitionDirectives.find(entry.first) == fragmentDefinitionDirectives.end())
		{
			fragmentDefinitionDirectives.emplace_back(std::string { entry.first }, response::Value(entry.second));
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

void SelectionVisitor::visitInlineFragment(const peg::ast_node& inlineFragment)
{
	DirectiveVisitor directiveVisitor(_variables);

	peg::on_first_child<peg::directives>(inlineFragment,
		[&directiveVisitor](const peg::ast_node& child)
	{
		directiveVisitor.visit(child);
	});

	if (directiveVisitor.shouldSkip())
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
		|| _typeNames.count(typeCondition->children.front()->string()) > 0)
	{
		peg::on_first_child<peg::selection_set>(inlineFragment,
			[this, &directiveVisitor](const peg::ast_node& child)
		{
			auto inlineFragmentDirectives = directiveVisitor.getDirectives();

			// Merge outer inline fragment directives as long as they don't conflict.
			for (const auto& entry : _fragmentDirectives.top().inlineFragmentDirectives)
			{
				if (inlineFragmentDirectives.find(entry.first) == inlineFragmentDirectives.end())
				{
					inlineFragmentDirectives.emplace_back(std::string { entry.first }, response::Value(entry.second));
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

Object::Object(TypeNames&& typeNames, ResolverMap&& resolvers)
	: _typeNames(std::move(typeNames))
	, _resolvers(std::move(resolvers))
{
}

std::future<response::Value> Object::resolve(const SelectionSetParams& selectionSetParams, const peg::ast_node& selection, const FragmentMap& fragments, const response::Value& variables) const
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

	return std::async(selectionSetParams.launch,
		[](std::queue<std::pair<std::string, std::future<response::Value>>>&& children)
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
						auto itrData = data.find(name);

						if (itrData == data.end())
						{
							data.emplace_back(std::move(name), std::move(entry.second));
						}
						else if (itrData->second != entry.second)
						{
							std::ostringstream message;
							response::Value error(response::Type::Map);

							message << "Ambiguous field error name: " << name;
							addErrorMessage(message.str(), error);
							errors.emplace_back(std::move(error));
						}
					}
				}
			}
			catch (schema_exception & scx)
			{
				auto messages = scx.getErrors().release<response::ListType>();

				errors.reserve(errors.size() + messages.size());
				for (auto& error : messages)
				{
					errors.emplace_back(std::move(error));
				}

				if (data.find(name) == data.end())
				{
					data.emplace_back(std::move(name), {});
				}
			}
			catch (const std::exception & ex)
			{
				std::ostringstream message;

				message << "Field error name: " << name
					<< " unknown error: " << ex.what();

				response::Value error(response::Type::Map);

				addErrorMessage(message.str(), error);
				errors.emplace_back(std::move(error));

				if (data.find(name) == data.end())
				{
					data.emplace_back(std::move(name), {});
				}
			}

			children.pop();
		}

		response::Value result(response::Type::Map);

		result.emplace_back(std::string { strData }, std::move(data));

		if (errors.size() > 0)
		{
			result.emplace_back(std::string { strErrors }, std::move(errors));
		}

		return result;
	}, std::move(selections));
}

bool Object::matchesType(const std::string& typeName) const
{
	return _typeNames.find(typeName) != _typeNames.cend();
}

void Object::beginSelectionSet(const SelectionSetParams&) const
{
}

void Object::endSelectionSet(const SelectionSetParams&) const
{
}

OperationData::OperationData(std::shared_ptr<RequestState>&& state, response::Value&& variables,
	response::Value&& directives, FragmentMap&& fragments)
	: state(std::move(state))
	, variables(std::move(variables))
	, directives(std::move(directives))
	, fragments(std::move(fragments))
{
}

using ValidateType = response::Value;

struct ValidateArgument
{
	bool defaultValue = false;
	ValidateType type;
};

using ValidateTypeFieldArguments = std::map<std::string, ValidateArgument>;

struct ValidateTypeField
{
	ValidateType returnType;
	ValidateTypeFieldArguments arguments;
};

using ValidateDirectiveArguments = std::map<std::string, ValidateArgument>;

struct ValidateDirective
{
	std::set<introspection::DirectiveLocation> locations;
	ValidateDirectiveArguments arguments;
};

struct ValidateArgumentVariable
{
	bool operator==(const ValidateArgumentVariable& other) const;

	std::string name;
};

struct ValidateArgumentEnumValue
{
	bool operator==(const ValidateArgumentEnumValue& other) const;

	std::string value;
};

struct ValidateArgumentValue;

struct ValidateArgumentValuePtr
{
	bool operator==(const ValidateArgumentValuePtr& other) const;

	std::unique_ptr<ValidateArgumentValue> value;
	schema_location position;
};

struct ValidateArgumentList
{
	bool operator==(const ValidateArgumentList& other) const;

	std::vector<ValidateArgumentValuePtr> values;
};

struct ValidateArgumentMap
{
	bool operator==(const ValidateArgumentMap& other) const;

	std::map<std::string, ValidateArgumentValuePtr> values;
};

using ValidateArgumentVariant = std::variant<
	ValidateArgumentVariable,
	response::IntType,
	response::FloatType,
	response::StringType,
	response::BooleanType,
	ValidateArgumentEnumValue,
	ValidateArgumentList,
	ValidateArgumentMap>;

struct ValidateArgumentValue
{
	ValidateArgumentValue(ValidateArgumentVariable&& value);
	ValidateArgumentValue(response::IntType value);
	ValidateArgumentValue(response::FloatType value);
	ValidateArgumentValue(response::StringType&& value);
	ValidateArgumentValue(response::BooleanType value);
	ValidateArgumentValue(ValidateArgumentEnumValue&& value);
	ValidateArgumentValue(ValidateArgumentList&& value);
	ValidateArgumentValue(ValidateArgumentMap&& value);

	ValidateArgumentVariant data;
};

bool ValidateArgumentVariable::operator==(const ValidateArgumentVariable& other) const
{
	return name == other.name;
}

bool ValidateArgumentEnumValue::operator==(const ValidateArgumentEnumValue& other) const
{
	return value == other.value;
}

bool ValidateArgumentValuePtr::operator==(const ValidateArgumentValuePtr& other) const
{
	return (!value
		? !other.value
		: (other.value && value->data == other.value->data));
}

bool ValidateArgumentList::operator==(const ValidateArgumentList& other) const
{
	return values == other.values;
}

bool ValidateArgumentMap::operator==(const ValidateArgumentMap& other) const
{
	return values == other.values;
}

ValidateArgumentValue::ValidateArgumentValue(ValidateArgumentVariable&& value)
	: data(std::move(value))
{
}

ValidateArgumentValue::ValidateArgumentValue(response::IntType value)
	: data(value)
{
}

ValidateArgumentValue::ValidateArgumentValue(response::FloatType value)
	: data(value)
{
}

ValidateArgumentValue::ValidateArgumentValue(response::StringType&& value)
	: data(std::move(value))
{
}

ValidateArgumentValue::ValidateArgumentValue(response::BooleanType value)
	: data(value)
{
}

ValidateArgumentValue::ValidateArgumentValue(ValidateArgumentEnumValue&& value)
	: data(std::move(value))
{
}

ValidateArgumentValue::ValidateArgumentValue(ValidateArgumentList&& value)
	: data(std::move(value))
{
}

ValidateArgumentValue::ValidateArgumentValue(ValidateArgumentMap&& value)
	: data(std::move(value))
{
}

// ValidateArgumentValueVisitor visits the AST and builds a record of a field return type and map
// of the arguments for comparison to see if 2 fields with the same result name can be merged.
class ValidateArgumentValueVisitor
{
public:
	ValidateArgumentValueVisitor();

	void visit(const peg::ast_node& value);

	ValidateArgumentValuePtr getArgumentValue();

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

	ValidateArgumentValuePtr _argumentValue;
};

ValidateArgumentValueVisitor::ValidateArgumentValueVisitor()
{
}

ValidateArgumentValuePtr ValidateArgumentValueVisitor::getArgumentValue()
{
	auto result = std::move(_argumentValue);

	return result;
}

void ValidateArgumentValueVisitor::visit(const peg::ast_node& value)
{
	if (value.is_type<peg::variable_value>())
	{
		visitVariable(value);
	}
	else if (value.is_type<peg::integer_value>())
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
	else if (value.is_type<peg::true_keyword>()
		|| value.is_type<peg::false_keyword>())
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
}

void ValidateArgumentValueVisitor::visitVariable(const peg::ast_node& variable)
{
	ValidateArgumentVariable value { std::string { variable.string_view().substr(1) } };
	auto position = variable.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.byte_in_line };
}

void ValidateArgumentValueVisitor::visitIntValue(const peg::ast_node& intValue)
{
	response::IntType value { std::atoi(intValue.string().c_str()) };
	auto position = intValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(value);
	_argumentValue.position = { position.line, position.byte_in_line };
}

void ValidateArgumentValueVisitor::visitFloatValue(const peg::ast_node& floatValue)
{
	response::FloatType value { std::atof(floatValue.string().c_str()) };
	auto position = floatValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(value);
	_argumentValue.position = { position.line, position.byte_in_line };
}

void ValidateArgumentValueVisitor::visitStringValue(const peg::ast_node& stringValue)
{
	response::StringType value { stringValue.unescaped };
	auto position = stringValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.byte_in_line };
}

void ValidateArgumentValueVisitor::visitBooleanValue(const peg::ast_node& booleanValue)
{
	response::BooleanType value { booleanValue.is_type<peg::true_keyword>() };
	auto position = booleanValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(value);
	_argumentValue.position = { position.line, position.byte_in_line };
}

void ValidateArgumentValueVisitor::visitNullValue(const peg::ast_node& nullValue)
{
	auto position = nullValue.begin();

	_argumentValue.value.reset();
	_argumentValue.position = { position.line, position.byte_in_line };
}

void ValidateArgumentValueVisitor::visitEnumValue(const peg::ast_node& enumValue)
{
	ValidateArgumentEnumValue value { enumValue.string() };
	auto position = enumValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.byte_in_line };
}

void ValidateArgumentValueVisitor::visitListValue(const peg::ast_node& listValue)
{
	ValidateArgumentList value;
	auto position = listValue.begin();

	value.values.reserve(listValue.children.size());

	for (const auto& child : listValue.children)
	{
		ValidateArgumentValueVisitor visitor;

		visitor.visit(*child);
		value.values.emplace_back(visitor.getArgumentValue());
	}

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.byte_in_line };
}

void ValidateArgumentValueVisitor::visitObjectValue(const peg::ast_node& objectValue)
{
	ValidateArgumentMap value;
	auto position = objectValue.begin();

	for (const auto& field : objectValue.children)
	{
		ValidateArgumentValueVisitor visitor;

		visitor.visit(*field->children.back());
		value.values[field->children.front()->string()] = visitor.getArgumentValue();
	}

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.byte_in_line };
}

using ValidateFieldArguments = std::map<std::string, ValidateArgumentValuePtr>;

struct ValidateField
{
	ValidateField(std::string&& returnType, std::optional<std::string>&& objectType, const std::string& fieldName, ValidateFieldArguments&& arguments);

	bool operator==(const ValidateField& other) const;

	std::string returnType;
	std::optional<std::string> objectType;
	std::string fieldName;
	ValidateFieldArguments arguments;
};

ValidateField::ValidateField(std::string&& returnType, std::optional<std::string>&& objectType, const std::string& fieldName, ValidateFieldArguments&& arguments)
	: returnType(std::move(returnType))
	, objectType(std::move(objectType))
	, fieldName(fieldName)
	, arguments(std::move(arguments))
{
}

bool ValidateField::operator==(const ValidateField& other) const
{
	return returnType == other.returnType
		&& ((objectType && other.objectType && *objectType != *other.objectType)
			|| (fieldName == other.fieldName && arguments == other.arguments));
}

// ValidateExecutableVisitor visits the AST and validates that it is executable against the service schema.
class ValidateExecutableVisitor
{
public:
	ValidateExecutableVisitor(const Request& service);

	void visit(const peg::ast_node& root);

	std::vector<schema_error> getStructuredErrors();

private:
	response::Value executeQuery(std::string_view query) const;

	static ValidateTypeFieldArguments getArguments(response::ListType&& argumentsMember);

	using FieldTypes = std::map<std::string, ValidateTypeField>;
	using TypeFields = std::map<std::string, FieldTypes>;

	std::optional<introspection::TypeKind> getScopedTypeKind() const;
	TypeFields::const_iterator getScopedTypeFields();
	static std::string getFieldType(const FieldTypes& fields, const std::string& name);
	static std::string getWrappedFieldType(const FieldTypes& fields, const std::string& name);
	static std::string getWrappedFieldType(const ValidateType& returnType);

	void visitFragmentDefinition(const peg::ast_node& fragmentDefinition);
	void visitOperationDefinition(const peg::ast_node& operationDefinition);

	void visitSelection(const peg::ast_node& selection);

	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	void visitDirectives(introspection::DirectiveLocation location, const peg::ast_node& directives);

	const Request& _service;
	std::vector<schema_error> _errors;

	using OperationTypes = std::map<std::string_view, std::string>;
	using TypeKinds = std::map<std::string, introspection::TypeKind>;
	using Directives = std::map<std::string, ValidateDirective>;
	using ExecutableNodes = std::map<std::string, const peg::ast_node&>;
	using FragmentSet = std::unordered_set<std::string>;

	OperationTypes _operationTypes;
	TypeKinds _typeKinds;
	Directives _directives;

	ExecutableNodes _fragmentDefinitions;
	ExecutableNodes _operationDefinitions;

	FragmentSet _referencedFragments;
	FragmentSet _fragmentStack;
	size_t _fieldCount = 0;
	TypeFields _typeFields;
	std::string _scopedType;
	std::map<std::string, ValidateField> _selectionFields;
};

ValidateExecutableVisitor::ValidateExecutableVisitor(const Request& service)
	: _service(service)
{
	// This is taking advantage of the fact that during validation we can choose to execute
	// unvalidated queries. Normally you can't have fragment cycles, so tools like GraphiQL work
	// around that limitation by nesting the ofType selection set many layers deep to eventually
	// read the underlying type when it's wrapped in List or NotNull types.
	auto data = executeQuery(R"gql(query {
			__schema {
				queryType {
					name
				}
				mutationType {
					name
				}
				subscriptionType {
					name
				}
				types {
					name
					kind
				}
				directives {
					name
					locations
					args {
						name
						defaultValue
						type {
							...nestedType
						}
					}
				}
			}
		}

		fragment nestedType on __Type {
			kind
			name
			ofType {
				...nestedType
			}
		})gql");
	auto members = data.release<response::MapType>();
	auto itrData = std::find_if(members.begin(), members.end(),
		[](const std::pair<std::string, response::Value>& entry) noexcept
	{
		return entry.first == R"gql(__schema)gql";
	});

	if (itrData != members.end()
		&& itrData->second.type() == response::Type::Map)
	{
		members = itrData->second.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.second.type() == response::Type::Map)
			{
				auto typeMembers = member.second.release<response::MapType>();
				auto itrType = std::find_if(typeMembers.begin(), typeMembers.end(),
					[](const std::pair<std::string, response::Value>& entry) noexcept
				{
					return entry.first == R"gql(name)gql";
				});

				if (itrType != typeMembers.end()
					&& itrType->second.type() == response::Type::String)
				{
					if (member.first == R"gql(queryType)gql")
					{
						_operationTypes[strQuery] = itrType->second.release<response::StringType>();
					}
					else if (member.first == R"gql(mutationType)gql")
					{
						_operationTypes[strMutation] = itrType->second.release<response::StringType>();
					}
					else if (member.first == R"gql(subscriptionType)gql")
					{
						_operationTypes[strSubscription] = itrType->second.release<response::StringType>();
					}
				}
			}
			else if (member.second.type() == response::Type::List
				&& member.first == R"gql(types)gql")
			{
				auto entries = member.second.release<response::ListType>();

				for (auto& entry : entries)
				{
					if (entry.type() != response::Type::Map)
					{
						continue;
					}

					auto typeMembers = entry.release<response::MapType>();
					auto itrName = std::find_if(typeMembers.begin(), typeMembers.end(),
						[](const std::pair<std::string, response::Value>& entry) noexcept
					{
						return entry.first == R"gql(name)gql";
					});
					auto itrKind = std::find_if(typeMembers.begin(), typeMembers.end(),
						[](const std::pair<std::string, response::Value>& entry) noexcept
					{
						return entry.first == R"gql(kind)gql";
					});

					if (itrName != typeMembers.end()
						&& itrName->second.type() == response::Type::String
						&& itrKind != typeMembers.end()
						&& itrKind->second.type() == response::Type::EnumValue)
					{
						_typeKinds[itrName->second.release<response::StringType>()] = ModifiedArgument<introspection::TypeKind>::convert(itrKind->second);
					}
				}
			}
			else if (member.second.type() == response::Type::List
				&& member.first == R"gql(directives)gql")
			{
				auto entries = member.second.release<response::ListType>();

				for (auto& entry : entries)
				{
					if (entry.type() != response::Type::Map)
					{
						continue;
					}

					auto directiveMembers = entry.release<response::MapType>();
					auto itrName = std::find_if(directiveMembers.begin(), directiveMembers.end(),
						[](const std::pair<std::string, response::Value>& entry) noexcept
					{
						return entry.first == R"gql(name)gql";
					});
					auto itrLocations = std::find_if(directiveMembers.begin(), directiveMembers.end(),
						[](const std::pair<std::string, response::Value>& entry) noexcept
					{
						return entry.first == R"gql(locations)gql";
					});

					if (itrName != directiveMembers.end()
						&& itrName->second.type() == response::Type::String
						&& itrLocations != directiveMembers.end()
						&& itrLocations->second.type() == response::Type::List)
					{
						ValidateDirective directive;
						auto locations = itrLocations->second.release<response::ListType>();

						for (const auto& location : locations)
						{
							if (location.type() != response::Type::EnumValue)
							{
								continue;
							}

							directive.locations.insert(ModifiedArgument<introspection::DirectiveLocation>::convert(location));
						}

						auto itrArgs = std::find_if(directiveMembers.begin(), directiveMembers.end(),
							[](const std::pair<std::string, response::Value>& entry) noexcept
						{
							return entry.first == R"gql(args)gql";
						});

						if (itrArgs != directiveMembers.end()
							&& itrArgs->second.type() == response::Type::List)
						{
							directive.arguments = getArguments(itrArgs->second.release<response::ListType>());
						}

						_directives[itrName->second.release<response::StringType>()] = std::move(directive);
					}
				}
			}
		}
	}
}

response::Value ValidateExecutableVisitor::executeQuery(std::string_view query) const
{
	response::Value data(response::Type::Map);
	std::shared_ptr<RequestState> state;
	auto ast = peg::parseString(query);
	const std::string operationName;
	response::Value variables(response::Type::Map);
	auto result = _service.resolve(state, *ast.root, operationName, std::move(variables)).get();
	auto members = result.release<response::MapType>();
	auto itrResponse = std::find_if(members.begin(), members.end(),
		[](const std::pair<std::string, response::Value>& entry) noexcept
	{
		return entry.first == strData;
	});

	if (itrResponse != members.end())
	{
		data = std::move(itrResponse->second);
	}

	return data;
}

void ValidateExecutableVisitor::visit(const peg::ast_node& root)
{
	// Visit all of the fragment definitions and check for duplicates.
	peg::for_each_child<peg::fragment_definition>(root,
		[this](const peg::ast_node& fragmentDefinition)
	{
		const auto& fragmentName = fragmentDefinition.children.front();
		const auto inserted = _fragmentDefinitions.insert({ fragmentName->string(), fragmentDefinition });

		if (!inserted.second)
		{
			// http://spec.graphql.org/June2018/#sec-Fragment-Name-Uniqueness
			auto position = fragmentDefinition.begin();
			std::ostringstream error;

			error << "Duplicate fragment name: " << inserted.first->first;

			_errors.push_back({ error.str(), { position.line, position.byte_in_line } });
		}
	});

	// Visit all of the operation definitions and check for duplicates.
	peg::for_each_child<peg::operation_definition>(root,
		[this](const peg::ast_node& operationDefinition)
	{
		std::string operationName;

		peg::on_first_child<peg::operation_name>(operationDefinition,
			[&operationName](const peg::ast_node& child)
		{
			operationName = child.string_view();
		});

		const auto inserted = _operationDefinitions.insert({ std::move(operationName), operationDefinition });

		if (!inserted.second)
		{
			// http://spec.graphql.org/June2018/#sec-Operation-Name-Uniqueness
			auto position = operationDefinition.begin();
			std::ostringstream error;

			error << "Duplicate operation name: " << inserted.first->first;

			_errors.push_back({ error.str(), { position.line, position.byte_in_line } });
		}
	});

	// Check for lone anonymous operations.
	if (_operationDefinitions.size() > 1)
	{
		auto itr = std::find_if(_operationDefinitions.cbegin(), _operationDefinitions.cend(),
			[](const std::pair<const std::string, const peg::ast_node&>& entry) noexcept
		{
			return entry.first.empty();
		});

		if (itr != _operationDefinitions.cend())
		{
			// http://spec.graphql.org/June2018/#sec-Lone-Anonymous-Operation
			auto position = itr->second.begin();

			_errors.push_back({ "Anonymous operation not alone", { position.line, position.byte_in_line } });
		}
	}

	// Visit the executable definitions recursively.
	for (const auto& child : root.children)
	{
		if (child->is_type<peg::fragment_definition>())
		{
			visitFragmentDefinition(*child);
		}
		else if (child->is_type<peg::operation_definition>())
		{
			visitOperationDefinition(*child);
		}
		else
		{
			// http://spec.graphql.org/June2018/#sec-Executable-Definitions
			auto position = child->begin();

			_errors.push_back({ "Unexpected type definition", { position.line, position.byte_in_line } });
		}
	}

	if (!_fragmentDefinitions.empty())
	{
		// http://spec.graphql.org/June2018/#sec-Fragments-Must-Be-Used
		const size_t originalSize = _errors.size();
		auto unreferencedFragments = std::move(_fragmentDefinitions);

		for (const auto& name : _referencedFragments)
		{
			unreferencedFragments.erase(name);
		}

		_errors.resize(originalSize + unreferencedFragments.size());
		std::transform(unreferencedFragments.cbegin(), unreferencedFragments.cend(), _errors.begin() + originalSize,
			[](const std::pair<const std::string, const peg::ast_node&>& fragmentDefinition) noexcept
		{
			auto position = fragmentDefinition.second.begin();
			std::ostringstream message;

			message << "Unused fragment name: " << fragmentDefinition.first;

			return schema_error { message.str(), { position.line, position.byte_in_line } };
		});
	}
}

std::vector<schema_error> ValidateExecutableVisitor::getStructuredErrors()
{
	auto errors = std::move(_errors);

	return errors;
}

void ValidateExecutableVisitor::visitFragmentDefinition(const peg::ast_node& fragmentDefinition)
{
	const auto name = fragmentDefinition.children.front()->string();
	const auto& selection = *fragmentDefinition.children.back();
	auto innerType = fragmentDefinition.children[1]->children.front()->string();

	peg::on_first_child<peg::directives>(fragmentDefinition,
		[this](const peg::ast_node& child)
	{
		visitDirectives(introspection::DirectiveLocation::FRAGMENT_DEFINITION, child);
	});

	_fragmentStack.insert(name);
	_scopedType = std::move(innerType);

	visitSelection(selection);

	_scopedType.clear();;
	_fragmentStack.clear();
	_selectionFields.clear();
}

void ValidateExecutableVisitor::visitOperationDefinition(const peg::ast_node& operationDefinition)
{
	auto operationType = strQuery;

	peg::on_first_child<peg::operation_type>(operationDefinition,
		[&operationType](const peg::ast_node& child)
	{
		operationType = child.string_view();
	});

	auto itrType = _operationTypes.find(operationType);

	if (itrType == _operationTypes.cend())
	{
		auto position = operationDefinition.begin();
		std::ostringstream error;

		error << "Unsupported operation type: " << operationType;

		_errors.push_back({ error.str(), { position.line, position.byte_in_line } });
		return;
	}

	peg::on_first_child<peg::directives>(operationDefinition,
		[this, &operationType](const peg::ast_node& child)
	{
		auto location = introspection::DirectiveLocation::QUERY;

		if (operationType == strMutation)
		{
			location = introspection::DirectiveLocation::MUTATION;
		}
		else if (operationType == strSubscription)
		{
			location = introspection::DirectiveLocation::SUBSCRIPTION;
		}

		visitDirectives(location, child);
	});

	_scopedType = itrType->second;
	_fieldCount = 0;

	const auto& selection = *operationDefinition.children.back();

	visitSelection(selection);

	if (_fieldCount > 1
		&& operationType == strSubscription)
	{
		// http://spec.graphql.org/June2018/#sec-Single-root-field
		std::string name;

		peg::on_first_child<peg::operation_name>(operationDefinition,
			[&name](const peg::ast_node& child)
			{
				name = child.string_view();
			});

		auto position = operationDefinition.begin();
		std::ostringstream error;

		error << "Subscription with more than one root field";

		if (!name.empty())
		{
			error << " name: " << name;
		}

		_errors.push_back({ error.str(), { position.line, position.byte_in_line } });
	}

	_scopedType.clear();;
	_fragmentStack.clear();
	_selectionFields.clear();
}

void ValidateExecutableVisitor::visitSelection(const peg::ast_node& selection)
{
	for (const auto& child : selection.children)
	{
		if (child->is_type<peg::field>())
		{
			visitField(*child);
		}
		else if (child->is_type<peg::fragment_spread>())
		{
			visitFragmentSpread(*child);
		}
		else if (child->is_type<peg::inline_fragment>())
		{
			visitInlineFragment(*child);
		}
	}
}

ValidateTypeFieldArguments ValidateExecutableVisitor::getArguments(response::ListType&& args)
{
	ValidateTypeFieldArguments result;

	for (auto& arg : args)
	{
		if (arg.type() != response::Type::Map)
		{
			continue;
		}

		auto members = arg.release<response::MapType>();
		auto itrName = std::find_if(members.begin(), members.end(),
			[](const std::pair<std::string, response::Value>& argEntry) noexcept
		{
			return argEntry.first == R"gql(name)gql";
		});
		auto itrType = std::find_if(members.begin(), members.end(),
			[](const std::pair<std::string, response::Value>& argEntry) noexcept
		{
			return argEntry.first == R"gql(type)gql";
		});
		auto itrDefaultValue = std::find_if(members.begin(), members.end(),
			[](const std::pair<std::string, response::Value>& argEntry) noexcept
		{
			return argEntry.first == R"gql(defaultValue)gql";
		});

		if (itrName != members.end()
			&& itrName->second.type() == response::Type::String
			&& itrType != members.end()
			&& itrType->second.type() == response::Type::Map)
		{
			ValidateArgument argument;

			argument.defaultValue = (itrDefaultValue != members.end()
				&& itrDefaultValue->second.type() == response::Type::String);
			argument.type = std::move(itrType->second);

			result[itrName->second.release<response::StringType>()] = std::move(argument);
		}
	}

	return result;
}

std::optional<introspection::TypeKind> ValidateExecutableVisitor::getScopedTypeKind() const
{
	auto itrKind = _typeKinds.find(_scopedType);

	return (itrKind == _typeKinds.cend()
		? std::nullopt
		: std::make_optional(itrKind->second));
}

ValidateExecutableVisitor::TypeFields::const_iterator ValidateExecutableVisitor::getScopedTypeFields()
{
	auto typeKind = getScopedTypeKind();
	auto itrType = _typeFields.find(_scopedType);

	if (itrType == _typeFields.cend())
	{
		if (!typeKind)
		{
			return itrType;
		}

		switch (*typeKind)
		{
			case introspection::TypeKind::OBJECT:
			case introspection::TypeKind::INTERFACE:
			case introspection::TypeKind::UNION:
				// These are the only types which support sub-fields.
				break;

			default:
				return itrType;
		}
	}

	if (itrType == _typeFields.cend())
	{
		std::ostringstream oss;

		// This is taking advantage of the fact that during validation we can choose to execute
		// unvalidated queries. Normally you can't have fragment cycles, so tools like GraphiQL work
		// around that limitation by nesting the ofType selection set many layers deep to eventually
		// read the underlying type when it's wrapped in List or NotNull types.
		oss << R"gql(query {
					__type(name: ")gql" << _scopedType << R"gql(") {
						fields(includeDeprecated: true) {
							name
							type {
								...nestedType
							}
							args {
								name
								defaultValue
								type {
									...nestedType
								}
							}
						}
					}
				}

			fragment nestedType on __Type {
				kind
				name
				ofType {
					...nestedType
				}
			})gql";

		auto data = executeQuery(oss.str());
		auto members = data.release<response::MapType>();
		auto itrResponse = std::find_if(members.begin(), members.end(),
			[](const std::pair<std::string, response::Value>& entry) noexcept
		{
			return entry.first == R"gql(__type)gql";
		});

		if (itrResponse != members.end()
			&& itrResponse->second.type() == response::Type::Map)
		{
			std::map<std::string, ValidateTypeField> fields;
			response::Value scalarKind(response::Type::EnumValue);
			response::Value nonNullKind(response::Type::EnumValue);

			scalarKind.set<response::StringType>(R"gql(SCALAR)gql");
			nonNullKind.set<response::StringType>(R"gql(NON_NULL)gql");

			members = itrResponse->second.release<response::MapType>();

			itrResponse = std::find_if(members.begin(), members.end(),
				[](const std::pair<std::string, response::Value>& entry) noexcept
			{
				return entry.first == R"gql(fields)gql";
			});

			if (itrResponse != members.end()
				&& itrResponse->second.type() == response::Type::List)
			{
				auto entries = itrResponse->second.release<response::ListType>();

				for (auto& entry : entries)
				{
					if (entry.type() != response::Type::Map)
					{
						continue;
					}

					members = entry.release<response::MapType>();

					auto itrFieldName = std::find_if(members.begin(), members.end(),
						[](const std::pair<std::string, response::Value>& entry) noexcept
					{
						return entry.first == R"gql(name)gql";
					});
					auto itrFieldType = std::find_if(members.begin(), members.end(),
						[](const std::pair<std::string, response::Value>& entry) noexcept
					{
						return entry.first == R"gql(type)gql";
					});

					if (itrFieldName != members.end()
						&& itrFieldName->second.type() == response::Type::String
						&& itrFieldType != members.end()
						&& itrFieldType->second.type() == response::Type::Map)
					{
						auto fieldName = itrFieldName->second.release<response::StringType>();
						ValidateTypeField subField;

						subField.returnType = std::move(itrFieldType->second);

						auto itrArgs = std::find_if(members.begin(), members.end(),
							[](const std::pair<std::string, response::Value>& entry) noexcept
						{
							return entry.first == R"gql(args)gql";
						});

						if (itrArgs != members.end()
							&& itrArgs->second.type() == response::Type::List)
						{
							subField.arguments = getArguments(itrArgs->second.release<response::ListType>());
						}

						fields[std::move(fieldName)] = std::move(subField);
					}
				}

				if (_scopedType == _operationTypes[strQuery])
				{
					response::Value objectKind(response::Type::EnumValue);

					objectKind.set<response::StringType>(R"gql(OBJECT)gql");

					ValidateTypeField schemaField;
					response::Value schemaType(response::Type::Map);
					response::Value notNullSchemaType(response::Type::Map);

					schemaType.emplace_back(R"gql(kind)gql", response::Value(objectKind));
					schemaType.emplace_back(R"gql(name)gql", response::Value(R"gql(__Schema)gql"));
					notNullSchemaType.emplace_back(R"gql(kind)gql", response::Value(nonNullKind));
					notNullSchemaType.emplace_back(R"gql(ofType)gql", std::move(schemaType));
					schemaField.returnType = std::move(notNullSchemaType);
					fields[R"gql(__schema)gql"] = std::move(schemaField);

					ValidateTypeField typeField;
					response::Value typeType(response::Type::Map);

					typeType.emplace_back(R"gql(kind)gql", response::Value(objectKind));
					typeType.emplace_back(R"gql(name)gql", response::Value(R"gql(__Type)gql"));
					typeField.returnType = std::move(typeType);

					ValidateArgument nameArgument;
					response::Value typeNameArg(response::Type::Map);
					response::Value nonNullTypeNameArg(response::Type::Map);

					typeNameArg.emplace_back(R"gql(kind)gql", response::Value(scalarKind));
					typeNameArg.emplace_back(R"gql(name)gql", response::Value(R"gql(String)gql"));
					nonNullTypeNameArg.emplace_back(R"gql(kind)gql", response::Value(nonNullKind));
					nonNullTypeNameArg.emplace_back(R"gql(ofType)gql", std::move(typeNameArg));
					nameArgument.type = std::move(nonNullTypeNameArg);

					typeField.arguments[R"gql(name)gql"] = std::move(nameArgument);

					fields[R"gql(__type)gql"] = std::move(typeField);
				}
			}

			ValidateTypeField typenameField;
			response::Value typenameType(response::Type::Map);
			response::Value notNullTypenameType(response::Type::Map);

			typenameType.emplace_back(R"gql(kind)gql", response::Value(scalarKind));
			typenameType.emplace_back(R"gql(name)gql", response::Value(R"gql(String)gql"));
			notNullTypenameType.emplace_back(R"gql(kind)gql", response::Value(nonNullKind));
			notNullTypenameType.emplace_back(R"gql(ofType)gql", std::move(typenameType));
			typenameField.returnType = std::move(notNullTypenameType);
			fields[R"gql(__typename)gql"] = std::move(typenameField);

			itrType = _typeFields.insert({ _scopedType, std::move(fields) }).first;
		}
	}

	return itrType;
}

std::string ValidateExecutableVisitor::getFieldType(const FieldTypes& fields, const std::string& name)
{
	std::string result;
	auto itrType = fields.find(name);

	if (itrType == fields.end())
	{
		return result;
	}

	// Iteratively expand nested types till we get the underlying field type.
	const std::string nameMember{ R"gql(name)gql" };
	const std::string ofTypeMember{ R"gql(ofType)gql" };
	auto itrName = itrType->second.returnType.find(nameMember);
	auto itrOfType = itrType->second.returnType.find(ofTypeMember);
	auto itrEnd = itrType->second.returnType.end();

	do
	{
		if (itrName != itrEnd
			&& itrName->second.type() == response::Type::String)
		{
			result = itrName->second.get<response::StringType>();
		}
		else if (itrOfType != itrEnd
			&& itrOfType->second.type() == response::Type::Map)
		{
			itrEnd = itrOfType->second.end();
			itrName = itrOfType->second.find(nameMember);
			itrOfType = itrOfType->second.find(ofTypeMember);
		}
		else
		{
			break;
		}
	} while (result.empty());

	return result;
}

std::string ValidateExecutableVisitor::getWrappedFieldType(const FieldTypes& fields, const std::string& name)
{
	std::string result;
	auto itrType = fields.find(name);

	if (itrType == fields.end())
	{
		return result;
	}

	result = getWrappedFieldType(itrType->second.returnType);

	return result;
}

std::string ValidateExecutableVisitor::getWrappedFieldType(const ValidateType& returnType)
{
	// Recursively expand nested types till we get the underlying field type.
	const std::string nameMember { R"gql(name)gql" };
	auto itrName = returnType.find(nameMember);
	auto itrEnd = returnType.end();

	if (itrName != itrEnd
		&& itrName->second.type() == response::Type::String)
	{
		return itrName->second.get<response::StringType>();
	}

	std::ostringstream oss;
	const std::string kindMember { R"gql(kind)gql" };
	const std::string ofTypeMember { R"gql(ofType)gql" };
	auto itrKind = returnType.find(kindMember);
	auto itrOfType = returnType.find(ofTypeMember);

	if (itrKind != itrEnd
		&& itrKind->second.type() == response::Type::EnumValue
		&& itrOfType != itrEnd
		&& itrOfType->second.type() == response::Type::Map)
	{
		switch (ModifiedArgument<introspection::TypeKind>::convert(itrKind->second))
		{
			case introspection::TypeKind::LIST:
				oss << '[' << getWrappedFieldType(itrOfType->second) << ']';
				break;

			case introspection::TypeKind::NON_NULL:
				oss << getWrappedFieldType(itrOfType->second) << '!';
				break;

			default:
				break;
		}
	}

	return oss.str();
}

void ValidateExecutableVisitor::visitField(const peg::ast_node& field)
{
	std::string name;

	peg::on_first_child<peg::field_name>(field,
		[&name](const peg::ast_node& child)
	{
		name = child.string_view();
	});

	auto kind = getScopedTypeKind();

	if (!kind)
	{
		// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
		auto position = field.begin();
		std::ostringstream message;

		message << "Field on unknown type: " << _scopedType
			<< " name: " << name;

		_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
		return;
	}

	std::string innerType;
	std::string wrappedType;
	auto itrType = getScopedTypeFields();

	if (itrType == _typeFields.cend())
	{
		// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
		auto position = field.begin();
		std::ostringstream message;

		message << "Field on scalar type: " << _scopedType
			<< " name: " << name;

		_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
		return;
	}

	switch (*kind)
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		{
			// http://spec.graphql.org/June2018/#sec-Field-Selections-on-Objects-Interfaces-and-Unions-Types
			innerType = getFieldType(itrType->second, name);
			wrappedType = getWrappedFieldType(itrType->second, name);
			break;
		}

		case introspection::TypeKind::UNION:
		{
			if (name != R"gql(__typename)gql")
			{
				// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
				auto position = field.begin();
				std::ostringstream message;

				message << "Field on union type: " << _scopedType
					<< " name: " << name;

				_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
				return;
			}

			// http://spec.graphql.org/June2018/#sec-Field-Selections-on-Objects-Interfaces-and-Unions-Types
			innerType = "String";
			wrappedType = "String!";
			break;
		}

		default:
			break;
	}

	if (innerType.empty())
	{
		// http://spec.graphql.org/June2018/#sec-Field-Selections-on-Objects-Interfaces-and-Unions-Types
		auto position = field.begin();
		std::ostringstream message;

		message << "Undefined field type: " << _scopedType
			<< " name: " << name;

		_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
		return;
	}

	std::string alias;

	peg::on_first_child<peg::alias_name>(field,
		[&alias](const peg::ast_node& child)
	{
		alias = child.string_view();
	});

	if (alias.empty())
	{
		alias = name;
	}

	ValidateFieldArguments validateArguments;
	std::map<std::string, schema_location> argumentLocations;
	std::queue<std::string> argumentNames;

	peg::on_first_child<peg::arguments>(field,
		[this, &name, &validateArguments, &argumentLocations, &argumentNames](const peg::ast_node& child)
	{
		for (auto& argument : child.children)
		{
			auto argumentName = argument->children.front()->string();
			auto position = argument->begin();

			if (validateArguments.find(argumentName) != validateArguments.end())
			{
				// http://spec.graphql.org/June2018/#sec-Argument-Uniqueness
				std::ostringstream message;

				message << "Conflicting argument type: " << _scopedType
					<< " field: " << name
					<< " name: " << argumentName;

				_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
				continue;
			}

			ValidateArgumentValueVisitor visitor;

			visitor.visit(*argument->children.back());
			validateArguments[argumentName] = visitor.getArgumentValue();
			argumentLocations[argumentName] = { position.line, position.byte_in_line };
			argumentNames.push(std::move(argumentName));
		}
	});

	std::optional<std::string> objectType = (*kind == introspection::TypeKind::OBJECT
		? std::make_optional(_scopedType)
		: std::nullopt);
	ValidateField validateField(std::move(wrappedType), std::move(objectType), name, std::move(validateArguments));
	auto itrValidateField = _selectionFields.find(alias);

	if (itrValidateField != _selectionFields.end())
	{
		if (itrValidateField->second == validateField)
		{
			// We already validated this field.
			return;
		}
		else
		{
			// http://spec.graphql.org/June2018/#sec-Field-Selection-Merging
			auto position = field.begin();
			std::ostringstream message;

			message << "Conflicting field type: " << _scopedType
				<< " name: " << name;

			_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
		}
	}

	auto itrField = itrType->second.find(name);

	if (itrField != itrType->second.end())
	{
		while (!argumentNames.empty())
		{
			auto argumentName = std::move(argumentNames.front());

			argumentNames.pop();

			auto itrArgument = itrField->second.arguments.find(argumentName);

			if (itrArgument == itrField->second.arguments.end())
			{
				// http://spec.graphql.org/June2018/#sec-Argument-Names
				std::ostringstream message;

				message << "Undefined argument type: " << _scopedType
					<< " field: " << name
					<< " name: " << argumentName;

				_errors.push_back({ message.str(), argumentLocations[argumentName] });
			}
		}

		for (auto& argument : itrField->second.arguments)
		{
			if (argument.second.defaultValue)
			{
				// The argument has a default value.
				continue;
			}

			auto itrArgument = validateField.arguments.find(argument.first);
			const bool missing = itrArgument == validateField.arguments.end();

			if (!missing
				&& itrArgument->second.value)
			{
				// The value was not null.
				continue;
			}

			// See if the argument is wrapped in NON_NULL
			auto itrKind = argument.second.type.find(R"gql(kind)gql");

			if (itrKind != argument.second.type.end()
				&& itrKind->second.type() == response::Type::EnumValue
				&& introspection::TypeKind::NON_NULL == ModifiedArgument<introspection::TypeKind>::convert(itrKind->second))
			{
				// http://spec.graphql.org/June2018/#sec-Required-Arguments
				auto position = field.begin();
				std::ostringstream message;

				message << (missing ?
					"Missing argument type: " :
					"Required non-null argument type: ") << _scopedType
					<< " field: " << name
					<< " name: " << argument.first;

				_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
			}
		}
	}

	_selectionFields.insert({ std::move(alias), std::move(validateField) });

	peg::on_first_child<peg::directives>(field,
		[this](const peg::ast_node& child)
	{
		visitDirectives(introspection::DirectiveLocation::FIELD, child);
	});

	const peg::ast_node* selection = nullptr;

	peg::on_first_child<peg::selection_set>(field,
		[&selection](const peg::ast_node& child)
	{
		selection = &child;
	});

	size_t subFieldCount = 0;

	if (selection != nullptr)
	{
		auto outerType = std::move(_scopedType);
		auto outerFields = std::move(_selectionFields);
		auto outerFieldCount = _fieldCount;

		_fieldCount = 0;
		_selectionFields.clear();
		_scopedType = std::move(innerType);

		visitSelection(*selection);

		innerType = std::move(_scopedType);
		_scopedType = std::move(outerType);
		_selectionFields = std::move(outerFields);
		subFieldCount = _fieldCount;
		_fieldCount = outerFieldCount;
	}

	if (subFieldCount == 0)
	{
		auto itrInnerKind = _typeKinds.find(innerType);

		if (itrInnerKind != _typeKinds.end())
		{
			switch (itrInnerKind->second)
			{
				case introspection::TypeKind::OBJECT:
				case introspection::TypeKind::INTERFACE:
				case introspection::TypeKind::UNION:
				{
					// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
					auto position = field.begin();
					std::ostringstream message;

					message << "Missing fields on non-scalar type: " << innerType;

					_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
					return;
				}

				default:
					break;
			}
		}
	}

	++_fieldCount;
}

void ValidateExecutableVisitor::visitFragmentSpread(const peg::ast_node& fragmentSpread)
{
	const std::string name(fragmentSpread.children.front()->string_view());
	auto itr = _fragmentDefinitions.find(name);

	if (itr == _fragmentDefinitions.cend())
	{
		// http://spec.graphql.org/June2018/#sec-Fragment-spread-target-defined
		auto position = fragmentSpread.begin();
		std::ostringstream message;

		message << "Fragment spread undefined name: " << name;

		_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
		return;
	}

	if (_fragmentStack.find(name) != _fragmentStack.cend())
	{
		// http://spec.graphql.org/June2018/#sec-Fragment-spreads-must-not-form-cycles
		auto position = fragmentSpread.begin();
		std::ostringstream message;

		message << "Fragment spread cycle name: " << name;

		_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
		return;
	}

	peg::on_first_child<peg::directives>(fragmentSpread,
		[this](const peg::ast_node& child)
	{
		visitDirectives(introspection::DirectiveLocation::FRAGMENT_SPREAD, child);
	});

	const auto& selection = *itr->second.children.back();
	std::string innerType{ itr->second.children[1]->children.front()->string_view() };
	auto outerType = std::move(_scopedType);

	_fragmentStack.insert(name);
	_scopedType = std::move(innerType);

	visitSelection(selection);

	_scopedType = std::move(outerType);
	_fragmentStack.erase(name);

	_referencedFragments.insert(name);
}

void ValidateExecutableVisitor::visitInlineFragment(const peg::ast_node& inlineFragment)
{
	std::string innerType;

	peg::on_first_child<peg::type_condition>(inlineFragment,
		[&innerType](const peg::ast_node& child)
	{
		innerType = child.children.front()->string();
	});

	if (innerType.empty())
	{
		innerType = _scopedType;
	}

	peg::on_first_child<peg::directives>(inlineFragment,
		[this](const peg::ast_node& child)
	{
		visitDirectives(introspection::DirectiveLocation::INLINE_FRAGMENT, child);
	});

	peg::on_first_child<peg::selection_set>(inlineFragment,
		[this, &innerType](const peg::ast_node& selection)
	{
		auto outerType = std::move(_scopedType);

		_scopedType = std::move(innerType);

		visitSelection(selection);

		_scopedType = std::move(outerType);
	});
}

void ValidateExecutableVisitor::visitDirectives(introspection::DirectiveLocation location, const peg::ast_node& directives)
{
	for (const auto& directive : directives.children)
	{
		std::string directiveName;

		peg::on_first_child<peg::directive_name>(*directive,
			[&directiveName](const peg::ast_node& child)
		{
			directiveName = child.string_view();
		});

		auto itrDirective = _directives.find(directiveName);

		if (itrDirective == _directives.end())
		{
			// http://spec.graphql.org/June2018/#sec-Directives-Are-Defined
			auto position = directive->begin();
			std::ostringstream message;

			message << "Undefined directive name: " << directiveName;

			_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
			continue;
		}

		peg::on_first_child<peg::arguments>(*directive,
			[this, &directive, &directiveName, itrDirective](const peg::ast_node& child)
		{
			ValidateFieldArguments validateArguments;
			std::map<std::string, schema_location> argumentLocations;
			std::queue<std::string> argumentNames;

			for (auto& argument : child.children)
			{
				auto position = argument->begin();
				auto argumentName = argument->children.front()->string();

				if (validateArguments.find(argumentName) != validateArguments.end())
				{
					// http://spec.graphql.org/June2018/#sec-Argument-Uniqueness
					std::ostringstream message;

					message << "Conflicting argument directive: " << directiveName
						<< " name: " << argumentName;

					_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
					continue;
				}

				ValidateArgumentValueVisitor visitor;

				visitor.visit(*argument->children.back());
				validateArguments[argumentName] = visitor.getArgumentValue();
				argumentLocations[argumentName] = { position.line, position.byte_in_line };
				argumentNames.push(std::move(argumentName));
			}

			while (!argumentNames.empty())
			{
				auto argumentName = std::move(argumentNames.front());

				argumentNames.pop();

				auto itrArgument = itrDirective->second.arguments.find(argumentName);

				if (itrArgument == itrDirective->second.arguments.end())
				{
					// http://spec.graphql.org/June2018/#sec-Argument-Names
					std::ostringstream message;

					message << "Undefined argument directive: " << directiveName
						<< " name: " << argumentName;

					_errors.push_back({ message.str(), argumentLocations[argumentName] });
				}
			}

			for (auto& argument : itrDirective->second.arguments)
			{
				if (argument.second.defaultValue)
				{
					// The argument has a default value.
					continue;
				}

				auto itrArgument = validateArguments.find(argument.first);
				const bool missing = itrArgument == validateArguments.end();

				if (!missing
					&& itrArgument->second.value)
				{
					// The value was not null.
					continue;
				}

				// See if the argument is wrapped in NON_NULL
				auto itrKind = argument.second.type.find(R"gql(kind)gql");

				if (itrKind != argument.second.type.end()
					&& itrKind->second.type() == response::Type::EnumValue
					&& introspection::TypeKind::NON_NULL == ModifiedArgument<introspection::TypeKind>::convert(itrKind->second))
				{
					// http://spec.graphql.org/June2018/#sec-Required-Arguments
					auto position = directive->begin();
					std::ostringstream message;

					message << (missing ?
						"Missing argument directive: " :
						"Required non-null argument directive: ") << directiveName
						<< " name: " << argument.first;

					_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
				}
			}
		});
	}
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

FragmentDefinitionVisitor::FragmentDefinitionVisitor(const response::Value& variables)
	: _variables(variables)
{
}

FragmentMap FragmentDefinitionVisitor::getFragments()
{
	FragmentMap result(std::move(_fragments));
	return result;
}

void FragmentDefinitionVisitor::visit(const peg::ast_node& fragmentDefinition)
{
	_fragments.insert({ fragmentDefinition.children.front()->string(), Fragment(fragmentDefinition, _variables) });
}

// OperationDefinitionVisitor visits the AST and executes the one with the specified
// operation name.
class OperationDefinitionVisitor
{
public:
	OperationDefinitionVisitor(std::launch launch, std::shared_ptr<RequestState> state, const TypeMap& operations, response::Value&& variables, FragmentMap&& fragments);

	std::future<response::Value> getValue();

	void visit(const std::string& operationType, const peg::ast_node& operationDefinition);

private:
	const std::launch _launch;
	std::shared_ptr<OperationData> _params;
	const TypeMap& _operations;
	std::future<response::Value> _result;
};

OperationDefinitionVisitor::OperationDefinitionVisitor(std::launch launch, std::shared_ptr<RequestState> state, const TypeMap& operations, response::Value&& variables, FragmentMap&& fragments)
	: _launch(launch)
	, _params(std::make_shared<OperationData>(
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

void OperationDefinitionVisitor::visit(const std::string& operationType, const peg::ast_node& operationDefinition)
{
	auto itr = _operations.find(operationType);

	// Filter the variable definitions down to the ones referenced in this operation
	response::Value operationVariables(response::Type::Map);

	peg::for_each_child<peg::variable>(operationDefinition,
		[this, &operationVariables](const peg::ast_node& variable)
	{
		std::string variableName;

		peg::on_first_child<peg::variable_name>(variable,
			[&variableName](const peg::ast_node& name)
		{
			// Skip the $ prefix
			variableName = name.string_view().substr(1);
		});

		auto itrVar = _params->variables.find(variableName);
		response::Value valueVar;

		if (itrVar != _params->variables.get<response::MapType>().cend())
		{
			valueVar = response::Value(itrVar->second);
		}
		else
		{
			peg::on_first_child<peg::default_value>(variable,
				[this, &valueVar](const peg::ast_node& defaultValue)
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
		[this, &operationDirectives](const peg::ast_node& child)
	{
		DirectiveVisitor directiveVisitor(_params->variables);

		directiveVisitor.visit(child);
		operationDirectives = directiveVisitor.getDirectives();
	});

	_params->directives = std::move(operationDirectives);

	// Keep the params alive until the deferred lambda has executed
	_result = std::async(_launch,
		[selectionLaunch = _launch, params = std::move(_params), operation = itr->second](const peg::ast_node& selection)
	{
		// The top level object doesn't come from inside of a fragment, so all of the fragment directives are empty.
		const response::Value emptyFragmentDirectives(response::Type::Map);
		const SelectionSetParams selectionSetParams {
			params->state,
			params->directives,
			emptyFragmentDirectives,
			emptyFragmentDirectives,
			emptyFragmentDirectives,
			{},
			selectionLaunch,
		};

		return operation->resolve(selectionSetParams, selection, params->fragments, params->variables).get();
	}, std::cref(*operationDefinition.children.back()));
}

SubscriptionData::SubscriptionData(std::shared_ptr<OperationData>&& data, SubscriptionName&& field, response::Value&& arguments,
	peg::ast&& query, std::string&& operationName, SubscriptionCallback&& callback,
	const peg::ast_node& selection)
	: data(std::move(data))
	, field(std::move(field))
	, arguments(std::move(arguments))
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
	SubscriptionName _field;
	response::Value _arguments;
	std::shared_ptr<SubscriptionData> _result;
};

SubscriptionDefinitionVisitor::SubscriptionDefinitionVisitor(SubscriptionParams&& params, SubscriptionCallback&& callback, FragmentMap&& fragments, const std::shared_ptr<Object>& subscriptionObject)
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

void SubscriptionDefinitionVisitor::visit(const peg::ast_node& operationDefinition)
{
	const auto& selection = *operationDefinition.children.back();

	for (const auto& child : selection.children)
	{
		if (child->is_type<peg::field>())
		{
			visitField(*child);
		}
		else if (child->is_type<peg::fragment_spread>())
		{
			visitFragmentSpread(*child);
		}
		else if (child->is_type<peg::inline_fragment>())
		{
			visitInlineFragment(*child);
		}
	}

	response::Value directives(response::Type::Map);

	peg::on_first_child<peg::directives>(operationDefinition,
		[this, &directives](const peg::ast_node& child)
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
		std::move(_field),
		std::move(_arguments),
		std::move(_params.query),
		std::move(_params.operationName),
		std::move(_callback),
		selection);
}

void SubscriptionDefinitionVisitor::visitField(const peg::ast_node& field)
{
	std::string name;

	peg::on_first_child<peg::field_name>(field,
		[&name](const peg::ast_node& child)
	{
		name = child.string_view();
	});

	// http://spec.graphql.org/June2018/#sec-Single-root-field
	if (!_field.empty())
	{
		auto position = field.begin();
		std::ostringstream error;

		error << "Extra subscription root field name: " << name;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line } } } };
	}

	DirectiveVisitor directiveVisitor(_params.variables);

	peg::on_first_child<peg::directives>(field,
		[&directiveVisitor](const peg::ast_node& child)
	{
		directiveVisitor.visit(child);
	});

	if (directiveVisitor.shouldSkip())
	{
		return;
	}

	response::Value arguments(response::Type::Map);

	peg::on_first_child<peg::arguments>(field,
		[this, &arguments](const peg::ast_node& child)
	{
		ValueVisitor visitor(_params.variables);

		for (auto& argument : child.children)
		{
			visitor.visit(*argument->children.back());

			arguments.emplace_back(argument->children.front()->string(), visitor.getValue());
		}
	});

	_field = std::move(name);
	_arguments = std::move(arguments);
}

void SubscriptionDefinitionVisitor::visitFragmentSpread(const peg::ast_node& fragmentSpread)
{
	const std::string name(fragmentSpread.children.front()->string_view());
	auto itr = _fragments.find(name);

	if (itr == _fragments.cend())
	{
		auto position = fragmentSpread.begin();
		std::ostringstream error;

		error << "Unknown fragment name: " << name;

		throw schema_exception { { schema_error{ error.str(), { position.line, position.byte_in_line } } } };
	}

	bool skip = !_subscriptionObject->matchesType(itr->second.getType());
	DirectiveVisitor directiveVisitor(_params.variables);

	if (!skip)
	{
		peg::on_first_child<peg::directives>(fragmentSpread,
			[&directiveVisitor](const peg::ast_node& child)
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

void SubscriptionDefinitionVisitor::visitInlineFragment(const peg::ast_node& inlineFragment)
{
	DirectiveVisitor directiveVisitor(_params.variables);

	peg::on_first_child<peg::directives>(inlineFragment,
		[&directiveVisitor](const peg::ast_node& child)
	{
		directiveVisitor.visit(child);
	});

	if (directiveVisitor.shouldSkip())
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
		|| _subscriptionObject->matchesType(typeCondition->children.front()->string()))
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

Request::Request(TypeMap&& operationTypes)
	: _operations(std::move(operationTypes))
{
}

std::vector<schema_error> Request::validate(const peg::ast_node& root) const
{
	ValidateExecutableVisitor visitor(*this);

	visitor.visit(root);

	return visitor.getStructuredErrors();
}

std::pair<std::string, const peg::ast_node*> Request::findOperationDefinition(const peg::ast_node& root, const std::string& operationName) const
{
	bool hasAnonymous = false;
	std::unordered_set<std::string> usedNames;
	std::pair<std::string, const peg::ast_node*> result = { {}, nullptr };

	peg::for_each_child<peg::operation_definition>(root,
		[this, &hasAnonymous, &usedNames, &operationName, &result](const peg::ast_node& operationDefinition)
	{
		std::string operationType(strQuery);

		peg::on_first_child<peg::operation_type>(operationDefinition,
			[&operationType](const peg::ast_node& child)
		{
			operationType = child.string_view();
		});

		std::string name;

		peg::on_first_child<peg::operation_name>(operationDefinition,
			[&name](const peg::ast_node& child)
		{
			name = child.string_view();
		});

		std::vector<schema_error> errors;
		auto position = operationDefinition.begin();

		// http://spec.graphql.org/June2018/#sec-Operation-Name-Uniqueness
		if (!usedNames.insert(name).second)
		{
			std::ostringstream message;

			if (name.empty())
			{
				message << "Multiple anonymous operations";
			}
			else
			{
				message << "Duplicate named operations name: " << name;
			}

			errors.push_back({ message.str(), { position.line, position.byte_in_line } });
		}

		hasAnonymous = hasAnonymous || name.empty();

		// http://spec.graphql.org/June2018/#sec-Lone-Anonymous-Operation
		if (name.empty()
			? usedNames.size() > 1
			: hasAnonymous)
		{
			std::ostringstream message;

			if (name.empty())
			{
				message << "Unexpected anonymous operation";
			}
			else
			{
				message << "Unexpected named operation name: " << name;
			}

			errors.push_back({ message.str(), { position.line, position.byte_in_line } });
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

			errors.push_back({ message.str(), { position.line, position.byte_in_line } });
		}

		if (!errors.empty())
		{
			throw schema_exception(std::move(errors));
		}
		else if (operationName.empty()
			|| name == operationName)
		{
			result = { std::move(operationType), &operationDefinition };
		}
	});

	return result;
}

std::future<response::Value> Request::resolve(const std::shared_ptr<RequestState>& state, const peg::ast_node& root, const std::string& operationName, response::Value&& variables) const
{
	return resolve(std::launch::deferred, state, root, operationName, std::move(variables));
}

std::future<response::Value> Request::resolve(std::launch launch, const std::shared_ptr<RequestState>& state, const peg::ast_node& root, const std::string& operationName, response::Value&& variables) const
{
	try
	{
		// http://spec.graphql.org/June2018/#sec-Executable-Definitions
		for (const auto& child : root.children)
		{
			if (!child->is_type<peg::fragment_definition>()
				&& !child->is_type<peg::operation_definition>())
			{
				auto position = child->begin();
				std::ostringstream message;

				message << "Unexpected type definition";

				throw schema_exception { { schema_error{ message.str(), { position.line, position.byte_in_line } } } };
			}
		}

		FragmentDefinitionVisitor fragmentVisitor(variables);

		peg::for_each_child<peg::fragment_definition>(root,
			[&fragmentVisitor](const peg::ast_node& child)
		{
			fragmentVisitor.visit(child);
		});

		auto fragments = fragmentVisitor.getFragments();
		auto operationDefinition = findOperationDefinition(root, operationName);

		if (!operationDefinition.second)
		{
			std::ostringstream message;

			message << "Missing operation";

			if (!operationName.empty())
			{
				message << " name: " << operationName;
			}

			throw schema_exception { { message.str() } };
		}
		else if (operationDefinition.first == strSubscription)
		{
			auto position = operationDefinition.second->begin();
			std::ostringstream message;

			message << "Unexpected subscription";

			if (!operationName.empty())
			{
				message << " name: " << operationName;
			}

			throw schema_exception { { schema_error{ message.str(), { position.line, position.byte_in_line } } } };
		}

		// http://spec.graphql.org/June2018/#sec-Normal-and-Serial-Execution
		if (operationDefinition.first == strMutation)
		{
			// Force mutations to perform serial execution
			launch = std::launch::deferred;
		}

		OperationDefinitionVisitor operationVisitor(launch, state, _operations, std::move(variables), std::move(fragments));

		operationVisitor.visit(operationDefinition.first, *operationDefinition.second);

		return operationVisitor.getValue();
	}
	catch (schema_exception & ex)
	{
		std::promise<response::Value> promise;
		response::Value document(response::Type::Map);

		document.emplace_back(std::string { strData }, response::Value());
		document.emplace_back(std::string { strErrors }, ex.getErrors());
		promise.set_value(std::move(document));

		return promise.get_future();
	}
}

SubscriptionKey Request::subscribe(SubscriptionParams&& params, SubscriptionCallback&& callback)
{
	FragmentDefinitionVisitor fragmentVisitor(params.variables);

	peg::for_each_child<peg::fragment_definition>(*params.query.root,
		[&fragmentVisitor](const peg::ast_node& child)
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

		throw schema_exception { { message.str() } };
	}
	else if (operationDefinition.first != strSubscription)
	{
		auto position = operationDefinition.second->begin();
		std::ostringstream message;

		message << "Unexpected operation type: " << operationDefinition.first;

		if (!params.operationName.empty())
		{
			message << " name: " << params.operationName;
		}

		throw schema_exception { { schema_error{ message.str(), { position.line, position.byte_in_line } } } };
	}

	auto itr = _operations.find(std::string { strSubscription });
	SubscriptionDefinitionVisitor subscriptionVisitor(std::move(params), std::move(callback), std::move(fragments), itr->second);

	peg::for_each_child<peg::operation_definition>(subscriptionVisitor.getRoot(),
		[&subscriptionVisitor](const peg::ast_node& child)
	{
		subscriptionVisitor.visit(child);
	});

	auto registration = subscriptionVisitor.getRegistration();
	auto key = _nextKey++;

	_listeners[registration->field].insert(key);
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

	auto itrListener = _listeners.find(itrSubscription->second->field);

	itrListener->second.erase(key);
	if (itrListener->second.empty())
	{
		_listeners.erase(itrListener);
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

void Request::deliver(const SubscriptionName& name, const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, subscriptionObject);
}

void Request::deliver(std::launch launch, const SubscriptionName& name, const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(launch, name, SubscriptionArguments {}, subscriptionObject);
}

void Request::deliver(const SubscriptionName& name, const SubscriptionArguments& arguments, const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, arguments, subscriptionObject);
}

void Request::deliver(std::launch launch, const SubscriptionName& name, const SubscriptionArguments& arguments, const std::shared_ptr<Object>& subscriptionObject) const
{
	SubscriptionFilterCallback exactMatch = [&arguments](response::MapType::const_reference required) noexcept -> bool
	{
		auto itrArgument = arguments.find(required.first);

		return (itrArgument != arguments.cend()
			&& itrArgument->second == required.second);
	};

	deliver(launch, name, exactMatch, subscriptionObject);
}

void Request::deliver(const SubscriptionName& name, const SubscriptionFilterCallback& apply, const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, apply, subscriptionObject);
}

void Request::deliver(std::launch launch, const SubscriptionName& name, const SubscriptionFilterCallback& apply, const std::shared_ptr<Object>& subscriptionObject) const
{
	const auto& optionalOrDefaultSubscription = subscriptionObject
		? subscriptionObject
		: _operations.find(std::string { strSubscription })->second;

	auto itrListeners = _listeners.find(name);

	if (itrListeners == _listeners.cend())
	{
		return;
	}

	std::queue<std::future<void>> callbacks;

	for (const auto& key : itrListeners->second)
	{
		auto itrSubscription = _subscriptions.find(key);
		auto registration = itrSubscription->second;
		const auto& subscriptionArguments = registration->arguments;
		bool matchedArguments = true;

		// If the field in this subscription had arguments that did not match what was provided
		// in this event, don't deliver the event to this subscription
		for (const auto& required : subscriptionArguments)
		{
			if (!apply(required))
			{
				matchedArguments = false;
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
			emptyFragmentDirectives,
			{},
			launch,
		};

		try
		{
			result = std::async(launch,
				[registration](std::future<response::Value> document)
			{
				return document.get();
			}, optionalOrDefaultSubscription->resolve(selectionSetParams, registration->selection, registration->data->fragments, registration->data->variables));
		}
		catch (schema_exception & ex)
		{
			std::promise<response::Value> promise;
			response::Value document(response::Type::Map);

			document.emplace_back(std::string { strData }, response::Value());
			document.emplace_back(std::string { strErrors }, ex.getErrors());

			result = promise.get_future();
		}

		callbacks.push(std::async(launch,
			[registration](std::future<response::Value> document)
		{
			registration->callback(std::move(document));
		}, std::move(result)));
	}

	while (!callbacks.empty())
	{
		callbacks.front().get();
		callbacks.pop();
	}
}

} /* namespace graphql::service */
