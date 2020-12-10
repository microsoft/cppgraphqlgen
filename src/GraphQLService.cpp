// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLService.h"
#include "graphqlservice/GraphQLGrammar.h"

#include "Validation.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <list>
#include <stack>

namespace graphql::service {

void addErrorMessage(std::string&& message, response::Value& error)
{
	graphql::response::addErrorMessage(std::move(message), error);
}

void addErrorLocation(const schema_location& location, response::Value& error)
{
	graphql::response::addErrorLocation(location, error);
}

void addErrorPath(field_path&& path, response::Value& error)
{
	graphql::response::addErrorPath(std::move(path), error);
}

response::Value buildErrorValues(const std::vector<schema_error>& structuredErrors)
{
	return graphql::response::buildErrorValues(structuredErrors);
}

schema_exception::schema_exception(std::vector<schema_error>&& structuredErrors)
	: graphql::error::schema_exception(std::move(structuredErrors))
	, _errors(buildErrorValues(std::as_const(*this).getStructuredErrors()))
{
}

schema_exception::schema_exception(std::vector<std::string>&& messages)
	: graphql::service::schema_exception(convertMessages(std::move(messages)))
{
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
	else if (value.is_type<peg::true_keyword>() || value.is_type<peg::false_keyword>())
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

		throw schema_exception {
			{ schema_error { error.str(), { position.line, position.column } } }
		};
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
			[&directiveName](const peg::ast_node& child) {
				directiveName = child.string_view();
			});

		if (directiveName.empty())
		{
			continue;
		}

		response::Value directiveArguments(response::Type::Map);

