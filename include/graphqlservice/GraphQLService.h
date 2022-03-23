// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLSERVICE_H
#define GRAPHQLSERVICE_H

// clang-format off
#ifdef GRAPHQL_DLLEXPORTS
	#ifdef IMPL_GRAPHQLSERVICE_DLL
		#define GRAPHQLSERVICE_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLSERVICE_DLL
		#define GRAPHQLSERVICE_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLSERVICE_DLL
#else // !GRAPHQL_DLLEXPORTS
	#define GRAPHQLSERVICE_EXPORT
#endif // !GRAPHQL_DLLEXPORTS
// clang-format on

#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLResponse.h"

#include "graphqlservice/internal/Awaitable.h"
#include "graphqlservice/internal/SortedMap.h"
#include "graphqlservice/internal/Version.h"

#include <chrono>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace graphql {
namespace schema {

class Schema;

} // namespace schema

namespace service {

// Errors should have a message string, and optional locations and a path.
struct schema_location
{
	size_t line = 0;
	size_t column = 1;
};

// The implementation details of the error path should be opaque to client code. It is carried along
// with the SelectionSetParams and automatically added to any schema errors or exceptions thrown
// from an accessor as part of error reporting.
using path_segment = std::variant<std::string_view, size_t>;

struct field_path
{
	std::optional<std::reference_wrapper<const field_path>> parent;
	std::variant<std::string_view, size_t> segment;
};

using error_path = std::vector<path_segment>;

GRAPHQLSERVICE_EXPORT error_path buildErrorPath(const std::optional<field_path>& path);

struct schema_error
{
	std::string message;
	schema_location location {};
	error_path path {};
};

GRAPHQLSERVICE_EXPORT response::Value buildErrorValues(std::list<schema_error>&& structuredErrors);

// This exception bubbles up 1 or more error messages to the JSON results.
class schema_exception : public std::exception
{
public:
	GRAPHQLSERVICE_EXPORT explicit schema_exception(std::list<schema_error>&& structuredErrors);
	GRAPHQLSERVICE_EXPORT explicit schema_exception(std::vector<std::string>&& messages);

	schema_exception() = delete;

	GRAPHQLSERVICE_EXPORT const char* what() const noexcept override;

	GRAPHQLSERVICE_EXPORT std::list<schema_error> getStructuredErrors() noexcept;
	GRAPHQLSERVICE_EXPORT response::Value getErrors();

private:
	static std::list<schema_error> convertMessages(std::vector<std::string>&& messages) noexcept;

	std::list<schema_error> _structuredErrors;
};

// The RequestState is nullable, but if you have multiple threads processing requests and there's
// any per-request state that you want to maintain throughout the request (e.g. optimizing or
// batching backend requests), you can inherit from RequestState and pass it to Request::resolve to
// correlate the asynchronous/recursive callbacks and accumulate state in it.
struct RequestState : std::enable_shared_from_this<RequestState>
{
	virtual ~RequestState() = default;
};

namespace {

using namespace std::literals;

constexpr std::string_view strData { "data"sv };
constexpr std::string_view strErrors { "errors"sv };
constexpr std::string_view strMessage { "message"sv };
constexpr std::string_view strLocations { "locations"sv };
constexpr std::string_view strLine { "line"sv };
constexpr std::string_view strColumn { "column"sv };
constexpr std::string_view strPath { "path"sv };
constexpr std::string_view strQuery { "query"sv };
constexpr std::string_view strMutation { "mutation"sv };
constexpr std::string_view strSubscription { "subscription"sv };

} // namespace

// Resolvers may be called in multiple different Operation contexts.
enum class ResolverContext
{
	// Resolving a Query operation.
	Query,

	// Resolving a Mutation operation.
	Mutation,

	// Adding a Subscription. If you need to prepare to send events for this Subsciption
	// (e.g. registering an event sink of your own), this is a chance to do that.
	NotifySubscribe,

	// Resolving a Subscription event.
	Subscription,

	// Removing a Subscription. If there are no more Subscriptions registered this is an
	// opportunity to release resources which are no longer needed.
	NotifyUnsubscribe,
};

// Resume coroutine execution on a new worker thread any time co_await is called. This emulates the
// behavior of std::async when passing std::launch::async.
struct await_worker_thread : coro::suspend_always
{
	GRAPHQLSERVICE_EXPORT void await_suspend(coro::coroutine_handle<> h) const;
};

// Queue coroutine execution on a single dedicated worker thread any time co_await is called from
// the thread which created it.
struct await_worker_queue : coro::suspend_always
{
	GRAPHQLSERVICE_EXPORT await_worker_queue();
	GRAPHQLSERVICE_EXPORT ~await_worker_queue();

	GRAPHQLSERVICE_EXPORT bool await_ready() const;
	GRAPHQLSERVICE_EXPORT void await_suspend(coro::coroutine_handle<> h);

private:
	void resumePending();

	const std::thread::id _startId;
	std::mutex _mutex {};
	std::condition_variable _cv {};
	std::list<coro::coroutine_handle<>> _pending {};
	bool _shutdown = false;
	std::thread _worker;
};

// Type-erased awaitable.
class await_async final
{
private:
	struct Concept
	{
		virtual ~Concept() = default;

