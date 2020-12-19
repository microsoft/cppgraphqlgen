// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLTREE_H
#define GRAPHQLTREE_H

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

struct ast_node
{
	// Requirements as per
	// https://github.com/taocpp/PEGTL/blob/master/doc/Parse-Tree.md#custom-node-class
	ast_node() = default;
	ast_node(const ast_node&) = delete;
	ast_node(ast_node&&) = delete;
	~ast_node() = default;
	ast_node& operator=(const ast_node&) = delete;
	ast_node& operator=(ast_node&&) = delete;

	using children_t = std::vector<std::unique_ptr<ast_node>>;
	children_t children;
	std::string_view type;
	std::string_view m_string_view;
	std::string_view unescaped;
	internal::iterator m_begin;
	std::string_view source;

	[[nodiscard]] bool is_root() const noexcept
	{
		return type.empty();
	}

	template <typename U>
	[[nodiscard]] bool is_type() const noexcept
	{
		const auto u = tao::demangle<U>();
		return ((type.data() == u.data()) && (type.size() == u.size())) || (type == u);
	}

	template <typename U>
	void set_type() noexcept
	{
		type = tao::demangle<U>();
	}

	[[nodiscard]] position begin() const
	{
		return position(m_begin, source);
	}

	[[nodiscard]] bool has_content() const noexcept
	{
		return m_string_view.size() > 0;
	}

	[[nodiscard]] std::string_view string_view() const noexcept
	{
		return m_string_view;
	}

	[[nodiscard]] std::string_view unescaped_view() const noexcept
	{
		return unescaped;
	}

	[[nodiscard]] std::string string() const
	{
		return std::string(m_string_view);
	}

	template <typename... States>
	void remove_content(States&&... /*unused*/) noexcept
	{
		m_string_view = {};
		unescaped = {};
	}

	// all non-root nodes are initialized by calling this method
	template <typename Rule, typename ParseInput, typename... States>
	void start(const ParseInput& in, States&&... /*unused*/)
	{
		set_type<Rule>();
		source = in.source();
		m_begin = internal::iterator(in.iterator());
		m_string_view = {};
		unescaped = {};
	}

	// if parsing of the rule succeeded, this method is called
	template <typename Rule, typename ParseInput, typename... States>
	void success(const ParseInput& in, States&&... /*unused*/) noexcept
	{
		const auto& end = in.iterator();
		m_string_view = std::string_view(m_begin.data, end.data - m_begin.data);
		unescaped = m_string_view;
	}

	// if parsing of the rule failed, this method is called
	template <typename Rule, typename ParseInput, typename... States>
	void failure(const ParseInput& /*unused*/, States&&... /*unused*/) noexcept
	{
	}

	// if parsing of the rule failed with an exception, this method is called
	template <typename Rule, typename ParseInput, typename... States>
	void unwind(const ParseInput& /*unused*/, States&&... /*unused*/) noexcept
	{
	}

	// if parsing succeeded and the (optional) transform call
	// did not discard the node, it is appended to its parent.
	// note that "child" is the node whose Rule just succeeded
	// and "*this" is the parent where the node should be appended.
	template <typename... States>
	void emplace_back(std::unique_ptr<ast_node>&& child, States&&... /*unused*/)
	{
		assert(child);
		children.emplace_back(std::move(child));
	}
};

struct ast_unescaped_string : ast_node
{
	ast_unescaped_string() = default;
	ast_unescaped_string(std::unique_ptr<ast_node>&& reference, std::string&& unescaped);

protected:
	std::string _unescaped; // storage to keep string_view alive
};

struct ast_input
{
	std::variant<std::vector<char>, std::unique_ptr<file_input<>>, std::string_view> data;
};

} /* namespace graphql::peg */

#endif // GRAPHQLTREE_H
