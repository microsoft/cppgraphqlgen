// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef VALIDATION_ALIENOBJECT_H
#define VALIDATION_ALIENOBJECT_H

#include "ValidationSchema.h"

namespace graphql::validation::object {
namespace implements {

template <class I>
concept AlienIs = std::is_same_v<I, Sentient> || std::is_same_v<I, HumanOrAlien>;

} // namespace implements

namespace methods::AlienHas {

template <class TImpl>
concept getNameWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::string> { impl.getName(std::move(params)) } };
};

template <class TImpl>
concept getName = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::string> { impl.getName() } };
};

template <class TImpl>
concept getHomePlanetWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getHomePlanet(std::move(params)) } };
};

template <class TImpl>
concept getHomePlanet = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getHomePlanet() } };
};

template <class TImpl>
concept beginSelectionSet = requires (TImpl impl, const service::SelectionSetParams params)
{
	{ impl.beginSelectionSet(params) };
};

template <class TImpl>
concept endSelectionSet = requires (TImpl impl, const service::SelectionSetParams params)
{
	{ impl.endSelectionSet(params) };
};

} // namespace methods::AlienHas

class [[nodiscard("unnecessary construction")]] Alien final
	: public service::Object
{
private:
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveName(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveHomePlanet(service::ResolverParams&& params) const;

	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::string> getName(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<std::string>> getHomePlanet(service::FieldParams&& params) const = 0;
	};

	template <class T>
	struct [[nodiscard("unnecessary construction")]] Model final
		: Concept
	{
		explicit Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::string> getName(service::FieldParams&& params) const override
		{
			if constexpr (methods::AlienHas::getNameWithParams<T>)
			{
				return { _pimpl->getName(std::move(params)) };
			}
			else if constexpr (methods::AlienHas::getName<T>)
			{
				return { _pimpl->getName() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Alien::getName)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<std::string>> getHomePlanet(service::FieldParams&& params) const override
		{
			if constexpr (methods::AlienHas::getHomePlanetWithParams<T>)
			{
				return { _pimpl->getHomePlanet(std::move(params)) };
			}
			else if constexpr (methods::AlienHas::getHomePlanet<T>)
			{
				return { _pimpl->getHomePlanet() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Alien::getHomePlanet)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::AlienHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::AlienHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit Alien(std::unique_ptr<const Concept> pimpl) noexcept;

	// Interfaces which this type implements
	friend Sentient;

	// Unions which include this type
	friend HumanOrAlien;

	template <class I>
	[[nodiscard("unnecessary call")]] static constexpr bool implements() noexcept
	{
		return implements::AlienIs<I>;
	}

	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit Alien(std::shared_ptr<T> pimpl) noexcept
		: Alien { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard("unnecessary call")]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(Alien)gql" };
	}
};

} // namespace graphql::validation::object

#endif // VALIDATION_ALIENOBJECT_H