		virtual bool await_ready() const = 0;
		virtual void await_suspend(coro::coroutine_handle<> h) const = 0;
		virtual void await_resume() const = 0;
	};

	template <class T>
	struct Model : Concept
	{
		Model(std::shared_ptr<T>&& pimpl)
			: _pimpl { std::move(pimpl) }
		{
		}

		bool await_ready() const final
		{
			return _pimpl->await_ready();
		}

		void await_suspend(coro::coroutine_handle<> h) const final
		{
			_pimpl->await_suspend(std::move(h));
		}

		void await_resume() const final
		{
			_pimpl->await_resume();
		}

	private:
		std::shared_ptr<T> _pimpl;
	};

	const std::shared_ptr<const Concept> _pimpl;

public:
	// Type-erased explicit constructor for a custom awaitable.
	template <class T>
	explicit await_async(std::shared_ptr<T> pimpl)
		: _pimpl { std::make_shared<Model<T>>(std::move(pimpl)) }
	{
	}

	// Default to immediate synchronous execution.
	GRAPHQLSERVICE_EXPORT await_async();

	// Implicitly convert a std::launch parameter used with std::async to an awaitable.
	GRAPHQLSERVICE_EXPORT await_async(std::launch launch);

	GRAPHQLSERVICE_EXPORT bool await_ready() const;
	GRAPHQLSERVICE_EXPORT void await_suspend(coro::coroutine_handle<> h) const;
	GRAPHQLSERVICE_EXPORT void await_resume() const;
};

// Directive order matters, and some of them are repeatable. So rather than passing them in a
// response::Value, pass directives in something like the underlying response::MapType which
// preserves the order of the elements without complete uniqueness.
using Directives = std::vector<std::pair<std::string_view, response::Value>>;

// Traversing a fragment spread adds a new set of directives.
using FragmentDefinitionDirectiveStack = std::list<std::reference_wrapper<const Directives>>;
using FragmentSpreadDirectiveStack = std::list<Directives>;

// Pass a common bundle of parameters to all of the generated Object::getField accessors in a
// SelectionSet
struct SelectionSetParams
{
	// Context for this selection set.
	const ResolverContext resolverContext;

	// The lifetime of each of these borrowed references is guaranteed until the future returned
	// by the accessor is resolved or destroyed. They are owned by the OperationData shared pointer.
	const std::shared_ptr<RequestState>& state;
	const Directives& operationDirectives;
	const std::shared_ptr<FragmentDefinitionDirectiveStack> fragmentDefinitionDirectives;

	// Fragment directives are shared for all fields in that fragment, but they aren't kept alive
	// after the call to the last accessor in the fragment. If you need to keep them alive longer,
	// you'll need to explicitly copy them into other instances of Directives.
	const std::shared_ptr<FragmentSpreadDirectiveStack> fragmentSpreadDirectives;
	const std::shared_ptr<FragmentSpreadDirectiveStack> inlineFragmentDirectives;

	// Field error path to this selection set.
	std::optional<field_path> errorPath;

	// Async launch policy for sub-field resolvers.
	const await_async launch {};
};

// Pass a common bundle of parameters to all of the generated Object::getField accessors.
struct FieldParams : SelectionSetParams
{
	GRAPHQLSERVICE_EXPORT explicit FieldParams(
		SelectionSetParams&& selectionSetParams, Directives directives);

	// Each field owns its own field-specific directives. Once the accessor returns it will be
	// destroyed, but you can move it into another instance of response::Value to keep it alive
	// longer.
	Directives fieldDirectives;
};

// Field accessors may return either a result of T, an awaitable of T, or a std::future<T>, so at
// runtime the implementer may choose to return by value or defer/parallelize expensive operations
// by returning an async future or an awaitable coroutine.
//
// If the overhead of conversion to response::Value is too expensive, scalar type field accessors
// can store and return a std::shared_ptr<const response::Value> directly.
template <typename T>
class AwaitableScalar
{
public:
	template <typename U>
	AwaitableScalar(U&& value)
		: _value { std::forward<U>(value) }
	{
	}

	struct promise_type
	{
		AwaitableScalar<T> get_return_object() noexcept
		{
			return { _promise.get_future() };
		}

		coro::suspend_never initial_suspend() const noexcept
		{
			return {};
		}

		coro::suspend_never final_suspend() const noexcept
		{
			return {};
		}

