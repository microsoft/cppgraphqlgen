// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLService.h"

#include "graphqlservice/internal/Grammar.h"

#include "Validation.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <thread>

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
	errorLocation.emplace_back(std::string { strLine },
		response::Value(static_cast<response::IntType>(location.line)));
	errorLocation.emplace_back(std::string { strColumn },
		response::Value(static_cast<response::IntType>(location.column)));

	response::Value errorLocations(response::Type::List);

	errorLocations.reserve(1);
	errorLocations.emplace_back(std::move(errorLocation));

	error.emplace_back(std::string { strLocations }, std::move(errorLocations));
}

void addErrorPath(const error_path& path, response::Value& error)
{
	if (path.empty())
	{
		return;
	}

	response::Value errorPath(response::Type::List);

	errorPath.reserve(path.size());
	for (const auto& segment : path)
	{
		if (std::holds_alternative<std::string_view>(segment))
		{
			errorPath.emplace_back(
				response::Value { std::string { std::get<std::string_view>(segment) } });
		}
		else if (std::holds_alternative<size_t>(segment))
		{
			errorPath.emplace_back(
				response::Value(static_cast<response::IntType>(std::get<size_t>(segment))));
		}
	}

	error.emplace_back(std::string { strPath }, std::move(errorPath));
}

error_path buildErrorPath(const std::optional<field_path>& path)
{
	error_path result;

	if (path)
	{
		std::list<std::reference_wrapper<const path_segment>> segments;

		for (auto segment = std::make_optional(std::cref(*path)); segment;
			 segment = segment->get().parent)
		{
			segments.push_front(std::cref(segment->get().segment));
		}

		result.reserve(segments.size());
		std::transform(segments.cbegin(),
			segments.cend(),
			std::back_inserter(result),
			[](const auto& segment) noexcept {
				return segment.get();
			});
	}

	return result;
}

response::Value buildErrorValues(std::list<schema_error>&& structuredErrors)
{
	response::Value errors(response::Type::List);

	errors.reserve(structuredErrors.size());

	for (auto& error : structuredErrors)
	{
		response::Value entry(response::Type::Map);

		entry.reserve(3);
		addErrorMessage(std::move(error.message), entry);
		addErrorLocation(error.location, entry);
		addErrorPath(error.path, entry);

		errors.emplace_back(std::move(entry));
	}

	return errors;
}

schema_exception::schema_exception(std::list<schema_error>&& structuredErrors)
	: _structuredErrors(std::move(structuredErrors))
{
}

schema_exception::schema_exception(std::vector<std::string>&& messages)
	: schema_exception(convertMessages(std::move(messages)))
{
}

std::list<schema_error> schema_exception::convertMessages(
	std::vector<std::string>&& messages) noexcept
{
	std::list<schema_error> errors;

	std::transform(messages.begin(),
		messages.end(),
		std::back_inserter(errors),
		[](std::string& message) noexcept {
			return schema_error { std::move(message) };
		});

	return errors;
}

const char* schema_exception::what() const noexcept
{
	const char* message = nullptr;

	if (!_structuredErrors.empty())
	{
		message = _structuredErrors.front().message.c_str();
	}

	return (message == nullptr) ? "Unknown schema error" : message;
}

std::list<schema_error> schema_exception::getStructuredErrors() noexcept
{
	auto structuredErrors = std::move(_structuredErrors);

	return structuredErrors;
}

response::Value schema_exception::getErrors()
{
	return buildErrorValues(std::move(_structuredErrors));
}

FieldParams::FieldParams(SelectionSetParams&& selectionSetParams, response::Value&& directives)
	: SelectionSetParams(std::move(selectionSetParams))
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
	const auto name = variable.string_view().substr(1);
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
	_value = response::Value(std::string { stringValue.unescaped_view() });
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

std::string_view Fragment::getType() const
{
	return _type;
}

const peg::ast_node& Fragment::getSelection() const
{
	return _selection.get();
}

const response::Value& Fragment::getDirectives() const
{
	return _directives;
}