		peg::on_first_child<peg::arguments>(*directive,
			[this, &directiveArguments](const peg::ast_node& child) {
				ValueVisitor visitor(_variables);

				for (auto& argument : child.children)
				{
					visitor.visit(*argument->children.back());

					directiveArguments.emplace_back(argument->children.front()->string(),
						visitor.getValue());
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
			if (argumentTrue || argumentFalse || argument.second.type() != response::Type::Boolean
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

			error << "Missing argument directive: " << entry.second << " name: if";

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
		[this, &variables](const peg::ast_node& child) {
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

ResolverParams::ResolverParams(const SelectionSetParams& selectionSetParams,
	const peg::ast_node& field, const std::string_view& fieldName, response::Value&& arguments,
	response::Value&& fieldDirectives, const peg::ast_node* selection, const FragmentMap& fragments,
	const response::Value& variables)
	: SelectionSetParams(selectionSetParams)
	, field(field)
	, fieldName(fieldName)
	, arguments(std::move(arguments))
	, fieldDirectives(std::move(fieldDirectives))
	, selection(selection)
	, fragments(fragments)
	, variables(variables)
{
}

ResolverParams::ResolverParams(const ResolverParams& parent, field_path&& ownErrorPath_)
	: SelectionSetParams(parent, std::move(ownErrorPath_))
	, field(parent.field)
	, fieldName(parent.fieldName)
	, arguments(parent.arguments)
	, fieldDirectives(parent.fieldDirectives)
	, selection(parent.selection)
	, fragments(parent.fragments)
	, variables(parent.variables)
{
}

schema_location ResolverParams::getLocation() const
{
	auto position = field.begin();

	return { position.line, position.column };
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
	while (count >= 4 && encoded[3] != padding)
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
				throw schema_exception {
					{ "invalid padding at the end of a base64 encoded string" }
				};
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
				throw schema_exception {
					{ "invalid padding at the end of a base64 encoded string" }
				};
			}

			result.emplace_back(static_cast<uint8_t>((segment & 0xFF00) >> 8));

			encoded += 2;
			count -= 2;
		}
	}

	// Make sure anything that's left is 0 - 2 characters of padding
	if ((count > 0 && padding != encoded[0]) || (count > 1 && padding != encoded[1]) || count > 2)
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
			| (static_cast<uint32_t>(data[1]) << 8) | static_cast<uint32_t>(data[2]);

		result.append({ verifyToBase64(static_cast<uint8_t>((segment & 0xFC0000) >> 18)),
			verifyToBase64(static_cast<uint8_t>((segment & 0x3F000) >> 12)),
			verifyToBase64(static_cast<uint8_t>((segment & 0xFC0) >> 6)),
			verifyToBase64(static_cast<uint8_t>(segment & 0x3F)) });

		data += 3;
		count -= 3;
	}

	// Get any leftover partial segment with 1 or 2 bytes
	if (count > 0)
	{
		const bool pair = (count > 1);
		const uint16_t segment =
			(static_cast<uint16_t>(data[0]) << 8) | (pair ? static_cast<uint16_t>(data[1]) : 0);
		const std::array<char, 4> remainder { verifyToBase64(
												  static_cast<uint8_t>((segment & 0xFC00) >> 10)),
			verifyToBase64(static_cast<uint8_t>((segment & 0x3F0) >> 4)),
			(pair ? verifyToBase64(static_cast<uint8_t>((segment & 0xF) << 2)) : padding),
			padding };

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
	if (value.type() != response::Type::Float && value.type() != response::Type::Int)
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

void blockSubFields(const ResolverParams& params)
{
	// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
	if (params.selection != nullptr)
	{
		auto position = params.selection->begin();
		std::ostringstream error;

		error << "Field may not have sub-fields name: " << params.fieldName;

		throw schema_exception { { schema_error { error.str(),
			{ position.line, position.column },
			{ params.errorPath() } } } };
	}
}

template <>
std::future<response::Value> ModifiedResult<response::IntType>::convert(
	FieldResult<response::IntType>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return result.get_future_result();
}

template <>
std::future<response::Value> ModifiedResult<response::FloatType>::convert(
	FieldResult<response::FloatType>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return result.get_future_result();
}

template <>
std::future<response::Value> ModifiedResult<response::StringType>::convert(
	FieldResult<response::StringType>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return result.get_future_result();
}

template <>
std::future<response::Value> ModifiedResult<response::BooleanType>::convert(
	FieldResult<response::BooleanType>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return result.get_future_result();
}

template <>
std::future<response::Value> ModifiedResult<response::Value>::convert(
	FieldResult<response::Value>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return result.get_future_result();
}

template <>
std::future<response::Value> ModifiedResult<response::IdType>::convert(
	FieldResult<response::IdType>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	std::string (*converter)(const response::IdType&) = &Base64::toBase64;

	return result.get_future_result(converter);
}

void requireSubFields(const ResolverParams& params)
{
	// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
	if (params.selection == nullptr)
	{
		auto position = params.field.begin();
		std::ostringstream error;

		error << "Field must have sub-fields name: " << params.fieldName;

		throw schema_exception { { schema_error { error.str(),
			{ position.line, position.column },
			{ params.errorPath() } } } };
	}
}

template <>
std::future<response::Value> ModifiedResult<Object>::convert(
	FieldResult<std::shared_ptr<Object>>&& result, ResolverParams&& params)
{
	requireSubFields(params);

	return std::async(
		params.launch,
		[](FieldResult<std::shared_ptr<Object>>&& resultFuture, ResolverParams&& paramsFuture) {
			auto wrappedResult = resultFuture.get();

			if (!wrappedResult)
			{
				return response::Value();
			}

			return wrappedResult
				->resolve(paramsFuture,
					*paramsFuture.selection,
					paramsFuture.fragments,
					paramsFuture.variables)
				.get();
		},
		std::move(result),
		std::move(params));
}

// As we recursively expand fragment spreads and inline fragments, we want to accumulate the
// directives at each location and merge them with any directives included in outer fragments to
// build the complete set of directives for nested fragments. Directives with the same name at the
// same location will be overwritten by the innermost fragment.
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
		const FragmentMap& fragments, const response::Value& variables, const TypeNames& typeNames,
		const ResolverMap& resolvers);

	void visit(const peg::ast_node& selection);

	std::list<std::pair<std::string, std::future<response::Value>>> getValues();

private:
	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	const SelectionSetParams& _selectionSetParams;
	const FragmentMap& _fragments;
	const response::Value& _variables;
	const TypeNames& _typeNames;
	const ResolverMap& _resolvers;

	std::stack<FragmentDirectives> _fragmentDirectives;
	std::unordered_set<std::string_view> _names;
	std::list<std::pair<std::string, std::future<response::Value>>> _values;
};

SelectionVisitor::SelectionVisitor(const SelectionSetParams& selectionSetParams,
	const FragmentMap& fragments, const response::Value& variables, const TypeNames& typeNames,
	const ResolverMap& resolvers)
	: _selectionSetParams(selectionSetParams)
	, _fragments(fragments)
	, _variables(variables)
	, _typeNames(typeNames)
	, _resolvers(resolvers)
{
	_fragmentDirectives.push({ response::Value(response::Type::Map),
		response::Value(response::Type::Map),
		response::Value(response::Type::Map) });
}

std::list<std::pair<std::string, std::future<response::Value>>> SelectionVisitor::getValues()
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
	std::string_view name;

	peg::on_first_child<peg::field_name>(field, [&name](const peg::ast_node& child) {
		name = child.string_view();
	});

	std::string_view aliasView = name;

	peg::on_first_child<peg::alias_name>(field, [&aliasView](const peg::ast_node& child) {
		aliasView = child.string_view();
	});

	if (!_names.insert(aliasView).second)
	{
		// Skip resolving fields which map to the same response name as a field we've already
		// resolved. Validation should handle merging multiple references to the same field or
		// to compatible fields.
		return;
	}

	std::string alias(aliasView);

	const auto [itr, itrEnd] = std::equal_range(_resolvers.cbegin(),
		_resolvers.cend(),
		std::make_pair(std::string_view { name }, Resolver {}),
		[](const auto& lhs, const auto& rhs) noexcept {
			return lhs.first < rhs.first;
		});

	if (itr == itrEnd)
	{
		std::promise<response::Value> promise;
		auto position = field.begin();
		std::ostringstream error;

		error << "Unknown field name: " << name;

		promise.set_exception(
			std::make_exception_ptr(schema_exception { { schema_error { error.str(),
				{ position.line, position.column },
				{ _selectionSetParams.errorPath() } } } }));

		_values.push_back({ std::move(alias), promise.get_future() });
		return;
	}

	DirectiveVisitor directiveVisitor(_variables);

	peg::on_first_child<peg::directives>(field, [&directiveVisitor](const peg::ast_node& child) {
		directiveVisitor.visit(child);
	});

	if (directiveVisitor.shouldSkip())
	{
		return;
	}

	response::Value arguments(response::Type::Map);

	peg::on_first_child<peg::arguments>(field, [this, &arguments](const peg::ast_node& child) {
		ValueVisitor visitor(_variables);

		for (auto& argument : child.children)
		{
			visitor.visit(*argument->children.back());

			arguments.emplace_back(argument->children.front()->string(), visitor.getValue());
		}
	});

	const peg::ast_node* selection = nullptr;

	peg::on_first_child<peg::selection_set>(field, [&selection](const peg::ast_node& child) {
		selection = &child;
	});

	SelectionSetParams selectionSetParams {
		_selectionSetParams.resolverContext,
		_selectionSetParams.state,
		_selectionSetParams.operationDirectives,
		_fragmentDirectives.top().fragmentDefinitionDirectives,
		_fragmentDirectives.top().fragmentSpreadDirectives,
		_fragmentDirectives.top().inlineFragmentDirectives,
		_selectionSetParams,
		{ { aliasView } },
		_selectionSetParams.launch,
	};

	try
	{
		auto result = itr->second(ResolverParams(selectionSetParams,
			field,
			aliasView,
			std::move(arguments),
			directiveVisitor.getDirectives(),
			selection,
			_fragments,
			_variables));

		_values.push_back({ std::move(alias), std::move(result) });
	}
	catch (schema_exception& scx)
	{
		std::promise<response::Value> promise;
		auto position = field.begin();
		auto messages = scx.getStructuredErrors();

		for (auto& message : messages)
		{
			if (message.location == graphql::error::emptyLocation)
			{
				message.location = { position.line, position.column };
			}

			if (message.path.empty())
			{
				message.path = { selectionSetParams.errorPath() };
			}
		}

		promise.set_exception(std::make_exception_ptr(schema_exception { std::move(messages) }));

		_values.push_back({ std::move(alias), promise.get_future() });
	}
	catch (const std::exception& ex)
	{
		std::promise<response::Value> promise;
		auto position = field.begin();
		std::ostringstream message;

		message << "Field error name: " << alias << " unknown error: " << ex.what();

		promise.set_exception(
			std::make_exception_ptr(schema_exception { { schema_error { message.str(),
				{ position.line, position.column },
				selectionSetParams.errorPath() } } }));

		_values.push_back({ std::move(alias), promise.get_future() });
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

		throw schema_exception { { schema_error { error.str(),
			{ position.line, position.column },
			{ _selectionSetParams.errorPath() } } } };
	}

	bool skip = (_typeNames.count(itr->second.getType()) == 0);
	DirectiveVisitor directiveVisitor(_variables);

	if (!skip)
	{
		peg::on_first_child<peg::directives>(fragmentSpread,
			[&directiveVisitor](const peg::ast_node& child) {
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
			fragmentSpreadDirectives.emplace_back(std::string { entry.first },
				response::Value(entry.second));
		}
	}

	response::Value fragmentDefinitionDirectives(itr->second.getDirectives());

	// Merge outer fragment definition directives as long as they don't conflict.
	for (const auto& entry : _fragmentDirectives.top().fragmentDefinitionDirectives)
	{
		if (fragmentDefinitionDirectives.find(entry.first) == fragmentDefinitionDirectives.end())
		{
			fragmentDefinitionDirectives.emplace_back(std::string { entry.first },
				response::Value(entry.second));
		}
	}

	_fragmentDirectives.push({ std::move(fragmentDefinitionDirectives),
		std::move(fragmentSpreadDirectives),
		response::Value(_fragmentDirectives.top().inlineFragmentDirectives) });

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
		[&directiveVisitor](const peg::ast_node& child) {
			directiveVisitor.visit(child);
		});

	if (directiveVisitor.shouldSkip())
	{
		return;
	}

	const peg::ast_node* typeCondition = nullptr;

	peg::on_first_child<peg::type_condition>(inlineFragment,
		[&typeCondition](const peg::ast_node& child) {
			typeCondition = &child;
		});

	if (typeCondition == nullptr || _typeNames.count(typeCondition->children.front()->string()) > 0)
	{
		peg::on_first_child<peg::selection_set>(inlineFragment,
			[this, &directiveVisitor](const peg::ast_node& child) {
				auto inlineFragmentDirectives = directiveVisitor.getDirectives();

				// Merge outer inline fragment directives as long as they don't conflict.
				for (const auto& entry : _fragmentDirectives.top().inlineFragmentDirectives)
				{
					if (inlineFragmentDirectives.find(entry.first)
						== inlineFragmentDirectives.end())
					{
						inlineFragmentDirectives.emplace_back(std::string { entry.first },
							response::Value(entry.second));
					}
				}

				_fragmentDirectives.push(
					{ response::Value(_fragmentDirectives.top().fragmentDefinitionDirectives),
						response::Value(_fragmentDirectives.top().fragmentSpreadDirectives),
						std::move(inlineFragmentDirectives) });

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

std::future<response::Value> Object::resolve(const SelectionSetParams& selectionSetParams,
	const peg::ast_node& selection, const FragmentMap& fragments,
	const response::Value& variables) const
{
	std::list<std::pair<std::string, std::future<response::Value>>> selections;

	beginSelectionSet(selectionSetParams);

	for (const auto& child : selection.children)
	{
		SelectionVisitor visitor(selectionSetParams, fragments, variables, _typeNames, _resolvers);

		visitor.visit(*child);

		selections.splice(selections.end(), visitor.getValues());
	}

	endSelectionSet(selectionSetParams);

	return std::async(
		selectionSetParams.launch,
		[](std::list<std::pair<std::string, std::future<response::Value>>>&& children) {
			response::Value data(response::Type::Map);
			std::vector<schema_error> errors;

			while (!children.empty())
			{
				auto name = std::move(children.front().first);

				try
				{
					auto value = children.front().second.get();
					if (value.type() == response::Type::Result)
					{
						auto result = value.release<response::ResultType>();
						if (result.errors.size())
						{
							errors.reserve(errors.size() + result.errors.size());
							for (auto& error : result.errors)
							{
								errors.emplace_back(std::move(error));
							}
						}
						value = std::move(result.data);
					}

					try
					{
						data.emplace_back(std::move(name), std::move(value));
					}
					catch (std::runtime_error& e)
					{
						// Duplicate Map member
						std::ostringstream message;

						message << "Ambiguous field error name: " << name;
						errors.emplace_back(schema_error { message.str() });
					}
				}
				catch (schema_exception& scx)
				{
					auto messages = scx.getStructuredErrors();

					errors.reserve(errors.size() + messages.size());
					for (auto& error : messages)
					{
						errors.emplace_back(std::move(error));
					}

					try
					{
						data.emplace_back(std::move(name), {});
					}
					catch (std::runtime_error& e)
					{
						// Duplicate Map member
					}
				}
				catch (const std::exception& ex)
				{
					std::ostringstream message;

					message << "Field error name: " << name << " unknown error: " << ex.what();

					errors.emplace_back(schema_error { message.str() });

					try
					{
						data.emplace_back(std::move(name), {});
					}
					catch (std::runtime_error& e)
					{
						// Duplicate Map member
					}
				}

				children.pop_front();
			}

			if (errors.size() == 0)
			{
				return std::move(data);
			}

			return response::Value(response::ResultType { std::move(data), std::move(errors) });
		},
		std::move(selections));
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
	_fragments.insert({ fragmentDefinition.children.front()->string(),
		Fragment(fragmentDefinition, _variables) });
}

// OperationDefinitionVisitor visits the AST and executes the one with the specified
// operation name.
class OperationDefinitionVisitor
{
public:
	OperationDefinitionVisitor(ResolverContext resolverContext, std::launch launch,
		std::shared_ptr<RequestState> state, const TypeMap& operations, response::Value&& variables,
		FragmentMap&& fragments);

	std::future<response::Value> getValue();

	void visit(const std::string_view& operationType, const peg::ast_node& operationDefinition);

private:
	const ResolverContext _resolverContext;
	const std::launch _launch;
	std::shared_ptr<OperationData> _params;
	const TypeMap& _operations;
	std::future<response::Value> _result;
};

OperationDefinitionVisitor::OperationDefinitionVisitor(ResolverContext resolverContext,
	std::launch launch, std::shared_ptr<RequestState> state, const TypeMap& operations,
	response::Value&& variables, FragmentMap&& fragments)
	: _resolverContext(resolverContext)
	, _launch(launch)
	, _params(std::make_shared<OperationData>(
		  std::move(state), std::move(variables), response::Value(), std::move(fragments)))
	, _operations(operations)
{
}

std::future<response::Value> OperationDefinitionVisitor::getValue()
{
	auto result = std::move(_result);

	return result;
}

void OperationDefinitionVisitor::visit(
	const std::string_view& operationType, const peg::ast_node& operationDefinition)
{
	auto itr = _operations.find(operationType);

	// Filter the variable definitions down to the ones referenced in this operation
	response::Value operationVariables(response::Type::Map);

	peg::for_each_child<peg::variable>(operationDefinition,
		[this, &operationVariables](const peg::ast_node& variable) {
			std::string variableName;

			peg::on_first_child<peg::variable_name>(variable,
				[&variableName](const peg::ast_node& name) {
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
					[this, &valueVar](const peg::ast_node& defaultValue) {
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
		[this, &operationDirectives](const peg::ast_node& child) {
			DirectiveVisitor directiveVisitor(_params->variables);

			directiveVisitor.visit(child);
			operationDirectives = directiveVisitor.getDirectives();
		});

	_params->directives = std::move(operationDirectives);

	// Keep the params alive until the deferred lambda has executed
	_result = std::async(
		_launch,
		[selectionContext = _resolverContext,
			selectionLaunch = _launch,
			params = std::move(_params),
			operation = itr->second](const peg::ast_node& selection) {
			// The top level object doesn't come from inside of a fragment, so all of the fragment
			// directives are empty.
			const response::Value emptyFragmentDirectives(response::Type::Map);
			const SelectionSetParams selectionSetParams {
				selectionContext,
				params->state,
				params->directives,
				emptyFragmentDirectives,
				emptyFragmentDirectives,
				emptyFragmentDirectives,
				std::nullopt,
				{},
				selectionLaunch,
			};

			return operation
				->resolve(selectionSetParams, selection, params->fragments, params->variables)
				.get();
		},
		std::cref(*operationDefinition.children.back()));
}

SubscriptionData::SubscriptionData(std::shared_ptr<OperationData>&& data, SubscriptionName&& field,
	response::Value&& arguments, response::Value&& fieldDirectives, peg::ast&& query,
	std::string&& operationName, SubscriptionCallback&& callback, const peg::ast_node& selection)
	: data(std::move(data))
	, field(std::move(field))
	, arguments(std::move(arguments))
	, fieldDirectives(std::move(fieldDirectives))
	, query(std::move(query))
	, operationName(std::move(operationName))
	, callback(std::move(callback))
	, selection(selection)
{
}

// SubscriptionDefinitionVisitor visits the AST collects the fields referenced in the subscription
// at the point where we create a subscription.
class SubscriptionDefinitionVisitor
{
public:
	SubscriptionDefinitionVisitor(SubscriptionParams&& params, SubscriptionCallback&& callback,
		FragmentMap&& fragments, const std::shared_ptr<Object>& subscriptionObject);

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
	response::Value _fieldDirectives;
	std::shared_ptr<SubscriptionData> _result;
};

SubscriptionDefinitionVisitor::SubscriptionDefinitionVisitor(SubscriptionParams&& params,
	SubscriptionCallback&& callback, FragmentMap&& fragments,
	const std::shared_ptr<Object>& subscriptionObject)
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
		[this, &directives](const peg::ast_node& child) {
			DirectiveVisitor directiveVisitor(_params.variables);

			directiveVisitor.visit(child);
			directives = directiveVisitor.getDirectives();
		});

	_result =
		std::make_shared<SubscriptionData>(std::make_shared<OperationData>(std::move(_params.state),
											   std::move(_params.variables),
											   std::move(directives),
											   std::move(_fragments)),
			std::move(_field),
			std::move(_arguments),
			std::move(_fieldDirectives),
			std::move(_params.query),
			std::move(_params.operationName),
			std::move(_callback),
			selection);
}

void SubscriptionDefinitionVisitor::visitField(const peg::ast_node& field)
{
	std::string name;

	peg::on_first_child<peg::field_name>(field, [&name](const peg::ast_node& child) {
		name = child.string_view();
	});

	// http://spec.graphql.org/June2018/#sec-Single-root-field
	if (!_field.empty())
	{
		auto position = field.begin();
		std::ostringstream error;

		error << "Extra subscription root field name: " << name;

		throw schema_exception {
			{ schema_error { error.str(), { position.line, position.column } } }
		};
	}

	DirectiveVisitor directiveVisitor(_params.variables);

	peg::on_first_child<peg::directives>(field, [&directiveVisitor](const peg::ast_node& child) {
		directiveVisitor.visit(child);
	});

	if (directiveVisitor.shouldSkip())
	{
		return;
	}

	_fieldDirectives = directiveVisitor.getDirectives();

	response::Value arguments(response::Type::Map);

	peg::on_first_child<peg::arguments>(field, [this, &arguments](const peg::ast_node& child) {
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

		throw schema_exception {
			{ schema_error { error.str(), { position.line, position.column } } }
		};
	}

	bool skip = !_subscriptionObject->matchesType(itr->second.getType());
	DirectiveVisitor directiveVisitor(_params.variables);

	if (!skip)
	{
		peg::on_first_child<peg::directives>(fragmentSpread,
			[&directiveVisitor](const peg::ast_node& child) {
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
		[&directiveVisitor](const peg::ast_node& child) {
			directiveVisitor.visit(child);
		});

	if (directiveVisitor.shouldSkip())
	{
		return;
	}

	const peg::ast_node* typeCondition = nullptr;

	peg::on_first_child<peg::type_condition>(inlineFragment,
		[&typeCondition](const peg::ast_node& child) {
			typeCondition = &child;
		});

	if (typeCondition == nullptr
		|| _subscriptionObject->matchesType(typeCondition->children.front()->string()))
	{
		peg::on_first_child<peg::selection_set>(inlineFragment, [this](const peg::ast_node& child) {
			for (const auto& selection : child.children)
			{
				visit(*selection);
			}
		});
	}
}

Request::Request(TypeMap&& operationTypes)
	: _operations(std::move(operationTypes))
	, _validationContext(std::make_unique<IntrospectionValidationContext>(*this))
	, _validation(std::make_unique<ValidateExecutableVisitor>(*_validationContext))
{
}

Request::Request(TypeMap&& operationTypes, std::unique_ptr<ValidationContext>&& validationContext)
	: _operations(std::move(operationTypes))
	, _validationContext(validationContext
			  ? std::move(validationContext)
			  : std::make_unique<IntrospectionValidationContext>(*this))
	, _validation(std::make_unique<ValidateExecutableVisitor>(*_validationContext))
{
}

Request::~Request()
{
	// The default implementation is fine, but it can't be declared as = default because it needs to
	// know how to destroy the _validation member and it can't do that with just a forward
	// declaration of the class.
}

std::vector<schema_error> Request::validate(peg::ast& query) const
{
	std::vector<schema_error> errors;

	if (!query.validated)
	{
		_validation->visit(*query.root);
		errors = _validation->getStructuredErrors();
		query.validated = errors.empty();
	}

	return errors;
}

std::pair<std::string, const peg::ast_node*> Request::findOperationDefinition(
	const peg::ast_node& root, const std::string& operationName) const noexcept
{
	auto result = findOperationDefinition(root, std::string_view { operationName });

	return std::pair<std::string, const peg::ast_node*> { std::string { result.first },
		result.second };
}

std::pair<std::string_view, const peg::ast_node*> Request::findOperationDefinition(
	const peg::ast_node& root, const std::string_view& operationName) const noexcept
{
	bool hasAnonymous = false;
	std::pair<std::string_view, const peg::ast_node*> result = { {}, nullptr };

	peg::find_child<peg::operation_definition>(root,
		[this, &hasAnonymous, &operationName, &result](const peg::ast_node& operationDefinition) {
			std::string_view operationType = strQuery;

			peg::on_first_child<peg::operation_type>(operationDefinition,
				[&operationType](const peg::ast_node& child) {
					operationType = child.string_view();
				});

			std::string_view name;

			peg::on_first_child<peg::operation_name>(operationDefinition,
				[&name](const peg::ast_node& child) {
					name = child.string_view();
				});

			if (operationName.empty() || name == operationName)
			{
				result = { operationType, &operationDefinition };
				return true;
			}

			return false;
		});

	return result;
}

std::future<response::Value> Request::resolve(const std::shared_ptr<RequestState>& state,
	const peg::ast_node& root, const std::string& operationName, response::Value&& variables) const
{
	return resolveValidated(std::launch::deferred,
		state,
		root,
		operationName,
		std::move(variables));
}

std::future<response::Value> Request::resolve(std::launch launch,
	const std::shared_ptr<RequestState>& state, const peg::ast_node& root,
	const std::string& operationName, response::Value&& variables) const
{
	return resolveValidated(launch, state, root, operationName, std::move(variables));
}

std::future<response::Value> Request::resolve(const std::shared_ptr<RequestState>& state,
	peg::ast& query, const std::string& operationName, response::Value&& variables) const
{
	return resolve(std::launch::deferred, state, query, operationName, std::move(variables));
}

std::future<response::Value> Request::resolve(std::launch launch,
	const std::shared_ptr<RequestState>& state, peg::ast& query, const std::string& operationName,
	response::Value&& variables) const
{
	auto errors = validate(query);

	if (!errors.empty())
	{
		std::promise<response::Value> promise;
		response::Value document(response::Type::Map);

		document.emplace_back(std::string { strData }, response::Value());
		document.emplace_back(std::string { strErrors }, buildErrorValues(errors));
		promise.set_value(std::move(document));

		return promise.get_future();
	}

	return std::async(
		launch,
		[](std::future<response::Value> result) {
			auto value = result.get();
			if (value.type() == response::Type::Result)
			{
				return value.toMap();
			}

			response::Value document(response::Type::Map);
			document.emplace_back(std::string { strData }, std::move(value));
			return std::move(document);
		},
		resolveValidated(launch, state, *query.root, operationName, std::move(variables)));
}

std::future<response::Value> Request::resolveValidated(std::launch launch,
	const std::shared_ptr<RequestState>& state, const peg::ast_node& root,
	const std::string& operationName, response::Value&& variables) const
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

				throw schema_exception {
					{ schema_error { message.str(), { position.line, position.column } } }
				};
			}
		}

		FragmentDefinitionVisitor fragmentVisitor(variables);

		peg::for_each_child<peg::fragment_definition>(root,
			[&fragmentVisitor](const peg::ast_node& child) {
				fragmentVisitor.visit(child);
			});

		auto fragments = fragmentVisitor.getFragments();
		auto operationDefinition =
			findOperationDefinition(root, std::string_view { operationName });

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

			throw schema_exception {
				{ schema_error { message.str(), { position.line, position.column } } }
			};
		}

		const bool isMutation = (operationDefinition.first == strMutation);

		// http://spec.graphql.org/June2018/#sec-Normal-and-Serial-Execution
		if (isMutation)
		{
			// Force mutations to perform serial execution
			launch = std::launch::deferred;
		}

		const auto resolverContext =
			isMutation ? ResolverContext::Mutation : ResolverContext::Query;

		OperationDefinitionVisitor operationVisitor(resolverContext,
			launch,
			state,
			_operations,
			std::move(variables),
			std::move(fragments));

		operationVisitor.visit(operationDefinition.first, *operationDefinition.second);

		return operationVisitor.getValue();
	}
	catch (schema_exception& ex)
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
	auto errors = validate(params.query);

	if (!errors.empty())
	{
		throw schema_exception { std::move(errors) };
	}

	FragmentDefinitionVisitor fragmentVisitor(params.variables);

	peg::for_each_child<peg::fragment_definition>(*params.query.root,
		[&fragmentVisitor](const peg::ast_node& child) {
			fragmentVisitor.visit(child);
		});

	auto fragments = fragmentVisitor.getFragments();
	auto operationDefinition =
		findOperationDefinition(*params.query.root, std::string_view { params.operationName });

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

		throw schema_exception {
			{ schema_error { message.str(), { position.line, position.column } } }
		};
	}

	auto itr = _operations.find(std::string { strSubscription });
	SubscriptionDefinitionVisitor subscriptionVisitor(std::move(params),
		std::move(callback),
		std::move(fragments),
		itr->second);

	peg::for_each_child<peg::operation_definition>(subscriptionVisitor.getRoot(),
		[&subscriptionVisitor](const peg::ast_node& child) {
			subscriptionVisitor.visit(child);
		});

	auto registration = subscriptionVisitor.getRegistration();
	auto key = _nextKey++;

	_listeners[registration->field].insert(key);
	_subscriptions.emplace(key, std::move(registration));

	return key;
}

std::future<SubscriptionKey> Request::subscribe(
	std::launch launch, SubscriptionParams&& params, SubscriptionCallback&& callback)
{
	return std::async(
		launch,
		[spThis = shared_from_this(), launch](SubscriptionParams&& paramsFuture,
			SubscriptionCallback&& callbackFuture) {
			const auto key = spThis->subscribe(std::move(paramsFuture), std::move(callbackFuture));
			const auto itrOperation = spThis->_operations.find(std::string { strSubscription });

			if (itrOperation != spThis->_operations.cend())
			{
				const auto& operation = itrOperation->second;
				const auto& registration = spThis->_subscriptions.at(key);
				response::Value emptyFragmentDirectives(response::Type::Map);
				const SelectionSetParams selectionSetParams {
					ResolverContext::NotifySubscribe,
					registration->data->state,
					registration->data->directives,
					emptyFragmentDirectives,
					emptyFragmentDirectives,
					emptyFragmentDirectives,
					std::nullopt,
					{},
					launch,
				};

				try
				{
					operation
						->resolve(selectionSetParams,
							registration->selection,
							registration->data->fragments,
							registration->data->variables)
						.get();
				}
				catch (const std::exception& ex)
				{
					// Rethrow the exception, but don't leave it subscribed if the resolver failed.
					spThis->unsubscribe(key);
					throw ex;
				}
			}

			return key;
		},
		std::move(params),
		std::move(callback));
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

std::future<void> Request::unsubscribe(std::launch launch, SubscriptionKey key)
{
	return std::async(launch, [spThis = shared_from_this(), launch, key]() {
		const auto itrOperation = spThis->_operations.find(std::string { strSubscription });

		if (itrOperation != spThis->_operations.cend())
		{
			const auto& operation = itrOperation->second;
			const auto& registration = spThis->_subscriptions.at(key);
			response::Value emptyFragmentDirectives(response::Type::Map);
			const SelectionSetParams selectionSetParams {
				ResolverContext::NotifyUnsubscribe,
				registration->data->state,
				registration->data->directives,
				emptyFragmentDirectives,
				emptyFragmentDirectives,
				emptyFragmentDirectives,
				std::nullopt,
				{},
				launch,
			};

			operation
				->resolve(selectionSetParams,
					registration->selection,
					registration->data->fragments,
					registration->data->variables)
				.get();
		}

		spThis->unsubscribe(key);
	});
}

void Request::deliver(
	const SubscriptionName& name, const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, subscriptionObject);
}

void Request::deliver(const SubscriptionName& name, const SubscriptionArguments& arguments,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, arguments, subscriptionObject);
}

void Request::deliver(const SubscriptionName& name, const SubscriptionArguments& arguments,
	const SubscriptionArguments& directives,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, arguments, directives, subscriptionObject);
}

void Request::deliver(const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, applyArguments, subscriptionObject);
}

void Request::deliver(const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const SubscriptionFilterCallback& applyDirectives,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, applyArguments, applyDirectives, subscriptionObject);
}

