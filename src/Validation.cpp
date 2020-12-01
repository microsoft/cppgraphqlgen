// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLGrammar.h"

#include "Validation.h"

#include <algorithm>
#include <iostream>
#include <iterator>

namespace graphql::service {

constexpr std::string_view introspectionQuery = R"gql(
query IntrospectionQuery {
  __schema {
    queryType { name }
    mutationType { name }
    subscriptionType { name }
    types { ...FullType }
    directives {
      name
      locations
      args { ...InputValue }
    }
  }
}

fragment FullType on __Type {
  kind
  name
  fields(includeDeprecated: true) {
    name
    args { ...InputValue }
    type { ...TypeRef }
  }
  inputFields { ...InputValue }
  interfaces { ...TypeRef }
  enumValues(includeDeprecated: true) { name }
  possibleTypes { ...TypeRef }
}

fragment InputValue on __InputValue {
  name
  type { ...TypeRef }
  defaultValue
}

fragment TypeRef on __Type {
  kind
  name
  ofType {
    kind
    name
    ofType {
      kind
      name
      ofType {
        kind
        name
        ofType {
          kind
          name
          ofType {
            kind
            name
            ofType {
              kind
              name
              ofType {
                kind
                name
              }
            }
          }
        }
      }
    }
  }
}
)gql";

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

ValidateArgumentValueVisitor::ValidateArgumentValueVisitor(std::vector<schema_error>& errors)
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
	ValidateArgumentVariable value { std::string { variable.string_view().substr(1) } };
	auto position = variable.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitIntValue(const peg::ast_node& intValue)
{
	response::IntType value { std::atoi(intValue.string().c_str()) };
	auto position = intValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(value);
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitFloatValue(const peg::ast_node& floatValue)
{
	response::FloatType value { std::atof(floatValue.string().c_str()) };
	auto position = floatValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(value);
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitStringValue(const peg::ast_node& stringValue)
{
	response::StringType value { stringValue.unescaped };
	auto position = stringValue.begin();

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.column };
}

void ValidateArgumentValueVisitor::visitBooleanValue(const peg::ast_node& booleanValue)
{
	response::BooleanType value { booleanValue.is_type<peg::true_keyword>() };
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
	ValidateArgumentEnumValue value { enumValue.string() };
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
		auto name = field->children.front()->string();

		if (value.values.find(name) != value.values.end())
		{
			// http://spec.graphql.org/June2018/#sec-Input-Object-Field-Uniqueness
			auto position = field->begin();
			std::ostringstream message;

			message << "Conflicting input field name: " << name;

			_errors.push_back({ message.str(), { position.line, position.column } });
			continue;
		}

		ValidateArgumentValueVisitor visitor(_errors);

		visitor.visit(*field->children.back());
		value.values[std::move(name)] = visitor.getArgumentValue();
	}

	_argumentValue.value = std::make_unique<ValidateArgumentValue>(std::move(value));
	_argumentValue.position = { position.line, position.column };
}

ValidateField::ValidateField(std::shared_ptr<const ValidateType> returnType,
	std::shared_ptr<const ValidateType>&& objectType, const std::string& fieldName,
	ValidateFieldArguments&& arguments)
	: returnType(returnType)
	, objectType(std::move(objectType))
	, fieldName(fieldName)
	, arguments(std::move(arguments))
{
}

bool ValidateField::operator==(const ValidateField& other) const
{
	return *returnType == *other.returnType
		&& ((objectType && other.objectType && objectType.get() != other.objectType.get())
			|| (fieldName == other.fieldName && arguments == other.arguments));
}

ValidateVariableTypeVisitor::ValidateVariableTypeVisitor(const ValidationContext& validationContext)
	: _validationContext(validationContext)
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
	_variableType = _validationContext.getNamedValidateType(namedType.string_view());
}

void ValidateVariableTypeVisitor::visitListType(const peg::ast_node& listType)
{
	ValidateVariableTypeVisitor visitor(_validationContext);

	visitor.visit(*listType.children.front());

	_variableType = _validationContext.getListOfType(visitor.getType());
}

void ValidateVariableTypeVisitor::visitNonNullType(const peg::ast_node& nonNullType)
{
	ValidateVariableTypeVisitor visitor(_validationContext);

	visitor.visit(*nonNullType.children.front());

	_variableType = _validationContext.getNonNullOfType(visitor.getType());
}

bool ValidateVariableTypeVisitor::isInputType() const
{
	return _variableType && _variableType->isInputType();
}

std::shared_ptr<ValidateType> ValidateVariableTypeVisitor::getType()
{
	auto result = std::move(_variableType);

	return result;
}

ValidationContext::ValidationContext(const Request& service)
{
	// TODO: we should execute this query only once per schema,
	// maybe it can be done and cached inside the Request itself to allow
	// this. Alternatively it could be provided at compile-time such as schema.json
	// that is parsed, this would allow us to drop introspection from the Request
	// and still have it to work
	auto ast = peg::parseString(introspectionQuery);
	// This is taking advantage of the fact that during validation we can choose to execute
	// unvalidated queries against the Introspection schema. This way we can use fragment
	// cycles to expand an arbitrary number of wrapper types.
	ast.validated = true;

	std::shared_ptr<RequestState> state;
	const std::string operationName;
	response::Value variables(response::Type::Map);
	auto result = service.resolve(state, ast, operationName, std::move(variables)).get();

	populate(result);
}

ValidationContext::ValidationContext(const response::Value& introspectionQuery)
{
	populate(introspectionQuery);
}

