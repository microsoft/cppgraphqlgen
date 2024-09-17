// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Validation.h"

#include "graphqlservice/internal/Base64.h"
#include "graphqlservice/internal/Grammar.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace std::literals;

namespace graphql::service {

SharedType getSharedType(const ValidateType& type) noexcept
{
	return type ? type->get().shared_from_this() : SharedType {};
}

ValidateType getValidateType(const SharedType& type) noexcept
{
	return type ? std::make_optional(std::cref(*type)) : std::nullopt;
}

bool operator==(const ValidateType& lhs, const ValidateType& rhs) noexcept
{
	// Equal if they're either both std::nullopt or they are both not empty and the addresses of the
	// references match.
	return (lhs ? (rhs && &lhs->get() == &rhs->get()) : !rhs);
}

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
	return (!value ? !other.value : (other.value && value->data == other.value->data));
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

ValidateArgumentValue::ValidateArgumentValue(int value)
	: data(value)
{
}

ValidateArgumentValue::ValidateArgumentValue(double value)
	: data(value)
{
}

ValidateArgumentValue::ValidateArgumentValue(std::string_view value)
	: data(std::move(value))
{
}

ValidateArgumentValue::ValidateArgumentValue(bool value)
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

ValidateArgumentValueVisitor::ValidateArgumentValueVisitor(std::list<schema_error>& errors)
	: _errors(errors)
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

void ValidateArgumentValueVisitor::visitVariable(const peg::ast_node& variable)
{
	ValidateArgumentVariable value { variable.string_view().substr(1) };
	auto position = variable.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitIntValue(const peg::ast_node& intValue)
{
	int value { std::atoi(intValue.string().c_str()) };
	auto position = intValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(value);
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitFloatValue(const peg::ast_node& floatValue)
{
	double value { std::atof(floatValue.string().c_str()) };
	auto position = floatValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(value);
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitStringValue(const peg::ast_node& stringValue)
{
	std::string_view value { stringValue.unescaped_view() };
	auto position = stringValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(value);
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitBooleanValue(const peg::ast_node& booleanValue)
{
	bool value { booleanValue.is_type<peg::true_keyword>() };
	auto position = booleanValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(value);
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitNullValue(const peg::ast_node& nullValue)
{
	auto position = nullValue.begin();

	_argumentValue.value.reset();
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitEnumValue(const peg::ast_node& enumValue)
{
	ValidateArgumentEnumValue value { enumValue.string_view() };
	auto position = enumValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitListValue(const peg::ast_node& listValue)
{
	ValidateArgumentList value;
	auto position = listValue.begin();

	value.values.reserve(listValue.children.size());

	for (const auto& child : listValue.children)
	{
		ValidateArgumentValueVisitor visitor(_errors);

		visitor.visit(*child);
		value.values.emplace_back(visitor.getArgumentValue());
	}

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitObjectValue(const peg::ast_node& objectValue)
{
	ValidateArgumentMap value;
	auto position = objectValue.begin();

	for (const auto& field : objectValue.children)
	{
		auto name = field->children.front()->string_view();

		if (value.values.find(name) != value.values.end())
		{
			// https://spec.graphql.org/October2021/#sec-Input-Object-Field-Uniqueness
			auto fieldPosition = field->begin();
			auto message = std::format("Conflicting input field name: {}", name);

			_errors.push_back({ std::move(message), { fieldPosition.line, fieldPosition.column } });
			continue;
		}

		ValidateArgumentValueVisitor visitor(_errors);

		visitor.visit(*field->children.back());
		value.values[std::move(name)] = visitor.getArgumentValue();
	}

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.column };
}

ValidateField::ValidateField(ValidateType&& returnType, ValidateType&& objectType,
	std::string_view fieldName, ValidateFieldArguments&& arguments)
	: returnType(std::move(returnType))
	, objectType(std::move(objectType))
	, fieldName(fieldName)
	, arguments(std::move(arguments))
{
}

bool ValidateField::operator==(const ValidateField& other) const
{
	return (returnType == other.returnType)
		&& ((objectType && other.objectType && &objectType->get() != &other.objectType->get())
			|| (fieldName == other.fieldName && arguments == other.arguments));
}

ValidateVariableTypeVisitor::ValidateVariableTypeVisitor(
	const std::shared_ptr<schema::Schema>& schema, const ValidateTypes& types)
	: _schema(schema)
	, _types(types)
{
}

void ValidateVariableTypeVisitor::visit(const peg::ast_node& typeName)
{
	if (typeName.is_type<peg::nonnull_type>())
	{
		visitNonNullType(typeName);
	}
	else if (typeName.is_type<peg::list_type>())
	{
		visitListType(typeName);
	}
	else if (typeName.is_type<peg::named_type>())
	{
		visitNamedType(typeName);
	}
}

void ValidateVariableTypeVisitor::visitNamedType(const peg::ast_node& namedType)
{
	auto name = namedType.string_view();
	auto itrType = _types.find(name);

	if (itrType == _types.end())
	{
		return;
	}

	switch (itrType->second->get().kind())
	{
		case introspection::TypeKind::SCALAR:
		case introspection::TypeKind::ENUM:
		case introspection::TypeKind::INPUT_OBJECT:
			_isInputType = true;
			_variableType = getValidateType(_schema->LookupType(name));
			break;

		default:
			break;
	}
}

void ValidateVariableTypeVisitor::visitListType(const peg::ast_node& listType)
{
	ValidateVariableTypeVisitor visitor(_schema, _types);

	visitor.visit(*listType.children.front());
	_isInputType = visitor.isInputType();
	_variableType = getValidateType(
		_schema->WrapType(introspection::TypeKind::LIST, getSharedType(visitor.getType())));
}

void ValidateVariableTypeVisitor::visitNonNullType(const peg::ast_node& nonNullType)
{
	ValidateVariableTypeVisitor visitor(_schema, _types);

	visitor.visit(*nonNullType.children.front());
	_isInputType = visitor.isInputType();
	_variableType = getValidateType(
		_schema->WrapType(introspection::TypeKind::NON_NULL, getSharedType(visitor.getType())));
}

bool ValidateVariableTypeVisitor::isInputType() const
{
	return _isInputType;
}

ValidateType ValidateVariableTypeVisitor::getType()
{
	auto result = std::move(_variableType);

	return result;
}

ValidateExecutableVisitor::ValidateExecutableVisitor(std::shared_ptr<schema::Schema> schema)
	: _schema(schema)
{
	const auto& queryType = _schema->queryType();
	const auto& mutationType = _schema->mutationType();
	const auto& subscriptionType = _schema->subscriptionType();

	_operationTypes.reserve(3);

	if (mutationType)
	{
		_operationTypes[strMutation] = getValidateType(mutationType);
	}

	if (queryType)
	{
		_operationTypes[strQuery] = getValidateType(queryType);
	}

	if (subscriptionType)
	{
		_operationTypes[strSubscription] = getValidateType(subscriptionType);
	}

	const auto& types = _schema->types();

	_types.reserve(types.size());

	for (const auto& entry : types)
	{
		const auto name = entry.first;
		const auto kind = entry.second->kind();

		if (!isScalarType(kind))
		{
			auto matchingTypes = std::move(_matchingTypes[name]);

			if (kind == introspection::TypeKind::OBJECT)
			{
				matchingTypes.emplace(name);
			}
			else
			{
				const auto& possibleTypes = entry.second->possibleTypes();

				if (kind == introspection::TypeKind::INTERFACE)
				{
					matchingTypes.reserve(possibleTypes.size() + 1);
					matchingTypes.emplace(name);
				}
				else
				{
					matchingTypes.reserve(possibleTypes.size());
				}

				for (const auto& possibleType : possibleTypes)
				{
					const auto spType = possibleType.lock();

					if (spType)
					{
						matchingTypes.emplace(spType->name());
					}
				}
			}

			if (!matchingTypes.empty())
			{
				_matchingTypes[name] = std::move(matchingTypes);
			}
		}
		else if (kind == introspection::TypeKind::ENUM)
		{
			const auto& enumValues = entry.second->enumValues();
			internal::string_view_set values;

			values.reserve(enumValues.size());

			for (const auto& value : enumValues)
			{
				if (value)
				{
					values.emplace(value->name());
				}
			}

			if (!enumValues.empty())
			{
				_enumValues[name] = std::move(values);
			}
		}
		else if (kind == introspection::TypeKind::SCALAR)
		{
			_scalarTypes.emplace(name);
		}

		_types[name] = getValidateType(entry.second);
	}

	const auto& directives = _schema->directives();

	_directives.reserve(directives.size());

	for (const auto& directive : directives)
	{
		const auto name = directive->name();
		const auto& locations = directive->locations();
		const auto& args = directive->args();
		ValidateDirective validateDirective;

		validateDirective.isRepeatable = directive->isRepeatable();

		for (const auto location : locations)
		{
			validateDirective.locations.emplace(location);
		}

		validateDirective.arguments = getArguments(args);
		_directives[name] = std::move(validateDirective);
	}
}

void ValidateExecutableVisitor::visit(const peg::ast_node& root)
{
	// Visit all of the fragment definitions and check for duplicates.
	peg::for_each_child<peg::fragment_definition>(root,
		[this](const peg::ast_node& fragmentDefinition) {
			const auto& fragmentName = fragmentDefinition.children.front();
			const auto inserted =
				_fragmentDefinitions.emplace(fragmentName->string_view(), fragmentDefinition);

			if (!inserted.second)
			{
				// https://spec.graphql.org/October2021/#sec-Fragment-Name-Uniqueness
				auto position = fragmentDefinition.begin();
				auto error = std::format("Duplicate fragment name: {}", inserted.first->first);

				_errors.push_back({ std::move(error), { position.line, position.column } });
			}
		});

	// Visit all of the operation definitions and check for duplicates.
	peg::for_each_child<peg::operation_definition>(root,
		[this](const peg::ast_node& operationDefinition) {
			std::string_view operationName;

			peg::on_first_child<peg::operation_name>(operationDefinition,
				[&operationName](const peg::ast_node& child) {
					operationName = child.string_view();
				});

			const auto inserted = _operationDefinitions.emplace(operationName, operationDefinition);

			if (!inserted.second)
			{
				// https://spec.graphql.org/October2021/#sec-Operation-Name-Uniqueness
				auto position = operationDefinition.begin();
				auto error = std::format("Duplicate operation name: {}", inserted.first->first);

				_errors.push_back({ std::move(error), { position.line, position.column } });
			}
		});

	// Check for lone anonymous operations.
	if (_operationDefinitions.size() > 1)
	{
		auto itr = std::find_if(_operationDefinitions.begin(),
			_operationDefinitions.end(),
			[](const auto& entry) noexcept {
				return entry.first.empty();
			});

		if (itr != _operationDefinitions.end())
		{
			// https://spec.graphql.org/October2021/#sec-Lone-Anonymous-Operation
			auto position = itr->second.get().begin();

			_errors.push_back(
				{ "Anonymous operation not alone", { position.line, position.column } });
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
			// https://spec.graphql.org/October2021/#sec-Executable-Definitions
			auto position = child->begin();

			_errors.push_back({ "Unexpected type definition", { position.line, position.column } });
		}
	}

	if (!_fragmentDefinitions.empty())
	{
		// https://spec.graphql.org/October2021/#sec-Fragments-Must-Be-Used
		auto unreferencedFragments = std::move(_fragmentDefinitions);

		for (const auto& name : _referencedFragments)
		{
			unreferencedFragments.erase(name);
		}

		std::ranges::transform(unreferencedFragments,
			std::back_inserter(_errors),
			[](const auto& fragmentDefinition) noexcept {
				auto position = fragmentDefinition.second.get().begin();
				auto message =
					std::format("Unused fragment definition name: {}", fragmentDefinition.first);

				return schema_error { std::move(message), { position.line, position.column } };
			});
	}
}

std::list<schema_error> ValidateExecutableVisitor::getStructuredErrors()
{
	auto errors = std::move(_errors);

	// Reset all of the state for this query, but keep the Introspection schema information.
	_fragmentDefinitions.clear();
	_operationDefinitions.clear();
	_referencedFragments.clear();
	_fragmentCycles.clear();

	return errors;
}

void ValidateExecutableVisitor::visitFragmentDefinition(const peg::ast_node& fragmentDefinition)
{
	peg::on_first_child<peg::directives>(fragmentDefinition, [this](const peg::ast_node& child) {
		visitDirectives(introspection::DirectiveLocation::FRAGMENT_DEFINITION, child);
	});

	const auto name = fragmentDefinition.children.front()->string_view();
	const auto& selection = *fragmentDefinition.children.back();
	const auto& typeCondition = fragmentDefinition.children[1];
	auto innerType = typeCondition->children.front()->string_view();

	auto itrType = _types.find(innerType);

	if (itrType == _types.end() || isScalarType(itrType->second->get().kind()))
	{
		// https://spec.graphql.org/October2021/#sec-Fragment-Spread-Type-Existence
		// https://spec.graphql.org/October2021/#sec-Fragments-On-Composite-Types
		auto position = typeCondition->begin();
		auto message = std::format("{} target type on fragment definition: {} name: {}",
			(itrType == _types.end() ? "Undefined" : "Scalar"),
			name,
			innerType);

		_errors.push_back({ std::move(message), { position.line, position.column } });
		return;
	}

	_fragmentStack.emplace(name);
	_scopedType = itrType->second;

	visitSelection(selection);

	_scopedType.reset();
	_fragmentStack.clear();
	_selectionFields.clear();
}

void ValidateExecutableVisitor::visitOperationDefinition(const peg::ast_node& operationDefinition)
{
	auto operationType = strQuery;

	peg::on_first_child<peg::operation_type>(operationDefinition,
		[&operationType](const peg::ast_node& child) {
			operationType = child.string_view();
		});

	std::string_view operationName;

	peg::on_first_child<peg::operation_name>(operationDefinition,
		[&operationName](const peg::ast_node& child) {
			operationName = child.string_view();
		});

	_operationVariables = std::make_optional<VariableTypes>();

	peg::for_each_child<
		peg::variable>(operationDefinition, [this, operationName](const peg::ast_node& variable) {
		std::string_view variableName;
		ValidateArgument variableArgument;

		for (const auto& child : variable.children)
		{
			if (child->is_type<peg::variable_name>())
			{
				// Skip the $ prefix
				variableName = child->string_view().substr(1);

				if (_operationVariables->find(variableName) != _operationVariables->end())
				{
					// https://spec.graphql.org/October2021/#sec-Variable-Uniqueness
					auto position = child->begin();
					auto message = "Conflicting variable"s;

					if (!operationName.empty())
					{
						message += std::format(" operation: {}", operationName);
					}

					message += std::format(" name: {}", variableName);

					_errors.push_back({ std::move(message), { position.line, position.column } });
					return;
				}
			}
			else if (child->is_type<peg::named_type>() || child->is_type<peg::list_type>()
				|| child->is_type<peg::nonnull_type>())
			{
				ValidateVariableTypeVisitor visitor(_schema, _types);

				visitor.visit(*child);

				if (!visitor.isInputType())
				{
					// https://spec.graphql.org/October2021/#sec-Variables-Are-Input-Types
					auto position = child->begin();
					auto message = "Invalid variable type"s;

					if (!operationName.empty())
					{
						message += std::format(" operation: {}", operationName);
					}

					message += std::format(" name: {}", variableName);

					_errors.push_back({ std::move(message), { position.line, position.column } });
					return;
				}

				variableArgument.type = visitor.getType();
			}
			else if (child->is_type<peg::default_value>())
			{
				ValidateArgumentValueVisitor visitor(_errors);

				visitor.visit(*child->children.back());

				auto argument = visitor.getArgumentValue();

				if (!validateInputValue(false, argument, variableArgument.type))
				{
					// https://spec.graphql.org/October2021/#sec-Values-of-Correct-Type
					auto position = child->begin();
					auto message = "Incompatible variable default value"s;

					if (!operationName.empty())
					{
						message += std::format(" operation: {}", operationName);
					}

					message += std::format(" name: {}", variableName);

					_errors.push_back({ std::move(message), { position.line, position.column } });
					return;
				}

				variableArgument.defaultValue = true;
				variableArgument.nonNullDefaultValue = argument.value != nullptr;
			}
		}

		_variableDefinitions.emplace(variableName, variable);
		_operationVariables->emplace(variableName, std::move(variableArgument));
	});

	peg::on_first_child<peg::directives>(operationDefinition,
		[this, &operationType](const peg::ast_node& child) {
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

	auto itrType = _operationTypes.find(operationType);

	if (itrType == _operationTypes.end())
	{
		auto position = operationDefinition.begin();
		auto error = std::format("Unsupported operation type: {}", operationType);

		_errors.push_back({ std::move(error), { position.line, position.column } });
		return;
	}

	_scopedType = itrType->second;
	_introspectionFieldCount = 0;
	_fieldCount = 0;

	const auto& selection = *operationDefinition.children.back();

	visitSelection(selection);

	if (operationType == strSubscription)
	{
		if (_fieldCount > 1)
		{
			// https://spec.graphql.org/October2021/#sec-Single-root-field
			auto position = operationDefinition.begin();
			auto error = "Subscription with more than one root field"s;

			if (!operationName.empty())
			{
				error += std::format(" name: {}", operationName);
			}

			_errors.push_back({ std::move(error), { position.line, position.column } });
		}

		if (_introspectionFieldCount != 0)
		{
			// https://spec.graphql.org/October2021/#sec-Single-root-field
			auto position = operationDefinition.begin();
			auto error = "Subscription with Introspection root field"s;

			if (!operationName.empty())
			{
				error += std::format(" name: {}", operationName);
			}

			_errors.push_back({ std::move(error), { position.line, position.column } });
		}
	}

	_scopedType.reset();
	_fragmentStack.clear();
	_selectionFields.clear();

	for (const auto& variable : _variableDefinitions)
	{
		if (_referencedVariables.find(variable.first) == _referencedVariables.end())
		{
			// https://spec.graphql.org/October2021/#sec-All-Variables-Used
			auto position = variable.second.get().begin();
			auto error = std::format("Unused variable name: {}", variable.first);

			_errors.push_back({ std::move(error), { position.line, position.column } });
		}
	}

	_operationVariables.reset();
	_variableDefinitions.clear();
	_referencedVariables.clear();
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

ValidateTypeFieldArguments ValidateExecutableVisitor::getArguments(
	const std::vector<std::shared_ptr<const schema::InputValue>>& args)
{
	ValidateTypeFieldArguments result;

	for (const auto& arg : args)
	{
		if (!arg)
		{
			continue;
		}

		ValidateArgument argument;

		argument.defaultValue = !arg->defaultValue().empty();
		argument.nonNullDefaultValue =
			argument.defaultValue && arg->defaultValue() != R"gql(null)gql"sv;
		argument.type = getValidateType(arg->type().lock());

		result[arg->name()] = std::move(argument);
	}

	return result;
}

constexpr bool ValidateExecutableVisitor::isScalarType(introspection::TypeKind kind)
{
	switch (kind)
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
			return false;

		default:
			return true;
	}
}

bool ValidateExecutableVisitor::matchesScopedType(std::string_view name) const
{
	if (name == _scopedType->get().name())
	{
		return true;
	}

	const auto itrScoped = _matchingTypes.find(_scopedType->get().name());
	const auto itrNamed = _matchingTypes.find(name);

	if (itrScoped != _matchingTypes.end() && itrNamed != _matchingTypes.end())
	{
		const auto itrMatch = std::find_if(itrScoped->second.begin(),
			itrScoped->second.end(),
			[itrNamed](std::string_view matchingType) noexcept {
				return itrNamed->second.find(matchingType) != itrNamed->second.end();
			});

		return itrMatch != itrScoped->second.end();
	}

	return false;
}

bool ValidateExecutableVisitor::validateInputValue(
	bool hasNonNullDefaultValue, const ValidateArgumentValuePtr& argument, const ValidateType& type)
{
	if (!type)
	{
		_errors.push_back({ "Unknown input type", argument.position });
		return false;
	}

	if (argument.value && std::holds_alternative<ValidateArgumentVariable>(argument.value->data))
	{
		if (_operationVariables)
		{
			const auto& variable = std::get<ValidateArgumentVariable>(argument.value->data);
			auto itrVariable = _operationVariables->find(variable.name);

			if (itrVariable == _operationVariables->end())
			{
				// https://spec.graphql.org/October2021/#sec-All-Variable-Uses-Defined
				auto message = std::format("Undefined variable name: {}", variable.name);

				_errors.push_back({ std::move(message), argument.position });
				return false;
			}

			_referencedVariables.emplace(variable.name);

			return validateVariableType(
				hasNonNullDefaultValue || itrVariable->second.nonNullDefaultValue,
				itrVariable->second.type,
				argument.position,
				type);
		}
		else
		{
			// In fragment definitions, variables can hold any type. It's only when we are
			// transitively visiting them through an operation definition that they are assigned a
			// type, and the type may not be exactly the same in all operations definitions which
			// reference the fragment.
			return true;
		}
	}

	const auto kind = type->get().kind();

	if (!argument.value)
	{
		// The null literal matches any nullable type and does not match a non-nullable type.
		if (kind == introspection::TypeKind::NON_NULL && !hasNonNullDefaultValue)
		{
			_errors.push_back({ "Expected Non-Null value", argument.position });
			return false;
		}

		return true;
	}

	switch (kind)
	{
		case introspection::TypeKind::NON_NULL:
		{
			// Unwrap and check the next one.
			const auto ofType = getValidateType(type->get().ofType().lock());

			if (!ofType)
			{
				_errors.push_back({ "Unknown Non-Null type", argument.position });
				return false;
			}

			return validateInputValue(hasNonNullDefaultValue, argument, ofType);
		}

		case introspection::TypeKind::LIST:
		{
			if (!std::holds_alternative<ValidateArgumentList>(argument.value->data))
			{
				_errors.push_back({ "Expected List value", argument.position });
				return false;
			}

			const auto ofType = getValidateType(type->get().ofType().lock());

			if (!ofType)
			{
				_errors.push_back({ "Unknown List type", argument.position });
				return false;
			}

			// Check every value against the target type.
			for (const auto& value : std::get<ValidateArgumentList>(argument.value->data).values)
			{
				if (!validateInputValue(false, value, ofType))
				{
					// Error messages are added in the recursive call, so just bubble up the result.
					return false;
				}
			}

			return true;
		}

		case introspection::TypeKind::INPUT_OBJECT:
		{
			const auto name = type->get().name();

			if (name.empty())
			{
				_errors.push_back({ "Unknown Input Object type", argument.position });
				return false;
			}

			if (!std::holds_alternative<ValidateArgumentMap>(argument.value->data))
			{
				auto message = std::format("Expected Input Object value name: {}", name);

				_errors.push_back({ std::move(message), argument.position });
				return false;
			}

			auto itrFields = getInputTypeFields(name);

			if (itrFields == _inputTypeFields.end())
			{
				auto message = std::format("Expected Input Object fields name: {}", name);

				_errors.push_back({ std::move(message), argument.position });
				return false;
			}

			const auto& values = std::get<ValidateArgumentMap>(argument.value->data).values;
			internal::string_view_set subFields;

			// Check every value against the target type.
			for (const auto& entry : values)
			{
				auto itrField = itrFields->second.find(entry.first);

				if (itrField == itrFields->second.end())
				{
					// https://spec.graphql.org/October2021/#sec-Input-Object-Field-Names
					auto message = std::format("Undefined Input Object field type: {} name: {}",
						name,
						entry.first);

					_errors.push_back({ std::move(message), entry.second.position });
					return false;
				}

				if (entry.second.value || !itrField->second.defaultValue)
				{
					if (!validateInputValue(itrField->second.nonNullDefaultValue,
							entry.second,
							itrField->second.type))
					{
						// Error messages are added in the recursive call, so just bubble up the
						// result.
						return false;
					}

					// The recursive call may invalidate the iterator, so reacquire it.
					itrFields = getInputTypeFields(name);
				}

				subFields.emplace(entry.first);
			}

			// See if all required fields were specified.
			for (const auto& entry : itrFields->second)
			{
				if (entry.second.defaultValue || subFields.find(entry.first) != subFields.end())
				{
					continue;
				}

				if (!entry.second.type)
				{
					auto message = std::format("Unknown Input Object field type: {} name: {}",
						name,
						entry.first);

					_errors.push_back({ std::move(message), argument.position });
					return false;
				}

				const auto fieldKind = entry.second.type->get().kind();

				if (fieldKind == introspection::TypeKind::NON_NULL)
				{
					// https://spec.graphql.org/October2021/#sec-Input-Object-Required-Fields
					auto message = std::format("Missing Input Object field type: {} name: {}",
						name,
						entry.first);

					_errors.push_back({ std::move(message), argument.position });
					return false;
				}
			}

			return true;
		}

		case introspection::TypeKind::ENUM:
		{
			const auto name = type->get().name();

			if (name.empty())
			{
				_errors.push_back({ "Unknown Enum value", argument.position });
				return false;
			}

			if (!std::holds_alternative<ValidateArgumentEnumValue>(argument.value->data))
			{
				auto message = std::format("Expected Enum value name: {}", name);

				_errors.push_back({ std::move(message), argument.position });
				return false;
			}

			const auto& value = std::get<ValidateArgumentEnumValue>(argument.value->data).value;
			auto itrEnumValues = _enumValues.find(name);

			if (itrEnumValues == _enumValues.end()
				|| itrEnumValues->second.find(value) == itrEnumValues->second.end())
			{
				auto message = std::format("Undefined Enum value type: {} name: {}", name, value);

				_errors.push_back({ std::move(message), argument.position });
				return false;
			}

			return true;
		}

		case introspection::TypeKind::SCALAR:
		{
			const auto name = type->get().name();

			if (name.empty())
			{
				_errors.push_back({ "Unknown Scalar value", argument.position });
				return false;
			}

			if (name == R"gql(Int)gql"sv)
			{
				if (!std::holds_alternative<int>(argument.value->data))
				{
					_errors.push_back({ "Expected Int value", argument.position });
					return false;
				}
			}
			else if (name == R"gql(Float)gql"sv)
			{
				if (!std::holds_alternative<double>(argument.value->data)
					&& !std::holds_alternative<int>(argument.value->data))
				{
					_errors.push_back({ "Expected Float value", argument.position });
					return false;
				}
			}
			else if (name == R"gql(String)gql"sv)
			{
				if (!std::holds_alternative<std::string_view>(argument.value->data))
				{
					_errors.push_back({ "Expected String value", argument.position });
					return false;
				}
			}
			else if (name == R"gql(ID)gql"sv)
			{
				if (!std::holds_alternative<std::string_view>(argument.value->data))
				{
					_errors.push_back({ "Expected ID value", argument.position });
					return false;
				}
			}
			else if (name == R"gql(Boolean)gql"sv)
			{
				if (!std::holds_alternative<bool>(argument.value->data))
				{
					_errors.push_back({ "Expected Boolean value", argument.position });
					return false;
				}
			}

			if (_scalarTypes.find(name) == _scalarTypes.end())
			{
				auto message = std::format("Undefined Scalar type name: {}", name);

				_errors.push_back({ std::move(message), argument.position });
				return false;
			}

			return true;
		}

		default:
		{
			_errors.push_back({ "Unexpected value type", argument.position });
			return false;
		}
	}
}

bool ValidateExecutableVisitor::validateVariableType(bool isNonNull,
	const ValidateType& variableType, const schema_location& position,
	const ValidateType& inputType)
{
	if (!variableType)
	{
		_errors.push_back({ "Unknown variable type", position });
		return false;
	}

	const auto variableKind = variableType->get().kind();

	if (variableKind == introspection::TypeKind::NON_NULL)
	{
		const auto ofType = getValidateType(variableType->get().ofType().lock());

		if (!ofType)
		{
			_errors.push_back({ "Unknown Non-Null variable type", position });
			return false;
		}

		return validateVariableType(true, ofType, position, inputType);
	}

	if (!inputType)
	{
		_errors.push_back({ "Unknown input type", position });
		return false;
	}

	const auto inputKind = inputType->get().kind();

	switch (inputKind)
	{
		case introspection::TypeKind::NON_NULL:
		{
			if (!isNonNull)
			{
				// https://spec.graphql.org/October2021/#sec-All-Variable-Usages-are-Allowed
				_errors.push_back({ "Expected Non-Null variable type", position });
				return false;
			}

			// Unwrap and check the next one.
			const auto ofType = getValidateType(inputType->get().ofType().lock());

			if (!ofType)
			{
				_errors.push_back({ "Unknown Non-Null input type", position });
				return false;
			}

			return validateVariableType(false, variableType, position, ofType);
		}

		case introspection::TypeKind::LIST:
		{
			if (variableKind != inputKind)
			{
				// https://spec.graphql.org/October2021/#sec-All-Variable-Usages-are-Allowed
				_errors.push_back({ "Expected List variable type", position });
				return false;
			}

			// Unwrap and check the next one.
			const auto variableOfType = getValidateType(variableType->get().ofType().lock());

			if (!variableOfType)
			{
				_errors.push_back({ "Unknown List variable type", position });
				return false;
			}

			const auto inputOfType = getValidateType(inputType->get().ofType().lock());

			if (!inputOfType)
			{
				_errors.push_back({ "Unknown List input type", position });
				return false;
			}

			return validateVariableType(false, variableOfType, position, inputOfType);
		}

		case introspection::TypeKind::INPUT_OBJECT:
		{
			if (variableKind != inputKind)
			{
				// https://spec.graphql.org/October2021/#sec-All-Variable-Usages-are-Allowed
				_errors.push_back({ "Expected Input Object variable type", position });
				return false;
			}

			break;
		}

		case introspection::TypeKind::ENUM:
		{
			if (variableKind != inputKind)
			{
				// https://spec.graphql.org/October2021/#sec-All-Variable-Usages-are-Allowed
				_errors.push_back({ "Expected Enum variable type", position });
				return false;
			}

			break;
		}

		case introspection::TypeKind::SCALAR:
		{
			if (variableKind != inputKind)
			{
				// https://spec.graphql.org/October2021/#sec-All-Variable-Usages-are-Allowed
				_errors.push_back({ "Expected Scalar variable type", position });
				return false;
			}

			break;
		}

		default:
		{
			// https://spec.graphql.org/October2021/#sec-All-Variable-Usages-are-Allowed
			_errors.push_back({ "Unexpected input type", position });
			return false;
		}
	}

	const auto variableName = variableType->get().name();

	if (variableName.empty())
	{
		_errors.push_back({ "Unknown variable type", position });
		return false;
	}

	const auto inputName = inputType->get().name();

	if (inputName.empty())
	{
		_errors.push_back({ "Unknown input type", position });
		return false;
	}

	if (variableName != inputName)
	{
		// https://spec.graphql.org/October2021/#sec-All-Variable-Usages-are-Allowed
		auto message =
			std::format("Incompatible variable type: {} name: {}", variableName, inputName);

		_errors.push_back({ std::move(message), position });
		return false;
	}

	return true;
}

ValidateExecutableVisitor::TypeFields::const_iterator ValidateExecutableVisitor::
	getScopedTypeFields()
{
	auto typeKind = _scopedType->get().kind();
	auto itrType = _typeFields.find(_scopedType->get().name());

	if (itrType == _typeFields.end() && !isScalarType(typeKind))
	{
		const auto& fields = _scopedType->get().fields();
		internal::string_view_map<ValidateTypeField> validateFields;

		for (auto& entry : fields)
		{
			if (!entry)
			{
				continue;
			}

			const auto fieldName = entry->name();
			ValidateTypeField subField;

			subField.returnType = getValidateType(entry->type().lock());

			if (fieldName.empty() || !subField.returnType)
			{
				continue;
			}

			subField.arguments = getArguments(entry->args());

			validateFields[fieldName] = std::move(subField);
		}

		if (_schema->supportsIntrospection() && _scopedType == _operationTypes[strQuery])
		{
			ValidateTypeField schemaField;

			schemaField.returnType =
				getValidateType(_schema->WrapType(introspection::TypeKind::NON_NULL,
					_schema->LookupType(R"gql(__Schema)gql"sv)));
			validateFields[R"gql(__schema)gql"sv] = std::move(schemaField);

			ValidateTypeField typeField;
			ValidateArgument nameArgument;

			typeField.returnType = getValidateType(_schema->LookupType(R"gql(__Type)gql"sv));

			nameArgument.type = getValidateType(_schema->WrapType(introspection::TypeKind::NON_NULL,
				_schema->LookupType(R"gql(String)gql"sv)));
			typeField.arguments[R"gql(name)gql"sv] = std::move(nameArgument);

			validateFields[R"gql(__type)gql"sv] = std::move(typeField);
		}

		ValidateTypeField typenameField;

		typenameField.returnType =
			getValidateType(_schema->WrapType(introspection::TypeKind::NON_NULL,
				_schema->LookupType(R"gql(String)gql"sv)));
		validateFields[R"gql(__typename)gql"sv] = std::move(typenameField);

		itrType = _typeFields.emplace(_scopedType->get().name(), std::move(validateFields)).first;
	}

	return itrType;
}

ValidateExecutableVisitor::InputTypeFields::const_iterator ValidateExecutableVisitor::
	getInputTypeFields(std::string_view name)
{
	auto itrFields = _inputTypeFields.find(name);

	if (itrFields == _inputTypeFields.end())
	{
		auto itrType = _types.find(name);

		if (itrType != _types.end()
			&& itrType->second->get().kind() == introspection::TypeKind::INPUT_OBJECT)
		{
			itrFields =
				_inputTypeFields.emplace(name, getArguments(itrType->second->get().inputFields()))
					.first;
		}
	}

	return itrFields;
}

template <class _FieldTypes>
ValidateType ValidateExecutableVisitor::getFieldType(
	const _FieldTypes& fields, std::string_view name)
{
	auto itrType = fields.find(name);

	if (itrType == fields.end())
	{
		return ValidateType {};
	}

	// Iteratively expand nested types till we get the underlying field type.
	auto fieldType = getValidateFieldType(itrType->second);

	while (fieldType && fieldType->get().name().empty())
	{
		fieldType = getValidateType(fieldType->get().ofType().lock());
	}

	return fieldType;
}

const ValidateType& ValidateExecutableVisitor::getValidateFieldType(
	const FieldTypes::mapped_type& value)
{
	return value.returnType;
}

const ValidateType& ValidateExecutableVisitor::getValidateFieldType(
	const InputFieldTypes::mapped_type& value)
{
	return value.type;
}

template <class _FieldTypes>
ValidateType ValidateExecutableVisitor::getWrappedFieldType(
	const _FieldTypes& fields, std::string_view name)
{
	auto itrType = fields.find(name);

	if (itrType == fields.end())
	{
		return std::nullopt;
	}

	return getValidateFieldType(itrType->second);
}

void ValidateExecutableVisitor::visitField(const peg::ast_node& field)
{
	peg::on_first_child<peg::directives>(field, [this](const peg::ast_node& child) {
		visitDirectives(introspection::DirectiveLocation::FIELD, child);
	});

	std::string_view name;

	peg::on_first_child<peg::field_name>(field, [&name](const peg::ast_node& child) {
		name = child.string_view();
	});

	auto itrType = getScopedTypeFields();

	if (itrType == _typeFields.end())
	{
		// https://spec.graphql.org/October2021/#sec-Leaf-Field-Selections
		auto position = field.begin();
		auto message =
			std::format("Field on scalar type: {} name: {}", _scopedType->get().name(), name);

		_errors.push_back({ std::move(message), { position.line, position.column } });
		return;
	}

	ValidateType innerType;
	ValidateType wrappedType;

	switch (_scopedType->get().kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		{
			// https://spec.graphql.org/October2021/#sec-Field-Selections
			innerType = getFieldType(itrType->second, name);
			wrappedType = getWrappedFieldType(itrType->second, name);
			break;
		}

		case introspection::TypeKind::UNION:
		{
			if (name != R"gql(__typename)gql"sv)
			{
				// https://spec.graphql.org/October2021/#sec-Leaf-Field-Selections
				auto position = field.begin();
				auto message = std::format("Field on union type: {} name: {}",
					_scopedType->get().name(),
					name);

				_errors.push_back({ std::move(message), { position.line, position.column } });
				return;
			}

			// https://spec.graphql.org/October2021/#sec-Field-Selections
			innerType = getValidateType(_schema->LookupType("String"sv));
			wrappedType = getValidateType(
				_schema->WrapType(introspection::TypeKind::NON_NULL, getSharedType(innerType)));
			break;
		}

		default:
			break;
	}

	if (!innerType)
	{
		// https://spec.graphql.org/October2021/#sec-Field-Selections
		auto position = field.begin();
		auto message =
			std::format("Undefined field type: {} name: {}", _scopedType->get().name(), name);

		_errors.push_back({ std::move(message), { position.line, position.column } });
		return;
	}

	std::string_view alias;

	peg::on_first_child<peg::alias_name>(field, [&alias](const peg::ast_node& child) {
		alias = child.string_view();
	});

	if (alias.empty())
	{
		alias = name;
	}

	ValidateFieldArguments validateArguments;
	internal::string_view_map<schema_location> argumentLocations;
	std::list<std::string_view> argumentNames;

	peg::on_first_child<peg::arguments>(field,
		[this, name, &validateArguments, &argumentLocations, &argumentNames](
			const peg::ast_node& child) {
			for (auto& argument : child.children)
			{
				auto argumentName = argument->children.front()->string_view();
				auto position = argument->begin();

				if (validateArguments.find(argumentName) != validateArguments.end())
				{
					// https://spec.graphql.org/October2021/#sec-Argument-Uniqueness
					auto message = std::format("Conflicting argument type: {} field: {} name: {}",
						_scopedType->get().name(),
						name,
						argumentName);

					_errors.push_back({ std::move(message), { position.line, position.column } });
					continue;
				}

				ValidateArgumentValueVisitor visitor(_errors);

				visitor.visit(*argument->children.back());
				validateArguments[argumentName] = visitor.getArgumentValue();
				argumentLocations[argumentName] = { position.line, position.column };
				argumentNames.push_back(argumentName);
			}
		});

	ValidateType objectType =
		(_scopedType->get().kind() == introspection::TypeKind::OBJECT ? _scopedType
																	  : ValidateType {});
	ValidateField validateField(std::move(wrappedType),
		std::move(objectType),
		name,
		std::move(validateArguments));
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
			// https://spec.graphql.org/October2021/#sec-Field-Selection-Merging
			auto position = field.begin();
			auto message =
				std::format("Conflicting field type: {} name: {}", _scopedType->get().name(), name);

			_errors.push_back({ std::move(message), { position.line, position.column } });
		}
	}

	auto itrField = itrType->second.find(name);

	if (itrField != itrType->second.end())
	{
		for (auto argumentName : argumentNames)
		{
			auto itrArgument = itrField->second.arguments.find(argumentName);

			if (itrArgument == itrField->second.arguments.end())
			{
				// https://spec.graphql.org/October2021/#sec-Argument-Names
				auto message = std::format("Undefined argument type: {} field: {} name: {}",
					_scopedType->get().name(),
					name,
					argumentName);

				_errors.push_back({ std::move(message), argumentLocations[argumentName] });
			}
		}

		for (auto& argument : itrField->second.arguments)
		{
			auto itrArgument = validateField.arguments.find(argument.first);
			const bool missing = itrArgument == validateField.arguments.end();

			if (!missing && itrArgument->second.value)
			{
				// The value was not null.
				if (!validateInputValue(argument.second.nonNullDefaultValue,
						itrArgument->second,
						argument.second.type))
				{
					// https://spec.graphql.org/October2021/#sec-Values-of-Correct-Type
					auto message = std::format("Incompatible argument type: {} field: {} name: {}",
						_scopedType->get().name(),
						name,
						argument.first);

					_errors.push_back({ std::move(message), argumentLocations[argument.first] });
				}

				continue;
			}
			else if (argument.second.defaultValue)
			{
				// The argument has a default value.
				continue;
			}

			// See if the argument is wrapped in NON_NULL
			if (argument.second.type
				&& introspection::TypeKind::NON_NULL == argument.second.type->get().kind())
			{
				// https://spec.graphql.org/October2021/#sec-Required-Arguments
				auto position = field.begin();
				auto message = std::format("{} argument type: {} field: {} name: {}",
					(missing ? "Missing" : "Required non-null"),
					_scopedType->get().name(),
					name,
					argument.first);

				_errors.push_back({ std::move(message), { position.line, position.column } });
			}
		}
	}

	_selectionFields.emplace(alias, std::move(validateField));

	const peg::ast_node* selection = nullptr;

	peg::on_first_child<peg::selection_set>(field, [&selection](const peg::ast_node& child) {
		selection = &child;
	});

	std::size_t subFieldCount = 0;

	if (selection != nullptr)
	{
		auto outerType = std::move(_scopedType);
		auto outerFields = std::move(_selectionFields);
		auto outerFieldCount = _fieldCount;
		auto outerIntrospectionFieldCount = _introspectionFieldCount;

		_fieldCount = 0;
		_introspectionFieldCount = 0;
		_selectionFields.clear();
		_scopedType = std::move(innerType);

		visitSelection(*selection);

		innerType = std::move(_scopedType);
		_scopedType = std::move(outerType);
		_selectionFields = std::move(outerFields);
		subFieldCount = _fieldCount;
		_introspectionFieldCount = outerIntrospectionFieldCount;
		_fieldCount = outerFieldCount;
	}

	if (subFieldCount == 0 && !isScalarType(innerType->get().kind()))
	{
		// https://spec.graphql.org/October2021/#sec-Leaf-Field-Selections
		auto position = field.begin();
		auto message =
			std::format("Missing fields on non-scalar type: {}", innerType->get().name());

		_errors.push_back({ std::move(message), { position.line, position.column } });
		return;
	}

	++_fieldCount;

	constexpr auto c_introspectionFieldPrefix = R"gql(__)gql"sv;

	if (name.size() >= c_introspectionFieldPrefix.size()
		&& name.substr(0, c_introspectionFieldPrefix.size()) == c_introspectionFieldPrefix)
	{
		++_introspectionFieldCount;
	}
}

void ValidateExecutableVisitor::visitFragmentSpread(const peg::ast_node& fragmentSpread)
{
	peg::on_first_child<peg::directives>(fragmentSpread, [this](const peg::ast_node& child) {
		visitDirectives(introspection::DirectiveLocation::FRAGMENT_SPREAD, child);
	});

	const auto name = fragmentSpread.children.front()->string_view();
	auto itr = _fragmentDefinitions.find(name);

	if (itr == _fragmentDefinitions.end())
	{
		// https://spec.graphql.org/October2021/#sec-Fragment-spread-target-defined
		auto position = fragmentSpread.begin();
		auto message = std::format("Undefined fragment spread name: {}", name);

		_errors.push_back({ std::move(message), { position.line, position.column } });
		return;
	}

	if (_fragmentStack.find(name) != _fragmentStack.end())
	{
		if (_fragmentCycles.emplace(name).second)
		{
			// https://spec.graphql.org/October2021/#sec-Fragment-spreads-must-not-form-cycles
			auto position = fragmentSpread.begin();
			auto message = std::format("Cyclic fragment spread name: {}", name);

			_errors.push_back({ std::move(message), { position.line, position.column } });
		}

		return;
	}

	const auto& selection = *itr->second.get().children.back();
	const auto& typeCondition = itr->second.get().children[1];
	const auto innerType = typeCondition->children.front()->string_view();
	const auto itrInner = _types.find(innerType);

	if (itrInner == _types.end() || !matchesScopedType(innerType))
	{
		// https://spec.graphql.org/October2021/#sec-Fragment-spread-is-possible
		auto position = fragmentSpread.begin();
		auto message =
			std::format("Incompatible fragment spread target type: {} name: {}", innerType, name);

		_errors.push_back({ std::move(message), { position.line, position.column } });
		return;
	}

	auto outerType = std::move(_scopedType);

	_fragmentStack.emplace(name);
	_scopedType = itrInner->second;

	visitSelection(selection);

	_scopedType = std::move(outerType);
	_fragmentStack.erase(name);

	_referencedFragments.emplace(name);
}

void ValidateExecutableVisitor::visitInlineFragment(const peg::ast_node& inlineFragment)
{
	peg::on_first_child<peg::directives>(inlineFragment, [this](const peg::ast_node& child) {
		visitDirectives(introspection::DirectiveLocation::INLINE_FRAGMENT, child);
	});

	std::string_view innerType;
	schema_location typeConditionLocation;

	peg::on_first_child<peg::type_condition>(inlineFragment,
		[&innerType, &typeConditionLocation](const peg::ast_node& child) {
			auto position = child.begin();

			innerType = child.children.front()->string_view();
			typeConditionLocation = { position.line, position.column };
		});

	ValidateType fragmentType;

	if (innerType.empty())
	{
		fragmentType = _scopedType;
	}
	else
	{
		auto itrInner = _types.find(innerType);

		if (itrInner == _types.end())
		{
			// https://spec.graphql.org/October2021/#sec-Fragment-Spread-Type-Existence
			auto message =
				std::format("Undefined target type on inline fragment name: {}", innerType);

			_errors.push_back({ std::move(message), std::move(typeConditionLocation) });
			return;
		}

		fragmentType = itrInner->second;

		if (isScalarType(fragmentType->get().kind()) || !matchesScopedType(innerType))
		{
			// https://spec.graphql.org/October2021/#sec-Fragments-On-Composite-Types
			// https://spec.graphql.org/October2021/#sec-Fragment-spread-is-possible
			auto message = std::format("{} target type on inline fragment name: {}",
				(isScalarType(fragmentType->get().kind()) ? "Scalar" : "Incompatible"),
				innerType);

			_errors.push_back({ std::move(message), std::move(typeConditionLocation) });
			return;
		}
	}

	peg::on_first_child<peg::selection_set>(inlineFragment,
		[this, &fragmentType](const peg::ast_node& selection) {
			auto outerType = std::move(_scopedType);

			_scopedType = std::move(fragmentType);

			visitSelection(selection);

			_scopedType = std::move(outerType);
		});
}

void ValidateExecutableVisitor::visitDirectives(
	introspection::DirectiveLocation location, const peg::ast_node& directives)
{
	internal::string_view_set uniqueDirectives;

	for (const auto& directive : directives.children)
	{
		std::string_view directiveName;

		peg::on_first_child<peg::directive_name>(*directive,
			[&directiveName](const peg::ast_node& child) {
				directiveName = child.string_view();
			});

		const auto itrDirective = _directives.find(directiveName);

		if (itrDirective == _directives.end())
		{
			// https://spec.graphql.org/October2021/#sec-Directives-Are-Defined
			auto position = directive->begin();
			auto message = std::format("Undefined directive name: {}", directiveName);

			_errors.push_back({ std::move(message), { position.line, position.column } });
			continue;
		}

		if (!itrDirective->second.isRepeatable && !uniqueDirectives.emplace(directiveName).second)
		{
			// https://spec.graphql.org/October2021/#sec-Directives-Are-Unique-Per-Location
			auto position = directive->begin();
			auto message = std::format("Conflicting directive name: {}", directiveName);

			_errors.push_back({ std::move(message), { position.line, position.column } });
			continue;
		}

		if (itrDirective->second.locations.find(location) == itrDirective->second.locations.end())
		{
			// https://spec.graphql.org/October2021/#sec-Directives-Are-In-Valid-Locations
			auto position = directive->begin();
			auto message = std::format("Unexpected location for directive: {}", directiveName);

			switch (location)
			{
				case introspection::DirectiveLocation::QUERY:
					message += " name: QUERY";
					break;

				case introspection::DirectiveLocation::MUTATION:
					message += " name: MUTATION";
					break;

				case introspection::DirectiveLocation::SUBSCRIPTION:
					message += " name: SUBSCRIPTION";
					break;

				case introspection::DirectiveLocation::FIELD:
					message += " name: FIELD";
					break;

				case introspection::DirectiveLocation::FRAGMENT_DEFINITION:
					message += " name: FRAGMENT_DEFINITION";
					break;

				case introspection::DirectiveLocation::FRAGMENT_SPREAD:
					message += " name: FRAGMENT_SPREAD";
					break;

				case introspection::DirectiveLocation::INLINE_FRAGMENT:
					message += " name: INLINE_FRAGMENT";
					break;

				default:
					break;
			}

			_errors.push_back({ std::move(message), { position.line, position.column } });
			continue;
		}

		peg::on_first_child<peg::arguments>(*directive,
			[this, &directive, &directiveName, itrDirective](const peg::ast_node& child) {
				ValidateFieldArguments validateArguments;
				internal::string_view_map<schema_location> argumentLocations;
				std::list<std::string_view> argumentNames;

				for (auto& argument : child.children)
				{
					auto position = argument->begin();
					auto argumentName = argument->children.front()->string_view();

					if (validateArguments.find(argumentName) != validateArguments.end())
					{
						// https://spec.graphql.org/October2021/#sec-Argument-Uniqueness
						auto message = std::format("Conflicting argument directive: {} name: {}",
							directiveName,
							argumentName);

						_errors.push_back(
							{ std::move(message), { position.line, position.column } });
						continue;
					}

					ValidateArgumentValueVisitor visitor(_errors);

					visitor.visit(*argument->children.back());
					validateArguments[argumentName] = visitor.getArgumentValue();
					argumentLocations[argumentName] = { position.line, position.column };
					argumentNames.push_back(argumentName);
				}

				for (auto argumentName : argumentNames)
				{
					auto itrArgument = itrDirective->second.arguments.find(argumentName);

					if (itrArgument == itrDirective->second.arguments.end())
					{
						// https://spec.graphql.org/October2021/#sec-Argument-Names
						auto message = std::format("Undefined argument directive: {} name: {}",
							directiveName,
							argumentName);

						_errors.push_back({ std::move(message), argumentLocations[argumentName] });
					}
				}

				for (auto& argument : itrDirective->second.arguments)
				{
					auto itrArgument = validateArguments.find(argument.first);
					const bool missing = itrArgument == validateArguments.end();

					if (!missing && itrArgument->second.value)
					{
						// The value was not null.
						if (!validateInputValue(argument.second.nonNullDefaultValue,
								itrArgument->second,
								argument.second.type))
						{
							// https://spec.graphql.org/October2021/#sec-Values-of-Correct-Type
							auto message =
								std::format("Incompatible argument directive: {} name: {}",
									directiveName,
									argument.first);

							_errors.push_back(
								{ std::move(message), argumentLocations[argument.first] });
						}

						continue;
					}
					else if (argument.second.defaultValue)
					{
						// The argument has a default value.
						continue;
					}

					// See if the argument is wrapped in NON_NULL
					if (argument.second.type
						&& introspection::TypeKind::NON_NULL == argument.second.type->get().kind())
					{
						// https://spec.graphql.org/October2021/#sec-Required-Arguments
						auto position = directive->begin();
						auto message = std::format("{} argument directive: {} name: {}",
							(missing ? "Missing" : "Required non-null"),
							directiveName,
							argument.first);

						_errors.push_back(
							{ std::move(message), { position.line, position.column } });
					}
				}
			});
	}
}

} // namespace graphql::service
