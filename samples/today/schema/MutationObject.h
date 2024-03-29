// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef MUTATIONOBJECT_H
#define MUTATIONOBJECT_H

#include "TodaySchema.h"

namespace graphql::today::object {
namespace methods::MutationHas {

template <class TImpl>
concept applyCompleteTaskWithParams = requires (TImpl impl, service::FieldParams params, CompleteTaskInput inputArg)
{
	{ service::AwaitableObject<std::shared_ptr<CompleteTaskPayload>> { impl.applyCompleteTask(std::move(params), std::move(inputArg)) } };
};

template <class TImpl>
concept applyCompleteTask = requires (TImpl impl, CompleteTaskInput inputArg)
{
	{ service::AwaitableObject<std::shared_ptr<CompleteTaskPayload>> { impl.applyCompleteTask(std::move(inputArg)) } };
};

template <class TImpl>
concept applySetFloatWithParams = requires (TImpl impl, service::FieldParams params, double valueArg)
{
	{ service::AwaitableScalar<double> { impl.applySetFloat(std::move(params), std::move(valueArg)) } };
};

template <class TImpl>
concept applySetFloat = requires (TImpl impl, double valueArg)
{
	{ service::AwaitableScalar<double> { impl.applySetFloat(std::move(valueArg)) } };
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

} // namespace methods::MutationHas

class [[nodiscard("unnecessary construction")]] Mutation final
	: public service::Object
{
private:
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveCompleteTask(service::ResolverParams&& params) const;
	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolveSetFloat(service::ResolverParams&& params) const;

	[[nodiscard("unnecessary call")]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		[[nodiscard("unnecessary call")]] virtual service::AwaitableObject<std::shared_ptr<CompleteTaskPayload>> applyCompleteTask(service::FieldParams&& params, CompleteTaskInput&& inputArg) const = 0;
		[[nodiscard("unnecessary call")]] virtual service::AwaitableScalar<double> applySetFloat(service::FieldParams&& params, double&& valueArg) const = 0;
	};

	template <class T>
	struct [[nodiscard("unnecessary construction")]] Model final
		: Concept
	{
		explicit Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard("unnecessary call")]] service::AwaitableObject<std::shared_ptr<CompleteTaskPayload>> applyCompleteTask(service::FieldParams&& params, CompleteTaskInput&& inputArg) const override
		{
			if constexpr (methods::MutationHas::applyCompleteTaskWithParams<T>)
			{
				return { _pimpl->applyCompleteTask(std::move(params), std::move(inputArg)) };
			}
			else if constexpr (methods::MutationHas::applyCompleteTask<T>)
			{
				return { _pimpl->applyCompleteTask(std::move(inputArg)) };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Mutation::applyCompleteTask)ex");
			}
		}

		[[nodiscard("unnecessary call")]] service::AwaitableScalar<double> applySetFloat(service::FieldParams&& params, double&& valueArg) const override
		{
			if constexpr (methods::MutationHas::applySetFloatWithParams<T>)
			{
				return { _pimpl->applySetFloat(std::move(params), std::move(valueArg)) };
			}
			else if constexpr (methods::MutationHas::applySetFloat<T>)
			{
				return { _pimpl->applySetFloat(std::move(valueArg)) };
			}
			else
			{
				throw service::unimplemented_method(R"ex(Mutation::applySetFloat)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::MutationHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const override
		{
			if constexpr (methods::MutationHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	explicit Mutation(std::unique_ptr<const Concept> pimpl) noexcept;

	[[nodiscard("unnecessary call")]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard("unnecessary call")]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const override;
	void endSelectionSet(const service::SelectionSetParams& params) const override;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	explicit Mutation(std::shared_ptr<T> pimpl) noexcept
		: Mutation { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard("unnecessary call")]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(Mutation)gql" };
	}
};

} // namespace graphql::today::object

#endif // MUTATIONOBJECT_H
