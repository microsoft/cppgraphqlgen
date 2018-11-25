// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// This grammar is based on the June 2018 Edition of the GraphQL spec:
// https://facebook.github.io/graphql/June2018/

#pragma once

#include <tao/pegtl.hpp>

namespace facebook {
namespace graphql {
namespace peg {

using namespace tao::graphqlpeg;

template <typename _Rule>
void for_each_child(const ast_node& n, std::function<bool(const ast_node&)>&& func)
{
	for (const auto& child : n.children)
	{
		if (child->is<_Rule>()
			&& !func(*child))
		{
			return;
		}
	}
}

// https://facebook.github.io/graphql/June2018/#sec-Source-Text
struct source_character
	: sor<one<0x0009, 0x000A, 0x000D>
	, utf8::range<0x0020, 0xFFFF>>
{
};

// https://facebook.github.io/graphql/June2018/#sec-Comments
struct comment
	: if_must<one<'#'>, until<eolf>>
{
};

// https://facebook.github.io/graphql/June2018/#sec-Source-Text.Ignored-Tokens
struct ignored
	: sor<space
	, one<','>
	, comment>
{
};

// https://facebook.github.io/graphql/June2018/#sec-Names
struct name
	: seq<sor<alpha, one<'_'>>, star<sor<alnum, one<'_'>>>>
{
};

// https://facebook.github.io/graphql/June2018/#Variable
struct variable_name
	: if_must<one<'$'>, name>
{
};

// https://facebook.github.io/graphql/June2018/#sec-Null-Value
struct null_keyword
	: TAO_PEGTL_KEYWORD("null")
{
};

struct quote_token
	: one<'"'>
{
};

struct backslash_token
	: one<'\\'>
{
};

// https://facebook.github.io/graphql/June2018/#EscapedUnicode
struct escaped_unicode
	: if_must<one<'u'>, rep<4, xdigit>>
{
};

// https://facebook.github.io/graphql/June2018/#EscapedCharacter
struct escaped_char
	: one<'"', '\\', '/', 'b', 'f', 'n', 'r', 't'>
{
};

struct string_escape_sequence
	: if_must<backslash_token, sor<escaped_unicode, escaped_char>>
{
};

struct string_quote_character
	: plus<seq<not_at<backslash_token>, not_at<quote_token>, not_at<ascii::eol>, source_character>>
{
};

// https://facebook.github.io/graphql/June2018/#StringCharacter
struct string_quote
	: if_must<quote_token, star<sor<string_escape_sequence, string_quote_character>>, quote_token>
{
};

struct block_quote_token
	: rep<3, quote_token>
{
};

struct block_escape_sequence
	: seq<backslash_token, block_quote_token>
{
};

struct block_quote_character
	: plus<seq<not_at<block_quote_token>, not_at<block_escape_sequence>, source_character>>
{
};

// https://facebook.github.io/graphql/June2018/#BlockStringCharacter
struct block_quote
	: if_must<block_quote_token, star<sor<block_escape_sequence, block_quote_character>>, block_quote_token>
{
};

// https://facebook.github.io/graphql/June2018/#StringValue
struct string_value
	: sor<block_quote
	, string_quote>
{
};

// https://facebook.github.io/graphql/June2018/#NonZeroDigit
struct nonzero_digit
	: range<'1', '9'>
{
};

struct zero_digit
	: one<'0'>
{
};

// https://facebook.github.io/graphql/June2018/#NegativeSign
struct negative_sign
	: one<'-'>
{
};

// https://facebook.github.io/graphql/June2018/#IntegerPart
struct integer_part
	: seq<opt<negative_sign>, sor<zero_digit, seq<nonzero_digit, star<digit>>>>
{
};

// https://facebook.github.io/graphql/June2018/#IntValue
struct integer_value
	: integer_part
{
};

// https://facebook.github.io/graphql/June2018/#FractionalPart
struct fractional_part
	: if_must<one<'.'>, plus<digit>>
{
};

// https://facebook.github.io/graphql/June2018/#ExponentIndicator
struct exponent_indicator
	: one<'e', 'E'>
{
};

// https://facebook.github.io/graphql/June2018/#Sign
struct sign
	: one<'+', '-'>
{
};

// https://facebook.github.io/graphql/June2018/#ExponentPart
struct exponent_part
	: if_must<exponent_indicator, opt<sign>, plus<digit>>
{
};

// https://facebook.github.io/graphql/June2018/#FloatValue
struct float_value
	: seq<integer_part, sor<fractional_part, exponent_part, seq<fractional_part, exponent_part>>>
{
};

struct true_keyword
	: TAO_PEGTL_KEYWORD("true")
{
};

struct false_keyword
	: TAO_PEGTL_KEYWORD("false")
{
};

// https://facebook.github.io/graphql/June2018/#BooleanValue
struct bool_value
	: sor<true_keyword
	, false_keyword>
{
};

// https://facebook.github.io/graphql/June2018/#EnumValue
struct enum_value
	: seq<not_at<true_keyword, false_keyword, null_keyword>, name>
{
};

// https://facebook.github.io/graphql/June2018/#OperationType
struct operation_type
	: sor<TAO_PEGTL_KEYWORD("query")
	, TAO_PEGTL_KEYWORD("mutation")
	, TAO_PEGTL_KEYWORD("subscription")>
{
};

struct alias_name
	: name
{
};

// https://facebook.github.io/graphql/June2018/#Alias
struct alias
	: seq<alias_name, star<ignored>, one<':'>>
{
};

struct argument_name
	: name
{
};

struct input_value;

// https://facebook.github.io/graphql/June2018/#Argument
struct argument
	: if_must<argument_name, star<ignored>, one<':'>, star<ignored>, input_value>
{
};

// https://facebook.github.io/graphql/June2018/#Arguments
struct arguments
	: if_must<one<'('>, star<ignored>, list<argument, plus<ignored>>, star<ignored>, one<')'>>
{
};

struct list_entry;

// https://facebook.github.io/graphql/June2018/#ListValue
struct list_value
	: if_must<one<'['>, star<ignored>, opt<list<list_entry, plus<ignored>>>, star<ignored>, one<']'>>
{
};

struct object_field_name
	: name
{
};

// https://facebook.github.io/graphql/June2018/#ObjectField
struct object_field
	: if_must<object_field_name, star<ignored>, one<':'>, star<ignored>, input_value>
{
};

// https://facebook.github.io/graphql/June2018/#ObjectValue
struct object_value
	: if_must<one<'{'>, star<ignored>, opt<list<object_field, plus<ignored>>>, star<ignored>, one<'}'>>
{
};

struct variable_value
	: variable_name
{
};

// https://facebook.github.io/graphql/June2018/#Value
struct input_value
	: must<sor<list_value
	, object_value
	, variable_value
	, integer_value
	, float_value
	, string_value
	, bool_value
	, null_keyword
	, enum_value>>
{
};

struct list_entry
	: input_value
{
};

// https://facebook.github.io/graphql/June2018/#DefaultValue
struct default_value
	: if_must<one<'='>, star<ignored>, input_value>
{
};

// https://facebook.github.io/graphql/June2018/#NamedType
struct named_type
	: name
{
};

struct nonnull_type;

// https://facebook.github.io/graphql/June2018/#ListType
struct list_type
	: if_must<one<'['>, star<ignored>, sor<nonnull_type, list_type, named_type>, star<ignored>, one<']'>>
{
};

// https://facebook.github.io/graphql/June2018/#NonNullType
struct nonnull_type
	: seq<sor<list_type, named_type>, star<ignored>, one<'!'>>
{
};

// https://facebook.github.io/graphql/June2018/#Type
struct type_name
	: must<sor<nonnull_type, list_type, named_type>>
{
};

// https://facebook.github.io/graphql/June2018/#VariableDefinition
struct variable
	: if_must<variable_name, star<ignored>, one<':'>, star<ignored>, type_name, opt<seq<star<ignored>, default_value>>>
{
};

// https://facebook.github.io/graphql/June2018/#VariableDefinitions
struct variable_definitions
	: if_must<one<'('>, star<ignored>, list<variable, plus<ignored>>, star<ignored>, one<')'>>
{
};

struct directive_name
	: name
{
};

// https://facebook.github.io/graphql/June2018/#Directive
struct directive
	: if_must<one<'@'>, directive_name, opt<seq<star<ignored>, arguments>>>
{
};

// https://facebook.github.io/graphql/June2018/#Directives
struct directives
	: list<directive, plus<ignored>>
{
};

struct selection_set;

struct field_name
	: name
{
};

// https://facebook.github.io/graphql/June2018/#Field
struct field
	: if_must<seq<opt<seq<alias, star<ignored>>>, field_name>,
		sor<seq<opt<seq<star<ignored>, arguments>>, opt<seq<star<ignored>, directives>>, seq<star<ignored>, selection_set>>
		, seq<opt<seq<star<ignored>, arguments>>, seq<star<ignored>, directives>>
		, opt<seq<star<ignored>, arguments>>>>
{
};

struct on_keyword
	: TAO_PEGTL_KEYWORD("on")
{
};

// https://facebook.github.io/graphql/June2018/#FragmentName
struct fragment_name
	: seq<not_at<on_keyword>, name>
{
};

struct fragment_token
	: rep<3, one<'.'>>
{
};

// https://facebook.github.io/graphql/June2018/#FragmentSpread
struct fragment_spread
	: seq<fragment_token, star<ignored>, fragment_name, opt<seq<star<ignored>, directives>>>
{
};

// https://facebook.github.io/graphql/June2018/#TypeCondition
struct type_condition
	: seq<on_keyword, plus<ignored>, named_type>
{
};

// https://facebook.github.io/graphql/June2018/#InlineFragment
struct inline_fragment
	: seq<fragment_token, opt<star<ignored>, type_condition>, opt<seq<star<ignored>, directives>>, star<ignored>, selection_set>
{
};

// https://facebook.github.io/graphql/June2018/#Selection
struct selection
	: sor<field
	, fragment_spread
	, inline_fragment>
{
};

// https://facebook.github.io/graphql/June2018/#SelectionSet
struct selection_set
	: if_must<one<'{'>, star<ignored>, list<selection, plus<ignored>>, star<ignored>, one<'}'>>
{
};

struct operation_name
	: name
{
};

// https://facebook.github.io/graphql/June2018/#OperationDefinition
struct operation_definition
	: sor<if_must<operation_type, seq<opt<seq<plus<ignored>, operation_name>>, opt<seq<star<ignored>, variable_definitions>>, star<ignored>, selection_set>>
	, selection_set>
{
};

// https://facebook.github.io/graphql/June2018/#FragmentDefinition
struct fragment_definition
	: if_must<TAO_PEGTL_KEYWORD("fragment"), plus<ignored>, fragment_name, plus<ignored>, type_condition, opt<seq<star<ignored>, directives>>, star<ignored>, selection_set>
{
};

// https://facebook.github.io/graphql/June2018/#ExecutableDefinition
struct executable_definition
	: sor<fragment_definition
	, operation_definition>
{
};

struct schema_keyword
	: TAO_PEGTL_KEYWORD("schema")
{
};

// https://facebook.github.io/graphql/June2018/#RootOperationTypeDefinition
struct root_operation_definition
	: if_must<operation_type, star<ignored>, one<':'>, star<ignored>, named_type>
{
};

// https://facebook.github.io/graphql/June2018/#SchemaDefinition
struct schema_definition
	: if_must<schema_keyword, opt<seq<star<ignored>, directives>>, star<ignored>, one<'{'>, star<ignored>, list<root_operation_definition, plus<ignored>>, star<ignored>, one<'}'>>
{
};

struct scalar_keyword
	: TAO_PEGTL_KEYWORD("scalar")
{
};

// https://facebook.github.io/graphql/June2018/#Description
struct description
	: string_value
{
};

struct scalar_name
	: name
{
};

// https://facebook.github.io/graphql/June2018/#ScalarTypeDefinition
struct scalar_type_definition
	: if_must<seq<opt<seq<description, star<ignored>>>, scalar_keyword>, plus<ignored>, scalar_name, opt<seq<star<ignored>, directives>>>
{
};

struct type_keyword
	: TAO_PEGTL_KEYWORD("type")
{
};

struct input_field_definition;

// https://facebook.github.io/graphql/June2018/#ArgumentsDefinition
struct arguments_definition
	: if_must<one<'('>, star<ignored>, list<input_field_definition, plus<ignored>>, star<ignored>, one<')'>>
{
};

// https://facebook.github.io/graphql/June2018/#FieldDefinition
struct field_definition
	: if_must<seq<opt<seq<description, star<ignored>>>, field_name>, opt<seq<star<ignored>, arguments_definition>>, star<ignored>, one<':'>, star<ignored>, type_name, opt<seq<star<ignored>, directives>>>
{
};

// https://facebook.github.io/graphql/June2018/#FieldsDefinition
struct fields_definition
	: if_must<one<'{'>, star<ignored>, list<field_definition, plus<ignored>>, star<ignored>, one<'}'>>
{
};

struct interface_type
	: named_type
{
};

// https://facebook.github.io/graphql/June2018/#ImplementsInterfaces
struct implements_interfaces
	: if_must<TAO_PEGTL_KEYWORD("implements"), opt<seq<star<ignored>, one<'&'>>>, star<ignored>, list<interface_type, seq<star<ignored>, one<'&'>, star<ignored>>>>
{
};

struct object_name
	: name
{
};

// https://facebook.github.io/graphql/June2018/#ObjectTypeDefinition
struct object_type_definition
	: if_must<seq<opt<seq<description, star<ignored>>>, type_keyword>, plus<ignored>, object_name,
		sor<seq<opt<seq<plus<ignored>, implements_interfaces>>, opt<seq<star<ignored>, directives>>, seq<star<ignored>, fields_definition>>
		, seq<opt<seq<plus<ignored>, implements_interfaces>>, seq<star<ignored>, directives>>
		, opt<seq<plus<ignored>, implements_interfaces>>>>
{
};

struct interface_keyword
	: TAO_PEGTL_KEYWORD("interface")
{
};

struct interface_name
	: name
{
};

// https://facebook.github.io/graphql/June2018/#InterfaceTypeDefinition
struct interface_type_definition
	: if_must<seq<opt<seq<description, star<ignored>>>, interface_keyword>, plus<ignored>, interface_name,
		sor<seq<opt<seq<star<ignored>, directives>>, seq<star<ignored>, fields_definition>>
		, opt<seq<star<ignored>, directives>>>>
{
};

struct union_keyword
	: TAO_PEGTL_KEYWORD("union")
{
};

struct union_name
	: name
{
};

struct union_type
	: named_type
{
};

// https://facebook.github.io/graphql/June2018/#UnionMemberTypes
struct union_member_types
	: if_must<one<'='>, opt<seq<star<ignored>, one<'|'>>>, star<ignored>, list<union_type, seq<star<ignored>, one<'|'>, star<ignored>>>>
{
};

// https://facebook.github.io/graphql/June2018/#UnionTypeDefinition
struct union_type_definition
	: if_must<seq<opt<seq<description, star<ignored>>>, union_keyword>, plus<ignored>, union_name,
		sor<seq<opt<seq<star<ignored>, directives>>, seq<star<ignored>, union_member_types>>,
		opt<seq<star<ignored>, directives>>>>
{
};

struct enum_keyword
	: TAO_PEGTL_KEYWORD("enum")
{
};

struct enum_name
	: name
{
};

// https://facebook.github.io/graphql/June2018/#EnumValueDefinition
struct enum_value_definition
	: seq<opt<seq<description, star<ignored>>>, enum_value, opt<seq<star<ignored>, directives>>>
{
};

// https://facebook.github.io/graphql/June2018/#EnumValuesDefinition
struct enum_values_definition
	: if_must<one<'{'>, star<ignored>, list<enum_value_definition, plus<ignored>>, star<ignored>, one<'}'>>
{
};

// https://facebook.github.io/graphql/June2018/#EnumTypeDefinition
struct enum_type_definition
	: if_must<seq<opt<seq<description, star<ignored>>>, enum_keyword>, plus<ignored>, enum_name,
		sor<seq<opt<seq<star<ignored>, directives>>, seq<star<ignored>, enum_values_definition>>
		, opt<seq<star<ignored>, directives>>>>
{
};

struct input_keyword
	: TAO_PEGTL_KEYWORD("input")
{
};

// https://facebook.github.io/graphql/June2018/#InputValueDefinition
struct input_field_definition
	: if_must<seq<opt<seq<description, star<ignored>>>, argument_name>, star<ignored>, one<':'>, star<ignored>, type_name,
		sor<seq<opt<seq<star<ignored>, default_value>>, seq<star<ignored>, directives>>
		, opt<seq<star<ignored>, default_value>>>>
{
};

// https://facebook.github.io/graphql/June2018/#InputFieldsDefinition
struct input_fields_definition
	: if_must<one<'{'>, star<ignored>, list<input_field_definition, plus<ignored>>, star<ignored>, one<'}'>>
{
};

// https://facebook.github.io/graphql/June2018/#InputObjectTypeDefinition
struct input_object_type_definition
	: if_must<seq<opt<seq<description, star<ignored>>>, input_keyword>, plus<ignored>, object_name,
		sor<seq<opt<seq<star<ignored>, directives>>, seq<star<ignored>, input_fields_definition>>
		, opt<seq<star<ignored>, directives>>>>
{
};

// https://facebook.github.io/graphql/June2018/#TypeDefinition
struct type_definition
	: sor<scalar_type_definition
	, object_type_definition
	, interface_type_definition
	, union_type_definition
	, enum_type_definition
	, input_object_type_definition>
{
};

// https://facebook.github.io/graphql/June2018/#ExecutableDirectiveLocation
struct executable_directive_location
	: sor<TAO_PEGTL_KEYWORD("QUERY")
	, TAO_PEGTL_KEYWORD("MUTATION")
	, TAO_PEGTL_KEYWORD("SUBSCRIPTION")
	, TAO_PEGTL_KEYWORD("FIELD")
	, TAO_PEGTL_KEYWORD("FIELD_DEFINITION")
	, TAO_PEGTL_KEYWORD("FRAGMENT_SPREAD")
	, TAO_PEGTL_KEYWORD("INLINE_FRAGMENT")>
{
};

// https://facebook.github.io/graphql/June2018/#TypeSystemDirectiveLocation
struct type_system_directive_location
	: sor<TAO_PEGTL_KEYWORD("SCHEMA")
	, TAO_PEGTL_KEYWORD("SCALAR")
	, TAO_PEGTL_KEYWORD("OBJECT")
	, TAO_PEGTL_KEYWORD("FIELD_DEFINITION")
	, TAO_PEGTL_KEYWORD("ARGUMENT_DEFINITION")
	, TAO_PEGTL_KEYWORD("INTERFACE")
	, TAO_PEGTL_KEYWORD("UNION")
	, TAO_PEGTL_KEYWORD("ENUM")
	, TAO_PEGTL_KEYWORD("ENUM_VALUE")
	, TAO_PEGTL_KEYWORD("INPUT_OBJECT")
	, TAO_PEGTL_KEYWORD("INPUT_FIELD_DEFINITION")>
{
};

// https://facebook.github.io/graphql/June2018/#DirectiveLocation
struct directive_location
	: must<sor<executable_directive_location
	, type_system_directive_location>>
{
};

// https://facebook.github.io/graphql/June2018/#DirectiveLocations
struct directive_locations
	: seq<opt<seq<one<'|'>, star<ignored>>>, list<directive_location, seq<star<ignored>, one<'|'>, star<ignored>>>>
{
};

// https://facebook.github.io/graphql/June2018/#DirectiveDefinition
struct directive_definition
	: if_must<seq<opt<seq<description, star<ignored>>>, TAO_PEGTL_KEYWORD("directive")>, star<ignored>, one<'@'>, directive_name, arguments_definition, plus<ignored>, on_keyword, plus<ignored>, directive_locations>
{
};

// https://facebook.github.io/graphql/June2018/#TypeSystemDefinition
struct type_system_definition
	: sor<schema_definition
	, type_definition
	, directive_definition>
{
};

struct extend_keyword
	: TAO_PEGTL_KEYWORD("extend")
{
};

// https://facebook.github.io/graphql/June2018/#OperationTypeDefinition
struct operation_type_definition
	: root_operation_definition
{
};

// https://facebook.github.io/graphql/June2018/#SchemaExtension
struct schema_extension
	: if_must<seq<extend_keyword, plus<ignored>, schema_keyword>, star<ignored>,
		sor<seq<opt<directives>, one<'{'>, star<ignored>, list<operation_type_definition, plus<ignored>>, star<ignored>, one<'}'>>
		, directives>>
{
};

// https://facebook.github.io/graphql/June2018/#ScalarTypeExtension
struct scalar_type_extension
	: if_must<seq<extend_keyword, plus<ignored>, scalar_keyword>, star<ignored>, scalar_name, star<ignored>, directives>
{
};

// https://facebook.github.io/graphql/June2018/#ObjectTypeExtension
struct object_type_extension
	: if_must<seq<extend_keyword, plus<ignored>, type_keyword>, plus<ignored>, object_name,
		sor<seq<opt<seq<plus<ignored>, implements_interfaces>>, opt<seq<star<ignored>, directives>>, star<ignored>, fields_definition>
		, seq<opt<seq<plus<ignored>, implements_interfaces>>, star<ignored>, directives>
		, seq<plus<ignored>, implements_interfaces>>>
{
};

// https://facebook.github.io/graphql/June2018/#InterfaceTypeExtension
struct interface_type_extension
	: if_must<seq<extend_keyword, plus<ignored>, interface_keyword>, plus<ignored>, interface_name, star<ignored>,
		sor<seq<opt<seq<directives, star<ignored>>>, fields_definition>
		, directives>>
{
};

// https://facebook.github.io/graphql/June2018/#UnionTypeExtension
struct union_type_extension
	: if_must<seq<extend_keyword, plus<ignored>, union_keyword>, plus<ignored>, union_name, star<ignored>,
		sor<seq<opt<seq<directives, star<ignored>>>, union_member_types>
		, directives>>
{
};

// https://facebook.github.io/graphql/June2018/#EnumTypeExtension
struct enum_type_extension
	: if_must<seq<extend_keyword, plus<ignored>, enum_keyword>, plus<ignored>, enum_name, star<ignored>,
		sor<seq<opt<seq<directives, star<ignored>>>, enum_values_definition>
		, directives>>
{
};

// https://facebook.github.io/graphql/June2018/#InputObjectTypeExtension
struct input_object_type_extension
	: if_must<seq<extend_keyword, plus<ignored>, input_keyword>, plus<ignored>, object_name, star<ignored>,
		sor<seq<opt<seq<directives, star<ignored>>>, input_fields_definition>
		, directives>>
{
};

// https://facebook.github.io/graphql/June2018/#TypeExtension
struct type_extension
	: sor<scalar_type_extension
	, object_type_extension
	, interface_type_extension
	, union_type_extension
	, enum_type_extension
	, input_object_type_extension>
{
};

// https://facebook.github.io/graphql/June2018/#TypeSystemExtension
struct type_system_extension
	: sor<schema_extension
	, type_extension>
{
};

// https://facebook.github.io/graphql/June2018/#Definition
struct definition
	: sor<executable_definition
	, type_system_definition
	, type_system_extension>
{
};

// https://facebook.github.io/graphql/June2018/#Document
struct document
	: must<bof, opt<utf8::bom>, star<ignored>, list<definition, plus<ignored>>, star<ignored>, tao::graphqlpeg::eof>
{
};

} /* namespace peg */
} /* namespace graphql */
} /* namespace facebook */