void ValidationContext::populate(const response::Value& introspectionQuery)
{
	commonTypes.string = makeScalarType("String");
	commonTypes.nonNullString = makeNonNullOfType(commonTypes.string);

	const auto& itrData = introspectionQuery.find(std::string { strData });
	if (itrData == introspectionQuery.end())
	{
		return;
	}

	const auto& data = itrData->second;
	const auto& itrSchema = data.find(R"gql(__schema)gql");
	if (itrSchema != data.end() && itrSchema->second.type() == response::Type::Map)
	{
		for (auto itr = itrSchema->second.begin(); itr < itrSchema->second.end(); itr++)
		{
			const auto& member = *itr;
			if (member.second.type() == response::Type::Map)
			{
				const auto& itrType = member.second.find(R"gql(name)gql");
				if (itrType != member.second.end()
					&& itrType->second.type() == response::Type::String)
				{
					if (member.first == R"gql(queryType)gql")
					{
						_operationTypes.queryType = itrType->second.get<response::StringType>();
					}
					else if (member.first == R"gql(mutationType)gql")
					{
						_operationTypes.mutationType = itrType->second.get<response::StringType>();
					}
					else if (member.first == R"gql(subscriptionType)gql")
					{
						_operationTypes.subscriptionType =
							itrType->second.get<response::StringType>();
					}
				}
			}
			else if (member.second.type() == response::Type::List
				&& member.first == R"gql(types)gql")
			{
				const auto& entries = member.second.get<response::ListType>();

				// first iteration add the named types
				for (const auto& entry : entries)
				{
					if (entry.type() != response::Type::Map)
					{
						continue;
					}

					const auto& itrName = entry.find(R"gql(name)gql");
					const auto& itrKind = entry.find(R"gql(kind)gql");

					if (itrName != entry.end() && itrName->second.type() == response::Type::String
						&& itrKind != entry.end()
						&& itrKind->second.type() == response::Type::EnumValue)
					{
						const auto& name = itrName->second.get<response::StringType>();
						const auto& kind =
							ModifiedArgument<introspection::TypeKind>::convert(itrKind->second);

						if (kind == introspection::TypeKind::OBJECT)
						{
							addObject(name);
						}
						else if (kind == introspection::TypeKind::INPUT_OBJECT)
						{
							addInputObject(name);
						}
						else if (kind == introspection::TypeKind::INTERFACE)
						{
							addInterface(name, entry);
						}
						else if (kind == introspection::TypeKind::UNION)
						{
							addUnion(name, entry);
						}
						else if (kind == introspection::TypeKind::ENUM)
						{
							addEnum(name, entry);
						}
						else if (kind == introspection::TypeKind::SCALAR)
						{
							addScalar(name);
						}
					}
				}

				// second iteration add the fields that refer to given types
				for (const auto& entry : entries)
				{
					if (entry.type() != response::Type::Map)
					{
						continue;
					}

					const auto& itrName = entry.find(R"gql(name)gql");
					const auto& itrKind = entry.find(R"gql(kind)gql");

					if (itrName != entry.end() && itrName->second.type() == response::Type::String
						&& itrKind != entry.end()
						&& itrKind->second.type() == response::Type::EnumValue)
					{
						const auto& name = itrName->second.get<response::StringType>();
						const auto& kind =
							ModifiedArgument<introspection::TypeKind>::convert(itrKind->second);

						if (kind == introspection::TypeKind::OBJECT)
						{
							auto type = getNamedValidateType<ObjectType>(name);
							addTypeFields(type, entry);
						}
						else if (kind == introspection::TypeKind::INTERFACE
							|| kind == introspection::TypeKind::UNION)
						{
							auto type =
								getNamedValidateType<PossibleTypesContainerValidateType>(name);
							addTypeFields(type, entry);
							addPossibleTypes(type, entry);
						}
						else if (kind == introspection::TypeKind::INPUT_OBJECT)
						{
							auto type = getNamedValidateType<InputObjectType>(name);
							addInputTypeFields(type, entry);
						}
					}
				}
			}
			else if (member.second.type() == response::Type::List
				&& member.first == R"gql(directives)gql")
			{
				const auto& entries = member.second.get<response::ListType>();

				for (const auto& entry : entries)
				{
					if (entry.type() != response::Type::Map)
					{
						continue;
					}

					const auto& itrName = entry.find(R"gql(name)gql");
					const auto& itrLocations = entry.find(R"gql(locations)gql");

					if (itrName != entry.end() && itrName->second.type() == response::Type::String
						&& itrLocations != entry.end()
						&& itrLocations->second.type() == response::Type::List)
					{
						const auto& name = itrName->second.get<response::StringType>();
						const auto& locations = itrLocations->second.get<response::ListType>();

						addDirective(name, locations, entry);
					}
				}
			}
		}
	}
}

ValidateExecutableVisitor::ValidateExecutableVisitor(const Request& service)
	: ValidateExecutableVisitor(std::make_shared<const ValidationContext>(service))
{
}

ValidateExecutableVisitor::ValidateExecutableVisitor(
	std::shared_ptr<const ValidationContext> validationContext)
	: _validationContext(validationContext)
{
	commonTypes.nonNullString = _validationContext->getNonNullOfType(
		_validationContext->getNamedValidateType<ScalarType>("String"));
}

