// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef APPOINTMENTOBJECT_H
#define APPOINTMENTOBJECT_H

#include "TodaySchema.h"

namespace graphql::today::object {
namespace implements {

template <class I>
concept AppointmentIs = std::is_same_v<I, Node> || std::is_same_v<I, UnionType>;

} // namespace implements

namespace methods::AppointmentHas {

template <class TImpl>
concept getIdWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<response::IdType> { impl.getId(std::move(params)) } };
};

template <class TImpl>
concept getId = requires (TImpl impl)
{
	{ service::AwaitableScalar<response::IdType> { impl.getId() } };
};

template <class TImpl>
concept getWhenWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<response::Value>> { impl.getWhen(std::move(params)) } };
};

template <class TImpl>
concept getWhen = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<response::Value>> { impl.getWhen() } };
};

template <class TImpl>
concept getSubjectWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getSubject(std::move(params)) } };
};

template <class TImpl>
concept getSubject = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getSubject() } };
};

template <class TImpl>
concept getIsNowWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<bool> { impl.getIsNow(std::move(params)) } };
};

template <class TImpl>
concept getIsNow = requires (TImpl impl)
{
	{ service::AwaitableScalar<bool> { impl.getIsNow() } };
};

template <class TImpl>
concept getForceErrorWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getForceError(std::move(params)) } };
};

template <class TImpl>
concept getForceError = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getForceError() } };
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

} // namespace methods::AppointmentHas

class [[nodiscard]] Appointment final
	: public service::Object
{
private:
	[[nodiscard]] service::AwaitableResolver resolveId(service::ResolverParams&& params) const;
	[[nodiscard]] service::AwaitableResolver resolveWhen(service::ResolverParams&& params) const;
	[[nodiscard]] service::AwaitableResolver resolveSubject(service::ResolverParams&& params) const;
	[[nodiscard]] service::AwaitableResolver resolveIsNow(service::ResolverParams&& params) const;
	[[nodiscard]] service::AwaitableResolver resolveForceError(service::ResolverParams&& params) const;

	[[nodiscard]] service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct [[nodiscard]] Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		[[nodiscard]] virtual service::AwaitableScalar<response::IdType> getId(service::FieldParams&& params) const = 0;
		[[nodiscard]] virtual service::AwaitableScalar<std::optional<response::Value>> getWhen(service::FieldParams&& params) const = 0;
		[[nodiscard]] virtual service::AwaitableScalar<std::optional<std::string>> getSubject(service::FieldParams&& params) const = 0;
		[[nodiscard]] virtual service::AwaitableScalar<bool> getIsNow(service::FieldParams&& params) const = 0;
		[[nodiscard]] virtual service::AwaitableScalar<std::optional<std::string>> getForceError(service::FieldParams&& params) const = 0;
	};

	template <class T>
	struct [[nodiscard]] Model
		: Concept
	{
		Model(std::shared_ptr<T> pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		[[nodiscard]] service::AwaitableScalar<response::IdType> getId(service::FieldParams&& params) const final
		{
			if constexpr (methods::AppointmentHas::getIdWithParams<T>)
			{
				return { _pimpl->getId(std::move(params)) };
			}
			else if constexpr (methods::AppointmentHas::getId<T>)
			{
				return { _pimpl->getId() };
			}
			else
			{
				throw std::runtime_error(R"ex(Appointment::getId is not implemented)ex");
			}
		}

		[[nodiscard]] service::AwaitableScalar<std::optional<response::Value>> getWhen(service::FieldParams&& params) const final
		{
			if constexpr (methods::AppointmentHas::getWhenWithParams<T>)
			{
				return { _pimpl->getWhen(std::move(params)) };
			}
			else if constexpr (methods::AppointmentHas::getWhen<T>)
			{
				return { _pimpl->getWhen() };
			}
			else
			{
				throw std::runtime_error(R"ex(Appointment::getWhen is not implemented)ex");
			}
		}

		[[nodiscard]] service::AwaitableScalar<std::optional<std::string>> getSubject(service::FieldParams&& params) const final
		{
			if constexpr (methods::AppointmentHas::getSubjectWithParams<T>)
			{
				return { _pimpl->getSubject(std::move(params)) };
			}
			else if constexpr (methods::AppointmentHas::getSubject<T>)
			{
				return { _pimpl->getSubject() };
			}
			else
			{
				throw std::runtime_error(R"ex(Appointment::getSubject is not implemented)ex");
			}
		}

		[[nodiscard]] service::AwaitableScalar<bool> getIsNow(service::FieldParams&& params) const final
		{
			if constexpr (methods::AppointmentHas::getIsNowWithParams<T>)
			{
				return { _pimpl->getIsNow(std::move(params)) };
			}
			else if constexpr (methods::AppointmentHas::getIsNow<T>)
			{
				return { _pimpl->getIsNow() };
			}
			else
			{
				throw std::runtime_error(R"ex(Appointment::getIsNow is not implemented)ex");
			}
		}

		[[nodiscard]] service::AwaitableScalar<std::optional<std::string>> getForceError(service::FieldParams&& params) const final
		{
			if constexpr (methods::AppointmentHas::getForceErrorWithParams<T>)
			{
				return { _pimpl->getForceError(std::move(params)) };
			}
			else if constexpr (methods::AppointmentHas::getForceError<T>)
			{
				return { _pimpl->getForceError() };
			}
			else
			{
				throw std::runtime_error(R"ex(Appointment::getForceError is not implemented)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const final
		{
			if constexpr (methods::AppointmentHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const final
		{
			if constexpr (methods::AppointmentHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	Appointment(std::unique_ptr<const Concept> pimpl) noexcept;

	// Interfaces which this type implements
	friend Node;

	// Unions which include this type
	friend UnionType;

	template <class I>
	[[nodiscard]] static constexpr bool implements() noexcept
	{
		return implements::AppointmentIs<I>;
	}

	[[nodiscard]] service::TypeNames getTypeNames() const noexcept;
	[[nodiscard]] service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const final;
	void endSelectionSet(const service::SelectionSetParams& params) const final;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	Appointment(std::shared_ptr<T> pimpl) noexcept
		: Appointment { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	[[nodiscard]] static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(Appointment)gql" };
	}
};

} // namespace graphql::today::object

#endif // APPOINTMENTOBJECT_H
