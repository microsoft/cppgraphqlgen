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
	explicit SelectionVisitor(const service::SelectionSetParams& selectionSetParams,
		const service::FragmentMap& fragments, const response::Value& variables,
		const service::TypeNames& typeNames, const service::ResolverMap& resolvers, size_t count);

	void visit(const peg::ast_node& selection);

	std::vector<std::pair<std::string_view, std::future<service::ResolverResult>>> getValues();

private:
	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	const service::ResolverContext _resolverContext;
	const std::shared_ptr<service::RequestState>& _state;
	const response::Value& _operationDirectives;
	const std::optional<std::reference_wrapper<const service::field_path>> _path;
	const std::launch _launch;
	const service::FragmentMap& _fragments;
	const response::Value& _variables;
	const service::TypeNames& _typeNames;
	const service::ResolverMap& _resolvers;

	std::list<FragmentDirectives> _fragmentDirectives;
	internal::string_view_set _names;
	std::vector<std::pair<std::string_view, std::future<service::ResolverResult>>> _values;
};

// FragmentDefinitionVisitor visits the AST and collects all of the fragment
// definitions in the document.
class FragmentDefinitionVisitor
{
public:
	FragmentDefinitionVisitor(const response::Value& variables);

	service::FragmentMap getFragments();

	void visit(const peg::ast_node& fragmentDefinition);

private:
	const response::Value& _variables;

	service::FragmentMap _fragments;
};

// OperationDefinitionVisitor visits the AST and executes the one with the specified
// operation name.
class OperationDefinitionVisitor
{
public:
	OperationDefinitionVisitor(service::ResolverContext resolverContext, std::launch launch,
		std::shared_ptr<service::RequestState> state, const service::TypeMap& operations, response::Value&& variables,
		service::FragmentMap&& fragments);

	std::future<service::ResolverResult> getValue();

	void visit(std::string_view operationType, const peg::ast_node& operationDefinition);

private:
	const service::ResolverContext _resolverContext;
	const std::launch _launch;
	std::shared_ptr<service::OperationData> _params;
	const service::TypeMap& _operations;
	std::future<service::ResolverResult> _result;
};

// SubscriptionDefinitionVisitor visits the AST collects the fields referenced in the subscription
// at the point where we create a subscription.
class SubscriptionDefinitionVisitor
{
public:
	SubscriptionDefinitionVisitor(service::SubscriptionParams&& params, service::SubscriptionCallback&& callback,
		service::FragmentMap&& fragments, const std::shared_ptr<service::Object>& subscriptionObject);

	const peg::ast_node& getRoot() const;
	std::shared_ptr<service::SubscriptionData> getRegistration();

	void visit(const peg::ast_node& operationDefinition);

private:
	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	service::SubscriptionParams _params;
	service::SubscriptionCallback _callback;
	service::FragmentMap _fragments;
	const std::shared_ptr<service::Object>& _subscriptionObject;
	service::SubscriptionName _field;
	response::Value _arguments;
	response::Value _fieldDirectives;
	std::shared_ptr<service::SubscriptionData> _result;
};

} /* namespace graphql::runtime */

#endif // REQUESTLOADER_H
