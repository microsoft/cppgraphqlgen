// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef CATOBJECT_H
#define CATOBJECT_H

#include "ValidationSchema.h"

namespace graphql::validation::object {
namespace implements {

template <class I>
concept CatIs = std::is_same_v<I, Pet> || std::is_same_v<I, CatOrDog>;

} // namespace implements

namespace methods::CatHas {

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
concept getNicknameWithParams = requires (TImpl impl, service::FieldParams params) 
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getNickname(std::move(params)) } };
};

template <class TImpl>
concept getNickname = requires (TImpl impl) 
{
	{ service::AwaitableScalar<std::optional<std::string>> { impl.getNickname() } };
};

template <class TImpl>
concept getDoesKnowCommandWithParams = requires (TImpl impl, service::FieldParams params, CatCommand catCommandArg) 
{
	{ service::AwaitableScalar<bool> { impl.getDoesKnowCommand(std::move(params), std::move(catCommandArg)) } };
};

template <class TImpl>
concept getDoesKnowCommand = requires (TImpl impl, CatCommand catCommandArg) 
{
	{ service::AwaitableScalar<bool> { impl.getDoesKnowCommand(std::move(catCommandArg)) } };
};

template <class TImpl>
concept getMeowVolumeWithParams = requires (TImpl impl, service::FieldParams params) 
{
	{ service::AwaitableScalar<std::optional<int>> { impl.getMeowVolume(std::move(params)) } };
};

template <class TImpl>
concept getMeowVolume = requires (TImpl impl) 
{
	{ service::AwaitableScalar<std::optional<int>> { impl.getMeowVolume() } };
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

} // namespace methods::CatHas

class Cat
	: public service::Object
{
private:
	service::AwaitableResolver resolveName(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveNickname(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveDoesKnowCommand(service::ResolverParams&& params) const;
	service::AwaitableResolver resolveMeowVolume(service::ResolverParams&& params) const;

	service::AwaitableResolver resolve_typename(service::ResolverParams&& params) const;

	struct Concept
	{
		virtual ~Concept() = default;

		virtual void beginSelectionSet(const service::SelectionSetParams& params) const = 0;
		virtual void endSelectionSet(const service::SelectionSetParams& params) const = 0;

		virtual service::AwaitableScalar<std::string> getName(service::FieldParams&& params) const = 0;
		virtual service::AwaitableScalar<std::optional<std::string>> getNickname(service::FieldParams&& params) const = 0;
		virtual service::AwaitableScalar<bool> getDoesKnowCommand(service::FieldParams&& params, CatCommand&& catCommandArg) const = 0;
		virtual service::AwaitableScalar<std::optional<int>> getMeowVolume(service::FieldParams&& params) const = 0;
	};

	template <class T>
	struct Model
		: Concept
	{
		Model(std::shared_ptr<T>&& pimpl) noexcept
			: _pimpl { std::move(pimpl) }
		{
		}

		service::AwaitableScalar<std::string> getName(service::FieldParams&& params) const final
		{
			if constexpr (methods::CatHas::getNameWithParams<T>)
			{
				return { _pimpl->getName(std::move(params)) };
			}
			else if constexpr (methods::CatHas::getName<T>)
			{
				return { _pimpl->getName() };
			}
			else
			{
				throw std::runtime_error(R"ex(Cat::getName is not implemented)ex");
			}
		}

		service::AwaitableScalar<std::optional<std::string>> getNickname(service::FieldParams&& params) const final
		{
			if constexpr (methods::CatHas::getNicknameWithParams<T>)
			{
				return { _pimpl->getNickname(std::move(params)) };
			}
			else if constexpr (methods::CatHas::getNickname<T>)
			{
				return { _pimpl->getNickname() };
			}
			else
			{
				throw std::runtime_error(R"ex(Cat::getNickname is not implemented)ex");
			}
		}

		service::AwaitableScalar<bool> getDoesKnowCommand(service::FieldParams&& params, CatCommand&& catCommandArg) const final
		{
			if constexpr (methods::CatHas::getDoesKnowCommandWithParams<T>)
			{
				return { _pimpl->getDoesKnowCommand(std::move(params), std::move(catCommandArg)) };
			}
			else if constexpr (methods::CatHas::getDoesKnowCommand<T>)
			{
				return { _pimpl->getDoesKnowCommand(std::move(catCommandArg)) };
			}
			else
			{
				throw std::runtime_error(R"ex(Cat::getDoesKnowCommand is not implemented)ex");
			}
		}

		service::AwaitableScalar<std::optional<int>> getMeowVolume(service::FieldParams&& params) const final
		{
			if constexpr (methods::CatHas::getMeowVolumeWithParams<T>)
			{
				return { _pimpl->getMeowVolume(std::move(params)) };
			}
			else if constexpr (methods::CatHas::getMeowVolume<T>)
			{
				return { _pimpl->getMeowVolume() };
			}
			else
			{
				throw std::runtime_error(R"ex(Cat::getMeowVolume is not implemented)ex");
			}
		}

		void beginSelectionSet(const service::SelectionSetParams& params) const final
		{
			if constexpr (methods::CatHas::beginSelectionSet<T>)
			{
				_pimpl->beginSelectionSet(params);
			}
		}

		void endSelectionSet(const service::SelectionSetParams& params) const final
		{
			if constexpr (methods::CatHas::endSelectionSet<T>)
			{
				_pimpl->endSelectionSet(params);
			}
		}

	private:
		const std::shared_ptr<T> _pimpl;
	};

	Cat(std::unique_ptr<Concept>&& pimpl) noexcept;

	// Interfaces which this type implements
	friend Pet;

	// Unions which include this type
	friend CatOrDog;

	template <class I>
	static constexpr bool implements() noexcept
	{
		return implements::CatIs<I>;
	}

	service::TypeNames getTypeNames() const noexcept;
	service::ResolverMap getResolvers() const noexcept;

	void beginSelectionSet(const service::SelectionSetParams& params) const final;
	void endSelectionSet(const service::SelectionSetParams& params) const final;

	const std::unique_ptr<Concept> _pimpl;

public:
	template <class T>
	Cat(std::shared_ptr<T> pimpl) noexcept
		: Cat { std::unique_ptr<Concept> { std::make_unique<Model<T>>(std::move(pimpl)) } }
	{
	}
};

} // namespace graphql::validation::object

#endif // CATOBJECT_H
