// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "RequestLoader.h"

#include "graphqlservice/GraphQLGrammar.h"

#include <array>
#include <sstream>

namespace graphql::runtime {

using namespace service;

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

std::vector<std::pair<std::string_view, std::future<ResolverResult>>> SelectionVisitor::getValues()
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

std::future<ResolverResult> OperationDefinitionVisitor::getValue()
{
	auto result = std::move(_result);

	return result;
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
				selectionLaunch,
			};

			return operation
				->resolve(selectionSetParams, selection, params->fragments, params->variables)
				.get();
		},
		std::cref(*operationDefinition.children.back()));
}

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

} /* namespace graphql::runtime */
