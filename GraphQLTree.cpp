// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphQLTree.h"
#include "GraphQLGrammar.h"

#include <tao/pegtl/contrib/unescape.hpp>

#include <memory>
#include <stack>
#include <tuple>
#include <functional>
#include <numeric>

namespace facebook {
namespace graphql {
namespace peg {

using namespace tao::graphqlpeg;

template <typename _Rule>
struct ast_selector
	: std::false_type
{
};

template <>
struct ast_selector<operation_type>
	: std::true_type
{
};

template <>
struct ast_selector<list_value>
	: std::true_type
{
};

template <>
struct ast_selector<object_field_name>
	: std::true_type
{
};

template <>
struct ast_selector<object_field>
	: std::true_type
{
};

template <>
struct ast_selector<object_value>
	: std::true_type
{
};

template <>
struct ast_selector<variable_value>
	: std::true_type
{
};

template <>
struct ast_selector<integer_value>
	: std::true_type
{
};

template <>
struct ast_selector<float_value>
	: std::true_type
{
};

template <>
struct ast_selector<escaped_unicode>
	: std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		if (n->has_content())
		{
			std::string content = n->content();

			if (unescape::utf8_append_utf32(n->unescaped, unescape::unhex_string<uint32_t>(content.data() + 1, content.data() + content.size())))
			{
				return;
			}
		}

		throw parse_error("invalid escaped unicode code point", { n->begin(), n->end() });
	}
};

template <>
struct ast_selector<escaped_char>
	: std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		if (n->has_content())
		{
			const char ch = n->content().front();

			switch (ch)
			{
				case '"':
					n->unescaped = "\"";
					return;

				case '\\':
					n->unescaped = "\\";
					return;

				case '/':
					n->unescaped = "/";
					return;

				case 'b':
					n->unescaped = "\b";
					return;

				case 'f':
					n->unescaped = "\f";
					return;

				case 'n':
					n->unescaped = "\n";
					return;

				case 'r':
					n->unescaped = "\r";
					return;

				case 't':
					n->unescaped = "\t";
					return;

				default:
					break;
			}
		}

		throw parse_error("invalid escaped character sequence", { n->begin(), n->end() });
	}
};

template <>
struct ast_selector<string_quote_character>
	: std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		n->unescaped = n->content();
	}
};

template <>
struct ast_selector<block_escape_sequence>
	: std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		n->unescaped = R"bq(""")bq";
	}
};

template <>
struct ast_selector<block_quote_character>
	: std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		n->unescaped = n->content();
	}
};

template <>
struct ast_selector<string_value>
	: std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		n->unescaped.reserve(std::accumulate(n->children.cbegin(), n->children.cend(), size_t(0),
			[](size_t total, const std::unique_ptr<ast_node>& child)
		{
			return total + child->unescaped.size();
		}));

		for (const auto& child : n->children)
		{
			n->unescaped.append(child->unescaped);
		}

		n->remove_content();
		n->children.clear();
	}
};


template <>
struct ast_selector<description>
	: std::true_type
{
};

template <>
struct ast_selector<true_keyword>
	: std::true_type
{
};

template <>
struct ast_selector<false_keyword>
	: std::true_type
{
};

template <>
struct ast_selector<null_keyword>
	: std::true_type
{
};

template <>
struct ast_selector<enum_value>
	: std::true_type
{
};

template <>
struct ast_selector<variable_name>
	: std::true_type
{
};

template <>
struct ast_selector<alias_name>
	: std::true_type
{
};

template <>
struct ast_selector<alias>
	: parse_tree::fold_one
{
};

template <>
struct ast_selector<argument_name>
	: std::true_type
{
};

template <>
struct ast_selector<named_type>
	: std::true_type
{
};

template <>
struct ast_selector<directive_name>
	: std::true_type
{
};

template <>
struct ast_selector<field_name>
	: std::true_type
{
};

template <>
struct ast_selector<operation_name>
	: std::true_type
{
};

template <>
struct ast_selector<fragment_name>
	: std::true_type
{
};

template <>
struct ast_selector<scalar_name>
	: std::true_type
{
};

template <>
struct ast_selector<list_type>
	: std::true_type
{
};

