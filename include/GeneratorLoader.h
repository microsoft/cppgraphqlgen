// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GENERATORLOADER_H
#define GENERATORLOADER_H

#include "graphqlservice/GraphQLService.h"

namespace graphql::generator {

// Types that we understand and use to generate the skeleton of a service.
enum class SchemaType
{
	Scalar,
	Enum,
	Input,
	Union,
	Interface,
	Object,
	Operation,
};

using SchemaTypeMap = std::map<std::string_view, SchemaType>;

// Any type can also have a list and/or non-nullable wrapper, and those can be nested.
// Since it's easier to express nullability than non-nullability in C++, we'll invert
// the presence of NonNull modifiers.
using TypeModifierStack = std::vector<service::TypeModifier>;

// Recursively visit a Type node until we reach a NamedType and we've
// taken stock of all of the modifier wrappers.
class TypeVisitor
{
public:
	std::pair<std::string_view, TypeModifierStack> getType();

	void visit(const peg::ast_node& typeName);

private:
	void visitNamedType(const peg::ast_node& namedType);
	void visitListType(const peg::ast_node& listType);
	void visitNonNullType(const peg::ast_node& nonNullType);

	std::string_view _type;
	TypeModifierStack _modifiers;
	bool _nonNull = false;
};

// Recursively visit a Value node representing the default value on an input field
// and build a JSON representation of the hardcoded value.
class DefaultValueVisitor
{
public:
	response::Value getValue();

	void visit(const peg::ast_node& value);

private:
	void visitIntValue(const peg::ast_node& intValue);
	void visitFloatValue(const peg::ast_node& floatValue);
	void visitStringValue(const peg::ast_node& stringValue);
	void visitBooleanValue(const peg::ast_node& booleanValue);
	void visitNullValue(const peg::ast_node& nullValue);
	void visitEnumValue(const peg::ast_node& enumValue);
	void visitListValue(const peg::ast_node& listValue);
	void visitObjectValue(const peg::ast_node& objectValue);

	response::Value _value;
};

} /* namespace graphql::generator */

#endif // GENERATORLOADER_H
