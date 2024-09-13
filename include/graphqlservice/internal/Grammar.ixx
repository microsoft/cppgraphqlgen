// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Grammar.h"

export module GraphQL.Internal.Grammar;

namespace included = graphql::peg;

export namespace graphql::peg {

namespace exported {

// clang-format off
using namespace tao::graphqlpeg;

using included::for_each_child;
using included::on_first_child;
using included::on_first_child_if;

using included::alias;
using included::alias_name;
using included::argument;
using included::argument_content;
using included::argument_name;
using included::arguments;
using included::arguments_content;
using included::backslash_token;
using included::block_escape_sequence;
using included::block_quote;
using included::block_quote_character;
using included::block_quote_content;
using included::block_quote_content_lines;
using included::block_quote_empty_line;
using included::block_quote_line;
using included::block_quote_line_content;
using included::block_quote_token;
using included::bool_value;
using included::comment;
using included::default_value;
using included::default_value_content;
using included::directive;
using included::directive_content;
using included::directive_name;
using included::directives;
using included::enum_value;
using included::escaped_char;
using included::escaped_unicode;
using included::escaped_unicode_codepoint;
using included::escaped_unicode_content;
using included::exponent_indicator;
using included::exponent_part;
using included::exponent_part_content;
using included::false_keyword;
using included::field;
using included::field_arguments;
using included::field_content;
using included::field_directives;
using included::field_name;
using included::field_selection_set;
using included::field_start;
using included::float_value;
using included::fractional_part;
using included::fractional_part_content;
using included::fragment_name;
using included::fragment_spread;
using included::fragment_token;
using included::ignored;
using included::inline_fragment;
using included::input_value;
using included::input_value_content;
using included::integer_part;
using included::integer_value;
using included::list_entry;
using included::list_type;
using included::list_type_content;
using included::list_value;
using included::list_value_content;
using included::name;
using included::named_type;
using included::negative_sign;
using included::nonnull_type;
using included::nonzero_digit;
using included::null_keyword;
using included::object_field;
using included::object_field_content;
using included::object_field_name;
using included::object_value;
using included::object_value_content;
using included::on_keyword;
using included::operation_type;
using included::quote_token;
using included::sign;
using included::source_character;
using included::string_escape_sequence;
using included::string_escape_sequence_content;
using included::string_quote;
using included::string_quote_character;
using included::string_quote_content;
using included::string_value;
using included::true_keyword;
using included::type_condition;
using included::type_condition_content;
using included::type_name;
using included::type_name_content;
using included::variable;
using included::variable_content;
using included::variable_definitions;
using included::variable_definitions_content;
using included::variable_name;
using included::variable_name_content;
using included::variable_value;
using included::zero_digit;
using included::fragement_spread_or_inline_fragment_content;
using included::fragement_spread_or_inline_fragment;
using included::operation_name;
using included::selection;
using included::selection_set;
using included::selection_set_content;
using included::operation_definition_operation_type_content;
using included::arguments_definition;
using included::arguments_definition_content;
using included::arguments_definition_start;
using included::description;
using included::executable_definition;
using included::field_definition;
using included::field_definition_content;
using included::field_definition_start;
using included::fields_definition;
using included::fields_definition_content;
using included::fragment_definition;
using included::fragment_definition_content;
using included::implements_interfaces;
using included::implements_interfaces_content;
using included::interface_type;
using included::object_name;
using included::object_type_definition_object_name;
using included::object_type_definition_start;
using included::operation_definition;
using included::root_operation_definition;
using included::root_operation_definition_content;
using included::scalar_keyword;
using included::scalar_name;
using included::scalar_type_definition;
using included::scalar_type_definition_content;
using included::scalar_type_definition_start;
using included::schema_definition;
using included::schema_definition_content;
using included::schema_definition_start;
using included::schema_keyword;
using included::type_keyword;
using included::object_type_definition_implements_interfaces;
using included::interface_keyword;
using included::interface_name;
using included::interface_type_definition_interface_name;
using included::interface_type_definition_start;
using included::object_type_definition;
using included::object_type_definition_content;
using included::object_type_definition_directives;
using included::object_type_definition_fields_definition;
using included::interface_type_definition_implements_interfaces;
using included::interface_type_definition_directives;
using included::interface_type_definition_fields_definition;
using included::enum_keyword;
using included::enum_name;
using included::enum_type_definition_directives;
using included::enum_type_definition_name;
using included::enum_type_definition_start;
using included::enum_value_definition;
using included::enum_value_definition_content;
using included::enum_value_definition_start;
using included::enum_values_definition;
using included::enum_values_definition_content;
using included::enum_values_definition_start;
using included::interface_type_definition;
using included::interface_type_definition_content;
using included::union_keyword;
using included::union_member_types;
using included::union_member_types_content;
using included::union_member_types_start;
using included::union_name;
using included::union_type;
using included::union_type_definition;
using included::union_type_definition_content;
using included::union_type_definition_directives;
using included::union_type_definition_start;
using included::enum_type_definition_enum_values_definition;
using included::enum_type_definition;
using included::enum_type_definition_content;
using included::input_field_definition;
using included::input_field_definition_content;
using included::input_field_definition_default_value;
using included::input_field_definition_directives;
using included::input_field_definition_start;
using included::input_field_definition_type_name;
using included::input_fields_definition;
using included::input_fields_definition_content;
using included::input_fields_definition_start;
using included::input_keyword;
using included::input_object_type_definition_directives;
using included::input_object_type_definition_object_name;
using included::input_object_type_definition_start;
using included::input_object_type_definition_fields_definition;
using included::directive_definition;
using included::directive_definition_content;
using included::directive_definition_start;
using included::directive_location;
using included::directive_locations;
using included::executable_directive_location;
using included::extend_keyword;
using included::input_object_type_definition;
using included::input_object_type_definition_content;
using included::operation_type_definition;
using included::repeatable_keyword;
using included::schema_extension_start;
using included::type_definition;
using included::type_system_definition;
using included::type_system_directive_location;
using included::schema_extension_operation_type_definitions;
using included::object_type_extension_start;
using included::scalar_type_extension;
using included::scalar_type_extension_content;
using included::scalar_type_extension_start;
using included::schema_extension;
using included::schema_extension_content;
using included::object_type_extension_implements_interfaces;
using included::interface_type_extension_start;
using included::object_type_extension;
using included::object_type_extension_content;
using included::object_type_extension_directives;
using included::object_type_extension_fields_definition;
using included::interface_type_extension_implements_interfaces;
using included::interface_type_extension_directives;
using included::interface_type_extension_fields_definition;
using included::enum_type_extension;
using included::enum_type_extension_content;
using included::enum_type_extension_start;
using included::executable_document;
using included::executable_document_content;
using included::input_object_type_extension;
using included::input_object_type_extension_content;
using included::input_object_type_extension_start;
using included::interface_type_extension;
using included::interface_type_extension_content;
using included::mixed_definition;
using included::mixed_document;
using included::mixed_document_content;
using included::schema_document;
using included::schema_document_content;
using included::schema_type_definition;
using included::type_extension;
using included::type_system_extension;
using included::union_type_extension;
using included::union_type_extension_content;
using included::union_type_extension_start;
// clang-format on

} // namespace exported

using namespace exported;

} // namespace graphql::peg
