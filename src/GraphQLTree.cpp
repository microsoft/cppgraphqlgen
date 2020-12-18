// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLTree.h"
#include "graphqlservice/GraphQLGrammar.h"
#include "graphqlservice/GraphQLParse.h"

#include <tao/pegtl/contrib/unescape.hpp>

#include <functional>
#include <memory>
#include <numeric>
#include <tuple>

using namespace std::literals;

namespace graphql {
namespace peg {

std::string_view ast_node::unescaped_view() const
{
	if (std::holds_alternative<std::uint16_t>(unescaped))
	{
		// The whole string_value was a single unicode character.
		std::string utf8;

		if (unescape::utf8_append_utf32(utf8, std::get<std::uint16_t>(unescaped)))
		{
			const_cast<ast_node*>(this)->unescaped = std::move(utf8);
		}
		else
		{
			throw parse_error("invalid escaped unicode code point", this->begin());
		}
	}
	else if (std::holds_alternative<std::list<string_or_utf16>>(unescaped))
	{
		// First convert all of the consecutive unicode sequences to UTF-8 strings together.
		auto& values = std::get<std::list<string_or_utf16>>(const_cast<ast_node*>(this)->unescaped);
		const auto isUtf16 = [](const string_or_utf16& value) noexcept {
			return std::holds_alternative<std::uint16_t>(value);
		};
		auto itrStart = std::find_if(values.begin(), values.end(), isUtf16);
		auto itrEnd = std::find_if_not(itrStart, values.end(), isUtf16);
		std::list<std::string> utf8;

		if (itrStart != itrEnd)
		{
			while (itrStart != itrEnd)
			{
				std::string unescaped;

				// Translate surrogate pairs (based on unescape::unescape_j from PEGTL)
				for (auto itr = itrStart; itr != itrEnd; ++itr)
				{
					const auto c = std::get<std::uint16_t>(*itr);

					if ((0xd800 <= c) && (c <= 0xdbff) && ++itr != itrEnd)
					{
						const auto d = std::get<std::uint16_t>(*itr);

						if ((0xdc00 <= d) && (d <= 0xdfff))
						{
							(void)unescape::utf8_append_utf32(unescaped,
								(((c & 0x03ff) << 10) | (d & 0x03ff)) + 0x10000);
							continue;
						}
					}

					if (!unescape::utf8_append_utf32(unescaped, c))
					{
						throw parse_error("invalid escaped unicode code point", this->begin());
					}
				}

				utf8.push_back(std::move(unescaped));

				values.erase(itrStart, itrEnd);
				values.insert(itrEnd, std::string_view { utf8.back() });

				itrStart = std::find_if(itrEnd, values.end(), isUtf16);
				itrEnd = std::find_if_not(itrStart, values.end(), isUtf16);
			}
		}

		// If the string_value had multiple unescaped sub-strings, concatenate them on
		// demand and store the result as a std::string.
		std::string joined;

		joined.reserve(std::accumulate(values.cbegin(),
			values.cend(),
			size_t(0),
			[](size_t total, const auto& child) {
				return total + std::get<std::string_view>(child).size();
			}));

		for (const auto& child : values)
		{
			joined.append(std::get<std::string_view>(child));
		}

		const_cast<ast_node*>(this)->unescaped = std::move(joined);
	}

	// By this point it should always be a std::string_view or a std::string.
	if (std::holds_alternative<std::string_view>(unescaped))
	{
		return std::get<std::string_view>(unescaped);
	}
	else if (std::holds_alternative<std::string>(unescaped))
	{
		return std::get<std::string>(unescaped);
	}

	throw parse_error("unexpected sub-string", this->begin());
}

using namespace tao::graphqlpeg;

template <typename Rule>
struct ast_selector : std::false_type
{
};

template <>
struct ast_selector<operation_type> : std::true_type
{
};

template <>
struct ast_selector<list_value> : std::true_type
{
};

template <>
struct ast_selector<object_field_name> : std::true_type
{
};

template <>
struct ast_selector<object_field> : std::true_type
{
};

template <>
struct ast_selector<object_value> : std::true_type
{
};

template <>
struct ast_selector<variable_value> : std::true_type
{
};

template <>
struct ast_selector<integer_value> : std::true_type
{
};

template <>
struct ast_selector<float_value> : std::true_type
{
};

template <>
struct ast_selector<escaped_unicode> : std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		if (n->has_content())
		{
			auto content = n->string_view();

