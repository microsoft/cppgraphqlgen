// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef VALIDATION_MESSAGEOBJECT_H
#define VALIDATION_MESSAGEOBJECT_H

#include "ValidationSchema.h"

namespace graphql::validation::object {
namespace methods::MessageHas {

template <class TImpl>
concept getBodyWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getBody(std::move(params)) } };
};

template <class TImpl>
concept getBody = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getBody() } };
};

template <class TImpl>
concept getSenderWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<response::IdType> { impl.getSender(std::move(params)) } };
};

template <class TImpl>
concept getSender = requires (TImpl impl)
{
	{ service::AwaitableScalar<response::IdType> { impl.getSender() } };
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

} // namespace methods::MessageHas

class [[nodiscard("unnecessary construction")]] Message final
	: public service::Object
{
private:
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveBody(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveSender(service::ResolverParams&& params) const;

	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<std::optional<std::string>> getBody(service::FieldParams&& params) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<response::IdType> getSender(service::FieldParams&& params) const = 0;
	};

	template <class T>
	struct [[nodiscard("unnecessary construction")]] Model final
		: Concept
	{
		explicit Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<std::optional<std::string>> getBody(service::FieldParams&& params) const override
		{
			if constexpr (methods::MessageHas::getBodyWithParams<T>)
			{
				return { _pimpl->getBody(std::move(params)) };
			}
			else if constexpr (methods::MessageHas::getBody<T>)
			{
				return { _pimpl->getBody() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Message::getBody)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<response::IdType> getSender(service::FieldParams&& params) const override
		{
			if constexpr (methods::MessageHas::getSenderWithParams<T>)
			{
				return { _pimpl->getSender(std::move(params)) };
			}
			else if constexpr (methods::MessageHas::getSender<T>)
			{
				return { _pimpl->getSender() };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Message::getSender)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::MessageHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::MessageHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit Message(std::unique_ptr<const Concept> pimpl) noexcept;

	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit Message(std::shared_ptr<T> pimpl) noexcept
		: Message { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard("unnecessary call")]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(Message)gql" };
	}
};

} // namespace graphql::validation::object

#endif // VALIDATION_MESSAGEOBJECT_H
