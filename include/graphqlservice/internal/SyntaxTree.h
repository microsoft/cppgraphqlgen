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

class [[nodiscard("unnecessary construction")]] ast_node : public parse_tree::basic_node<ast_node>
{
public:
	GRAPHQLPEG_EXPORT void remove_content() noexcept;

	GRAPHQLPEG_EXPORT void unescaped_view(std::string_view unescaped) noexcept;
	[[nodiscard("unnecessary call")]] GRAPHQLPEG_EXPORT std::string_view unescaped_view() const;

	template <typename U>
	[[nodiscard("unnecessary call")]] bool is_type() const noexcept
	{
		const auto u = type_name<U>();

		// The pointer comparison doesn't work with shared libraries where the parse tree is
		// constructed in one module and consumed in another. So to optimize this comparison, check
		// the size first, then the pointer (cached in a static local variable per specialization of
		// type_name<U>()), then the hash (cached in a static local variable per specialization of
		// type_hash<U>()) and a full string compare as fallback.
		return _type_name.size() == u.size()
			&& (_type_name.data() == u.data() || (_type_hash == type_hash<U>() && _type_name == u));
	}

	using basic_node_t = parse_tree::basic_node<ast_node>;

	template <typename Rule, typename ParseInput>
	void success(const ParseInput& in)
	{
		basic_node_t::template success<Rule>(in);
		_type_name = type_name<Rule>();
		_type_hash = type_hash<Rule>();
	}

private:
	template <typename U>
	[[nodiscard("unnecessary call")]] static std::string_view type_name() noexcept
	{
		// This is cached in a static local variable per-specialization, but each module may have
		// its own instance of the specialization and the local variable. Within a single module,
		// the pointer returned from std::string_view::data() should always be equal, which speeds
		// up the string comparison in is_type.
		static const std::string_view name { tao::graphqlpeg::demangle<U>() };

		return name;
	}

	template <typename U>
	[[nodiscard("unnecessary call")]] static size_t type_hash() noexcept
	{
		// This is cached in a static local variable per-specialization, but each module may have
		// its own instance of the specialization and the local variable.
		static const size_t hash = std::hash<std::string_view> {}(type_name<U>());

		return hash;
	}

	std::string_view _type_name;
	size_t _type_hash = 0;

	using unescaped_t = std::variant<std::string_view, std::string>;

	mutable std::unique_ptr<unescaped_t> _unescaped;
};

} // namespace graphql::peg

#endif // GRAPHQLSYNTAXTREE_H