		void return_value(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
		{
			_promise.set_value(value);
		}

		void return_value(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
		{
			_promise.set_value(std::move(value));
		}

		void unhandled_exception() noexcept
		{
			_promise.set_exception(std::current_exception());
		}

	private:
		std::promise<T> _promise;
	};

	bool await_ready() const noexcept
	{
		return std::visit(
			[](const auto& value) noexcept {
				using value_type = std::decay_t<decltype(value)>;

				if constexpr (std::is_same_v<value_type, T>)
				{
					return true;
				}
				else if constexpr (std::is_same_v<value_type, std::future<T>>)
				{
					using namespace std::literals;

					return value.wait_for(0s) != std::future_status::timeout;
				}
				else if constexpr (std::is_same_v<value_type,
									   std::shared_ptr<const response::Value>>)
				{
					return true;
				}
			},
			_value);
	}

	void await_suspend(coro::coroutine_handle<> h) const
	{
		std::thread(
			[this](coro::coroutine_handle<> h) noexcept {
				std::get<std::future<T>>(_value).wait();
				h.resume();
			},
			std::move(h))
			.detach();
	}

	T await_resume()
	{
		return std::visit(
			[](auto&& value) -> T {
				using value_type = std::decay_t<decltype(value)>;

				if constexpr (std::is_same_v<value_type, T>)
				{
					return T { std::move(value) };
				}
				else if constexpr (std::is_same_v<value_type, std::future<T>>)
				{
					return value.get();
				}
				else if constexpr (std::is_same_v<value_type,
									   std::shared_ptr<const response::Value>>)
				{
					throw std::logic_error("Cannot await std::shared_ptr<const response::Value>");
				}
			},
			std::move(_value));
	}

	std::shared_ptr<const response::Value> get_value() noexcept
	{
		return std::visit(
			[](auto&& value) noexcept {
				using value_type = std::decay_t<decltype(value)>;
				std::shared_ptr<const response::Value> result;

				if constexpr (std::is_same_v<value_type, std::shared_ptr<const response::Value>>)
				{
					result = std::move(value);
				}

				return result;
			},
			std::move(_value));
	}

private:
	std::variant<T, std::future<T>, std::shared_ptr<const response::Value>> _value;
};

// Field accessors may return either a result of T, an awaitable of T, or a std::future<T>, so at
// runtime the implementer may choose to return by value or defer/parallelize expensive operations
// by returning an async future or an awaitable coroutine.
template <typename T>
class AwaitableObject
{
public:
	template <typename U>
	AwaitableObject(U&& value)
		: _value { std::forward<U>(value) }
	{
	}

	struct promise_type
	{
		AwaitableObject<T> get_return_object() noexcept
		{
			return { _promise.get_future() };
		}

		coro::suspend_never initial_suspend() const noexcept
		{
			return {};
		}

		coro::suspend_never final_suspend() const noexcept
		{
			return {};
		}

		void return_value(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
		{
			_promise.set_value(value);
		}

		void return_value(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
		{
			_promise.set_value(std::move(value));
		}

		void unhandled_exception() noexcept
		{
			_promise.set_exception(std::current_exception());
		}

	private:
		std::promise<T> _promise;
	};

	bool await_ready() const noexcept
	{
		return std::visit(
			[](const auto& value) noexcept {
				using value_type = std::decay_t<decltype(value)>;

				if constexpr (std::is_same_v<value_type, T>)
				{
					return true;
				}
				else if constexpr (std::is_same_v<value_type, std::future<T>>)
				{
					using namespace std::literals;

					return value.wait_for(0s) != std::future_status::timeout;
				}
			},
			_value);
	}

	void await_suspend(coro::coroutine_handle<> h) const
	{
		std::thread(
			[this](coro::coroutine_handle<> h) noexcept {
				std::get<std::future<T>>(_value).wait();
				h.resume();
			},
			std::move(h))
			.detach();
	}

	T await_resume()
	{
		return std::visit(
			[](auto&& value) -> T {
				using value_type = std::decay_t<decltype(value)>;

				if constexpr (std::is_same_v<value_type, T>)
				{
					return T { std::move(value) };
				}
				else if constexpr (std::is_same_v<value_type, std::future<T>>)
				{
					return value.get();
				}
			},
			std::move(_value));
	}

private:
	std::variant<T, std::future<T>> _value;
};

// Fragments are referenced by name and have a single type condition (except for inline
// fragments, where the type condition is common but optional). They contain a set of fields
// (with optional aliases and sub-selections) and potentially references to other fragments.
class Fragment
{
public:
	explicit Fragment(const peg::ast_node& fragmentDefinition, const response::Value& variables);

	std::string_view getType() const;
	const peg::ast_node& getSelection() const;
	const Directives& getDirectives() const;

private:
	std::string_view _type;
	Directives _directives;

	std::reference_wrapper<const peg::ast_node> _selection;
};

// Resolvers for complex types need to be able to find fragment definitions anywhere in
// the request document by name.
using FragmentMap = internal::string_view_map<Fragment>;

// Resolver functors take a set of arguments encoded as members on a JSON object
// with an optional selection set for complex types and return a JSON value for
// a single field.
struct ResolverParams : SelectionSetParams
{
	GRAPHQLSERVICE_EXPORT explicit ResolverParams(const SelectionSetParams& selectionSetParams,
		const peg::ast_node& field, std::string&& fieldName, response::Value arguments,
		Directives fieldDirectives, const peg::ast_node* selection, const FragmentMap& fragments,
		const response::Value& variables);

	GRAPHQLSERVICE_EXPORT schema_location getLocation() const;

	// These values are different for each resolver.
	const peg::ast_node& field;
	std::string fieldName;
	response::Value arguments { response::Type::Map };
	Directives fieldDirectives;
	const peg::ast_node* selection;