ResolverParams::ResolverParams(const SelectionSetParams& selectionSetParams,
	const peg::ast_node& field, std::string&& fieldName, response::Value&& arguments,
	response::Value&& fieldDirectives, const peg::ast_node* selection, const FragmentMap& fragments,
	const response::Value& variables)
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

	return { position.line, position.column };
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

	response::IdType result;

	try
	{
		result = value.get<response::IdType>();
	}
	catch (const std::logic_error& ex)
	{
		throw schema_exception { { ex.what() } };
	}

	return result;
}

void await_async::await_suspend(coro::coroutine_handle<> h) const
{
	std::thread(
		[](coro::coroutine_handle<>&& h) noexcept {
			h.resume();
		},
		std::move(h))
		.detach();
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
			buildErrorPath(params.errorPath) } } };
	}
}

template <>
AwaitableResolver ModifiedResult<response::IntType>::convert(
	FieldResult<response::IntType>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return resolve(std::move(result),
		std::move(params),
		[](response::IntType&& value, const ResolverParams&) {
			return response::Value(value);
		});
}

template <>
AwaitableResolver ModifiedResult<response::FloatType>::convert(
	FieldResult<response::FloatType>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return resolve(std::move(result),
		std::move(params),
		[](response::FloatType&& value, const ResolverParams&) {
			return response::Value(value);
		});
}

template <>
AwaitableResolver ModifiedResult<response::StringType>::convert(
	FieldResult<response::StringType>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return resolve(std::move(result),
		std::move(params),
		[](response::StringType&& value, const ResolverParams&) {
			return response::Value(std::move(value));
		});
}

template <>
AwaitableResolver ModifiedResult<response::BooleanType>::convert(
	FieldResult<response::BooleanType>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return resolve(std::move(result),
		std::move(params),
		[](response::BooleanType&& value, const ResolverParams&) {
			return response::Value(value);
		});
}

template <>
AwaitableResolver ModifiedResult<response::Value>::convert(
	FieldResult<response::Value>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return resolve(std::move(result),
		std::move(params),
		[](response::Value&& value, const ResolverParams&) {
			return response::Value(std::move(value));
		});
}

template <>
AwaitableResolver ModifiedResult<response::IdType>::convert(
	FieldResult<response::IdType>&& result, ResolverParams&& params)
{
	blockSubFields(params);

	return resolve(std::move(result),
		std::move(params),
		[](response::IdType&& value, const ResolverParams&) {
			return response::Value(value);
		});
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
			buildErrorPath(params.errorPath) } } };
	}
}

template <>
AwaitableResolver ModifiedResult<Object>::convert(
	FieldResult<std::shared_ptr<Object>>&& result, ResolverParams&& params)
{
	auto pendingResult = std::move(result);
	auto pendingParams = std::move(params);

	requireSubFields(pendingParams);

	if ((pendingParams.launch & std::launch::async) == std::launch::async)
	{
		co_await await_async {};
	}

	auto awaitedResult = co_await pendingResult;

	if (!awaitedResult)
	{
		co_return ResolverResult {};
	}

	auto document = co_await awaitedResult->resolve(pendingParams,
		*pendingParams.selection,
		pendingParams.fragments,
		pendingParams.variables);

	co_return std::move(document);
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
		const ResolverMap& resolvers, size_t count);

	void visit(const peg::ast_node& selection);

	std::vector<std::pair<std::string_view, AwaitableResolver>> getValues();

private:
	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	const ResolverContext _resolverContext;
	const std::shared_ptr<RequestState>& _state;
	const response::Value& _operationDirectives;
	const std::optional<std::reference_wrapper<const field_path>> _path;
	const std::launch _launch;
	const FragmentMap& _fragments;
	const response::Value& _variables;
	const TypeNames& _typeNames;
	const ResolverMap& _resolvers;

	std::list<FragmentDirectives> _fragmentDirectives;
	internal::string_view_set _names;
	std::vector<std::pair<std::string_view, AwaitableResolver>> _values;
};

