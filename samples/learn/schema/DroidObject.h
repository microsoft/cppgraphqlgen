// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef DROIDOBJECT_H
#define DROIDOBJECT_H

#include "StarWarsSchema.h"

namespace graphql::learn::object {
namespace implements {

template <class I>
concept DroidIs = std::is_same_v<I, Character>;

} // namespace implements

namespace methods::DroidHas {

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
concept getNameWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getName(std::move(params)) } };
};

template <class TImpl>
concept getName = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getName() } };
};

template <class TImpl>
concept getFriendsWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableObject<std::optional<std::vector<std::shared_ptr<Character>>>> { impl.getFriends(std::move(params)) } };
};

template <class TImpl>
concept getFriends = requires (TImpl impl)
{
	{ service::AwaitableObject<std::optional<std::vector<std::shared_ptr<Character>>>> { impl.getFriends() } };
};

template <class TImpl>
concept getAppearsInWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::vector<std::optional<Episode>>>> { impl.getAppearsIn(std::move(params)) } };
};

template <class TImpl>
concept getAppearsIn = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::vector<std::optional<Episode>>>> { impl.getAppearsIn() } };
};

template <class TImpl>
concept getPrimaryFunctionWithParams = requires (TImpl impl, service::FieldParams params)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getPrimaryFunction(std::move(params)) } };
};

template <class TImpl>
concept getPrimaryFunction = requires (TImpl impl)
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getPrimaryFunction() } };
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

} // namespace methods::DroidHas

class Droid final
	: public service::Object
{
private:
	service::AwaitableResolver resolveId(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveName(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveFriends(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveAppearsIn(service::ResolverParams&& params) const;
	service::AwaitableResolver resolvePrimaryFunction(service::ResolverParams&& params) const;

	service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		virtual service::AwaitableScalar<response::IdType> getId(service::FieldParams&& params) const = 0;
		virtual service::AwaitableScalar<std::optional<std::string>> getName(service::FieldParams&& params) const = 0;
		virtual service::AwaitableObject<std::optional<std::vector<std::shared_ptr<Character>>>> getFriends(service::FieldParams&& params) const = 0;
		virtual service::AwaitableScalar<std::optional<std::vector<std::optional<Episode>>>> getAppearsIn(service::FieldParams&& params) const = 0;
		virtual service::AwaitableScalar<std::optional<std::string>> getPrimaryFunction(service::FieldParams&& params) const = 0;
	};

	template <class T>
	struct Model
		: Concept
	{
		Model(std::shared_ptr<T>&& pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		service::AwaitableScalar<response::IdType> getId(service::FieldParams&& params) const final
		{
			if constexpr (methods::DroidHas::getIdWithParams<T>)
			{
				return { _pimpl->getId(std::move(params)) };
			}
			else
			{
				static_assert(methods::DroidHas::getId<T>, R"msg(Droid::getId is not implemented)msg");
				return { _pimpl->getId() };
			}
		}

		service::AwaitableScalar<std::optional<std::string>> getName(service::FieldParams&& params) const final
		{
			if constexpr (methods::DroidHas::getNameWithParams<T>)
			{
				return { _pimpl->getName(std::move(params)) };
			}
			else
			{
				static_assert(methods::DroidHas::getName<T>, R"msg(Droid::getName is not implemented)msg");
				return { _pimpl->getName() };
			}
		}

		service::AwaitableObject<std::optional<std::vector<std::shared_ptr<Character>>>> getFriends(service::FieldParams&& params) const final
		{
			if constexpr (methods::DroidHas::getFriendsWithParams<T>)
			{
				return { _pimpl->getFriends(std::move(params)) };
			}
			else
			{
				static_assert(methods::DroidHas::getFriends<T>, R"msg(Droid::getFriends is not implemented)msg");
				return { _pimpl->getFriends() };
			}
		}

		service::AwaitableScalar<std::optional<std::vector<std::optional<Episode>>>> getAppearsIn(service::FieldParams&& params) const final
		{
			if constexpr (methods::DroidHas::getAppearsInWithParams<T>)
			{
				return { _pimpl->getAppearsIn(std::move(params)) };
			}
			else
			{
				static_assert(methods::DroidHas::getAppearsIn<T>, R"msg(Droid::getAppearsIn is not implemented)msg");
				return { _pimpl->getAppearsIn() };
			}
		}

		service::AwaitableScalar<std::optional<std::string>> getPrimaryFunction(service::FieldParams&& params) const final
		{
			if constexpr (methods::DroidHas::getPrimaryFunctionWithParams<T>)
			{
				return { _pimpl->getPrimaryFunction(std::move(params)) };
			}
			else
			{
				static_assert(methods::DroidHas::getPrimaryFunction<T>, R"msg(Droid::getPrimaryFunction is not implemented)msg");
				return { _pimpl->getPrimaryFunction() };
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const final
		{
			if constexpr (methods::DroidHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const final
		{
			if constexpr (methods::DroidHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	Droid(std::unique_ptr<const Concept>&& pimpl) noexcept;

	// Interfaces which this type implements
	friend Character;

	template <class I>
	static constexpr bool implements() noexcept
	{
		return implements::DroidIs<I>;
	}

	service::TypeNames getTypeNames() const noexcept;
	service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const final;
	void endSelectionSet(const service::SelectionSetParams& params) const final;

	const std::unique_ptr<const Concept> _pimpl;

public:
	template <class T>
	Droid(std::shared_ptr<T> pimpl) noexcept
		: Droid { std::unique_ptr<const Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}

	static constexpr std::string_view getObjectType() noexcept
	{
		return { R"gql(Droid)gql" };
	}
};

} // namespace graphql::learn::object

#endif // DROIDOBJECT_H