	// These values remain unchanged for the entire operation, but they're passed to each of the
	// resolvers recursively through ResolverParams.
	const FragmentMap& fragments;
	const response::Value& variables;
};

// Propagate data and errors together without bundling them into a response::Value struct until
// we're ready to return from the top level Operation.
struct ResolverResult
{
	response::Value data;
	std::list<schema_error> errors {};
};

using AwaitableResolver = internal::Awaitable<ResolverResult>;
using Resolver = std::function<AwaitableResolver(ResolverParams&&)>;
using ResolverMap = internal::string_view_map<Resolver>;

// GraphQL types are nullable by default, but they may be wrapped with non-null or list types.
// Since nullability is a more special case in C++, we invert the default and apply that modifier
// instead when the non-null wrapper is not present in that part of the wrapper chain.
enum class TypeModifier
{
	None,
	Nullable,
	List,
};

// Extract individual arguments with chained type modifiers which add nullable or list wrappers.
// If the argument is not optional, use require and let it throw a schema_exception when the
// argument is missing or not the correct type. If it's optional, use find and check the second
// element in the pair to see if it was found or if you just got the default value for that type.
template <typename Type>
struct ModifiedArgument
{
	// Peel off modifiers until we get to the underlying type.
	template <typename U, TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	struct ArgumentTraits
	{
		// Peel off modifiers until we get to the underlying type.
		using type = typename std::conditional_t<TypeModifier::Nullable == Modifier,
			std::optional<typename ArgumentTraits<U, Other...>::type>,
			typename std::conditional_t<TypeModifier::List == Modifier,
				std::vector<typename ArgumentTraits<U, Other...>::type>, U>>;
	};

	template <typename U>
	struct ArgumentTraits<U, TypeModifier::None>
	{
		using type = U;
	};

	// Convert a single value to the specified type.
	static Type convert(const response::Value& value);

	// Call convert on this type without any modifiers.
	static Type require(std::string_view name, const response::Value& arguments)
	{
		try
		{
			return convert(arguments[name]);
		}
		catch (schema_exception& ex)
		{
			auto errors = ex.getStructuredErrors();

			for (auto& error : errors)
			{
				std::ostringstream message;

				message << "Invalid argument: " << name << " error: " << error.message;

				error.message = message.str();
			}

			throw schema_exception(std::move(errors));
		}
	}

	// Wrap require in a try/catch block.
	static std::pair<Type, bool> find(
		const std::string& name, const response::Value& arguments) noexcept
	{
		try
		{
			return { require(name, arguments), true };
		}
		catch (const std::exception&)
		{
			return { Type {}, false };
		}
	}

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::None == Modifier, Type> require(
		std::string_view name, const response::Value& arguments)
	{
		static_assert(sizeof...(Other) == 0, "None modifier should always be last");

		// Just call through to the non-template method without the modifiers.
		return require(name, arguments);
	}

	// Peel off nullable modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::Nullable == Modifier,
		typename ArgumentTraits<Type, Modifier, Other...>::type>
	require(std::string_view name, const response::Value& arguments)
	{
		const auto& valueItr = arguments.find(name);

		if (valueItr == arguments.get<response::MapType>().cend()
			|| valueItr->second.type() == response::Type::Null)
		{
			return std::nullopt;
		}

		auto result = require<Other...>(name, arguments);

		return std::make_optional<decltype(result)>(std::move(result));
	}

	// Peel off list modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::List == Modifier,
		typename ArgumentTraits<Type, Modifier, Other...>::type>
	require(std::string_view name, const response::Value& arguments)
	{
		const auto& values = arguments[name];
		typename ArgumentTraits<Type, Modifier, Other...>::type result(values.size());
		const auto& elements = values.get<response::ListType>();

		std::transform(elements.cbegin(),
			elements.cend(),
			result.begin(),
			[name](const response::Value& element) {
				response::Value single(response::Type::Map);

				single.emplace_back(std::string { name }, response::Value(element));

				return require<Other...>(name, single);
			});

		return result;
	}

