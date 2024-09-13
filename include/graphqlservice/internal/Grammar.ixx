// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Grammar.h"

export module GraphQL.Internal.Grammar;

namespace included = graphql::peg;

export namespace graphql::peg {

namespace exported {

using namespace tao::graphqlpeg;

template <typename Rule>
inline void for_each_child(const ast_node& n, std::function<void(const ast_node&)>&& func)
{
	included::for_each_child<Rule>(n, std::move(func));
}

template <typename Rule>
inline void on_first_child_if(const ast_node& n, std::function<bool(const ast_node&)>&& func)
{
	included::on_first_child_if<Rule>(n, std::move(func));
}

template <typename Rule>
inline void on_first_child(const ast_node& n, std::function<void(const ast_node&)>&& func)
{
	included::on_first_child<Rule>(n, std::move(func));
}

using source_character = included::source_character;
using comment = included::comment;
using ignored = included::ignored;
using name = included::name;
using variable_name_content = included::variable_name_content;
using variable_name = included::variable_name;
using null_keyword = included::null_keyword;
using quote_token = included::quote_token;
using backslash_token = included::backslash_token;
using escaped_unicode_codepoint = included::escaped_unicode_codepoint;
using escaped_unicode_content = included::escaped_unicode_content;
using escaped_unicode = included::escaped_unicode;
using escaped_char = included::escaped_char;
using string_escape_sequence_content = included::string_escape_sequence_content;
using string_escape_sequence = included::string_escape_sequence;
using string_quote_character = included::string_quote_character;
using string_quote_content = included::string_quote_content;
using string_quote = included::string_quote;
using block_quote_token = included::block_quote_token;
using block_escape_sequence = included::block_escape_sequence;
using block_quote_character = included::block_quote_character;
using block_quote_empty_line = included::block_quote_empty_line;
using block_quote_line_content = included::block_quote_line_content;
using block_quote_line = included::block_quote_line;
using block_quote_content_lines = included::block_quote_content_lines;
using block_quote_content = included::block_quote_content;
using block_quote = included::block_quote;
using string_value = included::string_value;
using nonzero_digit = included::nonzero_digit;
using zero_digit = included::zero_digit;
using negative_sign = included::negative_sign;
using integer_part = included::integer_part;
using integer_value = included::integer_value;
using fractional_part_content = included::fractional_part_content;
using fractional_part = included::fractional_part;
using exponent_indicator = included::exponent_indicator;
using sign = included::sign;
using exponent_part_content = included::exponent_part_content;
using exponent_part = included::exponent_part;
using float_value = included::float_value;
using true_keyword = included::true_keyword;
using false_keyword = included::false_keyword;
using bool_value = included::bool_value;
using enum_value = included::enum_value;
using operation_type = included::operation_type;
using alias_name = included::alias_name;
using alias = included::alias;
using argument_name = included::argument_name;
using argument_content = included::argument_content;
using argument = included::argument;
using arguments_content = included::arguments_content;
using arguments = included::arguments;
using list_value_content = included::list_value_content;
using list_value = included::list_value;
using object_field_name = included::object_field_name;
using object_field_content = included::object_field_content;
using object_field = included::object_field;
using object_value_content = included::object_value_content;
using object_value = included::object_value;
using variable_value = included::variable_value;
using input_value_content = included::input_value_content;
using input_value = included::input_value;
using list_entry = included::list_entry;
using default_value_content = included::default_value_content;
using default_value = included::default_value;
using named_type = included::named_type;
using list_type_content = included::list_type_content;
using list_type = included::list_type;
using nonnull_type = included::nonnull_type;
using type_name_content = included::type_name_content;
using type_name = included::type_name;
using variable_content = included::variable_content;
using variable = included::variable;
using variable_definitions_content = included::variable_definitions_content;
using variable_definitions = included::variable_definitions;
using directive_name = included::directive_name;
using directive_content = included::directive_content;
using directive = included::directive;
using directives = included::directives;
using field_name = included::field_name;
using field_start = included::field_start;
using field_arguments = included::field_arguments;
using field_directives = included::field_directives;
using field_selection_set = included::field_selection_set;
using field_content = included::field_content;
using field = included::field;
using on_keyword = included::on_keyword;
using fragment_name = included::fragment_name;
using fragment_token = included::fragment_token;
using fragment_spread = included::fragment_spread;
using type_condition_content = included::type_condition_content;
using type_condition = included::type_condition;
using inline_fragment = included::inline_fragment;
using fragement_spread_or_inline_fragment_content =
	included::fragement_spread_or_inline_fragment_content;
using fragement_spread_or_inline_fragment = included::fragement_spread_or_inline_fragment;
using selection = included::selection;
using selection_set_content = included::selection_set_content;
using selection_set = included::selection_set;
using operation_name = included::operation_name;
using operation_definition_operation_type_content =
	included::operation_definition_operation_type_content;
using operation_definition = included::operation_definition;
using fragment_definition_content = included::fragment_definition_content;
using fragment_definition = included::fragment_definition;
using executable_definition = included::executable_definition;
using description = included::description;
using schema_keyword = included::schema_keyword;
using root_operation_definition_content = included::root_operation_definition_content;
using root_operation_definition = included::root_operation_definition;
using schema_definition_start = included::schema_definition_start;
using schema_definition_content = included::schema_definition_content;
using schema_definition = included::schema_definition;
using scalar_keyword = included::scalar_keyword;
using scalar_name = included::scalar_name;
using scalar_type_definition_start = included::scalar_type_definition_start;
using scalar_type_definition_content = included::scalar_type_definition_content;
using scalar_type_definition = included::scalar_type_definition;
using type_keyword = included::type_keyword;
using arguments_definition_start = included::arguments_definition_start;
using arguments_definition_content = included::arguments_definition_content;
using arguments_definition = included::arguments_definition;
using field_definition_start = included::field_definition_start;
using field_definition_content = included::field_definition_content;
using field_definition = included::field_definition;
using fields_definition_content = included::fields_definition_content;
using fields_definition = included::fields_definition;
using interface_type = included::interface_type;
using implements_interfaces_content = included::implements_interfaces_content;
using implements_interfaces = included::implements_interfaces;
using object_name = included::object_name;
using object_type_definition_start = included::object_type_definition_start;
using object_type_definition_object_name = included::object_type_definition_object_name;
using object_type_definition_implements_interfaces =
	included::object_type_definition_implements_interfaces;
using object_type_definition_directives = included::object_type_definition_directives;
using object_type_definition_fields_definition = included::object_type_definition_fields_definition;
using object_type_definition_content = included::object_type_definition_content;
using object_type_definition = included::object_type_definition;
using interface_keyword = included::interface_keyword;
using interface_name = included::interface_name;
using interface_type_definition_start = included::interface_type_definition_start;
using interface_type_definition_interface_name = included::interface_type_definition_interface_name;
using interface_type_definition_implements_interfaces =
	included::interface_type_definition_implements_interfaces;
using interface_type_definition_directives = included::interface_type_definition_directives;
using interface_type_definition_fields_definition =
	included::interface_type_definition_fields_definition;
using interface_type_definition_content = included::interface_type_definition_content;
using interface_type_definition = included::interface_type_definition;
using union_keyword = included::union_keyword;
using union_name = included::union_name;
using union_type = included::union_type;
using union_member_types_start = included::union_member_types_start;
using union_member_types_content = included::union_member_types_content;
using union_member_types = included::union_member_types;
using union_type_definition_start = included::union_type_definition_start;
using union_type_definition_directives = included::union_type_definition_directives;
using union_type_definition_content = included::union_type_definition_content;
using union_type_definition = included::union_type_definition;
using enum_keyword = included::enum_keyword;
using enum_name = included::enum_name;
using enum_value_definition_start = included::enum_value_definition_start;
using enum_value_definition_content = included::enum_value_definition_content;
using enum_value_definition = included::enum_value_definition;
using enum_values_definition_start = included::enum_values_definition_start;
using enum_values_definition_content = included::enum_values_definition_content;
using enum_values_definition = included::enum_values_definition;
using enum_type_definition_start = included::enum_type_definition_start;
using enum_type_definition_name = included::enum_type_definition_name;
using enum_type_definition_directives = included::enum_type_definition_directives;
using enum_type_definition_enum_values_definition =
	included::enum_type_definition_enum_values_definition;
using enum_type_definition_content = included::enum_type_definition_content;
using enum_type_definition = included::enum_type_definition;
using input_keyword = included::input_keyword;
using input_field_definition_start = included::input_field_definition_start;
using input_field_definition_type_name = included::input_field_definition_type_name;
using input_field_definition_default_value = included::input_field_definition_default_value;
using input_field_definition_directives = included::input_field_definition_directives;
using input_field_definition_content = included::input_field_definition_content;
using input_field_definition = included::input_field_definition;
using input_fields_definition_start = included::input_fields_definition_start;
using input_fields_definition_content = included::input_fields_definition_content;
using input_fields_definition = included::input_fields_definition;
using input_object_type_definition_start = included::input_object_type_definition_start;
using input_object_type_definition_object_name = included::input_object_type_definition_object_name;
using input_object_type_definition_directives = included::input_object_type_definition_directives;
using input_object_type_definition_fields_definition =
	included::input_object_type_definition_fields_definition;
using input_object_type_definition_content = included::input_object_type_definition_content;
using input_object_type_definition = included::input_object_type_definition;
using type_definition = included::type_definition;
using executable_directive_location = included::executable_directive_location;
using type_system_directive_location = included::type_system_directive_location;
using directive_location = included::directive_location;
using directive_locations = included::directive_locations;
using directive_definition_start = included::directive_definition_start;
using repeatable_keyword = included::repeatable_keyword;
using directive_definition_content = included::directive_definition_content;
using directive_definition = included::directive_definition;
using type_system_definition = included::type_system_definition;
using extend_keyword = included::extend_keyword;
using operation_type_definition = included::operation_type_definition;
using schema_extension_start = included::schema_extension_start;
using schema_extension_operation_type_definitions =
	included::schema_extension_operation_type_definitions;
using schema_extension_content = included::schema_extension_content;
using schema_extension = included::schema_extension;
using scalar_type_extension_start = included::scalar_type_extension_start;
using scalar_type_extension_content = included::scalar_type_extension_content;
using scalar_type_extension = included::scalar_type_extension;
using object_type_extension_start = included::object_type_extension_start;
using object_type_extension_implements_interfaces =
	included::object_type_extension_implements_interfaces;
using object_type_extension_directives = included::object_type_extension_directives;
using object_type_extension_fields_definition = included::object_type_extension_fields_definition;
using object_type_extension_content = included::object_type_extension_content;
using object_type_extension = included::object_type_extension;
using interface_type_extension_start = included::interface_type_extension_start;
using interface_type_extension_implements_interfaces =
	included::interface_type_extension_implements_interfaces;
using interface_type_extension_directives = included::interface_type_extension_directives;
using interface_type_extension_fields_definition =
	included::interface_type_extension_fields_definition;
using interface_type_extension_content = included::interface_type_extension_content;
using interface_type_extension = included::interface_type_extension;
using union_type_extension_start = included::union_type_extension_start;
using union_type_extension_content = included::union_type_extension_content;
using union_type_extension = included::union_type_extension;
using enum_type_extension_start = included::enum_type_extension_start;
using enum_type_extension_content = included::enum_type_extension_content;
using enum_type_extension = included::enum_type_extension;
using input_object_type_extension_start = included::input_object_type_extension_start;
using input_object_type_extension_content = included::input_object_type_extension_content;
using input_object_type_extension = included::input_object_type_extension;
using type_extension = included::type_extension;
using type_system_extension = included::type_system_extension;
using mixed_definition = included::mixed_definition;
using mixed_document_content = included::mixed_document_content;
using mixed_document = included::mixed_document;
using executable_document_content = included::executable_document_content;
using executable_document = included::executable_document;
using schema_type_definition = included::schema_type_definition;
using schema_document_content = included::schema_document_content;
using schema_document = included::schema_document;

} // namespace exported

using namespace exported;

} // namespace graphql::peg
