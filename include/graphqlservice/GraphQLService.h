// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <graphqlservice/GraphQLTree.h>
#include <graphqlservice/GraphQLResponse.h>

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <type_traits>
#include <future>
#include <queue>
#include <map>
#include <set>

namespace facebook {
namespace graphql {
namespace service {

// This exception bubbles up 1 or more error messages to the JSON results.
class schema_exception : public std::exception
{
public:
	schema_exception(std::vector<std::string>&& messages);

	const response::Value& getErrors() const noexcept;

private:
	response::Value _errors;
};

// The RequestState is nullable, but if you have multiple threads processing requests and there's any
// per-request state that you want to maintain throughout the request (e.g. optimizing or batching
// backend requests), you can inherit from RequestState and pass it to Request::resolve to correlate the
// asynchronous/recursive callbacks and accumulate state in it.
struct RequestState : std::enable_shared_from_this<RequestState>
{
};

// Pass a common bundle of parameters to all of the generated Object::getField accessors in a SelectionSet
struct SelectionSetParams
{
	// The lifetime of each of these borrowed references is guaranteed until the future returned
	// by the accessor is resolved or destroyed. They are owned by the OperationData shared pointer. 
	const std::shared_ptr<RequestState>& state;
	const response::Value& operationDirectives;
	const response::Value& fragmentDefinitionDirectives;

	// Fragment directives are shared for all fields in that fragment, but they aren't kept alive
	// after the call to the last accessor in the fragment. If you need to keep them alive longer,
	// you'll need to explicitly copy them into other instances of response::Value.
	const response::Value& fragmentSpreadDirectives;
	const response::Value& inlineFragmentDirectives;
};

// Pass a common bundle of parameters to all of the generated Object::getField accessors.
struct FieldParams : SelectionSetParams
{
	explicit FieldParams(const SelectionSetParams& selectionSetParams, response::Value&& directives);

	// Each field owns its own field-specific directives. Once the accessor returns it will be destroyed,
	// but you can move it into another instance of response::Value to keep it alive longer.
	response::Value fieldDirectives;
};

// Fragments are referenced by name and have a single type condition (except for inline
// fragments, where the type condition is common but optional). They contain a set of fields
// (with optional aliases and sub-selections) and potentially references to other fragments.
class Fragment
{
public:
	explicit Fragment(const peg::ast_node& fragmentDefinition, const response::Value& variables);

	const std::string& getType() const;
	const peg::ast_node& getSelection() const;
	const response::Value& getDirectives() const;

private:
	std::string _type;
	response::Value _directives;

	const peg::ast_node& _selection;
};

// Resolvers for complex types need to be able to find fragment definitions anywhere in
// the request document by name.
using FragmentMap = std::unordered_map<std::string, Fragment>;

// Resolver functors take a set of arguments encoded as members on a JSON object
// with an optional selection set for complex types and return a JSON value for
// a single field.
struct ResolverParams : SelectionSetParams
{
	explicit ResolverParams(const SelectionSetParams& selectionSetParams, response::Value&& arguments, response::Value&& fieldDirectives,
		const peg::ast_node* selection, const FragmentMap& fragments, const response::Value& variables);

	// These values are different for each resolver.
	response::Value arguments { response::Type::Map };
	response::Value fieldDirectives { response::Type::Map };
	const peg::ast_node* selection;

	// These values remain unchanged for the entire operation, but they're passed to each of the
	// resolvers recursively through ResolverParams.
	const FragmentMap& fragments;
	const response::Value& variables;
};

using Resolver = std::function<std::future<response::Value>(ResolverParams&&)>;
using ResolverMap = std::unordered_map<std::string, Resolver>;

// Binary data and opaque strings like IDs are encoded in Base64.
class Base64
{
public:
	// Map a single Base64-encoded character to its 6-bit integer value.
	static constexpr uint8_t fromBase64(char ch) noexcept
	{
		return (ch >= 'A' && ch <= 'Z' ? ch - 'A'
			: (ch >= 'a' && ch <= 'z' ? ch - 'a' + 26
				: (ch >= '0' && ch <= '9' ? ch - '0' + 52
					: (ch == '+' ? 62
						: (ch == '/' ? 63 : 0xFF)))));
	}

	// Convert a Base64-encoded string to a vector of bytes.
	static std::vector<uint8_t> fromBase64(const char* encoded, size_t count);