	// Wrap require with modifiers in a try/catch block.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	static std::pair<typename ArgumentTraits<Type, Modifier, Other...>::type, bool> find(
		std::string_view name, const response::Value& arguments) noexcept
	{
		try
		{
			return { require<Modifier, Other...>(name, arguments), true };
		}
		catch (const std::exception&)
		{
			return { typename ArgumentTraits<Type, Modifier, Other...>::type {}, false };
		}
	}
};

// Convenient type aliases for testing, generated code won't actually use these. These are also
// the specializations which are implemented in the GraphQLService library, other specializations
// for input types should be generated in schemagen.
using IntArgument = ModifiedArgument<int>;
using FloatArgument = ModifiedArgument<double>;
using StringArgument = ModifiedArgument<std::string>;
using BooleanArgument = ModifiedArgument<bool>;
using IdArgument = ModifiedArgument<response::IdType>;
using ScalarArgument = ModifiedArgument<response::Value>;

#ifdef GRAPHQL_DLLEXPORTS
// Export all of the built-in converters
template <>
GRAPHQLSERVICE_EXPORT int ModifiedArgument<int>::convert(const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT double ModifiedArgument<double>::convert(const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT std::string ModifiedArgument<std::string>::convert(
	const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT bool ModifiedArgument<bool>::convert(const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT response::IdType ModifiedArgument<response::IdType>::convert(
	const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT response::Value ModifiedArgument<response::Value>::convert(
	const response::Value& value);
#endif // GRAPHQL_DLLEXPORTS

// Each type should handle fragments with type conditions matching its own
// name and any inheritted interfaces.
using TypeNames = internal::string_view_set;

// Object parses argument values, performs variable lookups, expands fragments, evaluates @include
// and @skip directives, and calls through to the resolver functor for each selected field with
// its arguments. This may be a recursive process for fields which return another complex type,
// in which case it requires its own selection set.
class Object : public std::enable_shared_from_this<Object>
{
public:
	GRAPHQLSERVICE_EXPORT explicit Object(TypeNames&& typeNames, ResolverMap&& resolvers) noexcept;
	GRAPHQLSERVICE_EXPORT virtual ~Object() = default;

	GRAPHQLSERVICE_EXPORT AwaitableResolver resolve(const SelectionSetParams& selectionSetParams,
		const peg::ast_node& selection, const FragmentMap& fragments,
		const response::Value& variables) const;

	GRAPHQLSERVICE_EXPORT bool matchesType(std::string_view typeName) const;

protected:
	// These callbacks are optional, you may override either, both, or neither of them. The
	// implementer can use these to to accumulate state for the entire SelectionSet on this object,
	// as well as testing directives which were included at the operation or fragment level. It's up
	// to sub-classes to decide if they want to use const_cast, mutable members, or separate storage
	// in the RequestState to accumulate state. By default these callbacks should treat the Object
	// itself as const.
	GRAPHQLSERVICE_EXPORT virtual void beginSelectionSet(const SelectionSetParams& params) const;
	GRAPHQLSERVICE_EXPORT virtual void endSelectionSet(const SelectionSetParams& params) const;

	mutable std::mutex _resolverMutex {};

private:
	TypeNames _typeNames;
	ResolverMap _resolvers;
};

// Convert the result of a resolver function with chained type modifiers that add nullable or
// list wrappers. This is the inverse of ModifiedArgument for output types instead of input types.
template <typename Type>
struct ModifiedResult
{
	// Peel off modifiers until we get to the underlying type.
	template <typename U, TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	struct ResultTraits
	{
		using type = typename std::conditional_t<TypeModifier::Nullable == Modifier,
			typename std::conditional_t<
				std::is_base_of_v<Object,
					U> && std::is_same_v<std::shared_ptr<U>, typename ResultTraits<U, Other...>::type>,
				std::shared_ptr<U>, std::optional<typename ResultTraits<U, Other...>::type>>,
			typename std::conditional_t<TypeModifier::List == Modifier,
				std::vector<typename ResultTraits<U, Other...>::type>,
				typename std::conditional_t<std::is_base_of_v<Object, U>, std::shared_ptr<U>, U>>>;

		using future_type = typename std::conditional_t<std::is_base_of_v<Object, U>,
			AwaitableObject<type>, AwaitableScalar<type>>;
	};

	template <typename U>
	struct ResultTraits<U, TypeModifier::None>
	{
		using type =
			typename std::conditional_t<std::is_base_of_v<Object, U>, std::shared_ptr<U>, U>;

		using future_type = typename std::conditional_t<std::is_base_of_v<Object, U>,
			AwaitableObject<std::shared_ptr<const Object>>, AwaitableScalar<type>>;
	};

	// Convert a single value of the specified type to JSON.
	static AwaitableResolver convert(
		typename ResultTraits<Type>::future_type result, ResolverParams params);

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::None == Modifier
			&& !std::is_same_v<Object, Type> && std::is_base_of_v<Object, Type>,
		AwaitableResolver>
	convert(AwaitableObject<typename ResultTraits<Type>::type> result, ResolverParams params)
	{
		// Call through to the Object specialization with a static_pointer_cast for subclasses of
		// Object.
		static_assert(sizeof...(Other) == 0, "None modifier should always be last");
		static_assert(std::is_same_v<std::shared_ptr<Type>, typename ResultTraits<Type>::type>,
			"this is the derived object type");

		co_await params.launch;

		auto awaitedResult = co_await ModifiedResult<Object>::convert(
			std::static_pointer_cast<const Object>(co_await result),
			std::move(params));

		co_return std::move(awaitedResult);
	}

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::None == Modifier
			&& (std::is_same_v<Object, Type> || !std::is_base_of_v<Object, Type>),
		AwaitableResolver>
	convert(typename ResultTraits<Type>::future_type result, ResolverParams params)
	{
		static_assert(sizeof...(Other) == 0, "None modifier should always be last");

		// Just call through to the partial specialization without the modifier.
		return convert(std::move(result), std::move(params));
	}

	// Peel off final nullable modifiers for std::shared_ptr of Object and subclasses of Object.
	template <TypeModifier Modifier, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::Nullable == Modifier
			&& std::is_same_v<std::shared_ptr<Type>, typename ResultTraits<Type, Other...>::type>,
		AwaitableResolver>
	convert(
		typename ResultTraits<Type, Modifier, Other...>::future_type result, ResolverParams params)
	{
		co_await params.launch;

		auto awaitedResult = co_await std::move(result);

		if (!awaitedResult)
		{
			co_return ResolverResult {};
		}

		auto modifiedResult =
			co_await ModifiedResult::convert<Other...>(std::move(awaitedResult), std::move(params));

		co_return modifiedResult;
	}

	// Peel off nullable modifiers for anything else, which should all be std::optional.
	template <TypeModifier Modifier, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::Nullable == Modifier
			&& !std::is_same_v<std::shared_ptr<Type>, typename ResultTraits<Type, Other...>::type>,
		AwaitableResolver>
	convert(
		typename ResultTraits<Type, Modifier, Other...>::future_type result, ResolverParams params)
	{
		static_assert(std::is_same_v<std::optional<typename ResultTraits<Type, Other...>::type>,
						  typename ResultTraits<Type, Modifier, Other...>::type>,
			"this is the optional version");

		if constexpr (!std::is_base_of_v<Object, Type>)
		{
			auto value = result.get_value();

			if (value)
			{
				ModifiedResult::validateScalar<Modifier, Other...>(*value);
				co_return ResolverResult { response::Value {
					std::shared_ptr { std::move(value) } } };
			}
		}

		co_await params.launch;

		auto awaitedResult = co_await std::move(result);

		if (!awaitedResult)
		{
			co_return ResolverResult {};
		}

		auto modifiedResult = co_await ModifiedResult::convert<Other...>(std::move(*awaitedResult),
			std::move(params));

		co_return modifiedResult;
	}

	// Peel off list modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::List == Modifier, AwaitableResolver> convert(
		typename ResultTraits<Type, Modifier, Other...>::future_type result, ResolverParams params)
	{
		if constexpr (!std::is_base_of_v<Object, Type>)
		{
			auto value = result.get_value();

			if (value)
			{
				ModifiedResult::validateScalar<Modifier, Other...>(*value);
				co_return ResolverResult { response::Value {
					std::shared_ptr { std::move(value) } } };
			}
		}

		std::vector<AwaitableResolver> children;
		const auto parentPath = params.errorPath;

		co_await params.launch;

		auto awaitedResult = co_await std::move(result);

		children.reserve(awaitedResult.size());
		params.errorPath = std::make_optional(
			field_path { parentPath ? std::make_optional(std::cref(*parentPath)) : std::nullopt,
				path_segment { size_t { 0 } } });

		using vector_type = std::decay_t<decltype(awaitedResult)>;

		if constexpr (!std::is_same_v<std::decay_t<typename vector_type::reference>,
						  typename vector_type::value_type>)
		{
			// Special handling for std::vector<> specializations which don't return a
			// reference to the underlying type, i.e. std::vector<bool> on many platforms.
			// Copy the values from the std::vector<> rather than moving them.
			for (typename vector_type::value_type entry : awaitedResult)
			{
				children.push_back(
					ModifiedResult::convert<Other...>(std::move(entry), ResolverParams(params)));
				++std::get<size_t>(params.errorPath->segment);
			}
		}
		else
		{
			for (auto& entry : awaitedResult)
			{
				children.push_back(
					ModifiedResult::convert<Other...>(std::move(entry), ResolverParams(params)));
				++std::get<size_t>(params.errorPath->segment);
			}
		}

		ResolverResult document { response::Value { response::Type::List } };

		document.data.reserve(children.size());
		std::get<size_t>(params.errorPath->segment) = 0;

		for (auto& child : children)
		{
			try
			{
				co_await params.launch;

				auto value = co_await std::move(child);

				document.data.emplace_back(std::move(value.data));

				if (!value.errors.empty())
				{
					document.errors.splice(document.errors.end(), value.errors);
				}
			}
			catch (schema_exception& scx)
			{
				auto errors = scx.getStructuredErrors();

				if (!errors.empty())
				{
					document.errors.splice(document.errors.end(), errors);
				}
			}
			catch (const std::exception& ex)
			{
				std::ostringstream message;

				message << "Field error name: " << params.fieldName
						<< " unknown error: " << ex.what();

				document.errors.emplace_back(schema_error { message.str(),
					params.getLocation(),
					buildErrorPath(params.errorPath) });
			}

			++std::get<size_t>(params.errorPath->segment);
		}

		co_return document;
	}

private:
	// Validate a single scalar value is the expected type.
	static void validateScalar(const response::Value& value);

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	static void validateScalar(
		typename std::enable_if_t<TypeModifier::None == Modifier, const response::Value&> value)
	{
		static_assert(sizeof...(Other) == 0, "None modifier should always be last");

		// Just call through to the partial specialization without the modifier.
		validateScalar(value);
	}

	// Peel off nullable modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	static void validateScalar(
		typename std::enable_if_t<TypeModifier::Nullable == Modifier, const response::Value&> value)
	{
		if (value.type() != response::Type::Null)
		{
			ModifiedResult::validateScalar<Other...>(value);
		}
	}

	// Peel off list modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	static void validateScalar(
		typename std::enable_if_t<TypeModifier::List == Modifier, const response::Value&> value)
	{
		if (value.type() != response::Type::List)
		{
			throw schema_exception { { R"ex(not a valid List value)ex" } };
		}

		for (size_t i = 0; i < value.size(); ++i)
		{
			ModifiedResult::validateScalar<Other...>(value[i]);
		}
	}

	using ResolverCallback =
		std::function<response::Value(typename ResultTraits<Type>::type, const ResolverParams&)>;

	static AwaitableResolver resolve(typename ResultTraits<Type>::future_type result,
		ResolverParams params, ResolverCallback&& resolver)
	{
		static_assert(!std::is_base_of_v<Object, Type>,
			"ModfiedResult<Object> needs special handling");

		auto value = result.get_value();

		if (value)
		{
			ModifiedResult::validateScalar(*value);
			co_return ResolverResult { response::Value { std::shared_ptr { std::move(value) } } };
		}

		auto pendingResolver = std::move(resolver);
		ResolverResult document;

		try
		{
			co_await params.launch;
			document.data = pendingResolver(co_await result, params);
		}
		catch (schema_exception& scx)
		{
			auto errors = scx.getStructuredErrors();

			if (!errors.empty())
			{
				document.errors.splice(document.errors.end(), errors);
			}
		}
		catch (const std::exception& ex)
		{
			std::ostringstream message;

			message << "Field name: " << params.fieldName << " unknown error: " << ex.what();

			document.errors.emplace_back(schema_error { message.str(),
				params.getLocation(),
				buildErrorPath(params.errorPath) });
		}

		co_return document;
	}
};

// Convenient type aliases for testing, generated code won't actually use these. These are also
// the specializations which are implemented in the GraphQLService library, other specializations
// for output types should be generated in schemagen.
using IntResult = ModifiedResult<int>;
using FloatResult = ModifiedResult<double>;
using StringResult = ModifiedResult<std::string>;
using BooleanResult = ModifiedResult<bool>;
using IdResult = ModifiedResult<response::IdType>;
using ScalarResult = ModifiedResult<response::Value>;
using ObjectResult = ModifiedResult<Object>;

#ifdef GRAPHQL_DLLEXPORTS
// Export all of the built-in converters
template <>
GRAPHQLSERVICE_EXPORT AwaitableResolver ModifiedResult<int>::convert(
	AwaitableScalar<int> result, ResolverParams params);
template <>
GRAPHQLSERVICE_EXPORT AwaitableResolver ModifiedResult<double>::convert(
	AwaitableScalar<double> result, ResolverParams params);
template <>
GRAPHQLSERVICE_EXPORT AwaitableResolver ModifiedResult<std::string>::convert(
	AwaitableScalar<std::string> result, ResolverParams params);
template <>
GRAPHQLSERVICE_EXPORT AwaitableResolver ModifiedResult<bool>::convert(
	AwaitableScalar<bool> result, ResolverParams params);
template <>
GRAPHQLSERVICE_EXPORT AwaitableResolver ModifiedResult<response::IdType>::convert(
	AwaitableScalar<response::IdType> result, ResolverParams params);
template <>
GRAPHQLSERVICE_EXPORT AwaitableResolver ModifiedResult<response::Value>::convert(
	AwaitableScalar<response::Value> result, ResolverParams params);
template <>
GRAPHQLSERVICE_EXPORT AwaitableResolver ModifiedResult<Object>::convert(
	AwaitableObject<std::shared_ptr<const Object>> result, ResolverParams params);

// Export all of the scalar value validation methods
template <>
GRAPHQLSERVICE_EXPORT void ModifiedResult<int>::validateScalar(const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT void ModifiedResult<double>::validateScalar(const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT void ModifiedResult<std::string>::validateScalar(
	const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT void ModifiedResult<bool>::validateScalar(const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT void ModifiedResult<response::IdType>::validateScalar(
	const response::Value& value);
template <>
GRAPHQLSERVICE_EXPORT void ModifiedResult<response::Value>::validateScalar(
	const response::Value& value);
#endif // GRAPHQL_DLLEXPORTS

// Subscription callbacks receive the response::Value representing the result of evaluating the
// SelectionSet against the payload.
using SubscriptionCallback = std::function<void(response::Value)>;

// Subscriptions are stored in maps using these keys.
using SubscriptionKey = size_t;
using SubscriptionName = std::string;

using AwaitableSubscribe = internal::Awaitable<SubscriptionKey>;
using AwaitableUnsubscribe = internal::Awaitable<void>;
using AwaitableDeliver = internal::Awaitable<void>;

struct RequestResolveParams
{
	// Required query information.
	peg::ast& query;
	std::string_view operationName {};
	response::Value variables { response::Type::Map };

	// Optional async execution awaitable.
	await_async launch {};

	// Optional sub-class of RequestState which will be passed to each resolver and field accessor.
	std::shared_ptr<RequestState> state {};
};

struct RequestSubscribeParams
{
	// Callback which receives the event data.
	SubscriptionCallback callback;

	// Required query information.
	peg::ast query;
	std::string operationName {};
	response::Value variables { response::Type::Map };

	// Optional async execution awaitable.
	await_async launch {};

	// Optional sub-class of RequestState which will be passed to each resolver and field accessor.
	std::shared_ptr<RequestState> state {};
};

struct RequestUnsubscribeParams
{
	// Key returned by a previous call to subscribe.
	SubscriptionKey key;

	// Optional async execution awaitable.
	await_async launch {};
};

using SubscriptionArguments = std::map<std::string_view, response::Value>;
using SubscriptionArgumentFilterCallback = std::function<bool(response::MapType::const_reference)>;
using SubscriptionDirectiveFilterCallback = std::function<bool(Directives::const_reference)>;

struct SubscriptionFilter
{
	// Optional field argument filter, which can either be a set of required arguments, or a
	// callback which returns true if the arguments match custom criteria.
	std::optional<std::variant<SubscriptionArguments, SubscriptionArgumentFilterCallback>>
		arguments;

	// Optional field directives filter, which can either be a set of required directives and
	// arguments, or a callback which returns true if the directives match custom criteria.
	std::optional<std::variant<Directives, SubscriptionDirectiveFilterCallback>> directives {};
};

// Deliver to a specific subscription key, or apply custom criteria for the field name, arguments,
// and directives in the Subscription query.
using RequestDeliverFilter = std::optional<std::variant<SubscriptionKey, SubscriptionFilter>>;

struct RequestDeliverParams
{
	// Deliver to subscriptions on this field.
	std::string_view field;

	// Optional filter to control which subscriptions will receive the event. If not specified,
	// every subscription on this field will receive the event and evaluate their queries.
	RequestDeliverFilter filter {};

	// Optional async execution awaitable.
	await_async launch {};

	// Optional override for the default Subscription operation object.
	std::shared_ptr<const Object> subscriptionObject {};
};

using TypeMap = internal::string_view_map<std::shared_ptr<const Object>>;

// State which is captured and kept alive until all pending futures have been resolved for an
// operation. Note: SelectionSet is the other parameter that gets passed to the top level Object,
// it's a borrowed reference to an element in the AST. In the case of query and mutation operations,
// it's up to the caller to guarantee the lifetime of the AST exceeds the futures we return.
// Subscription operations need to hold onto the queries in SubscriptionData, so the lifetime is
// already tied to the registration and any pending futures passed to callbacks.
struct OperationData : std::enable_shared_from_this<OperationData>
{
	explicit OperationData(std::shared_ptr<RequestState> state, response::Value variables,
		Directives directives, FragmentMap fragments);

	std::shared_ptr<RequestState> state;
	response::Value variables;
	Directives directives;
	FragmentMap fragments;
};

// Registration information for subscription, cached in the Request::subscribe call.
struct SubscriptionData : std::enable_shared_from_this<SubscriptionData>
{
	explicit SubscriptionData(std::shared_ptr<OperationData> data, SubscriptionName&& field,
		response::Value arguments, Directives fieldDirectives, peg::ast&& query,
		std::string&& operationName, SubscriptionCallback&& callback,
		const peg::ast_node& selection);

	std::shared_ptr<OperationData> data;

	SubscriptionName field;
	response::Value arguments;
	Directives fieldDirectives;
	peg::ast query;
	std::string operationName;
	SubscriptionCallback callback;
	const peg::ast_node& selection;
};

// Forward declare just the class type so we can reference it in the Request::_validation member.
class ValidateExecutableVisitor;

// Request scans the fragment definitions and finds the right operation definition to interpret
// depending on the operation name (which might be empty for a single-operation document). It
// also needs the values of the request variables.
class Request : public std::enable_shared_from_this<Request>
{
protected:
	GRAPHQLSERVICE_EXPORT explicit Request(
		TypeMap operationTypes, std::shared_ptr<schema::Schema> schema);
	GRAPHQLSERVICE_EXPORT virtual ~Request();

public:
	GRAPHQLSERVICE_EXPORT std::list<schema_error> validate(peg::ast& query) const;

	GRAPHQLSERVICE_EXPORT std::pair<std::string_view, const peg::ast_node*> findOperationDefinition(
		peg::ast& query, std::string_view operationName) const;

	GRAPHQLSERVICE_EXPORT response::AwaitableValue resolve(RequestResolveParams params) const;
	GRAPHQLSERVICE_EXPORT AwaitableSubscribe subscribe(RequestSubscribeParams params);
	GRAPHQLSERVICE_EXPORT AwaitableUnsubscribe unsubscribe(RequestUnsubscribeParams params);
	GRAPHQLSERVICE_EXPORT AwaitableDeliver deliver(RequestDeliverParams params) const;

private:
	SubscriptionKey addSubscription(RequestSubscribeParams&& params);
	void removeSubscription(SubscriptionKey key);
	std::vector<std::shared_ptr<const SubscriptionData>> collectRegistrations(
		std::string_view field, RequestDeliverFilter&& filter) const noexcept;

	const TypeMap _operations;
	mutable std::mutex _validationMutex {};
	const std::unique_ptr<ValidateExecutableVisitor> _validation;
	mutable std::mutex _subscriptionMutex {};
	internal::sorted_map<SubscriptionKey, std::shared_ptr<const SubscriptionData>> _subscriptions;
	internal::sorted_map<SubscriptionName, internal::sorted_set<SubscriptionKey>> _listeners;
	SubscriptionKey _nextKey = 0;
};

} // namespace service
} // namespace graphql

#endif // GRAPHQLSERVICE_H