template <>
struct ast_selector<nonnull_type>
	: std::true_type
{
};

template <>
struct ast_selector<default_value>
	: std::true_type
{
};

template <>
struct ast_selector<variable>
	: std::true_type
{
};

template <>
struct ast_selector<object_name>
	: std::true_type
{
};

template <>
struct ast_selector<interface_name>
	: std::true_type
{
};

template <>
struct ast_selector<union_name>
	: std::true_type
{
};

template <>
struct ast_selector<enum_name>
	: std::true_type
{
};

template <>
struct ast_selector<argument>
	: std::true_type
{
};

template <>
struct ast_selector<arguments>
	: std::true_type
{
};

template <>
struct ast_selector<directive>
	: std::true_type
{
};

template <>
struct ast_selector<directives>
	: std::true_type
{
};

template <>
struct ast_selector<field>
	: std::true_type
{
};

template <>
struct ast_selector<fragment_spread>
	: std::true_type
{
};

template <>
struct ast_selector<inline_fragment>
	: std::true_type
{
};

template <>
struct ast_selector<selection_set>
	: std::true_type
{
};

template <>
struct ast_selector<operation_definition>
	: std::true_type
{
};

template <>
struct ast_selector<type_condition>
	: std::true_type
{
};

template <>
struct ast_selector<fragment_definition>
	: std::true_type
{
};

template <>
struct ast_selector<root_operation_definition>
	: std::true_type
{
};

template <>
struct ast_selector<schema_definition>
	: std::true_type
{
};

template <>
struct ast_selector<scalar_type_definition>
	: std::true_type
{
};

template <>
struct ast_selector<interface_type>
	: std::true_type
{
};

template <>
struct ast_selector<input_field_definition>
	: std::true_type
{
};

template <>
struct ast_selector<input_fields_definition>
	: std::true_type
{
};

template <>
struct ast_selector<arguments_definition>
	: std::true_type
{
};

template <>
struct ast_selector<field_definition>
	: std::true_type
{
};

template <>
struct ast_selector<fields_definition>
	: std::true_type
{
};

template <>
struct ast_selector<object_type_definition>
	: std::true_type
{
};

template <>
struct ast_selector<interface_type_definition>
	: std::true_type
{
};

template <>
struct ast_selector<union_type>
	: std::true_type
{
};

template <>
struct ast_selector<union_type_definition>
	: std::true_type
{
};

template <>
struct ast_selector<enum_value_definition>
	: std::true_type
{
};

template <>
struct ast_selector<enum_type_definition>
	: std::true_type
{
};

template <>
struct ast_selector<input_object_type_definition>
	: std::true_type
{
};

template <>
struct ast_selector<directive_location>
	: std::true_type
{
};

template <>
struct ast_selector<directive_definition>
	: std::true_type
{
};

template <>
struct ast_selector<schema_extension>
	: std::true_type
{
};

template <>
struct ast_selector<scalar_type_extension>
	: std::true_type
{
};

template <>
struct ast_selector<object_type_extension>
	: std::true_type
{
};

template <>
struct ast_selector<interface_type_extension>
	: std::true_type
{
};

template <>
struct ast_selector<union_type_extension>
	: std::true_type
{
};

template <>
struct ast_selector<enum_type_extension>
	: std::true_type
{
};

template <>
struct ast_selector<input_object_type_extension>
	: std::true_type
{
};

template <>
struct ast_selector<document>
	: std::true_type
{
};

std::unique_ptr<ast_node> parseString(const char* text)
{
	return parse_tree::parse<document, ast_node, ast_selector>(memory_input<>(text, "GraphQL"));
}

std::unique_ptr<ast_node> parseFile(file_input<>&& in)
{
	return parse_tree::parse<document, ast_node, ast_selector>(std::move(in));
}

} /* namespace peg */

std::unique_ptr<peg::ast_node> operator "" _graphql(const char* text, size_t size)
{
	return tao::graphqlpeg::parse_tree::parse<peg::document, peg::ast_node, peg::ast_selector>(tao::graphqlpeg::memory_input<>(text, size, "GraphQL"));
}

} /* namespace graphql */
} /* namespace facebook */