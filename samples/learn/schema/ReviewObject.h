// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef REVIEWOBJECT_H
#define REVIEWOBJECT_H

#include "StarWarsSchema.h"

namespace graphql::learn::object {
namespace methods::ReviewHas {

template <class TImpl>
concept getStarsWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<int> { impl.getStars(std::move(params)) } };
};

template <class TImpl>
concept getStars = requires (TImpl impl)
{
	{ service::AwaitableScalar<int> { impl.getStars() } };
};

template <class TImpl>
concept getCommentaryWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getCommentary(std::move(params)) } };
};

template <class TImpl>
concept getCommentary = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getCommentary() } };
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

} // namespace methods::ReviewHas

class [[nodiscard("unnecessary construction")]] Review final
	: public service::Object
{
private:
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveStars(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveCommentary(service::ResolverParams&& params) const;

	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<int> getStars(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<std::string>> getCommentary(service::FieldParams&& params) const = 0;
	};

	template <class T>
	struct [[nodiscard("unnecessary construction")]] Model final
		: Concept
	{
		explicit Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<int> getStars(service::FieldParams&& params) const override
		{
			if constexpr (methods::ReviewHas::getStarsWithParams<T>)
			{
				return { _pimpl->getStars(std::move(params)) };
			}
			else
			{
				static_assert(methods::ReviewHas::getStars<T>, R"msg(Review::getStars is not implemented)msg");
				return { _pimpl->getStars() };
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<std::string>> getCommentary(service::FieldParams&& params) const override
		{
			if constexpr (methods::ReviewHas::getCommentaryWithParams<T>)
			{
				return { _pimpl->getCommentary(std::move(params)) };
			}
			else
			{
				static_assert(methods::ReviewHas::getCommentary<T>, R"msg(Review::getCommentary is not implemented)msg");
				return { _pimpl->getCommentary() };
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::ReviewHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::ReviewHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit Review(std::unique_ptr<const Concept> pimpl) noexcept;

	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit Review(std::shared_ptr<T> pimpl) noexcept
		: Review { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard("unnecessary call")]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(Review)gql" };
	}
};

} // namespace graphql::learn::object

#endif // REVIEWOBJECT_H
