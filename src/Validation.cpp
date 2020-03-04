// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLGrammar.h"

#include "Validation.h"

#include <iostream>
#include <algorithm>

namespace graphql::service {

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

ValidateExecutableVisitor::ValidateExecutableVisitor(const Request& service)
	: _service(service)
{
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
	auto ast = peg::parseString(query);

	// This is taking advantage of the fact that during validation we can choose to execute
	// unvalidated queries against the Introspection schema. This way we can use fragment
	// cycles to expand an arbitrary number of wrapper types.
	ast.validated = true;

	response::Value data(response::Type::Map);
	std::shared_ptr<RequestState> state;
	const std::string operationName;
	response::Value variables(response::Type::Map);
	auto result = _service.resolve(state, ast, operationName, std::move(variables)).get();
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

			message << "Unused fragment definition name: " << fragmentDefinition.first;

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
	const auto& typeCondition = fragmentDefinition.children[1];
	auto innerType = typeCondition->children.front()->string();

	peg::on_first_child<peg::directives>(fragmentDefinition,
		[this](const peg::ast_node& child)
	{
		visitDirectives(introspection::DirectiveLocation::FRAGMENT_DEFINITION, child);
	});

	auto itrKind = _typeKinds.find(innerType);

	if (itrKind == _typeKinds.end())
	{
		// http://spec.graphql.org/June2018/#sec-Fragment-Spread-Type-Existence
		auto position = typeCondition->begin();
		std::ostringstream message;

		message << "Undefined target type on fragment definition: " << name
			<< " name: " << innerType;

		_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
		return;
	}

	switch (itrKind->second)
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
			break;

		default:
		{
			// http://spec.graphql.org/June2018/#sec-Fragments-On-Composite-Types
			auto position = typeCondition->begin();
			std::ostringstream message;

			message << "Scalar target type on fragment definition: " << name
				<< " name: " << innerType;

			_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
			return;
		}
	}

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

		message << "Undefined fragment spread name: " << name;

		_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
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

			_errors.push_back({ message.str(), { position.line, position.byte_in_line } });
		}

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
	schema_location typeConditionLocation;

	peg::on_first_child<peg::type_condition>(inlineFragment,
		[&innerType, &typeConditionLocation](const peg::ast_node& child)
	{
		auto position = child.begin();

		innerType = child.children.front()->string();
		typeConditionLocation = { position.line, position.byte_in_line };
	});

	if (innerType.empty())
	{
		innerType = _scopedType;
	}
	else
	{
		auto itrKind = _typeKinds.find(innerType);

		if (_typeKinds.find(innerType) == _typeKinds.end())
		{
			// http://spec.graphql.org/June2018/#sec-Fragment-Spread-Type-Existence
			std::ostringstream message;

			message << "Undefined target type on inline fragment name: " << innerType;

			_errors.push_back({ message.str(), std::move(typeConditionLocation) });
			return;
		}

		switch (itrKind->second)
		{
			case introspection::TypeKind::OBJECT:
			case introspection::TypeKind::INTERFACE:
			case introspection::TypeKind::UNION:
				break;

			default:
			{
				// http://spec.graphql.org/June2018/#sec-Fragments-On-Composite-Types
				std::ostringstream message;

				message << "Scalar target type on inline fragment name: " << innerType;

				_errors.push_back({ message.str(), std::move(typeConditionLocation) });
				return;
			}
		}
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

} /* namespace graphql::service */
