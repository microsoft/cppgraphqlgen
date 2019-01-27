// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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
		if (!n->children.empty())
		{
			if (n->children.size() > 1)
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
			}
			else
			{
				n->unescaped = std::move(n->children.front()->unescaped);
			}
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

template <typename _Rule>
struct ast_control
	: normal<_Rule>
{
	static const std::string error_message;

	template <typename _Input, typename... _States>
	static void raise(const _Input& in, _States&&...)
	{
		throw parse_error(error_message, in);
	}
};

template <> const std::string ast_control<bof>::error_message = "Expected beginning of file";
template <> const std::string ast_control<opt<utf8::bom>>::error_message = "Expected optional UTF8 byte-order-mark";
template <> const std::string ast_control<star<ignored>>::error_message = "Expected optional ignored characters";
template <> const std::string ast_control<plus<ignored>>::error_message = "Expected ignored characters";
template <> const std::string ast_control<until<ascii::eolf>>::error_message = "Expected Comment";
template <> const std::string ast_control<eof>::error_message = "Expected end of file";
template <> const std::string ast_control<list<definition, plus<ignored>>>::error_message = "Expected list of Definitions";
template <> const std::string ast_control<selection_set>::error_message = "Expected SelectionSet";
template <> const std::string ast_control<fragment_name>::error_message = "Expected FragmentName";
template <> const std::string ast_control<type_condition>::error_message = "Expected TypeCondition";
template <> const std::string ast_control<opt<seq<star<ignored>, directives>>>::error_message = "Expected optional Directives";
template <> const std::string ast_control<one<'{'>>::error_message = "Expected {";
template <> const std::string ast_control<list<root_operation_definition, plus<ignored>>>::error_message = "Expected RootOperationTypeDefinition";
template <> const std::string ast_control<one<'}'>>::error_message = "Expected }";
template <> const std::string ast_control<one<'@'>>::error_message = "Expected @";
template <> const std::string ast_control<directive_name>::error_message = "Expected Directive Name";
template <> const std::string ast_control<arguments_definition>::error_message = "Expected ArgumentsDefinition";
template <> const std::string ast_control<on_keyword>::error_message = "Expected \"on\" keyword";
template <> const std::string ast_control<directive_locations>::error_message = "Expected DirectiveLocations";
template <> const std::string ast_control<schema_extension>::error_message = "Expected SchemaExtension";
template <> const std::string ast_control<
	seq<opt<seq<plus<ignored>, operation_name>>, opt<seq<star<ignored>, variable_definitions>>, opt<seq<star<ignored>, directives>>, star<ignored>, selection_set>>::error_message = "Expected OperationDefinition";
template <> const std::string ast_control<list<selection, plus<ignored>>>::error_message = "Expected Selections";
template <> const std::string ast_control<scalar_name>::error_message = "Expected ScalarType Name";
template <> const std::string ast_control<object_name>::error_message = "Expected ObjectType Name";
template <> const std::string ast_control<
	sor<seq<opt<seq<plus<ignored>, implements_interfaces>>, opt<seq<star<ignored>, directives>>, seq<star<ignored>, fields_definition>>
	, seq<opt<seq<plus<ignored>, implements_interfaces>>, seq<star<ignored>, directives>>
	, opt<seq<plus<ignored>, implements_interfaces>>>>::error_message = "Expected ObjectTypeDefinition";
template <> const std::string ast_control<interface_name>::error_message = "Expected InterfaceType Name";
template <> const std::string ast_control<
	sor<seq<opt<seq<star<ignored>, directives>>, seq<star<ignored>, fields_definition>>
	, opt<seq<star<ignored>, directives>>>>::error_message = "Expected InterfaceTypeDefinition";
template <> const std::string ast_control<
	sor<seq<opt<directives>, one<'{'>, star<ignored>, list<operation_type_definition, plus<ignored>>, star<ignored>, one<'}'>>
	, directives>>::error_message = "Expected SchemaExtension";