	// Map a single 6-bit integer value to its Base64-encoded character.
	static constexpr char toBase64(uint8_t i) noexcept
	{
		return (i < 26 ? static_cast<char>(i + static_cast<uint8_t>('A'))
			: (i < 52 ? static_cast<char>(i - 26 + static_cast<uint8_t>('a'))
				: (i < 62 ? static_cast<char>(i - 52 + static_cast<uint8_t>('0'))
					: (i == 62 ? '+'
						: (i == 63 ? '/' : padding)))));
	}

	// Convert a set of bytes to Base64.
	static std::string toBase64(const std::vector<uint8_t>& bytes);

private:
	static constexpr char padding = '=';

	// Throw a schema_exception if the character is out of range.
	static uint8_t verifyFromBase64(char ch);

	// Throw a logic_error if the integer is out of range.
	static char verifyToBase64(uint8_t i);
};

// Types be wrapped non-null or list types in GraphQL. Since nullability is a more special case
// in C++, we invert the default and apply that modifier instead when the non-null wrapper is
// not present in that part of the wrapper chain.
enum class TypeModifier
{
	None,
	Nullable,
	List,
};

// Extract individual arguments with chained type modifiers which add nullable or list wrappers.
// If the argument is not optional, use require and let it throw a schema_exception when the
// argument is missing or not the correct type. If it's nullable, use find and check the second
// element in the pair to see if it was found or if you just got the default value for that type.
template <typename _Type>
struct ModifiedArgument
{
	// Peel off modifiers until we get to the underlying type.
	template <typename U, TypeModifier _Modifier = TypeModifier::None, TypeModifier... _Other>
	struct ArgumentTraits
	{
		// Peel off modifiers until we get to the underlying type.
		using type = typename std::conditional<TypeModifier::Nullable == _Modifier,
			std::unique_ptr<typename ArgumentTraits<U, _Other...>::type>,
			typename std::conditional<TypeModifier::List == _Modifier,
				std::vector<typename ArgumentTraits<U, _Other...>::type>,
				U>::type
		>::type;
	};

	template <typename U>
	struct ArgumentTraits<U, TypeModifier::None>
	{
		using type = U;
	};

	// Convert a single value to the specified type.
	static _Type convert(const response::Value& value);

	// Call convert on this type without any modifiers.
	static _Type require(const std::string& name, const response::Value& arguments)
	{
		try
		{
			return convert(arguments[name]);
		}
		catch (const schema_exception& ex)
		{
			std::ostringstream error;

			error << "Invalid argument: " << name
				<< " message: " << ex.getErrors()[0]["message"].get<const response::StringType&>();
			throw schema_exception({ error.str() });
		}
	}

	// Wrap require in a try/catch block.
	static std::pair<_Type, bool> find(const std::string& name, const response::Value& arguments) noexcept
	{
		try
		{
			return { require(name, arguments), true };
		}
		catch (const std::exception&)
		{
			return { _Type{}, false };
		}
	}

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier _Modifier = TypeModifier::None , TypeModifier... _Other >
	static typename std::enable_if<TypeModifier::None == _Modifier && sizeof...(_Other) == 0, _Type>::type require(
		const std::string& name, const response::Value& arguments)
	{
		// Just call through to the non-template method without the modifiers.
		return require(name, arguments);
	}

	// Peel off nullable modifiers.
	template <TypeModifier _Modifier, TypeModifier... _Other>
	static typename std::enable_if<TypeModifier::Nullable == _Modifier, typename ArgumentTraits<_Type, _Modifier, _Other...>::type>::type require(
		const std::string& name, const response::Value& arguments)
	{
		const auto& valueItr = arguments.find(name);

		if (valueItr == arguments.get<const response::MapType&>().cend()
			|| valueItr->second.type() == response::Type::Null)
		{
			return nullptr;
		}

		auto result = require<_Other...>(name, arguments);

		return std::unique_ptr<decltype(result)> { new decltype(result)(std::move(result)) };
	}

	// Peel off list modifiers.
	template <TypeModifier _Modifier, TypeModifier... _Other>
	static typename std::enable_if<TypeModifier::List == _Modifier, typename ArgumentTraits<_Type, _Modifier, _Other...>::type>::type require(
		const std::string& name, const response::Value& arguments)
	{
		const auto& values = arguments[name];
		typename ArgumentTraits<_Type, _Modifier, _Other...>::type result(values.size());
		const auto& elements = values.get<const response::ListType&>();

		std::transform(elements.cbegin(), elements.cend(), result.begin(),
			[&name](const response::Value& element)
		{
			response::Value single(response::Type::Map);

			single.emplace_back(std::string(name), response::Value(element));

			return require<_Other...>(name, single);
		});

		return result;
	}

