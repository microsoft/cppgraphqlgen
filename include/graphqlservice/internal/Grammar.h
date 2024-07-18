// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// This grammar is based on the October 2021 Edition of the GraphQL spec:
// https://spec.graphql.org/October2021/

#pragma once

#ifndef GRAPHQLGRAMMAR_H
#define GRAPHQLGRAMMAR_H

#include "graphqlservice/internal/SyntaxTree.h"

#include <functional>

namespace graphql::peg {

using namespace tao::graphqlpeg;

template <typename Rule>
inline void for_each_child(const ast_node& n, std::function<void(const ast_node&)>&& func)
{
	for (const auto& child : n.children)
	{
		if (child->is_type<Rule>())
		{
			func(*child);
		}
	}
}

template <typename Rule>
inline void on_first_child_if(const ast_node& n, std::function<bool(const ast_node&)>&& func)
{
	for (const auto& child : n.children)
	{
		if (child->is_type<Rule>() && func(*child))
		{
			return;
		}
	}
}

template <typename Rule>
inline void on_first_child(const ast_node& n, std::function<void(const ast_node&)>&& func)
{
	for (const auto& child : n.children)
	{
		if (child->is_type<Rule>())
		{
			func(*child);
			return;
		}
	}
}

// https://spec.graphql.org/October2021/#sec-Source-Text
struct source_character : sor<one<0x0009, 0x000A, 0x000D>, utf8::range<0x0020, 0xFFFF>>
{
};

// https://spec.graphql.org/October2021/#sec-Comments
struct comment : seq<one<'#'>, until<eolf>>
{
};

// https://spec.graphql.org/October2021/#sec-Source-Text.Ignored-Tokens
struct ignored : sor<space, one<','>, comment>
{
};

// https://spec.graphql.org/October2021/#sec-Names
struct name : identifier
{
};

struct variable_name_content : name
{
};

// https://spec.graphql.org/October2021/#Variable
struct variable_name : if_must<one<'$'>, variable_name_content>
{
};

// https://spec.graphql.org/October2021/#sec-Null-Value
struct null_keyword : TAO_PEGTL_KEYWORD("null")
{
};

struct quote_token : one<'"'>
{
};

struct backslash_token : one<'\\'>
{
};

struct escaped_unicode_codepoint : rep<4, xdigit>
{
};

struct escaped_unicode_content : list<escaped_unicode_codepoint, seq<backslash_token, one<'u'>>>
{
};

// https://spec.graphql.org/October2021/#EscapedUnicode
struct escaped_unicode : if_must<one<'u'>, escaped_unicode_content>
{
};

// https://spec.graphql.org/October2021/#EscapedCharacter
struct escaped_char : one<'"', '\\', '/', 'b', 'f', 'n', 'r', 't'>
{
};

struct string_escape_sequence_content : sor<escaped_unicode, escaped_char>
{
};

struct string_escape_sequence : if_must<backslash_token, string_escape_sequence_content>
{
};

struct string_quote_character
	: plus<not_at<backslash_token>, not_at<quote_token>, not_at<ascii::eol>, source_character>
{
};

struct string_quote_content
	: seq<star<sor<string_escape_sequence, string_quote_character>>, must<quote_token>>
{
};

// https://spec.graphql.org/October2021/#StringCharacter
struct string_quote : if_must<quote_token, string_quote_content>
{
};

struct block_quote_token : rep<3, quote_token>
{
};

struct block_escape_sequence : seq<backslash_token, block_quote_token>
{
};

struct block_quote_character
	: plus<not_at<ascii::eol>, not_at<block_quote_token>, not_at<block_escape_sequence>,
		  source_character>
{
};

struct block_quote_empty_line : star<not_at<eol>, space>
{
};

struct block_quote_line_content : plus<sor<block_escape_sequence, block_quote_character>>
{
};

struct block_quote_line : seq<block_quote_empty_line, block_quote_line_content>
{
};

struct block_quote_content_lines : opt<list<sor<block_quote_line, block_quote_empty_line>, eol>>
{
};

struct block_quote_content : seq<block_quote_content_lines, must<block_quote_token>>
{
};

// https://spec.graphql.org/October2021/#BlockStringCharacter
struct block_quote : if_must<block_quote_token, block_quote_content>
{
};

// https://spec.graphql.org/October2021/#StringValue
struct string_value : sor<block_quote, string_quote>
{
};

// https://spec.graphql.org/October2021/#NonZeroDigit
struct nonzero_digit : range<'1', '9'>
{
};

struct zero_digit : one<'0'>
{
};

// https://spec.graphql.org/October2021/#NegativeSign
struct negative_sign : one<'-'>
{
};

// https://spec.graphql.org/October2021/#IntegerPart
struct integer_part : seq<opt<negative_sign>, sor<zero_digit, seq<nonzero_digit, star<digit>>>>
{
};

// https://spec.graphql.org/October2021/#IntValue
struct integer_value : integer_part
{
};

struct fractional_part_content : plus<digit>
{
};

// https://spec.graphql.org/October2021/#FractionalPart
struct fractional_part : if_must<one<'.'>, fractional_part_content>
{
};

// https://spec.graphql.org/October2021/#ExponentIndicator
struct exponent_indicator : one<'e', 'E'>
{
};

// https://spec.graphql.org/October2021/#Sign
struct sign : one<'+', '-'>
{
};

struct exponent_part_content : seq<opt<sign>, plus<digit>>
{
};

// https://spec.graphql.org/October2021/#ExponentPart
struct exponent_part : if_must<exponent_indicator, exponent_part_content>
{
};

// https://spec.graphql.org/October2021/#FloatValue
struct float_value
	: seq<integer_part, sor<fractional_part, exponent_part, seq<fractional_part, exponent_part>>>
{
};

struct true_keyword : TAO_PEGTL_KEYWORD("true")
{
};

struct false_keyword : TAO_PEGTL_KEYWORD("false")
{
};

// https://spec.graphql.org/October2021/#BooleanValue
struct bool_value : sor<true_keyword, false_keyword>
{
};

// https://spec.graphql.org/October2021/#EnumValue
struct enum_value : seq<not_at<true_keyword, false_keyword, null_keyword>, name>
{
};

// https://spec.graphql.org/October2021/#OperationType
struct operation_type
	: sor<TAO_PEGTL_KEYWORD("query"), TAO_PEGTL_KEYWORD("mutation"),
		  TAO_PEGTL_KEYWORD("subscription")>
{
};

struct alias_name : name
{
};

// https://spec.graphql.org/October2021/#Alias
struct alias : seq<alias_name, star<ignored>, one<':'>>
{
};

struct argument_name : name
{
};

struct input_value;

struct argument_content : seq<star<ignored>, one<':'>, star<ignored>, input_value>
{
};

// https://spec.graphql.org/October2021/#Argument
struct argument : if_must<argument_name, argument_content>
{
};

struct arguments_content
	: seq<star<ignored>, list<argument, plus<ignored>>, star<ignored>, must<one<')'>>>
{
};

// https://spec.graphql.org/October2021/#Arguments
struct arguments : if_must<one<'('>, arguments_content>
{
};

struct list_entry;

struct list_value_content
	: seq<star<ignored>, opt<list<list_entry, plus<ignored>>>, star<ignored>, must<one<']'>>>
{
};

// https://spec.graphql.org/October2021/#ListValue
struct list_value : if_must<one<'['>, list_value_content>
{
};

struct object_field_name : name
{
};

struct object_field_content : seq<star<ignored>, one<':'>, star<ignored>, input_value>
{
};

// https://spec.graphql.org/October2021/#ObjectField
struct object_field : if_must<object_field_name, object_field_content>
{
};

struct object_value_content
	: seq<star<ignored>, opt<list<object_field, plus<ignored>>>, star<ignored>, must<one<'}'>>>
{
};

// https://spec.graphql.org/October2021/#ObjectValue
struct object_value : if_must<one<'{'>, object_value_content>
{
};

struct variable_value : variable_name
{
};

struct input_value_content
	: sor<list_value, object_value, variable_value, float_value, integer_value, string_value,
		  bool_value, null_keyword, enum_value>
{
};

// https://spec.graphql.org/October2021/#Value
struct input_value : must<input_value_content>
{
};

struct list_entry : input_value_content
{
};

struct default_value_content : seq<star<ignored>, input_value>
{
};

// https://spec.graphql.org/October2021/#DefaultValue
struct default_value : if_must<one<'='>, default_value_content>
{
};

// https://spec.graphql.org/October2021/#NamedType
struct named_type : name
{
};

struct list_type;
struct nonnull_type;

struct list_type_content
	: seq<star<ignored>, sor<nonnull_type, list_type, named_type>, star<ignored>, must<one<']'>>>
{
};

// https://spec.graphql.org/October2021/#ListType
struct list_type : if_must<one<'['>, list_type_content>
{
};

// https://spec.graphql.org/October2021/#NonNullType
struct nonnull_type : seq<sor<list_type, named_type>, star<ignored>, one<'!'>>
{
};

struct type_name_content : sor<nonnull_type, list_type, named_type>
{
};

// https://spec.graphql.org/October2021/#Type
struct type_name : must<type_name_content>
{
};

struct variable_content
	: seq<star<ignored>, one<':'>, star<ignored>, type_name, opt<star<ignored>, default_value>>
{
};

// https://spec.graphql.org/October2021/#VariableDefinition
struct variable : if_must<variable_name, variable_content>
{
};

struct variable_definitions_content
	: seq<star<ignored>, list<variable, plus<ignored>>, star<ignored>, must<one<')'>>>
{
};

// https://spec.graphql.org/October2021/#VariableDefinitions
struct variable_definitions : if_must<one<'('>, variable_definitions_content>
{
};

struct directive_name : name
{
};

struct directive_content : seq<directive_name, opt<star<ignored>, arguments>>
{
};

// https://spec.graphql.org/October2021/#Directive
struct directive : if_must<one<'@'>, directive_content>
{
};

// https://spec.graphql.org/October2021/#Directives
struct directives : list<directive, plus<ignored>>
{
};

struct selection_set;

struct field_name : name
{
};

struct field_start : seq<opt<alias, star<ignored>>, field_name>
{
};

struct field_arguments : opt<star<ignored>, arguments>
{
};

struct field_directives : seq<star<ignored>, directives>
{
};

struct field_selection_set : seq<star<ignored>, selection_set>
{
};

struct field_content : seq<field_arguments, opt<field_directives>, opt<field_selection_set>>
{
};

// https://spec.graphql.org/October2021/#Field
struct field : if_must<field_start, field_content>
{
};

struct on_keyword : TAO_PEGTL_KEYWORD("on")
{
};

// https://spec.graphql.org/October2021/#FragmentName
struct fragment_name : seq<not_at<on_keyword>, name>
{
};

struct fragment_token : ellipsis
{
};

// https://spec.graphql.org/October2021/#FragmentSpread
struct fragment_spread : seq<star<ignored>, fragment_name, opt<star<ignored>, directives>>
{
};

struct type_condition_content : seq<plus<ignored>, named_type>
{
};

// https://spec.graphql.org/October2021/#TypeCondition
struct type_condition : if_must<on_keyword, type_condition_content>
{
};

// https://spec.graphql.org/October2021/#InlineFragment
struct inline_fragment
	: seq<opt<star<ignored>, type_condition>, opt<star<ignored>, directives>, star<ignored>,
		  selection_set>
{
};

struct fragement_spread_or_inline_fragment_content : sor<fragment_spread, inline_fragment>
{
};

struct fragement_spread_or_inline_fragment
	: if_must<fragment_token, fragement_spread_or_inline_fragment_content>
{
};

// https://spec.graphql.org/October2021/#Selection
struct selection : sor<field, fragement_spread_or_inline_fragment>
{
};

struct selection_set_content
	: seq<star<ignored>, list<selection, plus<ignored>>, star<ignored>, must<one<'}'>>>
{
};

// https://spec.graphql.org/October2021/#SelectionSet
struct selection_set : if_must<one<'{'>, selection_set_content>
{
};

struct operation_name : name
{
};

struct operation_definition_operation_type_content
	: seq<opt<plus<ignored>, operation_name>, opt<star<ignored>, variable_definitions>,
		  opt<star<ignored>, directives>, star<ignored>, selection_set>
{
};

// https://spec.graphql.org/October2021/#OperationDefinition
struct operation_definition
	: sor<if_must<operation_type, operation_definition_operation_type_content>, selection_set>
{
};

struct fragment_definition_content
	: seq<plus<ignored>, fragment_name, plus<ignored>, type_condition,
		  opt<star<ignored>, directives>, star<ignored>, selection_set>
{
};

// https://spec.graphql.org/October2021/#FragmentDefinition
struct fragment_definition : if_must<TAO_PEGTL_KEYWORD("fragment"), fragment_definition_content>
{
};

// https://spec.graphql.org/October2021/#ExecutableDefinition
struct executable_definition : sor<fragment_definition, operation_definition>
{
};

// https://spec.graphql.org/October2021/#Description
struct description : string_value
{
};

struct schema_keyword : TAO_PEGTL_KEYWORD("schema")
{
};

struct root_operation_definition_content : seq<star<ignored>, one<':'>, star<ignored>, named_type>
{
};

// https://spec.graphql.org/October2021/#RootOperationTypeDefinition
struct root_operation_definition : if_must<operation_type, root_operation_definition_content>
{
};

struct schema_definition_start : seq<opt<description, star<ignored>>, schema_keyword>
{
};

struct schema_definition_content
	: seq<opt<star<ignored>, directives>, star<ignored>, one<'{'>, star<ignored>,
		  list<root_operation_definition, plus<ignored>>, star<ignored>, must<one<'}'>>>
{
};

// https://spec.graphql.org/October2021/#SchemaDefinition
struct schema_definition : if_must<schema_definition_start, schema_definition_content>
{
};

struct scalar_keyword : TAO_PEGTL_KEYWORD("scalar")
{
};

struct scalar_name : name
{
};

struct scalar_type_definition_start : seq<opt<description, star<ignored>>, scalar_keyword>
{
};

struct scalar_type_definition_content
	: seq<plus<ignored>, scalar_name, opt<star<ignored>, directives>>
{
};

// https://spec.graphql.org/October2021/#ScalarTypeDefinition
struct scalar_type_definition
	: if_must<scalar_type_definition_start, scalar_type_definition_content>
{
};

struct type_keyword : TAO_PEGTL_KEYWORD("type")
{
};

struct input_field_definition;

struct arguments_definition_start : one<'('>
{
};

struct arguments_definition_content
	: seq<star<ignored>, list<input_field_definition, plus<ignored>>, star<ignored>, must<one<')'>>>
{
};

// https://spec.graphql.org/October2021/#ArgumentsDefinition
struct arguments_definition : if_must<arguments_definition_start, arguments_definition_content>
{
};

struct field_definition_start : seq<opt<description, star<ignored>>, field_name>
{
};

struct field_definition_content
	: seq<opt<star<ignored>, arguments_definition>, star<ignored>, one<':'>, star<ignored>,
		  type_name, opt<star<ignored>, directives>>
{
};

// https://spec.graphql.org/October2021/#FieldDefinition
struct field_definition : if_must<field_definition_start, field_definition_content>
{
};

struct fields_definition_content
	: seq<star<ignored>, list<field_definition, plus<ignored>>, star<ignored>, must<one<'}'>>>
{
};

// https://spec.graphql.org/October2021/#FieldsDefinition
struct fields_definition : if_must<one<'{'>, fields_definition_content>
{
};

struct interface_type : named_type
{
};

struct implements_interfaces_content
	: seq<opt<star<ignored>, one<'&'>>, star<ignored>,
		  list<interface_type, seq<star<ignored>, one<'&'>, star<ignored>>>>
{
};

// https://spec.graphql.org/October2021/#ImplementsInterfaces
struct implements_interfaces
	: if_must<TAO_PEGTL_KEYWORD("implements"), implements_interfaces_content>
{
};

struct object_name : name
{
};

struct object_type_definition_start : seq<opt<description, star<ignored>>, type_keyword>
{
};

struct object_type_definition_object_name : seq<plus<ignored>, object_name>
{
};

struct object_type_definition_implements_interfaces : opt<plus<ignored>, implements_interfaces>
{
};

struct object_type_definition_directives : seq<star<ignored>, directives>
{
};

struct object_type_definition_fields_definition : seq<star<ignored>, fields_definition>
{
};

struct object_type_definition_content
	: seq<object_type_definition_object_name,
		  sor<seq<object_type_definition_implements_interfaces,
				  opt<object_type_definition_directives>, object_type_definition_fields_definition>,
			  seq<object_type_definition_implements_interfaces, object_type_definition_directives>,
			  object_type_definition_implements_interfaces>>
{
};

// https://spec.graphql.org/October2021/#ObjectTypeDefinition
struct object_type_definition
	: if_must<object_type_definition_start, object_type_definition_content>
{
};

struct interface_keyword : TAO_PEGTL_KEYWORD("interface")
{
};

struct interface_name : name
{
};

struct interface_type_definition_start : seq<opt<description, star<ignored>>, interface_keyword>
{
};

struct interface_type_definition_interface_name : seq<plus<ignored>, interface_name>
{
};

struct interface_type_definition_implements_interfaces : opt<plus<ignored>, implements_interfaces>
{
};

struct interface_type_definition_directives : seq<star<ignored>, directives>
{
};

struct interface_type_definition_fields_definition : seq<star<ignored>, fields_definition>
{
};

struct interface_type_definition_content
	: seq<interface_type_definition_interface_name,
		  sor<seq<interface_type_definition_implements_interfaces,
				  opt<interface_type_definition_directives>,
				  interface_type_definition_fields_definition>,
			  seq<interface_type_definition_implements_interfaces,
				  interface_type_definition_directives,
				  interface_type_definition_fields_definition>,
			  interface_type_definition_implements_interfaces>>
{
};

// https://spec.graphql.org/October2021/#InterfaceTypeDefinition
struct interface_type_definition
	: if_must<interface_type_definition_start, interface_type_definition_content>
{
};

struct union_keyword : TAO_PEGTL_KEYWORD("union")
{
};

struct union_name : name
{
};

struct union_type : named_type
{
};

struct union_member_types_start : one<'='>
{
};

struct union_member_types_content
	: seq<opt<star<ignored>, one<'|'>>, star<ignored>,
		  list<union_type, seq<star<ignored>, one<'|'>, star<ignored>>>>
{
};

// https://spec.graphql.org/October2021/#UnionMemberTypes
struct union_member_types : if_must<union_member_types_start, union_member_types_content>
{
};

struct union_type_definition_start : seq<opt<description, star<ignored>>, union_keyword>
{
};

struct union_type_definition_directives : opt<star<ignored>, directives>
{
};

struct union_type_definition_content
	: seq<plus<ignored>, union_name,
		  sor<seq<union_type_definition_directives, seq<star<ignored>, union_member_types>>,
			  union_type_definition_directives>>
{
};

// https://spec.graphql.org/October2021/#UnionTypeDefinition
struct union_type_definition : if_must<union_type_definition_start, union_type_definition_content>
{
};

struct enum_keyword : TAO_PEGTL_KEYWORD("enum")
{
};

struct enum_name : name
{
};

struct enum_value_definition_start : seq<opt<description, star<ignored>>, enum_value>
{
};

struct enum_value_definition_content : opt<star<ignored>, directives>
{
};

// https://spec.graphql.org/October2021/#EnumValueDefinition
struct enum_value_definition : if_must<enum_value_definition_start, enum_value_definition_content>
{
};

struct enum_values_definition_start : one<'{'>
{
};

struct enum_values_definition_content
	: seq<star<ignored>, list<enum_value_definition, plus<ignored>>, star<ignored>, must<one<'}'>>>
{
};

// https://spec.graphql.org/October2021/#EnumValuesDefinition
struct enum_values_definition
	: if_must<enum_values_definition_start, enum_values_definition_content>
{
};

struct enum_type_definition_start : seq<opt<description, star<ignored>>, enum_keyword>
{
};

struct enum_type_definition_name : seq<plus<ignored>, enum_name>
{
};

struct enum_type_definition_directives : opt<star<ignored>, directives>
{
};

struct enum_type_definition_enum_values_definition : seq<star<ignored>, enum_values_definition>
{
};

struct enum_type_definition_content
	: seq<enum_type_definition_name,
		  sor<seq<enum_type_definition_directives, enum_type_definition_enum_values_definition>,
			  enum_type_definition_directives>>
{
};

// https://spec.graphql.org/October2021/#EnumTypeDefinition
struct enum_type_definition : if_must<enum_type_definition_start, enum_type_definition_content>
{
};

struct input_keyword : TAO_PEGTL_KEYWORD("input")
{
};

struct input_field_definition_start : seq<opt<description, star<ignored>>, argument_name>
{
};

struct input_field_definition_type_name : seq<star<ignored>, one<':'>, star<ignored>, type_name>
{
};

struct input_field_definition_default_value : opt<star<ignored>, default_value>
{
};

struct input_field_definition_directives : seq<star<ignored>, directives>
{
};

struct input_field_definition_content
	: seq<input_field_definition_type_name,
		  sor<seq<input_field_definition_default_value, input_field_definition_directives>,
			  input_field_definition_default_value>>
{
};

// https://spec.graphql.org/October2021/#InputValueDefinition
struct input_field_definition
	: if_must<input_field_definition_start, input_field_definition_content>
{
};

struct input_fields_definition_start : one<'{'>
{
};

struct input_fields_definition_content
	: seq<star<ignored>, list<input_field_definition, plus<ignored>>, star<ignored>, must<one<'}'>>>
{
};

// https://spec.graphql.org/October2021/#InputFieldsDefinition
struct input_fields_definition
	: if_must<input_fields_definition_start, input_fields_definition_content>
{
};

struct input_object_type_definition_start : seq<opt<description, star<ignored>>, input_keyword>
{
};

struct input_object_type_definition_object_name : seq<plus<ignored>, object_name>
{
};

struct input_object_type_definition_directives : opt<star<ignored>, directives>
{
};

struct input_object_type_definition_fields_definition : seq<star<ignored>, input_fields_definition>
{
};

struct input_object_type_definition_content
	: seq<input_object_type_definition_object_name,
		  sor<seq<input_object_type_definition_directives,
				  input_object_type_definition_fields_definition>,
			  input_object_type_definition_directives>>
{
};

// https://spec.graphql.org/October2021/#InputObjectTypeDefinition
struct input_object_type_definition
	: if_must<input_object_type_definition_start, input_object_type_definition_content>
{
};

// https://spec.graphql.org/October2021/#TypeDefinition
struct type_definition
	: sor<scalar_type_definition, object_type_definition, interface_type_definition,
		  union_type_definition, enum_type_definition, input_object_type_definition>
{
};

// https://spec.graphql.org/October2021/#ExecutableDirectiveLocation
struct executable_directive_location
	: sor<TAO_PEGTL_KEYWORD("QUERY"), TAO_PEGTL_KEYWORD("MUTATION"),
		  TAO_PEGTL_KEYWORD("SUBSCRIPTION"), TAO_PEGTL_KEYWORD("FIELD"),
		  TAO_PEGTL_KEYWORD("FRAGMENT_DEFINITION"), TAO_PEGTL_KEYWORD("FRAGMENT_SPREAD"),
		  TAO_PEGTL_KEYWORD("INLINE_FRAGMENT")>
{
};

// https://spec.graphql.org/October2021/#TypeSystemDirectiveLocation
struct type_system_directive_location
	: sor<TAO_PEGTL_KEYWORD("SCHEMA"), TAO_PEGTL_KEYWORD("SCALAR"), TAO_PEGTL_KEYWORD("OBJECT"),
		  TAO_PEGTL_KEYWORD("FIELD_DEFINITION"), TAO_PEGTL_KEYWORD("ARGUMENT_DEFINITION"),
		  TAO_PEGTL_KEYWORD("INTERFACE"), TAO_PEGTL_KEYWORD("UNION"), TAO_PEGTL_KEYWORD("ENUM"),
		  TAO_PEGTL_KEYWORD("ENUM_VALUE"), TAO_PEGTL_KEYWORD("INPUT_OBJECT"),
		  TAO_PEGTL_KEYWORD("INPUT_FIELD_DEFINITION")>
{
};

// https://spec.graphql.org/October2021/#DirectiveLocation
struct directive_location : sor<executable_directive_location, type_system_directive_location>
{
};

// https://spec.graphql.org/October2021/#DirectiveLocations
struct directive_locations
	: seq<opt<one<'|'>, star<ignored>>,
		  list<directive_location, seq<star<ignored>, one<'|'>, star<ignored>>>>
{
};

struct directive_definition_start
	: seq<opt<description, star<ignored>>, TAO_PEGTL_KEYWORD("directive")>
{
};

struct repeatable_keyword : TAO_PEGTL_KEYWORD("repeatable")
{
};

struct directive_definition_content
	: seq<star<ignored>, one<'@'>, directive_name, opt<star<ignored>, arguments_definition>,
		  opt<plus<ignored>, repeatable_keyword>, plus<ignored>, on_keyword, plus<ignored>,
		  directive_locations>
{
};

// https://spec.graphql.org/October2021/#DirectiveDefinition
struct directive_definition : if_must<directive_definition_start, directive_definition_content>
{
};

// https://spec.graphql.org/October2021/#TypeSystemDefinition
struct type_system_definition : sor<schema_definition, type_definition, directive_definition>
{
};

struct extend_keyword : TAO_PEGTL_KEYWORD("extend")
{
};

// https://spec.graphql.org/October2021/#OperationTypeDefinition
struct operation_type_definition : root_operation_definition
{
};

struct schema_extension_start : seq<extend_keyword, plus<ignored>, schema_keyword>
{
};

struct schema_extension_operation_type_definitions
	: seq<one<'{'>, star<ignored>, list<operation_type_definition, plus<ignored>>, star<ignored>,
		  must<one<'}'>>>
{
};

struct schema_extension_content
	: seq<star<ignored>,
		  sor<seq<opt<directives>, schema_extension_operation_type_definitions>, directives>>
{
};

// https://spec.graphql.org/October2021/#SchemaExtension
struct schema_extension : if_must<schema_extension_start, schema_extension_content>
{
};

struct scalar_type_extension_start : seq<extend_keyword, plus<ignored>, scalar_keyword>
{
};

struct scalar_type_extension_content : seq<star<ignored>, scalar_name, star<ignored>, directives>
{
};

// https://spec.graphql.org/October2021/#ScalarTypeExtension
struct scalar_type_extension : if_must<scalar_type_extension_start, scalar_type_extension_content>
{
};

struct object_type_extension_start : seq<extend_keyword, plus<ignored>, type_keyword>
{
};

struct object_type_extension_implements_interfaces : seq<plus<ignored>, implements_interfaces>
{
};

struct object_type_extension_directives : seq<star<ignored>, directives>
{
};

struct object_type_extension_fields_definition : seq<star<ignored>, fields_definition>
{
};

struct object_type_extension_content
	: seq<plus<ignored>, object_name,
		  sor<seq<opt<object_type_extension_implements_interfaces>,
				  opt<object_type_extension_directives>, object_type_extension_fields_definition>,
			  seq<opt<object_type_extension_implements_interfaces>,
				  object_type_extension_directives>,
			  object_type_extension_implements_interfaces>>
{
};

// https://spec.graphql.org/October2021/#ObjectTypeExtension
struct object_type_extension : if_must<object_type_extension_start, object_type_extension_content>
{
};

struct interface_type_extension_start : seq<extend_keyword, plus<ignored>, interface_keyword>
{
};

struct interface_type_extension_implements_interfaces : seq<plus<ignored>, implements_interfaces>
{
};

struct interface_type_extension_directives : seq<star<ignored>, directives>
{
};

struct interface_type_extension_fields_definition : seq<star<ignored>, fields_definition>
{
};

struct interface_type_extension_content
	: seq<plus<ignored>, interface_name, star<ignored>,
		  sor<seq<opt<interface_type_extension_implements_interfaces>,
				  opt<interface_type_extension_directives>,
				  interface_type_extension_fields_definition>,
			  seq<opt<interface_type_extension_implements_interfaces>,
				  interface_type_extension_directives>,
			  interface_type_extension_implements_interfaces>>
{
};

// https://spec.graphql.org/October2021/#InterfaceTypeExtension
struct interface_type_extension
	: if_must<interface_type_extension_start, interface_type_extension_content>
{
};

struct union_type_extension_start : seq<extend_keyword, plus<ignored>, union_keyword>
{
};

struct union_type_extension_content
	: seq<plus<ignored>, union_name, star<ignored>,
		  sor<seq<opt<directives, star<ignored>>, union_member_types>, directives>>
{
};

// https://spec.graphql.org/October2021/#UnionTypeExtension
struct union_type_extension : if_must<union_type_extension_start, union_type_extension_content>
{
};

struct enum_type_extension_start : seq<extend_keyword, plus<ignored>, enum_keyword>
{
};

struct enum_type_extension_content
	: seq<plus<ignored>, enum_name, star<ignored>,
		  sor<seq<opt<directives, star<ignored>>, enum_values_definition>, directives>>
{
};

// https://spec.graphql.org/October2021/#EnumTypeExtension
struct enum_type_extension : if_must<enum_type_extension_start, enum_type_extension_content>
{
};

struct input_object_type_extension_start : seq<extend_keyword, plus<ignored>, input_keyword>
{
};

struct input_object_type_extension_content
	: seq<plus<ignored>, object_name, star<ignored>,
		  sor<seq<opt<directives, star<ignored>>, input_fields_definition>, directives>>
{
};

// https://spec.graphql.org/October2021/#InputObjectTypeExtension
struct input_object_type_extension
	: if_must<input_object_type_extension_start, input_object_type_extension_content>
{
};

// https://spec.graphql.org/October2021/#TypeExtension
struct type_extension
	: sor<scalar_type_extension, object_type_extension, interface_type_extension,
		  union_type_extension, enum_type_extension, input_object_type_extension>
{
};

// https://spec.graphql.org/October2021/#TypeSystemExtension
struct type_system_extension : sor<schema_extension, type_extension>
{
};

// https://spec.graphql.org/October2021/#Definition
struct mixed_definition : sor<executable_definition, type_system_definition, type_system_extension>
{
};

struct mixed_document_content
	: seq<bof, opt<utf8::bom>, star<ignored>,	 // leading whitespace/ignored
		  list<mixed_definition, star<ignored>>, // mixed definitions
		  star<ignored>, tao::graphqlpeg::eof>	 // trailing whitespace/ignored
{
};

// https://spec.graphql.org/October2021/#Document
struct mixed_document : must<mixed_document_content>
{
};

struct executable_document_content
	: seq<bof, opt<utf8::bom>, star<ignored>,		  // leading whitespace/ignored
		  list<executable_definition, star<ignored>>, // executable definitions
		  star<ignored>, tao::graphqlpeg::eof>		  // trailing whitespace/ignored
{
};

// https://spec.graphql.org/October2021/#Document
struct executable_document : must<executable_document_content>
{
};

// https://spec.graphql.org/October2021/#Definition
struct schema_type_definition : sor<type_system_definition, type_system_extension>
{
};

struct schema_document_content
	: seq<bof, opt<utf8::bom>, star<ignored>,		   // leading whitespace/ignored
		  list<schema_type_definition, star<ignored>>, // schema type definitions
		  star<ignored>, tao::graphqlpeg::eof>		   // trailing whitespace/ignored
{
};

// https://spec.graphql.org/October2021/#Document
struct schema_document : must<schema_document_content>
{
};

} // namespace graphql::peg

#endif // GRAPHQLGRAMMAR_H