			n->unescaped = unescape::unhex_string<uint16_t>(content.data() + 1,
				content.data() + content.size());

			return;
		}

		throw parse_error("invalid escaped unicode code point", n->begin());
	}
};

template <>
struct ast_selector<escaped_char> : std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		if (n->has_content())
		{
			const char ch = n->string_view().front();

			switch (ch)
			{
				case '"':
					n->unescaped = "\""sv;
					return;

				case '\\':
					n->unescaped = "\\"sv;
					return;

				case '/':
					n->unescaped = "/"sv;
					return;

				case 'b':
					n->unescaped = "\b"sv;
					return;

				case 'f':
					n->unescaped = "\f"sv;
					return;

				case 'n':
					n->unescaped = "\n"sv;
					return;

				case 'r':
					n->unescaped = "\r"sv;
					return;

				case 't':
					n->unescaped = "\t"sv;
					return;

				default:
					break;
			}
		}

		throw parse_error("invalid escaped character sequence", n->begin());
	}
};

template <>
struct ast_selector<string_quote_character> : std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		n->unescaped = n->string_view();
	}
};

template <>
struct ast_selector<block_escape_sequence> : std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		n->unescaped = R"bq(""")bq"sv;
	}
};

template <>
struct ast_selector<block_quote_character> : std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		n->unescaped = n->string_view();
	}
};

template <>
struct ast_selector<string_value> : std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		if (!n->children.empty())
		{
			if (n->children.size() > 1)
			{
				std::list<ast_node::string_or_utf16> unescaped;

				std::transform(n->children.cbegin(),
					n->children.cend(),
					std::back_inserter(unescaped),
					[](const auto& child) -> ast_node::string_or_utf16 {
						if (std::holds_alternative<std::uint16_t>(child->unescaped))
						{
							return { std::get<std::uint16_t>(child->unescaped) };
						}

						return { child->unescaped_view() };
					});

				n->unescaped = std::move(unescaped);
			}
			else
			{
				n->unescaped = std::move(n->children.front()->unescaped);
			}
		}
	}
};

template <>
struct ast_selector<description> : std::true_type
{
};

template <>
struct ast_selector<true_keyword> : std::true_type
{
};

template <>
struct ast_selector<false_keyword> : std::true_type
{
};

template <>
struct ast_selector<null_keyword> : std::true_type
{
};

template <>
struct ast_selector<enum_value> : std::true_type
{
};

template <>
struct ast_selector<variable_name> : std::true_type
{
};

template <>
struct ast_selector<alias_name> : std::true_type
{
};

template <>
struct ast_selector<alias> : parse_tree::fold_one
{
};

template <>
struct ast_selector<argument_name> : std::true_type
{
};

template <>
struct ast_selector<named_type> : std::true_type
{
};

template <>
struct ast_selector<directive_name> : std::true_type
{
};

template <>
struct ast_selector<field_name> : std::true_type
{
};

template <>
struct ast_selector<operation_name> : std::true_type
{
};

template <>
struct ast_selector<fragment_name> : std::true_type
{
};

template <>
struct ast_selector<scalar_name> : std::true_type
{
};

template <>
struct ast_selector<list_type> : std::true_type
{
};

template <>
struct ast_selector<nonnull_type> : std::true_type
{
};

template <>
struct ast_selector<default_value> : std::true_type
{
};

template <>
struct ast_selector<variable> : std::true_type
{
};

template <>
struct ast_selector<object_name> : std::true_type
{
};

template <>
struct ast_selector<interface_name> : std::true_type
{
};

template <>
struct ast_selector<union_name> : std::true_type
{
};

template <>
struct ast_selector<enum_name> : std::true_type
{
};

template <>
struct ast_selector<argument> : std::true_type
{
};

template <>
struct ast_selector<arguments> : std::true_type
{
};

template <>
struct ast_selector<directive> : std::true_type
{
};

template <>
struct ast_selector<directives> : std::true_type
{
};