	// Wrap require with modifiers in a try/catch block.
	template <TypeModifier _Modifier, TypeModifier... _Other>
	static std::pair<typename ArgumentTraits<_Type, _Modifier, _Other...>::type, bool> find(
		const std::string& name, const response::Value& arguments) noexcept
	{
		try
		{
			return { require<_Modifier, _Other...>(name, arguments), true };
		}
		catch (const std::exception&)
		{
			return { typename ArgumentTraits<_Type, _Modifier, _Other...>::type{}, false };
		}
	}
};

// Convenient type aliases for testing, generated code won't actually use these. These are also
// the specializations which are implemented in the GraphQLService library, other specializations
// for input types should be generated in schemagen.
using IntArgument = ModifiedArgument<response::IntType>;
using FloatArgument = ModifiedArgument<response::FloatType>;
using StringArgument = ModifiedArgument<response::StringType>;
using BooleanArgument = ModifiedArgument<response::BooleanType>;
using IdArgument = ModifiedArgument<std::vector<uint8_t>>;
using ScalarArgument = ModifiedArgument<response::Value>;

// Each type should handle fragments with type conditions matching its own
// name and any inheritted interfaces.
using TypeNames = std::unordered_set<std::string>;

// Object parses argument values, performs variable lookups, expands fragments, evaluates @include
// and @skip directives, and calls through to the resolver functor for each selected field with
// its arguments. This may be a recursive process for fields which return another complex type,
// in which case it requires its own selection set.
class Object : public std::enable_shared_from_this<Object>
{
public:
	explicit Object(TypeNames&& typeNames, ResolverMap&& resolvers);
	virtual ~Object() = default;

	std::future<response::Value> resolve(const SelectionSetParams& selectionSetParams, const peg::ast_node& selection, const FragmentMap& fragments, const response::Value& variables) const;

	bool matchesType(const std::string& typeName) const;

protected:
	// These callbacks are optional, you may override either, both, or neither of them. The implementer
	// can use these to to accumulate state for the entire SelectionSet on this object, as well as
	// testing directives which were included at the operation or fragment level. It's up to sub-classes
	// to decide if they want to use const_cast, mutable members, or separate storage in the RequestState
	// to accumulate state. By default these callbacks should treat the Object itself as const.
	virtual void beginSelectionSet(const SelectionSetParams& params) const;
	virtual void endSelectionSet(const SelectionSetParams& params) const;

private:
	TypeNames _typeNames;
	ResolverMap _resolvers;
};

// Convert the result of a resolver function with chained type modifiers that add nullable or
// list wrappers. This is the inverse of ModifiedArgument for output types instead of input types.
template <typename _Type>
struct ModifiedResult
{
	// Peel off modifiers until we get to the underlying type.
	template <typename U, TypeModifier _Modifier = TypeModifier::None, TypeModifier... _Other>
	struct ResultTraits
	{
		using type = typename std::conditional<TypeModifier::Nullable == _Modifier,
			typename std::conditional<std::is_base_of<Object, U>::value
				&& std::is_same<std::shared_ptr<U>, typename ResultTraits<U, _Other...>::type>::value,
				std::shared_ptr<U>,
				std::unique_ptr<typename ResultTraits<U, _Other...>::type>
			>::type,
			typename std::conditional<TypeModifier::List == _Modifier,
				std::vector<typename ResultTraits<U, _Other...>::type>,
				typename std::conditional<std::is_base_of<Object, U>::value,
					std::shared_ptr<U>,
					U>::type
			>::type
		>::type;
	};

	template <typename U>
	struct ResultTraits<U, TypeModifier::None>
	{
		using type = typename std::conditional<std::is_base_of<Object, U>::value,
			std::shared_ptr<U>,
			U>::type;
	};

