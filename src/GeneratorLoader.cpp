// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GeneratorLoader.h"

#include "graphqlservice/GraphQLGrammar.h"

#include <algorithm>

namespace graphql::generator {

void TypeVisitor::visit(const peg::ast_node& typeName)
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

void TypeVisitor::visitNamedType(const peg::ast_node& namedType)
{
	if (!_nonNull)
	{
		_modifiers.push_back(service::TypeModifier::Nullable);
	}

	_type = namedType.string_view();
}

void TypeVisitor::visitListType(const peg::ast_node& listType)
{
	if (!_nonNull)
	{
		_modifiers.push_back(service::TypeModifier::Nullable);
	}
	_nonNull = false;

	_modifiers.push_back(service::TypeModifier::List);

	visit(*listType.children.front());
}

void TypeVisitor::visitNonNullType(const peg::ast_node& nonNullType)
{
	_nonNull = true;

	visit(*nonNullType.children.front());
}

std::pair<std::string_view, TypeModifierStack> TypeVisitor::getType()
{
	return { std::move(_type), std::move(_modifiers) };
}

void DefaultValueVisitor::visit(const peg::ast_node& value)
{
	if (value.is_type<peg::integer_value>())
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
	else if (value.is_type<peg::variable>())
	{
		std::ostringstream error;
		const auto position = value.begin();

		error << "Unexpected variable in default value line: " << position.line
			  << " column: " << position.column;

		throw std::runtime_error(error.str());
	}
}

void DefaultValueVisitor::visitIntValue(const peg::ast_node& intValue)
{
	_value = response::Value(std::atoi(intValue.string().c_str()));
}

void DefaultValueVisitor::visitFloatValue(const peg::ast_node& floatValue)
{
	_value = response::Value(std::atof(floatValue.string().c_str()));
}

void DefaultValueVisitor::visitStringValue(const peg::ast_node& stringValue)
{
	_value = response::Value(std::string { stringValue.unescaped_view() });
}

void DefaultValueVisitor::visitBooleanValue(const peg::ast_node& booleanValue)
{
	_value = response::Value(booleanValue.is_type<peg::true_keyword>());
}

void DefaultValueVisitor::visitNullValue(const peg::ast_node& /*nullValue*/)
{
	_value = {};
}

void DefaultValueVisitor::visitEnumValue(const peg::ast_node& enumValue)
{
	_value = response::Value(response::Type::EnumValue);
	_value.set<response::StringType>(enumValue.string());
}

void DefaultValueVisitor::visitListValue(const peg::ast_node& listValue)
{
	_value = response::Value(response::Type::List);
	_value.reserve(listValue.children.size());

	for (const auto& child : listValue.children)
	{
		DefaultValueVisitor visitor;

		visitor.visit(*child);
		_value.emplace_back(visitor.getValue());
	}
}

void DefaultValueVisitor::visitObjectValue(const peg::ast_node& objectValue)
{
	_value = response::Value(response::Type::Map);
	_value.reserve(objectValue.children.size());

	for (const auto& field : objectValue.children)
	{
		DefaultValueVisitor visitor;

		visitor.visit(*field->children.back());
		_value.emplace_back(field->children.front()->string(), visitor.getValue());
	}
}

response::Value DefaultValueVisitor::getValue()
{
	return response::Value(std::move(_value));
}

} // namespace graphql::generator