SelectionVisitor::SelectionVisitor(const SelectionSetParams& selectionSetParams,
	const FragmentMap& fragments, const response::Value& variables, const TypeNames& typeNames,
	const ResolverMap& resolvers, size_t count)
	: _resolverContext(selectionSetParams.resolverContext)
	, _state(selectionSetParams.state)
	, _operationDirectives(selectionSetParams.operationDirectives)
	, _path(selectionSetParams.errorPath
			  ? std::make_optional(std::cref(*selectionSetParams.errorPath))
			  : std::nullopt)
	, _launch(selectionSetParams.launch)
	, _fragments(fragments)
	, _variables(variables)
	, _typeNames(typeNames)
	, _resolvers(resolvers)
{
	_fragmentDirectives.push_back({ response::Value(response::Type::Map),
		response::Value(response::Type::Map),
		response::Value(response::Type::Map) });

	_names.reserve(count);
	_values.reserve(count);
}

std::vector<std::pair<std::string_view, AwaitableResolver>> SelectionVisitor::getValues()
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

	std::string_view alias;

	peg::on_first_child<peg::alias_name>(field, [&alias](const peg::ast_node& child) {
		alias = child.string_view();
	});

	if (alias.empty())
	{
		alias = name;
	}

	if (!_names.emplace(alias).second)
	{
		// Skip resolving fields which map to the same response name as a field we've already
		// resolved. Validation should handle merging multiple references to the same field or
		// to compatible fields.
		return;
	}

	const auto itrResolver = _resolvers.find(name);

	if (itrResolver == _resolvers.end())
	{
		std::promise<ResolverResult> promise;
		auto position = field.begin();
		std::ostringstream error;

		error << "Unknown field name: " << name;

		promise.set_exception(
			std::make_exception_ptr(schema_exception { { schema_error { error.str(),
				{ position.line, position.column },
				buildErrorPath(_path ? std::make_optional(_path->get()) : std::nullopt) } } }));

		_values.push_back({ alias, promise.get_future() });
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

	const SelectionSetParams selectionSetParams {
		_resolverContext,
		_state,
		_operationDirectives,
		_fragmentDirectives.back().fragmentDefinitionDirectives,
		_fragmentDirectives.back().fragmentSpreadDirectives,
		_fragmentDirectives.back().inlineFragmentDirectives,
		std::make_optional(field_path { _path, path_segment { alias } }),
		_launch,
	};

	try
	{
		auto result = itrResolver->second(ResolverParams(selectionSetParams,
			field,
			std::string(alias),
			std::move(arguments),
			directiveVisitor.getDirectives(),
			selection,
			_fragments,
			_variables));

		_values.push_back({ alias, std::move(result) });
	}
	catch (schema_exception& scx)
	{
		std::promise<ResolverResult> promise;
		auto position = field.begin();
		auto messages = scx.getStructuredErrors();

		for (auto& message : messages)
		{
			if (message.location.line == 0)
			{
				message.location = { position.line, position.column };
			}

			if (message.path.empty())
			{
				message.path = buildErrorPath(selectionSetParams.errorPath);
			}
		}

		promise.set_exception(std::make_exception_ptr(schema_exception { std::move(messages) }));

		_values.push_back({ alias, promise.get_future() });
	}
	catch (const std::exception& ex)
	{
		std::promise<ResolverResult> promise;
		auto position = field.begin();
		std::ostringstream message;

		message << "Field error name: " << alias << " unknown error: " << ex.what();

		promise.set_exception(
			std::make_exception_ptr(schema_exception { { schema_error { message.str(),
				{ position.line, position.column },
				buildErrorPath(selectionSetParams.errorPath) } } }));

		_values.push_back({ alias, promise.get_future() });
	}
}