void ValidateExecutableVisitor::visit(const peg::ast_node& root)
{
	// Visit all of the fragment definitions and check for duplicates.
	peg::for_each_child<peg::fragment_definition>(root,
		[this](const peg::ast_node& fragmentDefinition) {
			const auto& fragmentName = fragmentDefinition.children.front();
			const auto inserted =
				_fragmentDefinitions.insert({ fragmentName->string(), fragmentDefinition });

			if (!inserted.second)
			{
				// http://spec.graphql.org/June2018/#sec-Fragment-Name-Uniqueness
				auto position = fragmentDefinition.begin();
				std::ostringstream error;

				error << "Duplicate fragment name: " << inserted.first->first;

				_errors.push_back({ error.str(), { position.line, position.column } });
			}
		});

	// Visit all of the operation definitions and check for duplicates.
	peg::for_each_child<peg::operation_definition>(root,
		[this](const peg::ast_node& operationDefinition) {
			std::string operationName;

			peg::on_first_child<peg::operation_name>(operationDefinition,
				[&operationName](const peg::ast_node& child) {
					operationName = child.string_view();
				});

			const auto inserted =
				_operationDefinitions.insert({ std::move(operationName), operationDefinition });

			if (!inserted.second)
			{
				// http://spec.graphql.org/June2018/#sec-Operation-Name-Uniqueness
				auto position = operationDefinition.begin();
				std::ostringstream error;

				error << "Duplicate operation name: " << inserted.first->first;

				_errors.push_back({ error.str(), { position.line, position.column } });
			}
		});

	// Check for lone anonymous operations.
	if (_operationDefinitions.size() > 1)
	{
		auto itr = std::find_if(_operationDefinitions.cbegin(),
			_operationDefinitions.cend(),
			[](const std::pair<const std::string, const peg::ast_node&>& entry) noexcept {
				return entry.first.empty();
			});

		if (itr != _operationDefinitions.cend())
		{
			// http://spec.graphql.org/June2018/#sec-Lone-Anonymous-Operation
			auto position = itr->second.begin();

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
			// http://spec.graphql.org/June2018/#sec-Executable-Definitions
			auto position = child->begin();

			_errors.push_back({ "Unexpected type definition", { position.line, position.column } });
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
		std::transform(unreferencedFragments.cbegin(),
			unreferencedFragments.cend(),
			_errors.begin() + originalSize,
			[](const std::pair<const std::string, const peg::ast_node&>&
					fragmentDefinition) noexcept {
				auto position = fragmentDefinition.second.begin();
				std::ostringstream message;

				message << "Unused fragment definition name: " << fragmentDefinition.first;

				return schema_error { message.str(), { position.line, position.column } };
			});
	}
}

std::vector<schema_error> ValidateExecutableVisitor::getStructuredErrors()
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

	const auto name = fragmentDefinition.children.front()->string();
	const auto& selection = *fragmentDefinition.children.back();
	const auto& typeCondition = fragmentDefinition.children[1];
	const auto& innerTypeName = typeCondition->children.front()->string_view();
	const auto& innerType = _validationContext->getNamedValidateType(innerTypeName);

	if (!innerType || innerType->isInputType())
	{
		// http://spec.graphql.org/June2018/#sec-Fragment-Spread-Type-Existence
		// http://spec.graphql.org/June2018/#sec-Fragments-On-Composite-Types
		auto position = typeCondition->begin();
		std::ostringstream message;

		message << (!innerType ? "Undefined target type on fragment definition: "
							   : "Scalar target type on fragment definition: ")
				<< name << " name: " << innerTypeName;

		_errors.push_back({ message.str(), { position.line, position.column } });
		return;
	}

	_fragmentStack.insert(name);
	_scopedType = std::move(innerType);

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

	std::string operationName;

	peg::on_first_child<peg::operation_name>(operationDefinition,
		[&operationName](const peg::ast_node& child) {
			operationName = child.string_view();
		});

	_operationVariables = std::make_optional<VariableTypes>();

	peg::for_each_child<peg::variable>(operationDefinition,
		[this, &operationName](const peg::ast_node& variable) {
			std::string variableName;
			ValidateArgument variableArgument;

			for (const auto& child : variable.children)
			{
				if (child->is_type<peg::variable_name>())
				{
					// Skip the $ prefix
					variableName = child->string_view().substr(1);

					if (_operationVariables->find(variableName) != _operationVariables->end())
					{
						// http://spec.graphql.org/June2018/#sec-Variable-Uniqueness
						auto position = child->begin();
						std::ostringstream message;

						message << "Conflicting variable";

						if (!operationName.empty())
						{
							message << " operation: " << operationName;
						}

						message << " name: " << variableName;

						_errors.push_back({ message.str(), { position.line, position.column } });
						return;
					}
				}
				else if (child->is_type<peg::named_type>() || child->is_type<peg::list_type>()
					|| child->is_type<peg::nonnull_type>())
				{
					ValidateVariableTypeVisitor visitor(*_validationContext);

					visitor.visit(*child);

					if (!visitor.isInputType())
					{
						// http://spec.graphql.org/June2018/#sec-Variables-Are-Input-Types
						auto position = child->begin();
						std::ostringstream message;

						message << "Invalid variable type";

						if (!operationName.empty())
						{
							message << " operation: " << operationName;
						}

						message << " name: " << variableName;

						_errors.push_back({ message.str(), { position.line, position.column } });
						return;
					}

					variableArgument.type = visitor.getType();
				}
				else if (child->is_type<peg::default_value>())
				{
					ValidateArgumentValueVisitor visitor(_errors);

					visitor.visit(*child->children.back());

					auto argument = visitor.getArgumentValue();

					if (!validateInputValue(false, argument, *variableArgument.type))
					{
						// http://spec.graphql.org/June2018/#sec-Values-of-Correct-Type
						auto position = child->begin();
						std::ostringstream message;

						message << "Incompatible variable default value";

						if (!operationName.empty())
						{
							message << " operation: " << operationName;
						}

						message << " name: " << variableName;

						_errors.push_back({ message.str(), { position.line, position.column } });
						return;
					}

					variableArgument.defaultValue = true;
					variableArgument.nonNullDefaultValue = argument.value != nullptr;
				}
			}

			_variableDefinitions.insert({ variableName, variable });
			_operationVariables->insert({ std::move(variableName), std::move(variableArgument) });
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

	const auto& typeRef = _validationContext->getOperationType(operationType);
	if (!typeRef)
	{
		auto position = operationDefinition.begin();
		std::ostringstream error;

		error << "Unsupported operation type: " << operationType;

		_errors.push_back({ error.str(), { position.line, position.column } });
		return;
	}

	_scopedType = _validationContext->getNamedValidateType(typeRef.value().get());
	_fieldCount = 0;

	const auto& selection = *operationDefinition.children.back();

	visitSelection(selection);

	if (_fieldCount > 1 && operationType == strSubscription)
	{
		// http://spec.graphql.org/June2018/#sec-Single-root-field
		auto position = operationDefinition.begin();
		std::ostringstream error;

		error << "Subscription with more than one root field";

		if (!operationName.empty())
		{
			error << " name: " << operationName;
		}

		_errors.push_back({ error.str(), { position.line, position.column } });
	}

	_scopedType.reset();
	_fragmentStack.clear();
	_selectionFields.clear();

	for (const auto& variable : _variableDefinitions)
	{
		if (_referencedVariables.find(variable.first) == _referencedVariables.end())
		{
			// http://spec.graphql.org/June2018/#sec-All-Variables-Used
			auto position = variable.second.begin();
			std::ostringstream error;

			error << "Unused variable name: " << variable.first;

			_errors.push_back({ error.str(), { position.line, position.column } });
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

ValidateTypeFieldArguments ValidationContext::getArguments(const response::ListType& args)
{
	ValidateTypeFieldArguments result;

	for (const auto& arg : args)
	{
		if (arg.type() != response::Type::Map)
		{
			continue;
		}

		const auto& itrName = arg.find(R"gql(name)gql");
		const auto& itrType = arg.find(R"gql(type)gql");
		const auto& itrDefaultValue = arg.find(R"gql(defaultValue)gql");

		if (itrName != arg.end() && itrName->second.type() == response::Type::String
			&& itrType != arg.end() && itrType->second.type() == response::Type::Map)
		{
			ValidateArgument argument;

			argument.defaultValue = (itrDefaultValue != arg.end()
				&& itrDefaultValue->second.type() == response::Type::String);
			argument.nonNullDefaultValue = argument.defaultValue
				&& itrDefaultValue->second.get<response::StringType>() != R"gql(null)gql";
			argument.type = getTypeFromMap(itrType->second);

			result[itrName->second.get<response::StringType>()] = std::move(argument);
		}
	}

	return result;
}

std::optional<std::reference_wrapper<const ValidateDirective>> ValidationContext::getDirective(
	const std::string& name) const
{
	const auto& itr = _directives.find(name);
	if (itr == _directives.cend())
	{
		return std::nullopt;
	}
	return std::optional<std::reference_wrapper<const ValidateDirective>>(itr->second);
}

std::optional<std::reference_wrapper<const std::string>> ValidationContext::getOperationType(
	const std::string_view& name) const
{
	if (name == strQuery)
	{
		return std::optional<std::reference_wrapper<const std::string>>(_operationTypes.queryType);
	}
	if (name == strMutation)
	{
		return std::optional<std::reference_wrapper<const std::string>>(
			_operationTypes.mutationType);
	}
	if (name == strSubscription)
	{
		return std::optional<std::reference_wrapper<const std::string>>(
			_operationTypes.subscriptionType);
	}
	return std::nullopt;
}

template <typename T, typename std::enable_if<std::is_base_of<NamedValidateType, T>::value>::type*>
std::shared_ptr<T> ValidationContext::getNamedValidateType(const std::string_view& name) const
{
	const auto& itr = _namedCache.find(name);
	if (itr != _namedCache.cend())
	{
		return std::dynamic_pointer_cast<T>(itr->second);
	}

	return nullptr;
}

template <typename T, typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type*>
std::shared_ptr<ListOfType> ValidationContext::getListOfType(std::shared_ptr<T>&& ofType) const
{
	const auto& itr = _listOfCache.find(ofType.get());
	if (itr != _listOfCache.cend())
	{
		return itr->second;
	}

	return nullptr;
}

template <typename T, typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type*>
std::shared_ptr<NonNullOfType> ValidationContext::getNonNullOfType(
	std::shared_ptr<T>&& ofType) const
{
	const auto& itr = _nonNullCache.find(ofType.get());
	if (itr != _nonNullCache.cend())
	{
		return itr->second;
	}

	return nullptr;
}

bool ValidateExecutableVisitor::matchesScopedType(const ValidateType& otherType) const
{
	switch (_scopedType->kind())
	{
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::UNION:
			return static_cast<const ContainerValidateType<ValidateTypeField>&>(*_scopedType)
				.matchesType(otherType);
		default:
			return false;
	}
}

bool ValidateExecutableVisitor::validateInputValue(
	bool hasNonNullDefaultValue, const ValidateArgumentValuePtr& argument, const ValidateType& type)
{
	if (!type.isValid())
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
				// http://spec.graphql.org/June2018/#sec-All-Variable-Uses-Defined
				std::ostringstream message;

				message << "Undefined variable name: " << variable.name;

				_errors.push_back({ message.str(), argument.position });
				return false;
			}

			_referencedVariables.insert(variable.name);

			return validateVariableType(
				hasNonNullDefaultValue || itrVariable->second.nonNullDefaultValue,
				*itrVariable->second.type,
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

	if (!argument.value)
	{
		// The null literal matches any nullable type and does not match a non-nullable type.
		if (type.kind() == introspection::TypeKind::NON_NULL && !hasNonNullDefaultValue)
		{
			_errors.push_back({ "Expected Non-Null value", argument.position });
			return false;
		}

		return true;
	}

	switch (type.kind())
	{
		case introspection::TypeKind::NON_NULL:
		{
			const auto& ofType = static_cast<const NonNullOfType&>(type).ofType();
			if (!ofType)
			{
				_errors.push_back({ "Unknown Non-Null type", argument.position });
				return false;
			}

			return validateInputValue(hasNonNullDefaultValue, argument, *ofType);
		}

		case introspection::TypeKind::LIST:
		{
			if (!std::holds_alternative<ValidateArgumentList>(argument.value->data))
			{
				_errors.push_back({ "Expected List value", argument.position });
				return false;
			}

			const auto& ofType = static_cast<const ListOfType&>(type).ofType();
			if (!ofType)
			{
				_errors.push_back({ "Unknown List type", argument.position });
				return false;
			}

			// Check every value against the target type.
			for (const auto& value : std::get<ValidateArgumentList>(argument.value->data).values)
			{
				if (!validateInputValue(false, value, *ofType))
				{
					// Error messages are added in the recursive call, so just bubble up the result.
					return false;
				}
			}

			return true;
		}

		case introspection::TypeKind::INPUT_OBJECT:
		{
			if (!type.isValid())
			{
				_errors.push_back({ "Unknown Input Object type", argument.position });
				return false;
			}

			const auto& name = type.name();
			if (!std::holds_alternative<ValidateArgumentMap>(argument.value->data))
			{
				std::ostringstream message;

				message << "Expected Input Object value name: " << name;

				_errors.push_back({ message.str(), argument.position });
				return false;
			}

			const auto& inputObj = static_cast<const InputObjectType&>(type);

			const auto& values = std::get<ValidateArgumentMap>(argument.value->data).values;
			std::set<std::string> subFields;

			// Check every value against the target type.
			for (const auto& entry : values)
			{
				const auto& fieldOpt = inputObj.getField(entry.first);
				if (!fieldOpt)
				{
					// http://spec.graphql.org/June2018/#sec-Input-Object-Field-Names
					std::ostringstream message;

					message << "Undefined Input Object field type: " << name
							<< " name: " << entry.first;

					_errors.push_back({ message.str(), entry.second.position });
					return false;
				}

				const auto& field = fieldOpt.value().get();

				if (entry.second.value || !field.defaultValue)
				{
					if (!validateInputValue(field.nonNullDefaultValue, entry.second, *field.type))
					{
						// Error messages are added in the recursive call, so just bubble up the
						// result.
						return false;
					}
				}

				subFields.insert(entry.first);
			}

			// See if all required fields were specified.
			for (const auto& entry : inputObj)
			{
				if (entry.second.defaultValue || subFields.find(entry.first) != subFields.end())
				{
					continue;
				}

				auto fieldKind = entry.second.type->kind();
				if (fieldKind == introspection::TypeKind::NON_NULL)
				{
					// http://spec.graphql.org/June2018/#sec-Input-Object-Required-Fields
					std::ostringstream message;

					message << "Missing Input Object field type: " << name
							<< " name: " << entry.first;

					_errors.push_back({ message.str(), argument.position });
					return false;
				}
			}

			return true;
		}

		case introspection::TypeKind::ENUM:
		{
			if (!type.isValid())
			{
				_errors.push_back({ "Unknown Enum value", argument.position });
				return false;
			}

			const auto& name = type.name();
			if (!std::holds_alternative<ValidateArgumentEnumValue>(argument.value->data))
			{
				std::ostringstream message;

				message << "Expected Enum value name: " << name;

				_errors.push_back({ message.str(), argument.position });
				return false;
			}

			const auto& value = std::get<ValidateArgumentEnumValue>(argument.value->data).value;
			const auto& enumValues = static_cast<const EnumType&>(type).values();

			if (enumValues.find(value) == enumValues.cend())
			{
				std::ostringstream message;

				message << "Undefined Enum value type: " << name << " name: " << value;

				_errors.push_back({ message.str(), argument.position });
				return false;
			}

			return true;
		}

		case introspection::TypeKind::SCALAR:
		{
			if (!type.isValid())
			{
				_errors.push_back({ "Unknown Scalar value", argument.position });
				return false;
			}

			const auto& name = type.name();
			if (name == R"gql(Int)gql")
			{
				if (!std::holds_alternative<response::IntType>(argument.value->data))
				{
					_errors.push_back({ "Expected Int value", argument.position });
					return false;
				}
			}
			else if (name == R"gql(Float)gql")
			{
				if (!std::holds_alternative<response::FloatType>(argument.value->data)
					&& !std::holds_alternative<response::IntType>(argument.value->data))
				{
					_errors.push_back({ "Expected Float value", argument.position });
					return false;
				}
			}
			else if (name == R"gql(String)gql")
			{
				if (!std::holds_alternative<response::StringType>(argument.value->data))
				{
					_errors.push_back({ "Expected String value", argument.position });
					return false;
				}
			}
			else if (name == R"gql(ID)gql")
			{
				if (std::holds_alternative<response::StringType>(argument.value->data))
				{
					try
					{
						const auto& value = std::get<response::StringType>(argument.value->data);
						auto decoded = Base64::fromBase64(value.data(), value.size());

						return true;
					}
					catch (const schema_exception&)
					{
						// Eat the exception and fail validation
					}
				}

				_errors.push_back({ "Expected ID value", argument.position });
				return false;
			}
			else if (name == R"gql(Boolean)gql")
			{
				if (!std::holds_alternative<response::BooleanType>(argument.value->data))
				{
					_errors.push_back({ "Expected Boolean value", argument.position });
					return false;
				}
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
	auto variableKind = variableType.kind();
	if (variableKind == introspection::TypeKind::NON_NULL)
	{
		const auto variableOfType = static_cast<const NonNullOfType&>(variableType).ofType();
		if (!variableOfType)
		{
			_errors.push_back({ "Unknown Non-Null variable type", position });
			return false;
		}

		return validateVariableType(true, *variableOfType, position, inputType);
	}

	auto inputKind = inputType.kind();
	switch (inputKind)
	{
		case introspection::TypeKind::NON_NULL:
		{
			if (!isNonNull)
			{
				// http://spec.graphql.org/June2018/#sec-All-Variable-Usages-are-Allowed
				_errors.push_back({ "Expected Non-Null variable type", position });
				return false;
			}

			// Unwrap and check the next one.
			const auto inputOfType = static_cast<const NonNullOfType&>(inputType).ofType();
			if (!inputOfType)
			{
				_errors.push_back({ "Unknown Non-Null input type", position });
				return false;
			}

			return validateVariableType(false, variableType, position, *inputOfType);
		}

		case introspection::TypeKind::LIST:
		{
			if (variableKind != inputKind)
			{
				// http://spec.graphql.org/June2018/#sec-All-Variable-Usages-are-Allowed
				_errors.push_back({ "Expected List variable type", position });
				return false;
			}

			// Unwrap and check the next one.
			const auto variableOfType = static_cast<const ListOfType&>(variableType).ofType();

			if (!variableOfType)
			{
				_errors.push_back({ "Unknown List variable type", position });
				return false;
			}

			const auto inputOfType = static_cast<const ListOfType&>(inputType).ofType();
			if (!inputOfType)
			{
				_errors.push_back({ "Unknown List input type", position });
				return false;
			}

			return validateVariableType(false, *variableOfType, position, *inputOfType);
		}

		case introspection::TypeKind::INPUT_OBJECT:
		{
			if (variableKind != inputKind)
			{
				// http://spec.graphql.org/June2018/#sec-All-Variable-Usages-are-Allowed
				_errors.push_back({ "Expected Input Object variable type", position });
				return false;
			}

			break;
		}

		case introspection::TypeKind::ENUM:
		{
			if (variableKind != inputKind)
			{
				// http://spec.graphql.org/June2018/#sec-All-Variable-Usages-are-Allowed
				_errors.push_back({ "Expected Enum variable type", position });
				return false;
			}

			break;
		}

		case introspection::TypeKind::SCALAR:
		{
			if (variableKind != inputKind)
			{
				// http://spec.graphql.org/June2018/#sec-All-Variable-Usages-are-Allowed
				_errors.push_back({ "Expected Scalar variable type", position });
				return false;
			}

			break;
		}

		default:
		{
			// http://spec.graphql.org/June2018/#sec-All-Variable-Usages-are-Allowed
			_errors.push_back({ "Unexpected input type", position });
			return false;
		}
	}

	const auto& variableName = variableType.name();
	if (variableName.empty())
	{
		_errors.push_back({ "Unknown variable type", position });
		return false;
	}

	const auto& inputName = inputType.name();
	if (inputName.empty())
	{
		_errors.push_back({ "Unknown input type", position });
		return false;
	}

	if (variableName != inputName)
	{
		// http://spec.graphql.org/June2018/#sec-All-Variable-Usages-are-Allowed
		std::ostringstream message;

		message << "Incompatible variable type: " << variableName << " name: " << inputName;

		_errors.push_back({ message.str(), position });
		return false;
	}

	return true;
}

void ValidationContext::addTypeFields(
	std::shared_ptr<ContainerValidateType<ValidateTypeField>> type,
	const response::Value& typeDescriptionMap)
{
	std::unordered_map<std::string, ValidateTypeField> fields;

	const auto& itrFields = typeDescriptionMap.find(R"gql(fields)gql");
	if (itrFields != typeDescriptionMap.end() && itrFields->second.type() == response::Type::List)
	{
		const auto& entries = itrFields->second.get<response::ListType>();

		for (const auto& entry : entries)
		{
			if (entry.type() != response::Type::Map)
			{
				continue;
			}

			const auto& itrFieldName = entry.find(R"gql(name)gql");
			const auto& itrFieldType = entry.find(R"gql(type)gql");

			if (itrFieldName != entry.end() && itrFieldName->second.type() == response::Type::String
				&& itrFieldType != entry.end()
				&& itrFieldType->second.type() == response::Type::Map)
			{
				const auto& fieldName = itrFieldName->second.get<response::StringType>();
				ValidateTypeField subField;

				subField.returnType = getTypeFromMap(itrFieldType->second);

				const auto& itrArgs = entry.find(R"gql(args)gql");
				if (itrArgs != entry.end() && itrArgs->second.type() == response::Type::List)
				{
					subField.arguments = getArguments(itrArgs->second.get<response::ListType>());
				}

				fields[std::move(fieldName)] = std::move(subField);
			}
		}

		if (type->name() == _operationTypes.queryType)
		{
			fields[R"gql(__schema)gql"] =
				ValidateTypeField { makeNonNullOfType(makeObjectType(R"gql(__Schema)gql")) };

			fields[R"gql(__type)gql"] = ValidateTypeField { makeObjectType(R"gql(__Type)gql"),
				ValidateTypeFieldArguments {
					{ R"gql(name)gql", ValidateArgument { commonTypes.nonNullString } } } };
		}
	}

	fields[R"gql(__typename)gql"] = ValidateTypeField { commonTypes.nonNullString };

	type->setFields(std::move(fields));
}

void ValidationContext::addPossibleTypes(std::shared_ptr<PossibleTypesContainerValidateType> type,
	const response::Value& typeDescriptionMap)
{
	const auto& itrPossibleTypes = typeDescriptionMap.find(R"gql(possibleTypes)gql");
	std::set<const ValidateType*> possibleTypes;

	if (itrPossibleTypes != typeDescriptionMap.end()
		&& itrPossibleTypes->second.type() == response::Type::List)
	{
		const auto& matchingTypeEntries = itrPossibleTypes->second.get<response::ListType>();

		for (const auto& matchingTypeEntry : matchingTypeEntries)
		{
			if (matchingTypeEntry.type() != response::Type::Map)
			{
				continue;
			}

			const auto& itrMatchingTypeName = matchingTypeEntry.find(R"gql(name)gql");
			if (itrMatchingTypeName != matchingTypeEntry.end()
				&& itrMatchingTypeName->second.type() == response::Type::String)
			{
				const auto& possibleType =
					getNamedValidateType(itrMatchingTypeName->second.get<response::StringType>());
				possibleTypes.insert(possibleType.get());
			}
		}
	}

	type->setPossibleTypes(std::move(possibleTypes));
}

void ValidationContext::addInputTypeFields(
	std::shared_ptr<InputObjectType> type, const response::Value& typeDescriptionMap)
{
	const auto& itrFields = typeDescriptionMap.find(R"gql(inputFields)gql");
	if (itrFields != typeDescriptionMap.end() && itrFields->second.type() == response::Type::List)
	{
		type->setFields(getArguments(itrFields->second.get<response::ListType>()));
	}
}

void ValidationContext::addEnum(
	const std::string_view& enumName, const response::Value& enumDescriptionMap)
{
	const auto& itrEnumValues = enumDescriptionMap.find(R"gql(enumValues)gql");
	if (itrEnumValues != enumDescriptionMap.end()
		&& itrEnumValues->second.type() == response::Type::List)
	{
		std::set<const std::string> enumValues;
		const auto& enumValuesEntries = itrEnumValues->second.get<response::ListType>();

		for (const auto& enumValuesEntry : enumValuesEntries)
		{
			if (enumValuesEntry.type() != response::Type::Map)
			{
				continue;
			}

			const auto& itrEnumValuesName = enumValuesEntry.find(R"gql(name)gql");
			if (itrEnumValuesName != enumValuesEntry.end()
				&& itrEnumValuesName->second.type() == response::Type::String)
			{
				enumValues.insert(itrEnumValuesName->second.get<response::StringType>());
			}
		}

		if (!enumValues.empty())
		{
			makeNamedValidateType(EnumType { enumName, std::move(enumValues) });
		}
	}
}

void ValidationContext::addObject(const std::string_view& name)
{
	makeNamedValidateType(ObjectType { name });
}

void ValidationContext::addInputObject(const std::string_view& name)
{
	makeNamedValidateType(InputObjectType { name });
}

void ValidationContext::addInterface(
	const std::string_view& name, const response::Value& typeDescriptionMap)
{
	makeNamedValidateType(InterfaceType { name });
}

void ValidationContext::addUnion(
	const std::string_view& name, const response::Value& typeDescriptionMap)
{
	makeNamedValidateType(UnionType { name });
}

template <typename T, typename std::enable_if<std::is_base_of<NamedValidateType, T>::value>::type*>
std::shared_ptr<T> ValidationContext::makeNamedValidateType(T&& typeDef)
{
	const std::string_view key(typeDef.name());

	const auto& itr = _namedCache.find(key);
	if (itr != _namedCache.cend())
	{
		return std::dynamic_pointer_cast<T>(itr->second);
	}

	auto type = std::make_shared<T>(std::move(typeDef));
	_namedCache.insert({ type->name(), type });

	return type;
}

void ValidationContext::addDirective(const std::string& name, const response::ListType& locations,
	const response::Value& descriptionMap)
{
	ValidateDirective directive;

	for (const auto& location : locations)
	{
		if (location.type() != response::Type::EnumValue)
		{
			continue;
		}

		directive.locations.insert(
			ModifiedArgument<introspection::DirectiveLocation>::convert(location));
	}

	const auto& itrArgs = descriptionMap.find(R"gql(args)gql");
	if (itrArgs != descriptionMap.end() && itrArgs->second.type() == response::Type::List)
	{
		directive.arguments = getArguments(itrArgs->second.get<response::ListType>());
	}

	_directives[name] = std::move(directive);
}

template <typename T, typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type*>
std::shared_ptr<ListOfType> ValidationContext::makeListOfType(std::shared_ptr<T>&& ofType)
{
	const ValidateType* key = ofType.get();

	const auto& itr = _listOfCache.find(key);
	if (itr != _listOfCache.cend())
	{
		return itr->second;
	}

	return _listOfCache.insert({ key, std::make_shared<ListOfType>(std::move(ofType)) })
		.first->second;
}

template <typename T, typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type*>
std::shared_ptr<NonNullOfType> ValidationContext::makeNonNullOfType(std::shared_ptr<T>&& ofType)
{
	const ValidateType* key = ofType.get();

	const auto& itr = _nonNullCache.find(key);
	if (itr != _nonNullCache.cend())
	{
		return itr->second;
	}

	return _nonNullCache.insert({ key, std::make_shared<NonNullOfType>(std::move(ofType)) })
		.first->second;
}

std::shared_ptr<ValidateType> ValidationContext::getTypeFromMap(const response::Value& typeMap)
{
	const auto& itrKind = typeMap.find(R"gql(kind)gql");
	if (itrKind == typeMap.end() || itrKind->second.type() != response::Type::EnumValue)
	{
		return std::shared_ptr<ValidateType>();
	}

	introspection::TypeKind kind =
		ModifiedArgument<introspection::TypeKind>::convert(itrKind->second);
	const auto& itrName = typeMap.find(R"gql(name)gql");
	if (itrName != typeMap.end() && itrName->second.type() == response::Type::String)
	{
		const auto& name = itrName->second.get<response::StringType>();
		if (!name.empty())
		{
			return getNamedValidateType(name);
		}
	}

	const auto& itrOfType = typeMap.find(R"gql(ofType)gql");
	if (itrOfType != typeMap.end() && itrOfType->second.type() == response::Type::Map)
	{
		std::shared_ptr<ValidateType> ofType = getTypeFromMap(itrOfType->second);
		if (ofType)
		{
			if (kind == introspection::TypeKind::LIST)
			{
				return makeListOfType(std::move(ofType));
			}
			else if (kind == introspection::TypeKind::NON_NULL)
			{
				return makeNonNullOfType(std::move(ofType));
			}
		}

		// should never reach
		return nullptr;
	}

	// should never reach
	return nullptr;
}

void ValidationContext::addScalar(const std::string_view& scalarName)
{
	makeNamedValidateType(ScalarType { scalarName });
}

void ValidateExecutableVisitor::visitField(const peg::ast_node& field)
{
	peg::on_first_child<peg::directives>(field, [this](const peg::ast_node& child) {
		visitDirectives(introspection::DirectiveLocation::FIELD, child);
	});

	std::string name;

	peg::on_first_child<peg::field_name>(field, [&name](const peg::ast_node& child) {
		name = child.string_view();
	});

	if (!_scopedType)
	{
		// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
		auto position = field.begin();
		std::ostringstream message;

		message << "Field on unknown type: " << _scopedType->name() << " name: " << name;

		_errors.push_back({ message.str(), { position.line, position.column } });
		return;
	}

	std::shared_ptr<ValidateType> wrappedType;
	std::optional<std::reference_wrapper<const ValidateTypeField>> objectFieldRef;

	switch (_scopedType->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		{
			// http://spec.graphql.org/June2018/#sec-Field-Selections-on-Objects-Interfaces-and-Unions-Types
			objectFieldRef =
				static_cast<const ContainerValidateType<ValidateTypeField>&>(*_scopedType)
					.getField(name);
			if (objectFieldRef)
			{
				wrappedType = objectFieldRef.value().get().returnType;
			}
			break;
		}

		case introspection::TypeKind::UNION:
		{
			if (name != R"gql(__typename)gql")
			{
				// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
				auto position = field.begin();
				std::ostringstream message;

				message << "Field on union type: " << _scopedType->name() << " name: " << name;

				_errors.push_back({ message.str(), { position.line, position.column } });
				return;
			}

			// http://spec.graphql.org/June2018/#sec-Field-Selections-on-Objects-Interfaces-and-Unions-Types
			wrappedType = commonTypes.nonNullString;
			break;
		}

		default:
			// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
			auto position = field.begin();
			std::ostringstream message;

			message << "Field on scalar type: " << _scopedType->name() << " name: " << name;

			_errors.push_back({ message.str(), { position.line, position.column } });
			return;
	}

	if (!wrappedType)
	{
		// http://spec.graphql.org/June2018/#sec-Field-Selections-on-Objects-Interfaces-and-Unions-Types
		auto position = field.begin();
		std::ostringstream message;

		message << "Undefined field type: " << _scopedType->name() << " name: " << name;

		_errors.push_back({ message.str(), { position.line, position.column } });
		return;
	}

	std::string alias;

	peg::on_first_child<peg::alias_name>(field, [&alias](const peg::ast_node& child) {
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
		[this, &name, &validateArguments, &argumentLocations, &argumentNames](
			const peg::ast_node& child) {
			for (auto& argument : child.children)
			{
				auto argumentName = argument->children.front()->string();
				auto position = argument->begin();

				if (validateArguments.find(argumentName) != validateArguments.end())
				{
					// http://spec.graphql.org/June2018/#sec-Argument-Uniqueness
					std::ostringstream message;

					message << "Conflicting argument type: " << _scopedType->name()
							<< " field: " << name << " name: " << argumentName;

					_errors.push_back({ message.str(), { position.line, position.column } });
					continue;
				}

				ValidateArgumentValueVisitor visitor(_errors);

				visitor.visit(*argument->children.back());
				validateArguments[argumentName] = visitor.getArgumentValue();
				argumentLocations[argumentName] = { position.line, position.column };
				argumentNames.push(std::move(argumentName));
			}
		});

	std::shared_ptr<const ValidateType> objectType =
		(_scopedType->kind() == introspection::TypeKind::OBJECT ? _scopedType : nullptr);
	ValidateField validateField(wrappedType,
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
			// http://spec.graphql.org/June2018/#sec-Field-Selection-Merging
			auto position = field.begin();
			std::ostringstream message;

			message << "Conflicting field type: " << _scopedType->name() << " name: " << name;

			_errors.push_back({ message.str(), { position.line, position.column } });
		}
	}

	if (objectFieldRef)
	{
		const auto& objectField = objectFieldRef.value().get();
		while (!argumentNames.empty())
		{
			auto argumentName = std::move(argumentNames.front());

			argumentNames.pop();

			auto itrArgument = objectField.arguments.find(argumentName);

			if (itrArgument == objectField.arguments.end())
			{
				// http://spec.graphql.org/June2018/#sec-Argument-Names
				std::ostringstream message;

				message << "Undefined argument type: " << _scopedType->name() << " field: " << name
						<< " name: " << argumentName;

				_errors.push_back({ message.str(), argumentLocations[argumentName] });
			}
		}

		for (auto& argument : objectField.arguments)
		{
			auto itrArgument = validateField.arguments.find(argument.first);
			const bool missing = itrArgument == validateField.arguments.end();

			if (!missing && itrArgument->second.value)
			{
				// The value was not null.
				if (!validateInputValue(argument.second.nonNullDefaultValue,
						itrArgument->second,
						*argument.second.type))
				{
					// http://spec.graphql.org/June2018/#sec-Values-of-Correct-Type
					std::ostringstream message;

					message << "Incompatible argument type: " << _scopedType->name()
							<< " field: " << name << " name: " << argument.first;

					_errors.push_back({ message.str(), argumentLocations[argument.first] });
				}

				continue;
			}
			else if (argument.second.defaultValue)
			{
				// The argument has a default value.
				continue;
			}

			// See if the argument is wrapped in NON_NULL
			if (argument.second.type->kind() == introspection::TypeKind::NON_NULL)
			{
				// http://spec.graphql.org/June2018/#sec-Required-Arguments
				auto position = field.begin();
				std::ostringstream message;

				message << (missing ? "Missing argument type: "
									: "Required non-null argument type: ")
						<< _scopedType->name() << " field: " << name << " name: " << argument.first;

				_errors.push_back({ message.str(), { position.line, position.column } });
			}
		}
	}

	_selectionFields.insert({ std::move(alias), std::move(validateField) });

	const peg::ast_node* selection = nullptr;

	peg::on_first_child<peg::selection_set>(field, [&selection](const peg::ast_node& child) {
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
		_scopedType = wrappedType->getInnerType();

		visitSelection(*selection);

		_scopedType = std::move(outerType);
		_selectionFields = std::move(outerFields);
		subFieldCount = _fieldCount;
		_fieldCount = outerFieldCount;
	}

	if (subFieldCount == 0)
	{
		const auto& innerType = wrappedType->getInnerType();
		if (!innerType->isInputType())
		{
			// http://spec.graphql.org/June2018/#sec-Leaf-Field-Selections
			auto position = field.begin();
			std::ostringstream message;

			message << "Missing fields on non-scalar type: " << innerType->name();

			_errors.push_back({ message.str(), { position.line, position.column } });
			return;
		}
	}

	++_fieldCount;
}

void ValidateExecutableVisitor::visitFragmentSpread(const peg::ast_node& fragmentSpread)
{
	peg::on_first_child<peg::directives>(fragmentSpread, [this](const peg::ast_node& child) {
		visitDirectives(introspection::DirectiveLocation::FRAGMENT_SPREAD, child);
	});

	const std::string name(fragmentSpread.children.front()->string_view());
	auto itr = _fragmentDefinitions.find(name);

	if (itr == _fragmentDefinitions.cend())
	{
		// http://spec.graphql.org/June2018/#sec-Fragment-spread-target-defined
		auto position = fragmentSpread.begin();
		std::ostringstream message;

		message << "Undefined fragment spread name: " << name;

		_errors.push_back({ message.str(), { position.line, position.column } });
		return;
	}

	if (_fragmentStack.find(name) != _fragmentStack.cend())
	{
		if (_fragmentCycles.insert(name).second)
		{
			// http://spec.graphql.org/June2018/#sec-Fragment-spreads-must-not-form-cycles
			auto position = fragmentSpread.begin();
			std::ostringstream message;

			message << "Cyclic fragment spread name: " << name;

			_errors.push_back({ message.str(), { position.line, position.column } });
		}

		return;
	}

	const auto& selection = *itr->second.children.back();
	const auto& typeCondition = itr->second.children[1];
	const auto& innerType =
		_validationContext->getNamedValidateType(typeCondition->children.front()->string_view());

	if (!matchesScopedType(*innerType))
	{
		// http://spec.graphql.org/June2018/#sec-Fragment-spread-is-possible
		auto position = fragmentSpread.begin();
		std::ostringstream message;

		message << "Incompatible fragment spread target type: " << innerType->name()
				<< " name: " << name;

		_errors.push_back({ message.str(), { position.line, position.column } });
		return;
	}

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
	peg::on_first_child<peg::directives>(inlineFragment, [this](const peg::ast_node& child) {
		visitDirectives(introspection::DirectiveLocation::INLINE_FRAGMENT, child);
	});

	std::string_view innerTypeName;
	schema_location typeConditionLocation;

	peg::on_first_child<peg::type_condition>(inlineFragment,
		[&innerTypeName, &typeConditionLocation](const peg::ast_node& child) {
			auto position = child.begin();

			innerTypeName = child.children.front()->string_view();
			typeConditionLocation = { position.line, position.column };
		});

	std::shared_ptr<const ValidateType> innerType;

	if (innerTypeName.empty())
	{
		innerType = _scopedType;
	}
	else
	{
		innerType = _validationContext->getNamedValidateType(innerTypeName);
		if (!innerType || innerType->isInputType())
		{
			// http://spec.graphql.org/June2018/#sec-Fragment-Spread-Type-Existence
			// http://spec.graphql.org/June2018/#sec-Fragments-On-Composite-Types
			std::ostringstream message;

			message << (!innerType ? "Undefined target type on inline fragment name: "
								   : "Scalar target type on inline fragment name: ")
					<< innerTypeName;

			_errors.push_back({ message.str(), std::move(typeConditionLocation) });
			return;
		}

		if (!matchesScopedType(*innerType))
		{
			// http://spec.graphql.org/June2018/#sec-Fragment-spread-is-possible
			std::ostringstream message;

			message << "Incompatible target type on inline fragment name: " << innerType->name();

			_errors.push_back({ message.str(), std::move(typeConditionLocation) });
			return;
		}
	}

	peg::on_first_child<peg::selection_set>(inlineFragment,
		[this, &innerType](const peg::ast_node& selection) {
			auto outerType = std::move(_scopedType);

			_scopedType = std::move(innerType);

			visitSelection(selection);

			_scopedType = std::move(outerType);
		});
}

void ValidateExecutableVisitor::visitDirectives(
	introspection::DirectiveLocation location, const peg::ast_node& directives)
{
	std::set<std::string> uniqueDirectives;

	for (const auto& directive : directives.children)
	{
		std::string directiveName;

		peg::on_first_child<peg::directive_name>(*directive,
			[&directiveName](const peg::ast_node& child) {
				directiveName = child.string_view();
			});

		if (!uniqueDirectives.insert(directiveName).second)
		{
			// http://spec.graphql.org/June2018/#sec-Directives-Are-Unique-Per-Location
			auto position = directive->begin();
			std::ostringstream message;

			message << "Conflicting directive name: " << directiveName;

			_errors.push_back({ message.str(), { position.line, position.column } });
			continue;
		}

		const auto& validateDirectiveRef = _validationContext->getDirective(directiveName);
		if (!validateDirectiveRef)
		{
			// http://spec.graphql.org/June2018/#sec-Directives-Are-Defined
			auto position = directive->begin();
			std::ostringstream message;

			message << "Undefined directive name: " << directiveName;

			_errors.push_back({ message.str(), { position.line, position.column } });
			continue;
		}

		const auto& validateDirective = validateDirectiveRef.value().get();
		if (validateDirective.locations.find(location) == validateDirective.locations.end())
		{
			// http://spec.graphql.org/June2018/#sec-Directives-Are-In-Valid-Locations
			auto position = directive->begin();
			std::ostringstream message;

			message << "Unexpected location for directive: " << directiveName;

			switch (location)
			{
				case introspection::DirectiveLocation::QUERY:
					message << " name: QUERY";
					break;

				case introspection::DirectiveLocation::MUTATION:
					message << " name: MUTATION";
					break;

				case introspection::DirectiveLocation::SUBSCRIPTION:
					message << " name: SUBSCRIPTION";
					break;

				case introspection::DirectiveLocation::FIELD:
					message << " name: FIELD";
					break;

				case introspection::DirectiveLocation::FRAGMENT_DEFINITION:
					message << " name: FRAGMENT_DEFINITION";
					break;

				case introspection::DirectiveLocation::FRAGMENT_SPREAD:
					message << " name: FRAGMENT_SPREAD";
					break;

				case introspection::DirectiveLocation::INLINE_FRAGMENT:
					message << " name: INLINE_FRAGMENT";
					break;

				default:
					break;
			}

			_errors.push_back({ message.str(), { position.line, position.column } });
			continue;
		}

		peg::on_first_child<peg::arguments>(*directive,
			[this, &directive, &directiveName, &validateDirective](const peg::ast_node& child) {
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

						_errors.push_back({ message.str(), { position.line, position.column } });
						continue;
					}

					ValidateArgumentValueVisitor visitor(_errors);

					visitor.visit(*argument->children.back());
					validateArguments[argumentName] = visitor.getArgumentValue();
					argumentLocations[argumentName] = { position.line, position.column };
					argumentNames.push(std::move(argumentName));
				}

				while (!argumentNames.empty())
				{
					auto argumentName = std::move(argumentNames.front());

					argumentNames.pop();

					const auto& itrArgument = validateDirective.arguments.find(argumentName);
					if (itrArgument == validateDirective.arguments.cend())
					{
						// http://spec.graphql.org/June2018/#sec-Argument-Names
						std::ostringstream message;

						message << "Undefined argument directive: " << directiveName
								<< " name: " << argumentName;

						_errors.push_back({ message.str(), argumentLocations[argumentName] });
					}
				}

				for (const auto& argument : validateDirective.arguments)
				{
					auto itrArgument = validateArguments.find(argument.first);
					const bool missing = itrArgument == validateArguments.end();

					if (!missing && itrArgument->second.value)
					{
						// The value was not null.
						if (!validateInputValue(argument.second.nonNullDefaultValue,
								itrArgument->second,
								*argument.second.type))
						{
							// http://spec.graphql.org/June2018/#sec-Values-of-Correct-Type
							std::ostringstream message;

							message << "Incompatible argument directive: " << directiveName
									<< " name: " << argument.first;

							_errors.push_back({ message.str(), argumentLocations[argument.first] });
						}

						continue;
					}
					else if (argument.second.defaultValue)
					{
						// The argument has a default value.
						continue;
					}

					// See if the argument is wrapped in NON_NULL
					if (argument.second.type->kind() == introspection::TypeKind::NON_NULL)
					{
						// http://spec.graphql.org/June2018/#sec-Required-Arguments
						auto position = directive->begin();
						std::ostringstream message;

						message << (missing ? "Missing argument directive: "
											: "Required non-null argument directive: ")
								<< directiveName << " name: " << argument.first;

						_errors.push_back({ message.str(), { position.line, position.column } });
					}
				}
			});
	}
}

} /* namespace graphql::service */