void Request::deliver(std::launch launch, const SubscriptionName& name,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(launch, name, SubscriptionArguments {}, SubscriptionArguments {}, subscriptionObject);
}

void Request::deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionArguments& arguments, const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(launch, name, arguments, SubscriptionArguments {}, subscriptionObject);
}

void Request::deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionArguments& arguments, const SubscriptionArguments& directives,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	SubscriptionFilterCallback argumentsMatch =
		[&arguments](response::MapType::const_reference required) noexcept -> bool {
		auto itrArgument = arguments.find(required.first);

		return (itrArgument != arguments.cend() && itrArgument->second == required.second);
	};

	SubscriptionFilterCallback directivesMatch =
		[&directives](response::MapType::const_reference required) noexcept -> bool {
		auto itrDirective = directives.find(required.first);

		return (itrDirective != directives.cend() && itrDirective->second == required.second);
	};

	deliver(launch, name, argumentsMatch, directivesMatch, subscriptionObject);
}

void Request::deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(
		launch,
		name,
		applyArguments,
		[](response::MapType::const_reference) noexcept {
			return true;
		},
		subscriptionObject);
}

void Request::deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const SubscriptionFilterCallback& applyDirectives,
	const std::shared_ptr<Object>& subscriptionObject) const
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
			if (!applyArguments(required))
			{
				matchedArguments = false;
				break;
			}
		}

		if (!matchedArguments)
		{
			continue;
		}

		// If the field in this subscription had field directives that did not match what was
		// provided in this event, don't deliver the event to this subscription
		const auto& subscriptionFieldDirectives = registration->fieldDirectives;
		bool matchedFieldDirectives = true;

		for (const auto& required : subscriptionFieldDirectives)
		{
			if (!applyDirectives(required))
			{
				matchedFieldDirectives = false;
				break;
			}
		}

		if (!matchedFieldDirectives)
		{
			continue;
		}

		std::future<response::Value> result;
		response::Value emptyFragmentDirectives(response::Type::Map);
		const SelectionSetParams selectionSetParams {
			ResolverContext::Subscription,
			registration->data->state,
			registration->data->directives,
			emptyFragmentDirectives,
			emptyFragmentDirectives,
			emptyFragmentDirectives,
			std::nullopt,
			{},
			launch,
		};

		try
		{
			result = std::async(
				launch,
				[registration](std::future<response::Value> document) {
					auto value = document.get();
					if (value.type() == response::Type::Result)
					{
						return value.toMap();
					}

					response::Value result(response::Type::Map);
					result.emplace_back(std::string { strData }, std::move(value));
					return std::move(result);
				},
				optionalOrDefaultSubscription->resolve(selectionSetParams,
					registration->selection,
					registration->data->fragments,
					registration->data->variables));
		}
		catch (schema_exception& ex)
		{
			std::promise<response::Value> promise;
			response::Value document(response::Type::Map);

			document.emplace_back(std::string { strData }, response::Value());
			document.emplace_back(std::string { strErrors }, ex.getErrors());
			promise.set_value(std::move(document));

			result = promise.get_future();
		}

		callbacks.push(std::async(
			launch,
			[registration](std::future<response::Value> document) {
				registration->callback(std::move(document));
			},
			std::move(result)));
	}

	while (!callbacks.empty())
	{
		callbacks.front().get();
		callbacks.pop();
	}
}

} /* namespace graphql::service */