void SelectionVisitor::visitFragmentSpread(const peg::ast_node& fragmentSpread)
{
	const auto name = fragmentSpread.children.front()->string_view();
	auto itr = _fragments.find(name);

	if (itr == _fragments.end())
	{
		auto position = fragmentSpread.begin();
		std::ostringstream error;

		error << "Unknown fragment name: " << name;

		throw schema_exception { { schema_error { error.str(),
			{ position.line, position.column },
			buildErrorPath(_path ? std::make_optional(_path->get()) : std::nullopt) } } };
	}

	bool skip = (_typeNames.find(itr->second.getType()) == _typeNames.end());
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
	for (const auto& entry : _fragmentDirectives.back().fragmentSpreadDirectives)
	{
		if (fragmentSpreadDirectives.find(entry.first) == fragmentSpreadDirectives.end())
		{
			fragmentSpreadDirectives.emplace_back(std::string { entry.first },
				response::Value(entry.second));
		}
	}

	response::Value fragmentDefinitionDirectives(itr->second.getDirectives());

	// Merge outer fragment definition directives as long as they don't conflict.
	for (const auto& entry : _fragmentDirectives.back().fragmentDefinitionDirectives)
	{
		if (fragmentDefinitionDirectives.find(entry.first) == fragmentDefinitionDirectives.end())
		{
			fragmentDefinitionDirectives.emplace_back(std::string { entry.first },
				response::Value(entry.second));
		}
	}

	_fragmentDirectives.push_back({ std::move(fragmentDefinitionDirectives),
		std::move(fragmentSpreadDirectives),
		response::Value(_fragmentDirectives.back().inlineFragmentDirectives) });

	const size_t count = itr->second.getSelection().children.size();

	if (count > 1)
	{
		_names.reserve(_names.capacity() + count - 1);
		_values.reserve(_values.capacity() + count - 1);
	}

	for (const auto& selection : itr->second.getSelection().children)
	{
		visit(*selection);
	}

	_fragmentDirectives.pop_back();
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

	if (typeCondition == nullptr
		|| _typeNames.find(typeCondition->children.front()->string_view()) != _typeNames.end())
	{
		peg::on_first_child<peg::selection_set>(inlineFragment,
			[this, &directiveVisitor](const peg::ast_node& child) {
				auto inlineFragmentDirectives = directiveVisitor.getDirectives();

				// Merge outer inline fragment directives as long as they don't conflict.
				for (const auto& entry : _fragmentDirectives.back().inlineFragmentDirectives)
				{
					if (inlineFragmentDirectives.find(entry.first)
						== inlineFragmentDirectives.end())
					{
						inlineFragmentDirectives.emplace_back(std::string { entry.first },
							response::Value(entry.second));
					}
				}

				_fragmentDirectives.push_back(
					{ response::Value(_fragmentDirectives.back().fragmentDefinitionDirectives),
						response::Value(_fragmentDirectives.back().fragmentSpreadDirectives),
						std::move(inlineFragmentDirectives) });

				const size_t count = child.children.size();

				if (count > 1)
				{
					_names.reserve(_names.capacity() + count - 1);
					_values.reserve(_values.capacity() + count - 1);
				}

				for (const auto& selection : child.children)
				{
					visit(*selection);
				}

				_fragmentDirectives.pop_back();
			});
	}
}

Object::Object(TypeNames&& typeNames, ResolverMap&& resolvers)
	: _typeNames(std::move(typeNames))
	, _resolvers(std::move(resolvers))
{
}

AwaitableResolver Object::resolve(const SelectionSetParams& selectionSetParams,
	const peg::ast_node& selection, const FragmentMap& fragments,
	const response::Value& variables) const
{
	SelectionVisitor visitor(selectionSetParams,
		fragments,
		variables,
		_typeNames,
		_resolvers,
		selection.children.size());

	beginSelectionSet(selectionSetParams);

	for (const auto& child : selection.children)
	{
		visitor.visit(*child);
	}

	endSelectionSet(selectionSetParams);

	auto children = visitor.getValues();
	const auto launch = selectionSetParams.launch;
	ResolverResult document { response::Value { response::Type::Map } };

	document.data.reserve(children.size());

	for (auto& child : children)
	{
		auto name = child.first;

		try
		{
			if ((launch & std::launch::async) == std::launch::async)
			{
				co_await await_async {};
			}

			auto value = co_await child.second;

			if (!document.data.emplace_back(std::string { name }, std::move(value.data)))
			{
				std::ostringstream message;

				message << "Ambiguous field error name: " << name;

				document.errors.push_back({ message.str() });
			}

			if (!value.errors.empty())
			{
				document.errors.splice(document.errors.end(), value.errors);
			}
		}
		catch (schema_exception& scx)
		{
			auto errors = scx.getStructuredErrors();

			if (!errors.empty())
			{
				std::copy(errors.begin(), errors.end(), std::back_inserter(document.errors));
			}

			document.data.emplace_back(std::string { name }, {});
		}
		catch (const std::exception& ex)
		{
			std::ostringstream message;

			message << "Field error name: " << name << " unknown error: " << ex.what();

			document.errors.push_back({ message.str() });
			document.data.emplace_back(std::string { name }, {});
		}
	}

	co_return std::move(document);
}