	// Convert a single value of the specified type to JSON.
	static std::future<response::Value> convert(
		typename std::conditional<std::is_base_of<Object, _Type>::value,
			std::future<std::shared_ptr<Object>>,
			std::future<typename ResultTraits<_Type>::type>&&>::type result,
		ResolverParams&& params);

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier _Modifier = TypeModifier::None, TypeModifier... _Other>
	static typename std::enable_if<TypeModifier::None == _Modifier && sizeof...(_Other) == 0 && !std::is_same<Object, _Type>::value && std::is_base_of<Object, _Type>::value,
		std::future<response::Value>>::type convert(std::future<typename ResultTraits<_Type>::type>&& result, ResolverParams&& params)
	{
		// Call through to the Object specialization with a static_pointer_cast for subclasses of Object.
		static_assert(std::is_same<std::shared_ptr<_Type>, typename ResultTraits<_Type>::type>::value, "this is the derived object type");
		auto resultFuture = std::async(std::launch::deferred,
			[](std::future<std::shared_ptr<_Type>>&& objectType)
		{
			return std::static_pointer_cast<Object>(objectType.get());
		}, std::move(result));

		return ModifiedResult<Object>::convert(std::move(resultFuture), std::move(params));
	}

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier _Modifier = TypeModifier::None, TypeModifier... _Other>
	static typename std::enable_if<TypeModifier::None == _Modifier && sizeof...(_Other) == 0 && (std::is_same<Object, _Type>::value || !std::is_base_of<Object, _Type>::value),
		std::future<response::Value>>::type convert(std::future<typename ResultTraits<_Type>::type>&& result, ResolverParams&& params)
	{
		// Just call through to the partial specialization without the modifier.
		return convert(std::move(result), std::move(params));
	}

	// Peel off final nullable modifiers for std::shared_ptr of Object and subclasses of Object.
	template <TypeModifier _Modifier, TypeModifier... _Other>
	static typename std::enable_if<TypeModifier::Nullable == _Modifier && std::is_same<std::shared_ptr<_Type>, typename ResultTraits<_Type, _Other...>::type>::value,
		std::future<response::Value>>::type convert(std::future<typename ResultTraits<_Type, _Modifier, _Other...>::type>&& result, ResolverParams&& params)
	{
		return std::async(std::launch::deferred,
			[](std::future<typename ResultTraits<_Type, _Modifier, _Other...>::type>&& wrappedFuture, ResolverParams&& wrappedParams)
		{
			auto wrappedResult = wrappedFuture.get();

			if (!wrappedResult)
			{
				return response::Value();
			}

			std::promise<typename ResultTraits<_Type, _Other...>::type> promise;

			promise.set_value(std::move(wrappedResult));

			return convert<_Other...>(promise.get_future(), std::move(wrappedParams)).get();
		}, std::move(result), std::move(params));
	}

	// Peel off nullable modifiers for anything else, which should all be std::unique_ptr.
	template <TypeModifier _Modifier, TypeModifier... _Other>
	static typename std::enable_if<TypeModifier::Nullable == _Modifier && !std::is_same<std::shared_ptr<_Type>, typename ResultTraits<_Type, _Other...>::type>::value,
		std::future<response::Value>>::type convert(std::future<typename ResultTraits<_Type, _Modifier, _Other...>::type>&& result, ResolverParams&& params)
	{
		static_assert(std::is_same<std::unique_ptr<typename ResultTraits<_Type, _Other...>::type>, typename ResultTraits<_Type, _Modifier, _Other...>::type>::value,
			"this is the unique_ptr version");

		return std::async(std::launch::deferred,
			[](std::future<typename ResultTraits<_Type, _Modifier, _Other...>::type>&& wrappedFuture, ResolverParams&& wrappedParams)
		{
			auto wrappedResult = wrappedFuture.get();

			if (!wrappedResult)
			{
				return response::Value();
			}

			std::promise<typename ResultTraits<_Type, _Other...>::type> promise;

			promise.set_value(std::move(*wrappedResult));

			return convert<_Other...>(promise.get_future(), std::move(wrappedParams)).get();
		}, std::move(result), std::move(params));
	}

