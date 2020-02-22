// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLSERVICE_H
#define GRAPHQLSERVICE_H

#include <graphqlservice/GraphQLParse.h>
#include <graphqlservice/GraphQLResponse.h>

#include <memory>
#include <optional>
#include <variant>
#include <string_view>
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

namespace graphql::introspection {

class Schema;

} /* namespace graphql::introspection */

namespace graphql::service {

// This exception bubbles up 1 or more error messages to the JSON results.
class schema_exception : public std::exception
{
public:
	explicit schema_exception(std::vector<std::string>&& messages);

	schema_exception() = delete;

	const char* what() const noexcept override;

	const response::Value& getErrors() const noexcept;
	response::Value getErrors() noexcept;

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

namespace {

using namespace std::literals;

constexpr std::string_view strData{ "data"sv };
constexpr std::string_view strErrors{ "errors"sv };
constexpr std::string_view strMessage{ "message"sv };
constexpr std::string_view strQuery{ "query"sv };
constexpr std::string_view strMutation{ "mutation"sv };
constexpr std::string_view strSubscription{ "subscription"sv };

}

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

// Field accessors may return either a result of T or a std::future<T>, so at runtime the implementer
// may choose to return by value or defer/parallelize expensive operations by returning an async future.
template <typename T>
class FieldResult
{
public:
	template <typename U>
	FieldResult(U&& value)
		: _value{ std::forward<U>(value) }
	{
	}

	T get()
	{
		if (std::holds_alternative<std::future<T>>(_value))
		{
			return std::get<std::future<T>>(std::move(_value)).get();
		}

		return std::get<T>(std::move(_value));
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
	explicit ResolverParams(const SelectionSetParams& selectionSetParams, std::string&& fieldName, response::Value&& arguments, response::Value&& fieldDirectives,
		const peg::ast_node* selection, const FragmentMap& fragments, const response::Value& variables);

	// These values are different for each resolver.
	std::string fieldName;
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
	static std::string toBase64(const std::vector<uint8_t> & bytes);

private:
	static constexpr char padding = '=';

	// Throw a schema_exception if the character is out of range.
	static uint8_t verifyFromBase64(char ch);

	// Throw a logic_error if the integer is out of range.
	static char verifyToBase64(uint8_t i);
};

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
				std::vector<typename ArgumentTraits<U, Other...>::type>,
				U>>;
	};

	template <typename U>
	struct ArgumentTraits<U, TypeModifier::None>
	{
		using type = U;
	};

	// Convert a single value to the specified type.
	static Type convert(const response::Value& value);

	// Call convert on this type without any modifiers.
	static Type require(const std::string& name, const response::Value& arguments)
	{
		try
		{
			return convert(arguments[name]);
		}
		catch (schema_exception & ex)
		{
			auto errors = ex.getErrors().release<response::ListType>();
			std::vector<std::string> messages(errors.size());

			std::transform(errors.begin(), errors.end(), messages.begin(),
				[&name](response::Value & error)
				{
					auto errorMessages = error.release<response::MapType>();
					auto messageText = errorMessages.front().second.release<response::StringType>();
					std::ostringstream message;

					message << "Invalid argument: " << name
						<< " error: " << messageText;

					return message.str();
				});

			throw schema_exception(std::move(messages));
		}
	}

	// Wrap require in a try/catch block.
	static std::pair<Type, bool> find(const std::string& name, const response::Value& arguments) noexcept
	{
		try
		{
			return { require(name, arguments), true };
		}
		catch (const std::exception&)
		{
			return { Type{}, false };
		}
	}

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other >
	static typename std::enable_if_t<TypeModifier::None == Modifier && sizeof...(Other) == 0, Type> require(
		const std::string& name, const response::Value& arguments)
	{
		// Just call through to the non-template method without the modifiers.
		return require(name, arguments);
	}

	// Peel off nullable modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::Nullable == Modifier, typename ArgumentTraits<Type, Modifier, Other...>::type> require(
		const std::string& name, const response::Value& arguments)
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
	static typename std::enable_if_t<TypeModifier::List == Modifier, typename ArgumentTraits<Type, Modifier, Other...>::type> require(
		const std::string& name, const response::Value& arguments)
	{
		const auto& values = arguments[name];
		typename ArgumentTraits<Type, Modifier, Other...>::type result(values.size());
		const auto& elements = values.get<response::ListType>();

		std::transform(elements.cbegin(), elements.cend(), result.begin(),
			[&name](const response::Value & element)
			{
				response::Value single(response::Type::Map);

				single.emplace_back(std::string{ name }, response::Value(element));

				return require<Other...>(name, single);
			});

		return result;
	}