bool Object::matchesType(std::string_view typeName) const
{
	return _typeNames.find(typeName) != _typeNames.end();
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
	_fragments.emplace(fragmentDefinition.children.front()->string_view(),
		Fragment(fragmentDefinition, _variables));
}

// OperationDefinitionVisitor visits the AST and executes the one with the specified
// operation name.
class OperationDefinitionVisitor
{
public:
	OperationDefinitionVisitor(ResolverContext resolverContext, std::launch launch,
		std::shared_ptr<RequestState> state, const TypeMap& operations, response::Value&& variables,
		FragmentMap&& fragments);

	AwaitableResolver getValue();

	void visit(std::string_view operationType, const peg::ast_node& operationDefinition);

private:
	const ResolverContext _resolverContext;
	const std::launch _launch;
	std::shared_ptr<OperationData> _params;
	const TypeMap& _operations;
	std::optional<AwaitableResolver> _result;
};

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

AwaitableResolver OperationDefinitionVisitor::getValue()
{
	if (!_result)
	{
		co_return ResolverResult {};
	}

	auto result = std::move(*_result);

	if ((_launch & std::launch::async) == std::launch::async)
	{
		co_await await_async {};
	}

	co_return co_await result;
}

void OperationDefinitionVisitor::visit(
	std::string_view operationType, const peg::ast_node& operationDefinition)
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

	const response::Value emptyFragmentDirectives(response::Type::Map);
	const SelectionSetParams selectionSetParams {
		_resolverContext,
		_params->state,
		_params->directives,
		emptyFragmentDirectives,
		emptyFragmentDirectives,
		emptyFragmentDirectives,
		std::nullopt,
		_launch,
	};

	_result = std::make_optional(itr->second->resolve(selectionSetParams,
		*operationDefinition.children.back(),
		_params->fragments,
		_params->variables));
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
	std::string_view name;

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
	const auto name = fragmentSpread.children.front()->string_view();
	auto itr = _fragments.find(name);

	if (itr == _fragments.end())
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

Request::Request(TypeMap&& operationTypes, const std::shared_ptr<schema::Schema>& schema)
	: _operations(std::move(operationTypes))
	, _validation(std::make_unique<ValidateExecutableVisitor>(schema))
{
}

Request::~Request()
{
	// The default implementation is fine, but it can't be declared as = default because it needs to
	// know how to destroy the _validation member and it can't do that with just a forward
	// declaration of the class.
}

std::list<schema_error> Request::validate(peg::ast& query) const
{
	std::list<schema_error> errors;

	if (!query.validated)
	{
		_validation->visit(*query.root);
		errors = _validation->getStructuredErrors();
		query.validated = errors.empty();
	}

	return errors;
}

