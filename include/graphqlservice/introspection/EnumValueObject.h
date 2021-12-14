// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef ENUMVALUEOBJECT_H
#define ENUMVALUEOBJECT_H

#include "IntrospectionSchema.h"

namespace graphql::introspection::object {

class EnumValue
	: public service::Object
{
private:
	service::AwaitableResolver resolveName(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveDescription(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveIsDeprecated(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveDeprecationReason(service::ResolverParams&& params) const;

	service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct Concept
	{
		virtual ~Concept() = default;

		virtual service::AwaitableScalar<std::string> getName() const = 0;
		virtual service::AwaitableScalar<std::optional<std::string>> getDescription() const = 0;
		virtual service::AwaitableScalar<bool> getIsDeprecated() const = 0;
		virtual service::AwaitableScalar<std::optional<std::string>> getDeprecationReason() const = 0;
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

		service::AwaitableScalar<bool> getIsDeprecated() const final
		{
			return { _pimpl->getIsDeprecated() };
		}

		service::AwaitableScalar<std::optional<std::string>> getDeprecationReason() const final
		{
			return { _pimpl->getDeprecationReason() };
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	const std::unique_ptr<Concept> _pimpl;

	service::TypeNames getTypeNames() const noexcept;
	service::ResolverMap getResolvers() const noexcept;

public:
	GRAPHQLINTROSPECTION_EXPORT EnumValue(std::shared_ptr<introspection::EnumValue> pimpl) noexcept;
	GRAPHQLINTROSPECTION_EXPORT ~EnumValue();
};

} // namespace graphql::introspection::object

#endif // ENUMVALUEOBJECT_H
