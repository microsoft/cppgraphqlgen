// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <tao/pegtl.hpp>

namespace facebook {
namespace graphql {
namespace grammar {

using namespace tao::pegtl;

struct source_character
	: sor<utf8::one<0x0009, 0x000A, 0x000D>, utf8::range<0x0020, 0xFFFF>>
{
};

struct comment
	: seq<utf8::one<'#'>, until<eolf>>
{
};

struct ignored
	: sor<space, utf8::one<','>, comment>
{
};

struct name
	: seq<sor<alpha, utf8::one<'_'>>, star<sor<alnum, utf8::one<'_'>>>>
{
};

struct variable_name
	: seq<utf8::one<'$'>, name>
{
};

struct non_nullable_type
	: seq<utf8::one<'!'>, name>
{
};

struct null_keyword
	: keyword<'n', 'u', 'l', 'l'>
{
};

struct quote_token
	: utf8::one<'"'>
{
};

struct backslash_token
	: utf8::one<'\\'>
{
};

struct escaped_unicode
	: seq<utf8::one<'u'>, rep<4, xdigit>>
{
};

struct escaped_char
	: utf8::one<'"', '\\', '/', 'b', 'f', 'n', 'r', 't'>
{
};

struct string_escape_sequence
	: seq<backslash_token, must<sor<escaped_unicode, escaped_char>>>
{
};

struct string_quote
	: seq<quote_token, star<seq<not_at<quote_token>, sor<string_escape_sequence, source_character>>>, quote_token>
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

struct block_quote
	: seq<block_quote_token, star<seq<not_at<block_quote_token>, sor<block_escape_sequence, source_character>>>, block_quote_token>
{
};

struct string_value
	: sor<block_quote, string_quote>
{
};

struct nonzero_digit
	: utf8::range<'1', '9'>
{
};

struct zero_digit
	: utf8::one<'0'>
{
};

struct negative_sign
	: utf8::one<'-'>
{
};

struct integer_part
	: seq<opt<negative_sign>, sor<zero_digit, seq<nonzero_digit, star<digit>>>>
{
};

struct integer_value
	: integer_part
{
};

struct fractional_part
	: seq<utf8::one<'.'>, plus<digit>>
{
};

struct exponent_indicator
	: utf8::one<'e', 'E'>
{
};

struct positive_sign
	: utf8::one<'+'>
{
};

struct exponent_part
	: seq<exponent_indicator, opt<sor<positive_sign, negative_sign>>, plus<digit>>
{
};

struct float_value
	: seq<integer_part, sor<fractional_part, exponent_part, seq<fractional_part, exponent_part>>>
{
};

struct true_keyword
	: keyword<'t', 'r', 'u', 'e'>
{
};

struct false_keyword
	: keyword<'f', 'a', 'l', 's', 'e'>
{
};

struct bool_value
	: sor<true_keyword, false_keyword>
{
};

struct enum_value
	: seq<not_at<true_keyword, false_keyword, null_keyword>, name>
{
};

struct query_keyword
	: keyword<'q', 'u', 'e', 'r', 'y'>
{
};

struct mutation_keyword
	: keyword<'m', 'u', 't', 'a', 't', 'i', 'o', 'n'>
{
};

struct subscription_keyword
	: keyword<'s', 'u', 'b', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n'>
{
};

struct operation_type
	: sor<query_keyword, mutation_keyword, subscription_keyword>
{
};

struct alias
	: seq<name, star<ignored>, utf8::one<':'>>
{
};

struct argument_name
	: alias
{
};

struct input_value;

struct argument
	: seq<argument_name, star<ignored>, input_value>
{
};

struct arguments
	: seq<utf8::one<'('>, star<ignored>, list<argument, plus<ignored>>, star<ignored>, utf8::one<')'>>
{
};

struct list_value
	: seq<utf8::one<'['>, star<ignored>, list<input_value, plus<ignored>>, star<ignored>, utf8::one<']'>>
{
};

struct object_value
	: seq<utf8::one<'{'>, star<ignored>, opt<list<argument, plus<ignored>>>, star<ignored>, utf8::one<'}'>>
{
};

struct input_value
	: sor<list_value, object_value, variable_name, integer_value, float_value, string_value, bool_value, null_keyword, enum_value>
{
};

struct default_value
	: seq<utf8::one<'='>, star<ignored>, input_value>
{
};

struct named_type
	: name
{
};

struct nonnull_type;

struct list_type
	: seq<utf8::one<'['>, star<ignored>, sor<nonnull_type, list_type, named_type>, star<ignored>, utf8::one<']'>>
{
};

struct nonnull_type
	: seq<sor<list_type, named_type>, star<ignored>, utf8::one<'!'>>
{
};

struct type_name
	: sor<nonnull_type, list_type, named_type>
{
};

struct variable
	: seq<variable_name, star<ignored>, utf8::one<':'>, star<ignored>, type_name, opt<seq<star<ignored>, default_value>>>
{
};

struct variable_definitions
	: seq<utf8::one<'('>, star<ignored>, list<variable, plus<ignored>>, star<ignored>, utf8::one<')'>>
{
};

struct directive
	: seq<utf8::one<'@'>, name, opt<seq<star<ignored>, arguments>>>
{
};

struct directives
	: list<directive, plus<ignored>>
{
};

struct selection_set;

struct field
	: sor<seq<opt<seq<alias, star<ignored>>>, name, opt<seq<star<ignored>, arguments>>, opt<seq<star<ignored>, directives>>, seq<star<ignored>, selection_set>>,
	seq<opt<seq<alias, star<ignored>>>, name, opt<seq<star<ignored>, arguments>>, seq<star<ignored>, directives>>,
	seq<opt<seq<alias, star<ignored>>>, name, opt<seq<star<ignored>, arguments>>>>
{
};

struct on_keyword
	: keyword<'o', 'n'>
{
};

struct fragment_name
	: seq<not_at<on_keyword>, name>
{
};

struct fragment_token
	: rep<3, utf8::one<'.'>>
{
};

struct fragment_spread
	: seq<fragment_token, star<ignored>, fragment_name, opt<seq<star<ignored>, directives>>>
{
};

struct type_condition
	: seq<on_keyword, plus<ignored>, named_type>
{
};

struct inline_fragment
	: seq<fragment_token, opt<star<ignored>, type_condition>, opt<seq<star<ignored>, directives>>, star<ignored>, selection_set>
{
};

struct selection
	: sor<field, fragment_spread, inline_fragment>
{
};

struct selection_set
	: seq<utf8::one<'{'>, star<ignored>, list<selection, plus<ignored>>, star<ignored>, utf8::one<'}'>>
{
};

struct operation_definition
	: sor<seq<operation_type, opt<seq<plus<ignored>, name>>, opt<seq<star<ignored>, variable_definitions>>, star<ignored>, selection_set>, selection_set>
{
};

struct fragment_keyword
	: keyword<'f', 'r', 'a', 'g', 'm', 'e', 'n', 't'>
{
};

struct fragment_definition
	: seq<fragment_keyword, plus<ignored>, fragment_name, plus<ignored>, type_condition, opt<seq<star<ignored>, directives>>, star<ignored>, selection_set>
{
};

struct executable_definition
	: sor<operation_definition, fragment_definition>
{
};

struct schema_keyword
	: keyword<'s', 'c', 'h', 'e', 'm', 'a'>
{
};

struct root_operation_definition
	: seq<operation_type, star<ignored>, utf8::one<':'>, star<ignored>, named_type>
{
};

struct schema_definition
	: seq<schema_keyword, opt<seq<star<ignored>, directives>>, star<ignored>, utf8::one<'{'>, star<ignored>, list<root_operation_definition, plus<ignored>>, star<ignored>, utf8::one<'}'>>
{
};

struct scalar_keyword
	: keyword<'s', 'c', 'a', 'l', 'a', 'r'>
{
};

struct description
	: string_value
{
};

struct scalar_type_definition
	: seq<opt<seq<description, star<ignored>>>, scalar_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>>
{
};

struct type_keyword
	: keyword<'t', 'y', 'p', 'e'>
{
};

struct implements_keyword
	: keyword<'i', 'm', 'p', 'l', 'e', 'm', 'e', 'n', 't', 's'>
{
};

struct implements_interfaces
	: seq<implements_keyword, opt<seq<star<ignored>, utf8::one<'&'>>>, star<ignored>, list<named_type, seq<star<ignored>, utf8::one<'&'>, star<ignored>>>>
{
};

struct input_field_definition;

struct arguments_definition
	: seq<utf8::one<'('>, star<ignored>, list<input_field_definition, plus<ignored>>, star<ignored>, utf8::one<')'>>
{
};

struct field_definition
	: seq<opt<seq<description, star<ignored>>>, name, opt<seq<star<ignored>, arguments_definition>>, star<ignored>, utf8::one<':'>, star<ignored>, type_name, opt<seq<star<ignored>, directives>>>
{
};

struct fields_definition
	: seq<utf8::one<'{'>, star<ignored>, list<field_definition, plus<ignored>>, star<ignored>, utf8::one<'}'>>
{
};

struct object_type_definition
	: sor<seq<opt<seq<description, star<ignored>>>, type_keyword, plus<ignored>, name, opt<seq<plus<ignored>, implements_interfaces>>, opt<seq<star<ignored>, directives>>, seq<star<ignored>, fields_definition>>,
	seq<opt<seq<description, star<ignored>>>, type_keyword, plus<ignored>, name, opt<seq<plus<ignored>, implements_interfaces>>, seq<star<ignored>, directives>>,
	seq<opt<seq<description, star<ignored>>>, type_keyword, plus<ignored>, name, opt<seq<plus<ignored>, implements_interfaces>>>>
{
};

struct interface_keyword
	: keyword<'i', 'n', 't', 'e', 'r', 'f', 'a', 'c', 'e'>
{
};

struct interface_type_definition
	: sor<seq<opt<seq<description, star<ignored>>>, interface_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>, seq<star<ignored>, fields_definition>>,
	seq<opt<seq<description, star<ignored>>>, interface_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>>>
{
};

struct union_keyword
	: keyword<'u', 'n', 'i', 'o', 'n'>
{
};

struct union_member_types
	: seq<utf8::one<'='>, opt<seq<star<ignored>, utf8::one<'|'>>>, star<ignored>, list<named_type, seq<star<ignored>, utf8::one<'|'>, star<ignored>>>>
{
};

struct union_type_definition
	: sor<seq<opt<seq<description, star<ignored>>>, union_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>, seq<star<ignored>, union_member_types>>,
	seq<opt<seq<description, star<ignored>>>, union_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>>>
{
};

struct enum_keyword
	: keyword<'e', 'n', 'u', 'm'>
{
};

struct enum_value_definition
	: seq<opt<seq<description, star<ignored>>>, enum_value, opt<seq<star<ignored>, directives>>>
{
};

struct enum_values_definition
	: seq<utf8::one<'{'>, star<ignored>, list<enum_value_definition, plus<ignored>>, star<ignored>, utf8::one<'}'>>
{
};

struct enum_type_definition
	: sor<seq<opt<seq<description, star<ignored>>>, enum_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>, seq<star<ignored>, enum_values_definition>>,
	seq<opt<seq<description, star<ignored>>>, enum_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>>>
{
};

struct input_keyword
	: keyword<'i', 'n', 'p', 'u', 't'>
{
};

struct input_field_definition
	: sor<seq<opt<seq<description, star<ignored>>>, name, star<ignored>, utf8::one<':'>, star<ignored>, type_name, opt<seq<star<ignored>, default_value>>, seq<star<ignored>, directives>>,
	seq<opt<seq<description, star<ignored>>>, name, star<ignored>, utf8::one<':'>, star<ignored>, type_name, opt<seq<star<ignored>, default_value>>>>
{
};

struct input_fields_definition
	: seq<utf8::one<'{'>, star<ignored>, list<input_field_definition, plus<ignored>>, star<ignored>, utf8::one<'}'>>
{
};

struct input_object_type_definition
	: sor<seq<opt<seq<description, star<ignored>>>, input_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>, seq<star<ignored>, input_fields_definition>>,
	seq<opt<seq<description, star<ignored>>>, input_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>>>
{
};

struct type_definition
	: sor<scalar_type_definition, object_type_definition, interface_type_definition, union_type_definition, enum_type_definition, input_object_type_definition>
{
};

struct directive_keyword
	: keyword<'d', 'i', 'r', 'e', 'c', 't', 'i', 'v', 'e'>
{
};

struct QUERY_keyword
	: keyword<'Q', 'U', 'E', 'R', 'Y'>
{
};

struct MUTATION_keyword
	: keyword<'M', 'U', 'T', 'A', 'T', 'I', 'O', 'N'>
{
};

struct SUBSCRIPTION_keyword
	: keyword<'S', 'U', 'B', 'S', 'C', 'R', 'I', 'P', 'T', 'I', 'O', 'N'>
{
};

struct FIELD_keyword
	: keyword<'F', 'I', 'E', 'L', 'D'>
{
};

struct FRAGMENT_DEFINITION_keyword
	: keyword<'F', 'R', 'A', 'G', 'M', 'E', 'N', 'T', '_', 'D', 'E', 'F', 'I', 'N', 'I', 'T', 'I', 'O', 'N'>
{
};

struct FRAGMENT_SPREAD_keyword
	: keyword<'F', 'R', 'A', 'G', 'M', 'E', 'N', 'T', '_', 'S', 'P', 'R', 'E', 'A', 'D'>
{
};

struct INLINE_FRAGMENT_keyword
	: keyword<'I', 'N', 'L', 'I', 'N', 'E', '_', 'F', 'R', 'A', 'G', 'M', 'E', 'N', 'T'>
{
};

struct executable_directive_location
	: sor<QUERY_keyword, MUTATION_keyword, SUBSCRIPTION_keyword, FIELD_keyword, FRAGMENT_DEFINITION_keyword, FRAGMENT_SPREAD_keyword, INLINE_FRAGMENT_keyword>
{
};

struct SCHEMA_keyword
	: keyword<'S', 'C', 'H', 'E', 'M', 'A'>
{
};

struct SCALAR_keyword
	: keyword<'S', 'C', 'A', 'L', 'A', 'R'>
{
};

struct OBJECT_keyword
	: keyword<'O', 'B', 'J', 'E', 'C', 'T'>
{
};

struct FIELD_DEFINITION_keyword
	: keyword<'F', 'I', 'E', 'L', 'D', '_', 'D', 'E', 'F', 'I', 'N', 'I', 'T', 'I', 'O', 'N'>
{
};

struct ARGUMENT_DEFINITION_keyword
	: keyword<'A', 'R', 'G', 'U', 'M', 'E', 'N', 'T', '_', 'D', 'E', 'F', 'I', 'N', 'I', 'T', 'I', 'O', 'N'>
{
};

struct INTERFACE_keyword
	: keyword<'I', 'N', 'T', 'E', 'R', 'F', 'A', 'C', 'E'>
{
};

struct UNION_keyword
	: keyword<'U', 'N', 'I', 'O', 'N'>
{
};

struct ENUM_keyword
	: keyword<'E', 'N', 'U', 'M'>
{
};

struct ENUM_VALUE_keyword
	: keyword<'E', 'N', 'U', 'M', '_', 'V', 'A', 'L', 'U', 'E'>
{
};

struct INPUT_OBJECT_keyword
	: keyword<'I', 'N', 'P', 'U', 'T', '_', 'O', 'B', 'J', 'E', 'C', 'T'>
{
};

struct INPUT_FIELD_DEFINITION_keyword
	: keyword<'I', 'N', 'P', 'U', 'T', '_', 'F', 'I', 'E', 'L', 'D', '_', 'D', 'E', 'F', 'I', 'N', 'I', 'T', 'I', 'O', 'N'>
{
};

struct type_system_directive_location
	: sor<SCHEMA_keyword, SCALAR_keyword, OBJECT_keyword, FIELD_DEFINITION_keyword, ARGUMENT_DEFINITION_keyword, INTERFACE_keyword, UNION_keyword, ENUM_keyword, ENUM_VALUE_keyword, INPUT_OBJECT_keyword, INPUT_FIELD_DEFINITION_keyword>
{
};

struct directive_location
	: sor<executable_directive_location, type_system_directive_location>
{
};

struct directive_locations
	: seq<opt<seq<utf8::one<'|'>, star<ignored>>>, list<directive_location, seq<star<ignored>, utf8::one<'|'>, star<ignored>>>>
{
};

struct directive_definition
	: seq<opt<seq<description, star<ignored>>>, directive_keyword, star<ignored>, utf8::one<'@'>, name, arguments_definition, plus<ignored>, on_keyword, plus<ignored>, directive_locations>
{
};

struct type_system_definition
	: sor<schema_definition, type_definition, directive_definition>
{
};

struct extend_keyword
	: keyword<'e', 'x', 't', 'e', 'n', 'd'>
{
};

struct operation_type_definition
	: root_operation_definition
{
};

struct schema_extension
	: sor<seq<extend_keyword, plus<ignored>, schema_keyword, star<ignored>, opt<directives>, utf8::one<'{'>, star<ignored>, list<operation_type_definition, plus<ignored>>, star<ignored>, utf8::one<'}'>>,
	seq<extend_keyword, plus<ignored>, schema_keyword, star<ignored>, directives>>
{
};

struct scalar_type_extension
	: seq<extend_keyword, plus<ignored>, scalar_keyword, star<ignored>, directives>
{
};

struct object_type_extension
	: sor<seq<extend_keyword, plus<ignored>, type_keyword, plus<ignored>, name, opt<seq<plus<ignored>, implements_interfaces>>, opt<seq<star<ignored>, directives>>, star<ignored>, fields_definition>,
	seq<extend_keyword, plus<ignored>, type_keyword, plus<ignored>, name, opt<seq<plus<ignored>, implements_interfaces>>, star<ignored>, directives>,
	seq<extend_keyword, plus<ignored>, type_keyword, plus<ignored>, name, plus<ignored>, implements_interfaces>>
{
};

struct interface_type_extension
	: sor<seq<extend_keyword, plus<ignored>, interface_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>, star<ignored>, fields_definition>,
	seq<extend_keyword, plus<ignored>, interface_keyword, plus<ignored>, name, star<ignored>, directives>>
{
};

struct union_type_extension
	: sor<seq<extend_keyword, plus<ignored>, union_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>, star<ignored>, union_member_types>,
	seq<extend_keyword, plus<ignored>, union_keyword, plus<ignored>, name, star<ignored>, directives>>
{
};

struct enum_type_extension
	: sor<seq<extend_keyword, plus<ignored>, enum_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>, star<ignored>, enum_values_definition>,
	seq<extend_keyword, plus<ignored>, enum_keyword, plus<ignored>, name, star<ignored>, directives>>
{
};

struct input_object_type_extension
	: sor<seq<extend_keyword, plus<ignored>, input_keyword, plus<ignored>, name, opt<seq<star<ignored>, directives>>, star<ignored>, input_fields_definition>,
	seq<extend_keyword, plus<ignored>, input_keyword, plus<ignored>, name, star<ignored>, directives>>
{
};

struct type_extension
	: sor<scalar_type_extension, object_type_extension, interface_type_extension, union_type_extension, enum_type_extension, input_object_type_extension>
{
};

struct type_system_extension
	: sor<schema_extension, type_extension>
{
};

struct definition
	: sor<executable_definition, type_system_definition, type_system_extension>
{
};

struct document
	: seq<bof, opt<utf8::bom>, star<ignored>, list<definition, plus<ignored>>, star<ignored>, tao::pegtl::eof>
{
};

} /* namespace grammar */
} /* namespace graphql */
} /* namespace facebook */