template <> const std::string ast_control<union_name>::error_message = "Expected UnionType Name";
template <> const std::string ast_control<
	sor<seq<opt<seq<star<ignored>, directives>>, seq<star<ignored>, union_member_types>>
	, opt<seq<star<ignored>, directives>>>>::error_message = "Expected UnionTypeDefinition";
template <> const std::string ast_control<enum_name>::error_message = "Expected EnumType Name";
template <> const std::string ast_control<
	sor<seq<opt<seq<star<ignored>, directives>>, seq<star<ignored>, enum_values_definition>>
	, opt<seq<star<ignored>, directives>>>>::error_message = "Expected EnumTypeDefinition";
template <> const std::string ast_control<
	sor<seq<opt<seq<star<ignored>, directives>>, seq<star<ignored>, input_fields_definition>>
	, opt<seq<star<ignored>, directives>>>>::error_message = "Expected InputObjectTypeDefinition";
template <> const std::string ast_control<directives>::error_message = "Expected list of Directives";
template <> const std::string ast_control<
	sor<seq<opt<seq<plus<ignored>, implements_interfaces>>, opt<seq<star<ignored>, directives>>, star<ignored>, fields_definition>
	, seq<opt<seq<plus<ignored>, implements_interfaces>>, star<ignored>, directives>
	, seq<plus<ignored>, implements_interfaces>>>::error_message = "Expected ObjectTypeExtension";
template <> const std::string ast_control<
	sor<seq<opt<seq<directives, star<ignored>>>, fields_definition>
	, directives>>::error_message = "Expected InterfaceTypeExtension";
template <> const std::string ast_control<
	sor<seq<opt<seq<directives, star<ignored>>>, union_member_types>
	, directives>>::error_message = "Expected UnionTypeExtension";
template <> const std::string ast_control<
	sor<seq<opt<seq<directives, star<ignored>>>, enum_values_definition>
	, directives>>::error_message = "Expected EnumTypeExtension";
template <> const std::string ast_control<
	sor<seq<opt<seq<directives, star<ignored>>>, input_fields_definition>
	, directives>>::error_message = "Expected InputObjectTypeExtension";
template <> const std::string ast_control<name>::error_message = "Expected Name";
template <> const std::string ast_control<named_type>::error_message = "Expected NamedType";
template <> const std::string ast_control<list<input_field_definition, plus<ignored>>>::error_message = "Expected InputValueDefinitions";
template <> const std::string ast_control<one<')'>>::error_message = "Expected )";
template <> const std::string ast_control<one<':'>>::error_message = "Expected :";
template <> const std::string ast_control<
	sor<executable_directive_location
	, type_system_directive_location>>::error_message = "Expected DirectiveLocation";
template <> const std::string ast_control<opt<seq<star<ignored>, arguments>>>::error_message = "Expected optional Arguments";
template <> const std::string ast_control<block_quote_token>::error_message = "Expected \"\"\"";
template <> const std::string ast_control<quote_token>::error_message = "Expected \"";
template <> const std::string ast_control<
	sor<seq<opt<seq<star<ignored>, arguments>>, opt<seq<star<ignored>, directives>>, seq<star<ignored>, selection_set>>
	, seq<opt<seq<star<ignored>, arguments>>, seq<star<ignored>, directives>>
	, opt<seq<star<ignored>, arguments>>>>::error_message = "Expected Field";
template <> const std::string ast_control<type_name>::error_message = "Expected Type";
template <> const std::string ast_control<
	sor<seq<opt<seq<star<ignored>, default_value>>, seq<star<ignored>, directives>>
	, opt<seq<star<ignored>, default_value>>>>::error_message = "Expected InputValueDefinition";
