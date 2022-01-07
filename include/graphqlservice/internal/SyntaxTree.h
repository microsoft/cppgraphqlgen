// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLSYNTAXTREE_H
#define GRAPHQLSYNTAXTREE_H

#include "graphqlservice/GraphQLParse.h"

#define TAO_PEGTL_NAMESPACE tao::graphqlpeg

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace graphql::peg {

using namespace tao::graphqlpeg;
namespace peginternal = tao::graphqlpeg::internal;

class ast_node
{
public:
	// Must be default constructible
	ast_node() = default;

	// Nodes are always owned/handled by a std::unique_ptr
	// and never copied or assigned...
	ast_node(const ast_node&) = delete;
	ast_node(ast_node&&) = delete;
	ast_node& operator=(const ast_node&) = delete;
	ast_node& operator=(ast_node&&) = delete;

	// Must be destructible
	~ast_node() = default;

	[[nodiscard]] GRAPHQLPEG_EXPORT bool is_root() const noexcept;
	[[nodiscard]] GRAPHQLPEG_EXPORT position begin() const noexcept;
	[[nodiscard]] GRAPHQLPEG_EXPORT std::string_view string_view() const noexcept;
	[[nodiscard]] GRAPHQLPEG_EXPORT std::string string() const noexcept;
	[[nodiscard]] GRAPHQLPEG_EXPORT bool has_content() const noexcept;

	GRAPHQLPEG_EXPORT void unescaped_view(std::string_view unescaped) noexcept;
	[[nodiscard]] GRAPHQLPEG_EXPORT std::string_view unescaped_view() const;

	template <typename U>
	[[nodiscard]] bool is_type() const noexcept
	{
		const auto u = type_name<U>();

		// The pointer comparison doesn't work with shared libraries where the parse tree is
		// constructed in one module and consumed in another. So to optimize this comparison, check
		// the size first, then the hash (cached in a static local variable per specialization of
		// type_hash<U>()), then the pointer comparison with a full string compare as fallback.
		return _type.size() == u.size() && _type_hash == type_hash<U>()
			&& (_type.data() == u.data() || _type == u);
	}

	template <typename... States>
	void remove_content(States&&...) noexcept
	{
		_content = {};
		_unescaped.reset();
	}

	// All non-root nodes receive a call to start() when
	// a match is attempted for Rule in a parsing run...
	template <typename Rule, typename ParseInput, typename... States>
	void start(const ParseInput& in, States&&...)
	{
		_begin = in.iterator();
	}

	// ...and later a call to success() when the match succeeded...
	template <typename Rule, typename ParseInput, typename... States>
	void success(const ParseInput& in, States&&...)
	{
		const char* end = in.iterator().data;

		_type = type_name<Rule>();
		_type_hash = type_hash<Rule>();
		_source = in.source();
		_content = { _begin.data, static_cast<size_t>(end - _begin.data) };
		_unescaped.reset();
	}

	// ...or to failure() when a (local) failure was encountered.
	template <typename Rule, typename ParseInput, typename... States>
	void failure(const ParseInput&, States&&...)
	{
	}

	// if parsing of the rule failed with an exception, this method is called
	template <typename Rule, typename ParseInput, typename... States>
	void unwind(const ParseInput&, States&&...) noexcept
	{
	}

	// After a call to success(), and the (optional) call to the selector's
	// transform() did not discard a node, it is passed to its parent node
	// with a call to the parent node's emplace_back() member function.
	template <typename... States>
	void emplace_back(std::unique_ptr<ast_node> child, States&&...)
	{
		children.emplace_back(std::move(child));
	}

	using children_t = std::vector<std::unique_ptr<ast_node>>;

	children_t children;

private:
	template <typename U>
	[[nodiscard]] static std::string_view type_name() noexcept
	{
		// This is cached in a static local variable per-specialization, but each module may have
		// its own instance of the specialization and the local variable.
		static const std::string_view name { tao::graphqlpeg::demangle<U>() };

		return name;
	}

	template <typename U>
	[[nodiscard]] static size_t type_hash() noexcept
	{
		// This is cached in a static local variable per-specialization, but each module may have
		// its own instance of the specialization and the local variable.
		static const size_t hash = std::hash<std::string_view>()(type_name<U>());

		return hash;
	}

	std::string_view _source;
	peginternal::iterator _begin;
	std::string_view _type;
	size_t _type_hash = 0;
	std::string_view _content;

	using unescaped_t = std::variant<std::string_view, std::string>;

	std::unique_ptr<unescaped_t> _unescaped;
};

struct ast_input
{
	std::variant<std::vector<char>, std::unique_ptr<file_input<>>, std::string_view> data;
};

} // namespace graphql::peg

#endif // GRAPHQLSYNTAXTREE_H
