// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef SCHEMAOBJECT_H
#define SCHEMAOBJECT_H

#include "IntrospectionSchema.h"

namespace graphql::introspection::object {

class [[nodiscard]] Schema final
	: public service::Object
{
private:
	[[nodiscard]] service::AwaitableResolver resolveDescription(service::ResolverParams&& params) const;
	[[nodiscard]] service::AwaitableResolver resolveTypes(service::ResolverParams&& params) const;
	[[nodiscard]] service::AwaitableResolver resolveQueryType(service::ResolverParams&& params) const;
	[[nodiscard]] service::AwaitableResolver resolveMutationType(service::ResolverParams&& params) const;
	[[nodiscard]] service::AwaitableResolver resolveSubscriptionType(service::ResolverParams&& params) const;
	[[nodiscard]] service::AwaitableResolver resolveDirectives(service::ResolverParams&& params) const;

	[[nodiscard]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard]] Concept
	{
		virtual ~Concept() = default;

		[[nodiscard]] virtual service::AwaitableScalar<std::optional<std::string>> getDescription() const = 0;
		[[nodiscard]] virtual service::AwaitableObject<std::vector<std::shared_ptr<Type>>> getTypes() const = 0;
		[[nodiscard]] virtual service::AwaitableObject<std::shared_ptr<Type>> getQueryType() const = 0;
		[[nodiscard]] virtual service::AwaitableObject<std::shared_ptr<Type>> getMutationType() const = 0;
		[[nodiscard]] virtual service::AwaitableObject<std::shared_ptr<Type>> getSubscriptionType() const = 0;
		[[nodiscard]] virtual service::AwaitableObject<std::vector<std::shared_ptr<Directive>>> getDirectives() const = 0;
	};

	template <class T>
	struct [[nodiscard]] Model
		: Concept
	{
		Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard]] service::AwaitableScalar<std::optional<std::string>> getDescription() const final
		{
			return { _pimpl->getDescription() };
		}

		[[nodiscard]] service::AwaitableObject<std::vector<std::shared_ptr<Type>>> getTypes() const final
		{
			return { _pimpl->getTypes() };
		}

		[[nodiscard]] service::AwaitableObject<std::shared_ptr<Type>> getQueryType() const final
		{
			return { _pimpl->getQueryType() };
		}

		[[nodiscard]] service::AwaitableObject<std::shared_ptr<Type>> getMutationType() const final
		{
			return { _pimpl->getMutationType() };
		}

		[[nodiscard]] service::AwaitableObject<std::shared_ptr<Type>> getSubscriptionType() const final
		{
			return { _pimpl->getSubscriptionType() };
		}

		[[nodiscard]] service::AwaitableObject<std::vector<std::shared_ptr<Directive>>> getDirectives() const final
		{
			return { _pimpl->getDirectives() };
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	const std::unique_ptr<const Concept> _pimpl;

	[[nodiscard]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard]] service::ResolverMap getResolvers() const noexcept;

public:
	GRAPHQLSERVICE_EXPORT Schema(std::shared_ptr<introspection::Schema> pimpl) noexcept;
	GRAPHQLSERVICE_EXPORT ~Schema();
};

} // namespace graphql::introspection::object

#endif // SCHEMAOBJECT_H
