// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef DIRECTIVEOBJECT_H
#define DIRECTIVEOBJECT_H

#include "IntrospectionSchema.h"

namespace graphql::introspection::object {

class Directive
	: public service::Object
{
private:
	service::AwaitableResolver resolveName(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveDescription(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveLocations(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveArgs(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveIsRepeatable(service::ResolverParams&& params) const;

	service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct Concept
	{
		virtual ~Concept() = default;

		virtual service::AwaitableScalar<std::string> getName() const = 0;
		virtual service::AwaitableScalar<std::optional<std::string>> getDescription() const = 0;
		virtual service::AwaitableScalar<std::vector<DirectiveLocation>> getLocations() const = 0;
		virtual service::AwaitableObject<std::vector<std::shared_ptr<InputValue>>> getArgs() const = 0;
		virtual service::AwaitableScalar<bool> getIsRepeatable() const = 0;
	};

	template <class T>
	struct Model
		: Concept
	{
		Model(std::shared_ptr<T>&& pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		service::AwaitableScalar<std::string> getName() const final
		{
			return { _pimpl->getName() };
		}

		service::AwaitableScalar<std::optional<std::string>> getDescription() const final
		{
			return { _pimpl->getDescription() };
		}

		service::AwaitableScalar<std::vector<DirectiveLocation>> getLocations() const final
		{
			return { _pimpl->getLocations() };
		}

		service::AwaitableObject<std::vector<std::shared_ptr<InputValue>>> getArgs() const final
		{
			return { _pimpl->getArgs() };
		}

		service::AwaitableScalar<bool> getIsRepeatable() const final
		{
			return { _pimpl->getIsRepeatable() };
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	const std::unique_ptr<Concept> _pimpl;

	service::TypeNames getTypeNames() const noexcept;
	service::ResolverMap getResolvers() const noexcept;

public:
	GRAPHQLINTROSPECTION_EXPORT Directive(std::shared_ptr<introspection::Directive> pimpl) noexcept;
	GRAPHQLINTROSPECTION_EXPORT ~Directive();
};

} // namespace graphql::introspection::object

#endif // DIRECTIVEOBJECT_H
