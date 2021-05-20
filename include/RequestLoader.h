// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef REQUESTLOADER_H
#define REQUESTLOADER_H

#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLService.h"

namespace graphql::runtime {

// ValueVisitor visits the AST and builds a response::Value representation of any value
// hardcoded or referencing a variable in an operation.
class ValueVisitor
{
public:
	GRAPHQLSERVICE_EXPORT ValueVisitor(const response::Value& variables);

	GRAPHQLSERVICE_EXPORT void visit(const peg::ast_node& value);

	GRAPHQLSERVICE_EXPORT response::Value getValue();

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

// DirectiveVisitor visits the AST and builds a 2-level map of directive names to argument
// name/value pairs.
class DirectiveVisitor
{
public:
	GRAPHQLSERVICE_EXPORT explicit DirectiveVisitor(const response::Value& variables);

	GRAPHQLSERVICE_EXPORT void visit(const peg::ast_node& directives);

	GRAPHQLSERVICE_EXPORT bool shouldSkip() const;
	GRAPHQLSERVICE_EXPORT response::Value getDirectives();

private:
	const response::Value& _variables;

	response::Value _directives;
};

// FragmentDefinitionVisitor visits the AST and collects all of the fragment
// definitions in the document.
class FragmentDefinitionVisitor
{
public:
	GRAPHQLSERVICE_EXPORT FragmentDefinitionVisitor(const response::Value& variables);

	GRAPHQLSERVICE_EXPORT service::FragmentMap getFragments();

	GRAPHQLSERVICE_EXPORT void visit(const peg::ast_node& fragmentDefinition);

private:
	const response::Value& _variables;

	service::FragmentMap _fragments;
};

} /* namespace graphql::runtime */

#endif // REQUESTLOADER_H