	// Wrap require with modifiers in a try/catch block.
	template <TypeModifier Modifier, TypeModifier... Other>
	static std::pair<typename ArgumentTraits<Type, Modifier, Other...>::type, bool> find(
		const std::string& name, const response::Value& arguments) noexcept
	{
		try
		{
			return { require<Modifier, Other...>(name, arguments), true };
		}
		catch (const std::exception&)
		{
			return { typename ArgumentTraits<Type, Modifier, Other...>::type{}, false };
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
using IdArgument = ModifiedArgument<response::IdType>;
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
template <typename Type>
struct ModifiedResult
{
	// Peel off modifiers until we get to the underlying type.
	template <typename U, TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	struct ResultTraits
	{
		using type = typename std::conditional_t<TypeModifier::Nullable == Modifier,
			typename std::conditional_t<std::is_base_of_v<Object, U>
				&& std::is_same_v<std::shared_ptr<U>, typename ResultTraits<U, Other...>::type>,
				std::shared_ptr<U>,
				std::optional<typename ResultTraits<U, Other...>::type>>,
			typename std::conditional_t<TypeModifier::List == Modifier,
				std::vector<typename ResultTraits<U, Other...>::type>,
				typename std::conditional_t<std::is_base_of_v<Object, U>,
					std::shared_ptr<U>,
					U>>>;

		using future_type = FieldResult<type> &&;
	};

	template <typename U>
	struct ResultTraits<U, TypeModifier::None>
	{
		using type = typename std::conditional_t<std::is_base_of_v<Object, U>,
			std::shared_ptr<U>,
			U>;

		using future_type = typename std::conditional_t<std::is_base_of_v<Object, Type>,
			FieldResult<std::shared_ptr<Object>>,
			FieldResult<type>> &&;
	};

	// Convert a single value of the specified type to JSON.
	static std::future<response::Value> convert(typename ResultTraits<Type>::future_type result, ResolverParams&& params);

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::None == Modifier && sizeof...(Other) == 0 && !std::is_same_v<Object, Type> && std::is_base_of_v<Object, Type>,
		std::future<response::Value>> convert(FieldResult<typename ResultTraits<Type>::type> && result, ResolverParams && params)
	{
		// Call through to the Object specialization with a static_pointer_cast for subclasses of Object.
		static_assert(std::is_same_v<std::shared_ptr<Type>, typename ResultTraits<Type>::type>, "this is the derived object type");
		auto resultFuture = std::async(std::launch::deferred,
			[](auto && objectType)
			{
				return std::static_pointer_cast<Object>(objectType.get());
			}, std::move(result));

		return ModifiedResult<Object>::convert(std::move(resultFuture), std::move(params));
	}

	// Peel off the none modifier. If it's included, it should always be last in the list.
	template <TypeModifier Modifier = TypeModifier::None, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::None == Modifier && sizeof...(Other) == 0 && (std::is_same_v<Object, Type> || !std::is_base_of_v<Object, Type>),
		std::future<response::Value>> convert(typename ResultTraits<Type>::future_type result, ResolverParams && params)
	{
		// Just call through to the partial specialization without the modifier.
		return convert(std::move(result), std::move(params));
	}

	// Peel off final nullable modifiers for std::shared_ptr of Object and subclasses of Object.
	template <TypeModifier Modifier, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::Nullable == Modifier && std::is_same_v<std::shared_ptr<Type>, typename ResultTraits<Type, Other...>::type>,
		std::future<response::Value>> convert(typename ResultTraits<Type, Modifier, Other...>::future_type result, ResolverParams && params)
	{
		return std::async(std::launch::deferred,
			[](auto && wrappedFuture, ResolverParams && wrappedParams)
			{
				auto wrappedResult = wrappedFuture.get();

				if (!wrappedResult)
				{
					response::Value document(response::Type::Map);

					document.emplace_back(std::string{ strData }, response::Value());

					return document;
				}

				std::promise<typename ResultTraits<Type, Other...>::type> promise;

				promise.set_value(std::move(wrappedResult));

				return convert<Other...>(promise.get_future(), std::move(wrappedParams)).get();
			}, std::move(result), std::move(params));
	}

	// Peel off nullable modifiers for anything else, which should all be std::optional.
	template <TypeModifier Modifier, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::Nullable == Modifier && !std::is_same_v<std::shared_ptr<Type>, typename ResultTraits<Type, Other...>::type>,
		std::future<response::Value>> convert(typename ResultTraits<Type, Modifier, Other...>::future_type result, ResolverParams && params)
	{
		static_assert(std::is_same_v<std::optional<typename ResultTraits<Type, Other...>::type>, typename ResultTraits<Type, Modifier, Other...>::type>,
			"this is the optional version");

		return std::async(std::launch::deferred,
			[](auto && wrappedFuture, ResolverParams && wrappedParams)
			{
				auto wrappedResult = wrappedFuture.get();

				if (!wrappedResult)
				{
					response::Value document(response::Type::Map);

					document.emplace_back(std::string{ strData }, response::Value());

					return document;
				}

				std::promise<typename ResultTraits<Type, Other...>::type> promise;

				promise.set_value(std::move(*wrappedResult));

				return convert<Other...>(promise.get_future(), std::move(wrappedParams)).get();
			}, std::move(result), std::move(params));
	}

	// Peel off list modifiers.
	template <TypeModifier Modifier, TypeModifier... Other>
	static typename std::enable_if_t<TypeModifier::List == Modifier,
		std::future<response::Value>> convert(typename ResultTraits<Type, Modifier, Other...>::future_type result, ResolverParams && params)
	{
		return std::async(std::launch::deferred,
			[](auto && wrappedFuture, ResolverParams && wrappedParams)
			{
				auto wrappedResult = wrappedFuture.get();
				std::queue<std::future<response::Value>> children;

				for (auto& entry : wrappedResult)
				{
					children.push(convert<Other...>(std::move(entry), ResolverParams(wrappedParams)));
				}

				response::Value data(response::Type::List);
				response::Value errors(response::Type::List);
				size_t index = 0;

				while (!children.empty())
				{
					try
					{
						auto value = children.front().get();
						auto members = value.release<response::MapType>();

						for (auto& entry : members)
						{
							if (entry.second.type() == response::Type::List
								&& entry.first == strErrors)
							{
								auto errorEntries = entry.second.release<response::ListType>();

								for (auto& errorEntry : errorEntries)
								{
									errors.emplace_back(std::move(errorEntry));
								}
							}
							else if (entry.first == strData)
							{
								data.emplace_back(std::move(entry.second));
							}
						}
					}
					catch (schema_exception& scx)
					{
						auto messages = scx.getErrors().release<response::ListType>();

						errors.reserve(errors.size() + messages.size());
						for (auto& error : messages)
						{
							errors.emplace_back(std::move(error));
						}
					}
					catch (const std::exception & ex)
					{
						std::ostringstream message;

						message << "Field error name: " << wrappedParams.fieldName
							<< "[" << index << "] "
							<< " unknown error: " << ex.what();

						response::Value error(response::Type::Map);

						error.emplace_back(std::string{ strMessage }, response::Value(message.str()));
						errors.emplace_back(std::move(error));
					}

					children.pop();
					++index;
				}

				response::Value document(response::Type::Map);

				document.emplace_back(std::string{ strData }, std::move(data));

				if (errors.size() > 0)
				{
					document.emplace_back(std::string{ strErrors }, std::move(errors));
				}

				return document;
			}, std::move(result), std::move(params));
	}

private:
	using ResolverCallback = std::function<response::Value(typename ResultTraits<Type>::type&&, const ResolverParams&)>;

	static std::future<response::Value> resolve(typename ResultTraits<Type>::future_type result, ResolverParams&& params, ResolverCallback&& resolver)
	{
		static_assert(!std::is_base_of_v<Object, Type>, "ModfiedResult<Object> needs special handling");
		return std::async(std::launch::deferred,
			[](auto && resultFuture, ResolverParams && paramsFuture, ResolverCallback && resolverFuture) noexcept
			{
				response::Value data;
				response::Value errors(response::Type::List);

				try
				{
					data = resolverFuture(resultFuture.get(), paramsFuture);
				}
				catch (schema_exception& scx)
				{
					errors = scx.getErrors();
				}
				catch (const std::exception & ex)
				{
					std::ostringstream message;

					message << "Field name: " << paramsFuture.fieldName
						<< " unknown error: " << ex.what();

					response::Value error(response::Type::Map);

					error.emplace_back(std::string{ strMessage }, response::Value(message.str()));
					errors.emplace_back(std::move(error));
				}

				response::Value document(response::Type::Map);

				document.emplace_back(std::string{ strData }, std::move(data));

				if (errors.size() > 0)
				{
					document.emplace_back(std::string{ strErrors }, std::move(errors));
				}

				return document;
			}, std::move(result), std::move(params), std::move(resolver));
	}
};

// Convenient type aliases for testing, generated code won't actually use these. These are also
// the specializations which are implemented in the GraphQLService library, other specializations
// for output types should be generated in schemagen.
using IntResult = ModifiedResult<response::IntType>;
using FloatResult = ModifiedResult<response::FloatType>;
using StringResult = ModifiedResult<response::StringType>;
using BooleanResult = ModifiedResult<response::BooleanType>;
using IdResult = ModifiedResult<response::IdType>;
using ScalarResult = ModifiedResult<response::Value>;
using ObjectResult = ModifiedResult<Object>;

using TypeMap = std::unordered_map<std::string, std::shared_ptr<Object>>;

// You can still sub-class RequestState and use that in the state parameter to Request::subscribe
// to add your own state to the service callbacks that you receive while executing the subscription
// query.
struct SubscriptionParams
{
	std::shared_ptr<RequestState> state;
	peg::ast query;
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
using SubscriptionFilterCallback = std::function<bool(response::MapType::const_reference)>;

// Subscriptions are stored in maps using these keys.
using SubscriptionKey = size_t;
using SubscriptionName = std::string;

// Registration information for subscription, cached in the Request::subscribe call.
struct SubscriptionData : std::enable_shared_from_this<SubscriptionData>
{
	explicit SubscriptionData(std::shared_ptr<OperationData>&& data, std::unordered_map<SubscriptionName, std::vector<response::Value>>&& fieldNamesAndArgs,
		peg::ast&& query, std::string&& operationName, SubscriptionCallback&& callback,
		const peg::ast_node& selection);

	std::shared_ptr<OperationData> data;
	std::unordered_map<SubscriptionName, std::vector<response::Value>> fieldNamesAndArgs;
	peg::ast query;
	std::string operationName;
	SubscriptionCallback callback;
	const peg::ast_node& selection;
};

// Request scans the fragment definitions and finds the right operation definition to interpret
// depending on the operation name (which might be empty for a single-operation document). It
// also needs the values of the request variables.
class Request : public std::enable_shared_from_this<Request>
{
protected:
	explicit Request(TypeMap&& operationTypes, const std::shared_ptr<introspection::Schema>& schema);
	virtual ~Request() = default;

public:
	std::pair<std::string, const peg::ast_node*> findOperationDefinition(const peg::ast_node& root, const std::string& operationName) const;

	std::future<response::Value> resolve(const std::shared_ptr<RequestState>& state, const peg::ast_node& root, const std::string& operationName, response::Value&& variables) const;
	std::future<response::Value> resolve(std::launch launch, const std::shared_ptr<RequestState>& state, const peg::ast_node& root, const std::string& operationName, response::Value&& variables) const;

	SubscriptionKey subscribe(SubscriptionParams&& params, SubscriptionCallback&& callback);
	void unsubscribe(SubscriptionKey key);

	void deliver(const SubscriptionName& name, const std::shared_ptr<Object>& subscriptionObject) const;
	void deliver(const SubscriptionName& name, const SubscriptionArguments& arguments, const std::shared_ptr<Object>& subscriptionObject) const;
	void deliver(const SubscriptionName& name, const SubscriptionFilterCallback& apply, const std::shared_ptr<Object>& subscriptionObject) const;

private:
	TypeMap _operations;
	std::shared_ptr<introspection::Schema> _schema;
	std::map<SubscriptionKey, std::shared_ptr<SubscriptionData>> _subscriptions;
	std::unordered_map<SubscriptionName, std::set<SubscriptionKey>> _listeners;
	SubscriptionKey _nextKey = 0;
};

} /* namespace graphql::service */

#endif // GRAPHQLSERVICE_H