std::pair<std::string_view, const peg::ast_node*> Request::findOperationDefinition(
	peg::ast& query, std::string_view operationName) const
{
	// Ensure the query has been validated.
	auto errors = validate(query);

	if (!errors.empty())
	{
		throw schema_exception { std::move(errors) };
	}

	std::pair<std::string_view, const peg::ast_node*> result = { {}, nullptr };

	peg::on_first_child_if<peg::operation_definition>(*query.root,
		[this, &operationName, &result](const peg::ast_node& operationDefinition) noexcept -> bool {
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

response::AwaitableValue Request::resolve(const std::shared_ptr<RequestState>& state,
	peg::ast& query, const std::string& operationName, response::Value&& variables) const
{
	return resolve(std::launch::deferred, state, query, operationName, std::move(variables));
}

response::AwaitableValue Request::resolve(std::launch launch,
	const std::shared_ptr<RequestState>& state, peg::ast& query, const std::string& operationName,
	response::Value&& variables) const
{
	try
	{
		FragmentDefinitionVisitor fragmentVisitor(variables);

		peg::for_each_child<peg::fragment_definition>(*query.root,
			[&fragmentVisitor](const peg::ast_node& child) {
				fragmentVisitor.visit(child);
			});

		auto fragments = fragmentVisitor.getFragments();
		auto operationDefinition = findOperationDefinition(query, operationName);

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
		const auto resolverContext =
			isMutation ? ResolverContext::Mutation : ResolverContext::Query;
		// http://spec.graphql.org/June2018/#sec-Normal-and-Serial-Execution
		const auto operationLaunch = isMutation ? std::launch::deferred : launch;

		OperationDefinitionVisitor operationVisitor(resolverContext,
			operationLaunch,
			state,
			_operations,
			std::move(variables),
			std::move(fragments));

		operationVisitor.visit(operationDefinition.first, *operationDefinition.second);

		if ((launch & std::launch::async) == std::launch::async)
		{
			co_await await_async {};
		}

		auto result = co_await operationVisitor.getValue();
		response::Value document { response::Type::Map };

		document.emplace_back(std::string { strData }, std::move(result.data));

		if (!result.errors.empty())
		{
			document.emplace_back(std::string { strErrors },
				buildErrorValues(std::move(result.errors)));
		}

		co_return std::move(document);
	}
	catch (schema_exception& ex)
	{
		response::Value document(response::Type::Map);

		document.emplace_back(std::string { strData }, response::Value());
		document.emplace_back(std::string { strErrors }, ex.getErrors());

		co_return std::move(document);
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
	auto operationDefinition = findOperationDefinition(params.query, params.operationName);

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

	auto itr = _operations.find(strSubscription);
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

	_listeners[registration->field].emplace(key);
	_subscriptions.emplace(key, std::move(registration));

	return key;
}

internal::Awaitable<SubscriptionKey> Request::subscribe(
	std::launch launch, SubscriptionParams&& params, SubscriptionCallback&& callback)
{
	const auto spThis = shared_from_this();
	const auto key = spThis->subscribe(std::move(params), std::move(callback));
	const auto itrOperation = spThis->_operations.find(strSubscription);

	if (itrOperation != spThis->_operations.end())
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
			{},
			launch,
		};

		try
		{
			if ((launch & std::launch::async) == std::launch::async)
			{
				co_await await_async {};
			}

			co_await operation->resolve(selectionSetParams,
				registration->selection,
				registration->data->fragments,
				registration->data->variables);
		}
		catch (const std::exception& ex)
		{
			// Rethrow the exception, but don't leave it subscribed if the resolver failed.
			spThis->unsubscribe(key);
			throw ex;
		}
	}

	co_return key;
}

void Request::unsubscribe(SubscriptionKey key)
{
	auto itrSubscription = _subscriptions.find(key);

	if (itrSubscription == _subscriptions.end())
	{
		return;
	}

	const auto listenerKey = std::string_view { itrSubscription->second->field };
	auto& listener = _listeners.at(listenerKey);

	listener.erase(key);
	if (listener.empty())
	{
		_listeners.erase(listenerKey);
	}

	_subscriptions.erase(itrSubscription);

	if (_subscriptions.empty())
	{
		_nextKey = 0;
	}
	else
	{
		_nextKey = _subscriptions.rbegin()->first + 1;
	}
}

internal::Awaitable<void> Request::unsubscribe(std::launch launch, SubscriptionKey key)
{
	const auto spThis = shared_from_this();
	const auto itrOperation = spThis->_operations.find(strSubscription);

	if (itrOperation != spThis->_operations.end())
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
			{},
			launch,
		};

		if ((launch & std::launch::async) == std::launch::async)
		{
			co_await await_async {};
		}

		co_await operation->resolve(selectionSetParams,
			registration->selection,
			registration->data->fragments,
			registration->data->variables);
	}

	spThis->unsubscribe(key);

	co_return;
}

void Request::deliver(
	const SubscriptionName& name, const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, subscriptionObject).get();
}

void Request::deliver(const SubscriptionName& name, const SubscriptionArguments& arguments,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, arguments, subscriptionObject).get();
}