template <> const std::string ast_control<list<field_definition, plus<ignored>>>::error_message = "Expected FieldsDefinitions";
template <> const std::string ast_control<opt<seq<star<ignored>, one<'&'>>>>::error_message = "Expected optional '&'";
template <> const std::string ast_control<list<interface_type, seq<star<ignored>, one<'&'>, star<ignored>>>>::error_message = "Expected ImplementsInterfaces";
template <> const std::string ast_control<opt<seq<star<ignored>, one<'|'>>>>::error_message = "Expected optional '|'";
template <> const std::string ast_control<list<union_type, seq<star<ignored>, one<'|'>, star<ignored>>>>::error_message = "Expected UnionMemberTypes";
template <> const std::string ast_control<list<enum_value_definition, plus<ignored>>>::error_message = "Expected EnumValuesDefinition";
template <> const std::string ast_control<star<sor<block_escape_sequence, block_quote_character>>>::error_message = "Expected optional BlockStringCharacters";
template <> const std::string ast_control<star<sor<string_escape_sequence, string_quote_character>>>::error_message = "Expected optional StringCharacters";
template <> const std::string ast_control<sor<nonnull_type, list_type, named_type>>::error_message = "Expected Type";
template <> const std::string ast_control<opt<seq<star<ignored>, default_value>>>::error_message = "Expected optional DefaultValue";
template <> const std::string ast_control<list<variable, plus<ignored>>>::error_message = "Expected VariableDefinitions";
template <> const std::string ast_control<opt<seq<star<ignored>, arguments_definition>>>::error_message = "Expected optional ArgumentsDefinition";
template <> const std::string ast_control<list<argument, plus<ignored>>>::error_message = "Expected Arguments";
template <> const std::string ast_control<one<']'>>::error_message = "Expected ]";
template <> const std::string ast_control<sor<escaped_unicode, escaped_char>>::error_message = "Expected EscapeSequence";
template <> const std::string ast_control<input_value>::error_message = "Expected Value";
template <> const std::string ast_control<
	sor<list_value
	, object_value
	, variable_value
	, integer_value
	, float_value
	, string_value
	, bool_value
	, null_keyword
	, enum_value>>::error_message = "Expected Value";
template <> const std::string ast_control<rep<4, xdigit>>::error_message = "Expected EscapedUnicode";
template <> const std::string ast_control<opt<list<list_entry, plus<ignored>>>>::error_message = "Expected optional ListValues";
template <> const std::string ast_control<opt<list<object_field, plus<ignored>>>>::error_message = "Expected optional ObjectValues";
template <> const std::string ast_control<plus<digit>>::error_message = "Expected Digits";
template <> const std::string ast_control<opt<sign>>::error_message = "Expected optional Sign";

std::unique_ptr<ast<std::string>> parseString(std::string&& input)
{
	std::unique_ptr<ast<std::string>> result(new ast<std::string> { std::move(input), nullptr });
	memory_input<> in(result->input.c_str(), result->input.size(), "GraphQL");

	result->root = parse_tree::parse<document, ast_node, ast_selector>(std::move(in));

	return result;
}

std::unique_ptr<ast<std::unique_ptr<file_input<>>>> parseFile(const char* filename)
{
	std::unique_ptr<ast<std::unique_ptr<file_input<>>>> result(new ast<std::unique_ptr<file_input<>>> {
		std::unique_ptr<file_input<>>(new file_input<>(std::string(filename))),
		nullptr
		});

	result->root = parse_tree::parse<document, ast_node, ast_selector>(std::move(*result->input));

	return result;
}

} /* namespace peg */

std::unique_ptr<peg::ast<const char*>> operator "" _graphql(const char* text, size_t size)
{
	std::unique_ptr<peg::ast<const char*>> result(new peg::ast<const char*> { text, nullptr });
	peg::memory_input<> in(text, size, "GraphQL");

	result->root = peg::parse_tree::parse<peg::document, peg::ast_node, peg::ast_selector>(std::move(in));

	return result;
}

} /* namespace graphql */
} /* namespace facebook */
