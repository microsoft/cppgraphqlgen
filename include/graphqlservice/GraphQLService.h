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

// The RequestId is optional, but if you have multiple threads processing requests and there's any
// per-request state that you want to maintain throughout the request (e.g. optimizing or batching
// backend requests), you can use the RequestId to correlate the asynchronous/recursive callbacks.
using RequestId = size_t;

// Fragments are referenced by name and have a single type condition (except for inline
// fragments, where the type condition is common but optional). They contain a set of fields
// (with optional aliases and sub-selections) and potentially references to other fragments.
class Fragment
{
public:
	explicit Fragment(const peg::ast_node& fragmentDefinition);

	const std::string& getType() const;
	const peg::ast_node& getSelection() const;

private:
	std::string _type;

	const peg::ast_node& _selection;
};

// Resolvers for complex types need to be able to find fragment definitions anywhere in
// the request document by name.
using FragmentMap = std::unordered_map<std::string, Fragment>;

// Resolver functors take a set of arguments encoded as members on a JSON object
// with an optional selection set for complex types and return a JSON value for
// a single field.
struct ResolverParams
{
	RequestId requestId;
	const response::Value& arguments;
	const peg::ast_node* selection;
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
		catch (const schema_exception&)
		{
			return { {}, false };
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
		catch (const schema_exception&)
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

	std::future<response::Value> resolve(RequestId requestId, const peg::ast_node& selection, const FragmentMap& fragments, const response::Value& variables) const;

protected:
	// It's up to sub-classes to decide if they want to use const_cast, mutable, or separate storage
	// to accumulate state. By default these callbacks should treat the Object itself as const.
	virtual void beginSelectionSet(RequestId requestId) const;
	virtual void endSelectionSet(RequestId requestId) const;

private:
	TypeNames _typeNames;
	ResolverMap _resolvers;
};

using TypeMap = std::unordered_map<std::string, std::shared_ptr<Object>>;

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

// Request scans the fragment definitions and finds the right operation definition to interpret
// depending on the operation name (which might be empty for a single-operation document). It
// also needs the values of hte request variables.
class Request : public std::enable_shared_from_this<Request>
{
public:
	explicit Request(TypeMap&& operationTypes);
	virtual ~Request() = default;

	std::future<response::Value> resolve(RequestId requestId, const peg::ast_node& root, const std::string& operationName, const response::Value& variables) const;

private:
	TypeMap _operations;
};

// SelectionVisitor visits the AST and resolves a field or fragment, unless it's skipped by
// a directive or type condition.
class SelectionVisitor
{
public:
	SelectionVisitor(RequestId requestId, const FragmentMap& fragments, const response::Value& variables, const TypeNames& typeNames, const ResolverMap& resolvers);

	void visit(const peg::ast_node& selection);

	std::future<response::Value> getValues();

private:
	bool shouldSkip(const std::vector<std::unique_ptr<peg::ast_node>>* directives) const;

	void visitField(const peg::ast_node& field);
	void visitFragmentSpread(const peg::ast_node& fragmentSpread);
	void visitInlineFragment(const peg::ast_node& inlineFragment);

	const RequestId _requestId;
	const FragmentMap& _fragments;
	const response::Value& _variables;
	const TypeNames& _typeNames;
	const ResolverMap& _resolvers;

	std::queue<std::pair<std::string, std::future<response::Value>>> _values;
};

// ValueVisitor visits the AST and builds a JSON representation of any value
// hardcoded or referencing a variable in an operation.
class ValueVisitor
{
public:
	ValueVisitor(const response::Value& variables);

	void visit(const peg::ast_node& value);

	response::Value getValue();

private:
	void visitVariable(const peg::ast_node& variable);
	void visitIntValue(const peg::ast_node& intValue);
	void visitFloatValue(const peg::ast_node& floatValue);
	void visitStringValue(const peg::ast_node& stringValue);
	void visitBooleanValue(const peg::ast_node& booleanValue);
	void visitNullValue(const peg::ast_node& nullValue);
	void visitEnumValue(const peg::ast_node& enumValue);
	void visitListValue(const peg::ast_node& listValue);
	void visitObjectValue(const peg::ast_node& objectValue);

	const response::Value& _variables;
	response::Value _value;
};

// FragmentDefinitionVisitor visits the AST and collects all of the fragment
// definitions in the document.
class FragmentDefinitionVisitor
{
public:
	FragmentDefinitionVisitor();

	FragmentMap getFragments();

	void visit(const peg::ast_node& fragmentDefinition);

private:
	FragmentMap _fragments;
};

// OperationDefinitionVisitor visits the AST and executes the one with the specified
// operation name.
class OperationDefinitionVisitor
{
public:
	OperationDefinitionVisitor(RequestId requestId, const TypeMap& operations, const std::string& operationName, const response::Value& variables, const FragmentMap& fragments);

	std::future<response::Value> getValue();

	void visit(const peg::ast_node& operationDefinition);

private:
	const RequestId _requestId;
	const TypeMap& _operations;
	const std::string& _operationName;
	const response::Value& _variables;
	const FragmentMap& _fragments;
	std::future<response::Value> _result;
};

} /* namespace service */
} /* namespace graphql */
} /* namespace facebook */