void Request::deliver(const SubscriptionName& name, const SubscriptionArguments& arguments,
	const SubscriptionArguments& directives,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, arguments, directives, subscriptionObject).get();
}

void Request::deliver(const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, applyArguments, subscriptionObject).get();
}

void Request::deliver(const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const SubscriptionFilterCallback& applyDirectives,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	deliver(std::launch::deferred, name, applyArguments, applyDirectives, subscriptionObject).get();
}

internal::Awaitable<void> Request::deliver(std::launch launch, const SubscriptionName& name,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	return deliver(launch,
		name,
		SubscriptionArguments {},
		SubscriptionArguments {},
		subscriptionObject);
}

internal::Awaitable<void> Request::deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionArguments& arguments, const std::shared_ptr<Object>& subscriptionObject) const
{
	return deliver(launch, name, arguments, SubscriptionArguments {}, subscriptionObject);
}

internal::Awaitable<void> Request::deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionArguments& arguments, const SubscriptionArguments& directives,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	SubscriptionFilterCallback argumentsMatch =
		[&arguments](response::MapType::const_reference required) noexcept -> bool {
		auto itrArgument = arguments.find(required.first);

		return (itrArgument != arguments.end() && itrArgument->second == required.second);
	};

	SubscriptionFilterCallback directivesMatch =
		[&directives](response::MapType::const_reference required) noexcept -> bool {
		auto itrDirective = directives.find(required.first);

		return (itrDirective != directives.end() && itrDirective->second == required.second);
	};

	return deliver(launch, name, argumentsMatch, directivesMatch, subscriptionObject);
}

internal::Awaitable<void> Request::deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	return deliver(
		launch,
		name,
		applyArguments,
		[](response::MapType::const_reference) noexcept {
			return true;
		},
		subscriptionObject);
}

internal::Awaitable<void> Request::deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const SubscriptionFilterCallback& applyDirectives,
	const std::shared_ptr<Object>& subscriptionObject) const
{
	const auto itrOperation = _operations.find(strSubscription);

	if (itrOperation == _operations.end())
	{
		// There may be an empty entry in the operations map, but if it's completely missing then
		// that means the schema doesn't support subscriptions at all.
		throw std::logic_error("Subscriptions not supported");
	}

	const auto optionalOrDefaultSubscription =
		subscriptionObject ? subscriptionObject : itrOperation->second;

	if (!optionalOrDefaultSubscription)
	{
		// If there is no default in the operations map, you must pass a non-empty
		// subscriptionObject parameter to deliver.
		throw std::invalid_argument("Missing subscriptionObject");
	}

	auto itrListeners = _listeners.find(name);

	if (itrListeners == _listeners.end())
	{
		co_return;
	}

	std::vector<std::shared_ptr<SubscriptionData>> registrations;

	registrations.reserve(itrListeners->second.size());
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

		registrations.push_back(std::move(registration));
	}

	for (const auto& registration : registrations)
	{
		response::Value emptyFragmentDirectives(response::Type::Map);
		const SelectionSetParams selectionSetParams {
			ResolverContext::Subscription,
			registration->data->state,
			registration->data->directives,
			emptyFragmentDirectives,
			emptyFragmentDirectives,
			emptyFragmentDirectives,
			std::nullopt,
			launch,
		};

		response::Value document { response::Type::Map };

		try
		{
			if ((launch & std::launch::async) == std::launch::async)
			{
				co_await await_async {};
			}

			auto result = co_await optionalOrDefaultSubscription->resolve(selectionSetParams,
				registration->selection,
				registration->data->fragments,
				registration->data->variables);

			document.emplace_back(std::string { strData }, std::move(result.data));

			if (!result.errors.empty())
			{
				document.emplace_back(std::string { strErrors },
					buildErrorValues(std::move(result.errors)));
			}
		}
		catch (schema_exception& ex)
		{
			document.emplace_back(std::string { strData }, response::Value());
			document.emplace_back(std::string { strErrors }, ex.getErrors());
		}

		registration->callback(std::move(document));
	}

	co_return;
}

} // namespace graphql::service