	// Peel off list modifiers.
	template <TypeModifier _Modifier, TypeModifier... _Other>
	static typename std::enable_if<TypeModifier::List == _Modifier,
		std::future<response::Value>>::type convert(std::future<typename ResultTraits<_Type, _Modifier, _Other...>::type>&& result, ResolverParams&& params)
	{
		return std::async(std::launch::deferred,
			[](std::future<typename ResultTraits<_Type, _Modifier, _Other...>::type>&& wrappedFuture, ResolverParams&& wrappedParams)
		{
			auto wrappedResult = wrappedFuture.get();
			std::queue<std::future<response::Value>> children;

			for (auto& entry : wrappedResult)
			{
				std::promise<typename ResultTraits<_Type, _Other...>::type> promise;

				promise.set_value(std::move(entry));

				children.push(convert<_Other...>(promise.get_future(), ResolverParams(wrappedParams)));
			}

			auto value = response::Value(response::Type::List);

			value.reserve(wrappedResult.size());

			while (!children.empty())
			{
				value.emplace_back(children.front().get());
				children.pop();
			}

			return value;
		}, std::move(result), std::move(params));
	}
};

// Convenient type aliases for testing, generated code won't actually use these. These are also
// the specializations which are implemented in the GraphQLService library, other specializations
// for output types should be generated in schemagen.
using IntResult = ModifiedResult<response::IntType>;
using FloatResult = ModifiedResult<response::FloatType>;
using StringResult = ModifiedResult<response::StringType>;
using BooleanResult = ModifiedResult<response::BooleanType>;
using IdResult = ModifiedResult<std::vector<uint8_t>>;
using ScalarResult = ModifiedResult<response::Value>;
using ObjectResult = ModifiedResult<Object>;

using TypeMap = std::unordered_map<std::string, std::shared_ptr<Object>>;

// You can still sub-class RequestState and use that in the state parameter to Request::subscribe
// to add your own state to the service callbacks that you receive while executing the subscription
// query.
struct SubscriptionParams
{
	std::shared_ptr<RequestState> state;
	peg::ast<std::string> query;
	std::string operationName;
	response::Value variables;
};

// State which is captured and kept alive until all pending futures have been resolved for an operation.
// Note: SelectionSet is the other parameter that gets passed to the top level Object, it's a borrowed
// reference to an element in the AST. In the case of query and mutation operations, it's up to the caller
// to guarantee the lifetime of the AST exceeds the futures we return. Subscription operations need to
// hold onto the queries in SubscriptionData, so the lifetime is already tied to the registration and
// any pending futures passed to callbacks.
struct OperationData : std::enable_shared_from_this<OperationData>
{
	explicit OperationData(std::shared_ptr<RequestState>&& state, response::Value&& variables,
		response::Value&& directives, FragmentMap&& fragments);

	std::shared_ptr<RequestState> state;
	response::Value variables;
	response::Value directives;
	FragmentMap fragments;
};

// Subscription callbacks receive the response::Value representing the result of evaluating the
// SelectionSet against the payload.
using SubscriptionCallback = std::function<void(std::future<response::Value>)>;
using SubscriptionArguments = std::unordered_map<std::string, response::Value>;
using SubscriptionFilterCallback = std::function<bool(response::MapType::const_reference) noexcept>;

// Subscriptions are stored in maps using these keys.
using SubscriptionKey = size_t;
using SubscriptionName = std::string;

// Registration information for subscription, cached in the Request::subscribe call.
struct SubscriptionData : std::enable_shared_from_this<SubscriptionData>
{
	explicit SubscriptionData(std::shared_ptr<OperationData>&& data, std::unordered_map<SubscriptionName, std::vector<response::Value>>&& fieldNamesAndArgs,
		peg::ast<std::string>&& query, std::string&& operationName, SubscriptionCallback&& callback,
		const peg::ast_node& selection);

	std::shared_ptr<OperationData> data;
	std::unordered_map<SubscriptionName, std::vector<response::Value>> fieldNamesAndArgs;
	peg::ast<std::string> query;
	std::string operationName;
	SubscriptionCallback callback;
	const peg::ast_node& selection;
};

// Request scans the fragment definitions and finds the right operation definition to interpret
// depending on the operation name (which might be empty for a single-operation document). It
// also needs the values of the request variables.
class Request : public std::enable_shared_from_this<Request>
{
public:
	explicit Request(TypeMap&& operationTypes);
	virtual ~Request() = default;

	std::future<response::Value> resolve(const std::shared_ptr<RequestState>& state, const peg::ast_node& root, const std::string& operationName, response::Value&& variables) const;

	SubscriptionKey subscribe(SubscriptionParams&& params, SubscriptionCallback&& callback);
	void unsubscribe(SubscriptionKey key);

	void deliver(const SubscriptionName& name, const std::shared_ptr<Object>& subscriptionObject) const;
	void deliver(const SubscriptionName& name, const SubscriptionArguments& arguments, const std::shared_ptr<Object>& subscriptionObject) const;
	void deliver(const SubscriptionName& name, const SubscriptionFilterCallback& apply, const std::shared_ptr<Object>& subscriptionObject) const;

private:
	TypeMap _operations;
	std::map<SubscriptionKey, std::shared_ptr<SubscriptionData>> _subscriptions;
	std::unordered_map<SubscriptionName, std::set<SubscriptionKey>> _listeners;
	SubscriptionKey _nextKey = 0;
};

} /* namespace service */
} /* namespace graphql */
} /* namespace facebook */
