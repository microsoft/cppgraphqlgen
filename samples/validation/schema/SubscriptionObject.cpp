// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "SubscriptionObject.h"
#include "MessageObject.h"

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

namespace graphql::validation {
namespace object {

Subscription::Subscription(std::unique_ptr<const Concept> pimpl) noexcept
	: service::Object{ getTypeNames(), getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}

service::TypeNames Subscription::getTypeNames() const noexcept
{
	return {
		R"gql(Subscription)gql"sv
	};
}

service::ResolverMap Subscription::getResolvers() const noexcept
{
	return {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(newMessage)gql"sv, [this](service::ResolverParams&& params) { return resolveNewMessage(std::move(params)); } },
		{ R"gql(disallowedSecondRootField)gql"sv, [this](service::ResolverParams&& params) { return resolveDisallowedSecondRootField(std::move(params)); } }
	};
}

void Subscription::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void Subscription::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}

service::AwaitableResolver Subscription::resolveNewMessage(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getNewMessage(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<Message>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Subscription::resolveDisallowedSecondRootField(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getDisallowedSecondRootField(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<bool>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Subscription::resolve_typename(service::ResolverParams&& params) const
{
	return service::Result<std::string>::convert(std::string{ R"gql(Subscription)gql" }, std::move(params));
}

} // namespace object

void AddSubscriptionDetails(const std::shared_ptr<schema::ObjectType>& typeSubscription, const std::shared_ptr<schema::Schema>& schema)
{
	typeSubscription->AddFields({
		schema::Field::Make(R"gql(newMessage)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Message)gql"sv))),
		schema::Field::Make(R"gql(disallowedSecondRootField)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Boolean)gql"sv)))
	});
}

} // namespace graphql::validation
