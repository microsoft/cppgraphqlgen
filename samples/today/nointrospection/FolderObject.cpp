// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "FolderObject.h"

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

namespace graphql::today {
namespace object {

Folder::Folder(std::unique_ptr<const Concept> pimpl) noexcept
	: service::Object{ getTypeNames(), getResolvers() }
	, _pimpl { std::move(pimpl) }
{
}

service::TypeNames Folder::getTypeNames() const noexcept
{
	return {
		R"gql(Node)gql"sv,
		R"gql(UnionType)gql"sv,
		R"gql(Folder)gql"sv
	};
}

service::ResolverMap Folder::getResolvers() const noexcept
{
	return {
		{ R"gql(id)gql"sv, [this](service::ResolverParams&& params) { return resolveId(std::move(params)); } },
		{ R"gql(name)gql"sv, [this](service::ResolverParams&& params) { return resolveName(std::move(params)); } },
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(unreadCount)gql"sv, [this](service::ResolverParams&& params) { return resolveUnreadCount(std::move(params)); } }
	};
}

void Folder::beginSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->beginSelectionSet(params);
}

void Folder::endSelectionSet(const service::SelectionSetParams& params) const
{
	_pimpl->endSelectionSet(params);
}

service::AwaitableResolver Folder::resolveId(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getId(service::FieldParams { std::move(selectionSetParams), std::move(directives) });
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Folder::resolveName(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getName(service::FieldParams { std::move(selectionSetParams), std::move(directives) });
	resolverLock.unlock();

	return service::ModifiedResult<std::string>::convert<service::TypeModifier::Nullable>(std::move(result), std::move(params));
}

service::AwaitableResolver Folder::resolveUnreadCount(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	service::SelectionSetParams selectionSetParams { static_cast<const service::SelectionSetParams&>(params) };
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getUnreadCount(service::FieldParams { std::move(selectionSetParams), std::move(directives) });
	resolverLock.unlock();

	return service::ModifiedResult<int>::convert(std::move(result), std::move(params));
}

service::AwaitableResolver Folder::resolve_typename(service::ResolverParams&& params) const
{
	return service::Result<std::string>::convert(std::string{ R"gql(Folder)gql" }, std::move(params));
}

} // namespace object

void AddFolderDetails(const std::shared_ptr<schema::ObjectType>& typeFolder, const std::shared_ptr<schema::Schema>& schema)
{
	typeFolder->AddInterfaces({
		std::static_pointer_cast<const schema::InterfaceType>(schema->LookupType(R"gql(Node)gql"sv))
	});
	typeFolder->AddFields({
		schema::Field::Make(R"gql(id)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(ID)gql"sv))),
		schema::Field::Make(R"gql(name)gql"sv, R"md()md"sv, std::nullopt, schema->LookupType(R"gql(String)gql"sv)),
		schema::Field::Make(R"gql(unreadCount)gql"sv, R"md()md"sv, std::nullopt, schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType(R"gql(Int)gql"sv)))
	});
}

} // namespace graphql::today