template <>
struct ast_selector<field> : std::true_type
{
};

template <>
struct ast_selector<fragment_spread> : std::true_type
{
};

template <>
struct ast_selector<inline_fragment> : std::true_type
{
};

template <>
struct ast_selector<selection_set> : std::true_type
{
};

template <>
struct ast_selector<operation_definition> : std::true_type
{
};

template <>
struct ast_selector<type_condition> : std::true_type
{
};

template <>
struct ast_selector<fragment_definition> : std::true_type
{
};

template <>
struct ast_selector<root_operation_definition> : std::true_type
{
};

template <>
struct ast_selector<schema_definition> : std::true_type
{
};

template <>
struct ast_selector<scalar_type_definition> : std::true_type
{
};

template <>
struct ast_selector<interface_type> : std::true_type
{
};

template <>
struct ast_selector<input_field_definition> : std::true_type
{
};

template <>
struct ast_selector<input_fields_definition> : std::true_type
{
};

template <>
struct ast_selector<arguments_definition> : std::true_type
{
};

template <>
struct ast_selector<field_definition> : std::true_type
{
};

template <>
struct ast_selector<fields_definition> : std::true_type
{
};

template <>
struct ast_selector<object_type_definition> : std::true_type
{
};

template <>
struct ast_selector<interface_type_definition> : std::true_type
{
};

template <>
struct ast_selector<union_type> : std::true_type
{
};

template <>
struct ast_selector<union_type_definition> : std::true_type
{
};

template <>
struct ast_selector<enum_value_definition> : std::true_type
{
};

template <>
struct ast_selector<enum_type_definition> : std::true_type
{
};

template <>
struct ast_selector<input_object_type_definition> : std::true_type
{
};

template <>
struct ast_selector<directive_location> : std::true_type
{
};

template <>
struct ast_selector<directive_definition> : std::true_type
{
};

template <>
struct ast_selector<schema_extension> : std::true_type
{
};

template <>
struct ast_selector<operation_type_definition> : std::true_type
{
};

template <>
struct ast_selector<scalar_type_extension> : std::true_type
{
};

template <>
struct ast_selector<object_type_extension> : std::true_type
{
};

template <>
struct ast_selector<interface_type_extension> : std::true_type
{
};

template <>
struct ast_selector<union_type_extension> : std::true_type
{
};

template <>
struct ast_selector<enum_type_extension> : std::true_type
{
};

template <>
struct ast_selector<input_object_type_extension> : std::true_type
{
};

template <typename Rule>
struct ast_control : normal<Rule>
{
	static const std::string error_message;

	template <typename Input, typename... State>
	[[noreturn]] static void raise(const Input& in, State&&...)
	{
		throw parse_error(error_message, in);
	}
};

template <>
const std::string ast_control<one<'}'>>::error_message = "Expected }";
template <>
const std::string ast_control<one<']'>>::error_message = "Expected ]";
template <>
const std::string ast_control<one<')'>>::error_message = "Expected )";
template <>
const std::string ast_control<quote_token>::error_message = "Expected \"";
template <>
const std::string ast_control<block_quote_token>::error_message = "Expected \"\"\"";

template <>
const std::string ast_control<variable_name_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#Variable";
template <>
const std::string ast_control<escaped_unicode_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#EscapedUnicode";
template <>
const std::string ast_control<string_escape_sequence_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#EscapedCharacter";
template <>
const std::string ast_control<string_quote_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#StringCharacter";
template <>
const std::string ast_control<block_quote_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#BlockStringCharacter";
template <>
const std::string ast_control<fractional_part_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#FractionalPart";
template <>
const std::string ast_control<exponent_part_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ExponentPart";
template <>
const std::string ast_control<argument_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#Argument";
template <>
const std::string ast_control<arguments_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#Arguments";
template <>
const std::string ast_control<list_value_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ListValue";
template <>
const std::string ast_control<object_field_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ObjectField";
template <>
const std::string ast_control<object_value_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ObjectValue";
template <>
const std::string ast_control<input_value_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#Value";
template <>
const std::string ast_control<default_value_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#DefaultValue";
template <>
const std::string ast_control<list_type_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ListType";
template <>
const std::string ast_control<type_name_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#Type";
template <>
const std::string ast_control<variable_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#VariableDefinition";
template <>
const std::string ast_control<variable_definitions_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#VariableDefinitions";
template <>
const std::string ast_control<directive_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#Directive";
template <>
const std::string ast_control<field_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#Field";
template <>
const std::string ast_control<type_condition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#TypeCondition";
template <>
const std::string ast_control<fragement_spread_or_inline_fragment_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#FragmentSpread or "
	"http://spec.graphql.org/June2018/#InlineFragment";
