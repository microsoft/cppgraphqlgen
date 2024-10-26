// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "TodayExpensiveObject.h"

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

namespace graphql::today {
namespace object {

Expensive::Expensive(std::unique_ptr<const Concept> pimpl) noexcept
	: service::Object{ getTypeNames(), getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}

service::TypeNames Expensive::getTypeNames() const noexcept
{
	return {
		R"gql(Expensive)gql"sv
	};
}

service::ResolverMap Expensive::getResolvers() const noexcept
{
	return {
		{ R"gql(order)gql"sv, [this](service::ResolverParams&& params) { return resolveOrder(std::move(params)); } },
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } }
	};
}

void Expensive::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void Expensive::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}

service::AwaitableResolver Expensive::resolveOrder(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getOrder(service::FieldParams { std::move(selectionSetParams), std::move(directives) });
	resolverLock.unlock();

	return service::ModifiedResult<int>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Expensive::resolve_typename(service::ResolverParams&& params) const
{
	return service::Result<std::string>::convert(std::string{ R"gql(Expensive)gql" }, std::move(params));
}

} // namespace object

void AddExpensiveDetails(const std::shared_ptr<schema::ObjectType>& typeExpensive, const std::shared_ptr<schema::Schema>& schema)
{
	typeExpensive->AddFields({
		schema::Field::Make(R"gql(order)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Int)gql"sv)))
	});
}

} // namespace graphql::today