template <>
const std::string ast_control<selection_set_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#SelectionSet";
template <>
const std::string ast_control<operation_definition_operation_type_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#OperationDefinition";
template <>
const std::string ast_control<fragment_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#FragmentDefinition";
template <>
const std::string ast_control<root_operation_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#RootOperationTypeDefinition";
template <>
const std::string ast_control<schema_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#SchemaDefinition";
template <>
const std::string ast_control<scalar_type_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ScalarTypeDefinition";
template <>
const std::string ast_control<arguments_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ArgumentsDefinition";
template <>
const std::string ast_control<field_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#FieldDefinition";
template <>
const std::string ast_control<fields_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#FieldsDefinition";
template <>
const std::string ast_control<implements_interfaces_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ImplementsInterfaces";
template <>
const std::string ast_control<object_type_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ObjectTypeDefinition";
template <>
const std::string ast_control<interface_type_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#InterfaceTypeDefinition";
template <>
const std::string ast_control<union_member_types_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#UnionMemberTypes";
template <>
const std::string ast_control<union_type_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#UnionTypeDefinition";
template <>
const std::string ast_control<enum_value_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#EnumValueDefinition";
template <>
const std::string ast_control<enum_values_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#EnumValuesDefinition";
template <>
const std::string ast_control<enum_type_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#EnumTypeDefinition";
template <>
const std::string ast_control<input_field_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#InputValueDefinition";
template <>
const std::string ast_control<input_fields_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#InputFieldsDefinition";
template <>
const std::string ast_control<input_object_type_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#InputObjectTypeDefinition";
template <>
const std::string ast_control<directive_definition_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#DirectiveDefinition";
template <>
const std::string ast_control<schema_extension_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#SchemaExtension";
template <>
const std::string ast_control<scalar_type_extension_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ScalarTypeExtension";
template <>
const std::string ast_control<object_type_extension_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#ObjectTypeExtension";
template <>
const std::string ast_control<interface_type_extension_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#InterfaceTypeExtension";
template <>
const std::string ast_control<union_type_extension_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#UnionTypeExtension";
template <>
const std::string ast_control<enum_type_extension_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#EnumTypeExtension";
template <>
const std::string ast_control<input_object_type_extension_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#InputObjectTypeExtension";
template <>
const std::string ast_control<document_content>::error_message =
	"Expected http://spec.graphql.org/June2018/#Document";

ast parseString(std::string_view input)
{
	ast result { std::make_shared<ast_input>(
					 ast_input { std::vector<char> { input.cbegin(), input.cend() } }),
		{} };
	const auto& data = std::get<std::vector<char>>(result.input->data);
	memory_input<> in(data.data(), data.size(), "GraphQL");

	result.root =
		parse_tree::parse<document, ast_node, ast_selector, nothing, ast_control>(std::move(in));

	return result;
}

ast parseFile(std::string_view filename)
{
	ast result { std::make_shared<ast_input>(
					 ast_input { std::make_unique<file_input<>>(filename) }),
		{} };
	auto& in = *std::get<std::unique_ptr<file_input<>>>(result.input->data);

	result.root =
		parse_tree::parse<document, ast_node, ast_selector, nothing, ast_control>(std::move(in));

	return result;
}

} /* namespace peg */

peg::ast operator"" _graphql(const char* text, size_t size)
{
	peg::memory_input<> in(text, size, "GraphQL");

	return { std::make_shared<peg::ast_input>(
				 peg::ast_input { { std::string_view { text, size } } }),
		peg::parse_tree::
			parse<peg::document, peg::ast_node, peg::ast_selector, peg::nothing, peg::ast_control>(
				std::move(in)) };
}

} /* namespace graphql */
