// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLParse.h"

#include "graphqlservice/internal/Grammar.h"
#include "graphqlservice/internal/SyntaxTree.h"

#include <tao/pegtl/contrib/unescape.hpp>

#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <utility>

using namespace std::literals;

namespace graphql {
namespace peg {

void ast_node::unescaped_view(std::string_view unescaped) noexcept
{
	_unescaped = std::make_unique<unescaped_t>(unescaped);
}

std::string_view ast_node::unescaped_view() const
{
	if (!_unescaped)
	{
		if (is_type<block_quote_content_lines>())
		{
			// Trim leading and trailing empty lines
			const auto isNonEmptyLine = [](const std::unique_ptr<ast_node>& child) noexcept {
				return child->is_type<block_quote_line>();
			};
			const auto itrEndRev = std::make_reverse_iterator(
				std::find_if(children.cbegin(), children.cend(), isNonEmptyLine));
			const auto itrRev = std::find_if(children.crbegin(), itrEndRev, isNonEmptyLine);
			std::vector<std::optional<std::pair<std::string_view, std::string_view>>> lines(
				std::distance(itrRev, itrEndRev));

			std::transform(itrRev,
				itrEndRev,
				lines.rbegin(),
				[](const std::unique_ptr<ast_node>& child) noexcept {
					return (child->is_type<block_quote_line>() && !child->children.empty()
							   && child->children.front()->is_type<block_quote_empty_line>()
							   && child->children.back()->is_type<block_quote_line_content>())
						? std::make_optional(std::make_pair(child->children.front()->string_view(),
							child->children.back()->unescaped_view()))
						: std::nullopt;
				});

			// Calculate the common indent
			const auto commonIndent = std::accumulate(lines.cbegin(),
				lines.cend(),
				std::optional<size_t> {},
				[](auto value, const auto& line) noexcept {
					if (line)
					{
						const auto indent = line->first.size();

						if (!value || indent < *value)
						{
							value = indent;
						}
					}

					return value;
				});

			const auto trimIndent = commonIndent ? *commonIndent : 0;
			std::string joined;

			if (!lines.empty())
			{
				joined.reserve(std::accumulate(lines.cbegin(),
								   lines.cend(),
								   size_t {},
								   [trimIndent](auto value, const auto& line) noexcept {
									   if (line)
									   {
										   value += line->first.size() - trimIndent;
										   value += line->second.size();
									   }

									   return value;
								   })
					+ lines.size() - 1);

				bool firstLine = true;

				for (const auto& line : lines)
				{
					if (!firstLine)
					{
						joined.append(1, '\n');
					}

					if (line)
					{
						joined.append(line->first.substr(trimIndent));
						joined.append(line->second);
					}

					firstLine = false;
				}
			}

			_unescaped = std::make_unique<unescaped_t>(std::move(joined));
		}
		else if (children.size() > 1)
		{
			std::string joined;

			joined.reserve(std::accumulate(children.cbegin(),
				children.cend(),
				size_t(0),
				[](size_t total, const std::unique_ptr<ast_node>& child) {
					return total + child->string_view().size();
				}));

			for (const auto& child : children)
			{
				joined.append(child->string_view());
			}

			_unescaped = std::make_unique<unescaped_t>(std::move(joined));
		}
		else if (!children.empty())
		{
			_unescaped = std::make_unique<unescaped_t>(children.front()->string_view());
		}
		else if (has_content() && is_type<escaped_unicode>())
		{
			const auto content = string_view();
			memory_input<> in(content.data(), content.size(), "escaped unicode");
			std::string utf8;

			utf8.reserve((content.size() + 1) / 2);
			unescape::unescape_j::apply(in, utf8);

			_unescaped = std::make_unique<unescaped_t>(std::move(utf8));
		}
		else
		{
			_unescaped = std::make_unique<unescaped_t>(std::string_view {});
		}
	}

	return std::visit(
		[](const auto& value) noexcept {
			return std::string_view { value };
		},
		*_unescaped);
}

void ast_node::remove_content() noexcept
{
	basic_node_t::remove_content();
	_unescaped.reset();
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
					n->unescaped_view("\""sv);
					return;

				case '\\':
					n->unescaped_view("\\"sv);
					return;

				case '/':
					n->unescaped_view("/"sv);
					return;

				case 'b':
					n->unescaped_view("\b"sv);
					return;

				case 'f':
					n->unescaped_view("\f"sv);
					return;

				case 'n':
					n->unescaped_view("\n"sv);
					return;

				case 'r':
					n->unescaped_view("\r"sv);
					return;

				case 't':
					n->unescaped_view("\t"sv);
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
		n->unescaped_view(n->string_view());
	}
};

template <>
struct ast_selector<block_escape_sequence> : std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		n->unescaped_view(R"bq(""")bq"sv);
	}
};

template <>
struct ast_selector<block_quote_content_lines> : std::true_type
{
};

template <>
struct ast_selector<block_quote_empty_line> : std::true_type
{
};

template <>
struct ast_selector<block_quote_line> : std::true_type
{
};

template <>
struct ast_selector<block_quote_line_content> : std::true_type
{
};

template <>
struct ast_selector<block_quote_character> : std::true_type
{
	static void transform(std::unique_ptr<ast_node>& n)
	{
		n->unescaped_view(n->string_view());
	}
};

template <>
struct ast_selector<string_value> : std::true_type
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
struct ast_selector<field_name> : std::true_type
{
};

template <>
struct ast_selector<argument_name> : std::true_type
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
struct ast_selector<directive_name> : std::true_type
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
struct ast_selector<variable> : std::true_type
{
};

template <>
struct ast_selector<scalar_name> : std::true_type
{
};

template <>
struct ast_selector<named_type> : std::true_type
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
struct ast_selector<operation_definition> : std::true_type
{
};

template <>
struct ast_selector<fragment_definition> : std::true_type
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
struct ast_selector<object_type_definition> : std::true_type
{
};

template <>
struct ast_selector<interface_type_definition> : std::true_type
{
};

template <>
struct ast_selector<union_type_definition> : std::true_type
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
struct ast_selector<directive_definition> : std::true_type
{
};

template <>
struct ast_selector<schema_extension> : std::true_type
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
struct schema_selector : ast_selector<Rule>
{
};

template <>
struct schema_selector<description> : std::true_type
{
};

template <>
struct schema_selector<object_name> : std::true_type
{
};

template <>
struct schema_selector<interface_name> : std::true_type
{
};

template <>
struct schema_selector<union_name> : std::true_type
{
};

template <>
struct schema_selector<enum_name> : std::true_type
{
};

template <>
struct schema_selector<root_operation_definition> : std::true_type
{
};

template <>
struct schema_selector<interface_type> : std::true_type
{
};

template <>
struct schema_selector<input_field_definition> : std::true_type
{
};

template <>
struct schema_selector<input_fields_definition> : std::true_type
{
};

template <>
struct schema_selector<arguments_definition> : std::true_type
{
};

template <>
struct schema_selector<field_definition> : std::true_type
{
};

template <>
struct schema_selector<fields_definition> : std::true_type
{
};

template <>
struct schema_selector<union_type> : std::true_type
{
};

template <>
struct schema_selector<enum_value_definition> : std::true_type
{
};

template <>
struct schema_selector<repeatable_keyword> : std::true_type
{
};

template <>
struct schema_selector<directive_location> : std::true_type
{
};

template <>
struct schema_selector<operation_type_definition> : std::true_type
{
};

template <typename Rule>
struct executable_selector : ast_selector<Rule>
{
};

template <>
struct executable_selector<variable_name> : std::true_type
{
};

template <>
struct executable_selector<alias_name> : std::true_type
{
};

template <>
struct executable_selector<alias> : parse_tree::fold_one
{
};

template <>
struct executable_selector<operation_name> : std::true_type
{
};

template <>
struct executable_selector<fragment_name> : std::true_type
{
};

template <>
struct executable_selector<field> : std::true_type
{
};

template <>
struct executable_selector<fragment_spread> : std::true_type
{
};

template <>
struct executable_selector<inline_fragment> : std::true_type
{
};

template <>
struct executable_selector<selection_set> : std::true_type
{
};

template <>
struct executable_selector<type_condition> : std::true_type
{
};

template <typename Rule>
struct ast_action : nothing<Rule>
{
};

struct [[nodiscard]] depth_guard
{
	explicit depth_guard(size_t& depth) noexcept
		: _depth(depth)
	{
		++_depth;
	}

	~depth_guard()
	{
		--_depth;
	}

	depth_guard(depth_guard&&) noexcept = delete;
	depth_guard(const depth_guard&) = delete;

	depth_guard& operator=(depth_guard&&) noexcept = delete;
	depth_guard& operator=(const depth_guard&) = delete;

private:
	size_t& _depth;
};

template <>
struct ast_action<selection_set> : maybe_nothing
{
	template <typename Rule, apply_mode A, rewind_mode M, template <typename...> class Action,
		template <typename...> class Control, typename ParseInput, typename... States>
	[[nodiscard]] static bool match(ParseInput& in, States&&... st)
	{
		depth_guard guard(in.selectionSetDepth);
		if (in.selectionSetDepth > in.depthLimit())
		{
			std::ostringstream oss;

			oss << "Exceeded nested depth limit: " << in.depthLimit()
				<< " for https://spec.graphql.org/October2021/#SelectionSet";

			throw parse_error(oss.str(), in);
		}

		return tao::graphqlpeg::template match<Rule, A, M, Action, Control>(in, st...);
	}
};

template <typename Rule>
struct ast_control : normal<Rule>
{
	static const char* error_message;

	template <typename Input, typename... State>
	[[noreturn]] static void raise(const Input& in, State&&...)
	{
		throw parse_error(error_message, in);
	}
};

template <>
const char* ast_control<one<'}'>>::error_message = "Expected }";
template <>
const char* ast_control<one<']'>>::error_message = "Expected ]";
template <>
const char* ast_control<one<')'>>::error_message = "Expected )";
template <>
const char* ast_control<quote_token>::error_message = "Expected \"";
template <>
const char* ast_control<block_quote_token>::error_message = "Expected \"\"\"";

template <>
const char* ast_control<variable_name_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#Variable";
template <>
const char* ast_control<escaped_unicode_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#EscapedUnicode";
template <>
const char* ast_control<string_escape_sequence_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#EscapedCharacter";
template <>
const char* ast_control<string_quote_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#StringCharacter";
template <>
const char* ast_control<block_quote_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#BlockStringCharacter";
template <>
const char* ast_control<fractional_part_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#FractionalPart";
template <>
const char* ast_control<exponent_part_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ExponentPart";
template <>
const char* ast_control<argument_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#Argument";
template <>
const char* ast_control<arguments_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#Arguments";
template <>
const char* ast_control<list_value_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ListValue";
template <>
const char* ast_control<object_field_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ObjectField";
template <>
const char* ast_control<object_value_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ObjectValue";
template <>
const char* ast_control<input_value_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#Value";
template <>
const char* ast_control<default_value_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#DefaultValue";
template <>
const char* ast_control<list_type_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ListType";
template <>
const char* ast_control<type_name_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#Type";
template <>
const char* ast_control<variable_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#VariableDefinition";
template <>
const char* ast_control<variable_definitions_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#VariableDefinitions";
template <>
const char* ast_control<directive_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#Directive";
template <>
const char* ast_control<field_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#Field";
template <>
const char* ast_control<type_condition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#TypeCondition";
template <>
const char* ast_control<fragement_spread_or_inline_fragment_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#FragmentSpread or "
	"https://spec.graphql.org/October2021/#InlineFragment";
template <>
const char* ast_control<selection_set_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#SelectionSet";
template <>
const char* ast_control<operation_definition_operation_type_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#OperationDefinition";
template <>
const char* ast_control<fragment_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#FragmentDefinition";
template <>
const char* ast_control<root_operation_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#RootOperationTypeDefinition";
template <>
const char* ast_control<schema_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#SchemaDefinition";
template <>
const char* ast_control<scalar_type_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ScalarTypeDefinition";
template <>
const char* ast_control<arguments_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ArgumentsDefinition";
template <>
const char* ast_control<field_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#FieldDefinition";
template <>
const char* ast_control<fields_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#FieldsDefinition";
template <>
const char* ast_control<implements_interfaces_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ImplementsInterfaces";
template <>
const char* ast_control<object_type_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ObjectTypeDefinition";
template <>
const char* ast_control<interface_type_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#InterfaceTypeDefinition";
template <>
const char* ast_control<union_member_types_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#UnionMemberTypes";
template <>
const char* ast_control<union_type_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#UnionTypeDefinition";
template <>
const char* ast_control<enum_value_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#EnumValueDefinition";
template <>
const char* ast_control<enum_values_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#EnumValuesDefinition";
template <>
const char* ast_control<enum_type_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#EnumTypeDefinition";
template <>
const char* ast_control<input_field_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#InputValueDefinition";
template <>
const char* ast_control<input_fields_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#InputFieldsDefinition";
template <>
const char* ast_control<input_object_type_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#InputObjectTypeDefinition";
template <>
const char* ast_control<directive_definition_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#DirectiveDefinition";
template <>
const char* ast_control<schema_extension_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#SchemaExtension";
template <>
const char* ast_control<scalar_type_extension_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ScalarTypeExtension";
template <>
const char* ast_control<object_type_extension_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#ObjectTypeExtension";
template <>
const char* ast_control<interface_type_extension_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#InterfaceTypeExtension";
template <>
const char* ast_control<union_type_extension_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#UnionTypeExtension";
template <>
const char* ast_control<enum_type_extension_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#EnumTypeExtension";
template <>
const char* ast_control<input_object_type_extension_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#InputObjectTypeExtension";
template <>
const char* ast_control<mixed_document_content>::error_message =
	"Expected https://spec.graphql.org/October2021/#Document";
template <>
const char* ast_control<executable_document_content>::error_message =
	"Expected executable https://spec.graphql.org/October2021/#Document";
template <>
const char* ast_control<schema_document_content>::error_message =
	"Expected schema type https://spec.graphql.org/October2021/#Document";

namespace graphql_parse_tree {
namespace internal {

using ast_state = parse_tree::internal::state<ast_node>;

template <template <typename...> class Selector>
struct make_control
{
	template <typename Rule, bool, bool>
	struct state_handler;

	template <typename Rule>
	using type = state_handler<Rule, parse_tree::internal::is_selected_node<Rule, Selector>,
		parse_tree::internal::is_leaf<8, typename Rule::subs_t, Selector>>;
};

template <template <typename...> class Selector>
template <typename Rule>
struct make_control<Selector>::state_handler<Rule, false, true> : ast_control<Rule>
{
};

template <template <typename...> class Selector>
template <typename Rule>
struct make_control<Selector>::state_handler<Rule, false, false> : ast_control<Rule>
{
	static constexpr bool enable = true;

	template <typename ParseInput>
	static void start(const ParseInput& /*unused*/, ast_state& state)
	{
		state.emplace_back();
	}

	template <typename ParseInput>
	static void success(const ParseInput& /*unused*/, ast_state& state)
	{
		auto n = std::move(state.back());
		state.pop_back();
		for (auto& c : n->children)
		{
			state.back()->children.emplace_back(std::move(c));
		}
	}

	template <typename ParseInput>
	static void failure(const ParseInput& /*unused*/, ast_state& state)
	{
		state.pop_back();
	}
};

template <template <typename...> class Selector>
template <typename Rule, bool B>
struct make_control<Selector>::state_handler<Rule, true, B> : ast_control<Rule>
{
	template <typename ParseInput>
	static void start(const ParseInput& in, ast_state& state)
	{
		state.emplace_back();
		state.back()->template start<Rule>(in);
	}

	template <typename ParseInput>
	static void success(const ParseInput& in, ast_state& state)
	{
		auto n = std::move(state.back());
		state.pop_back();
		n->template success<Rule>(in);
		parse_tree::internal::transform<Selector<Rule>>(in, n);
		if (n)
		{
			state.back()->children.emplace_back(std::move(n));
		}
	}

	template <typename ParseInput>
	static void failure(const ParseInput& /*unused*/, ast_state& state)
	{
		state.pop_back();
	}
};

} // namespace internal

template <typename Rule, template <typename...> class Action, template <typename...> class Selector,
	typename ParseInput>
[[nodiscard]] std::unique_ptr<ast_node> parse(ParseInput&& in)
{
	internal::ast_state state;
	if (!tao::graphqlpeg::parse<Rule, Action, internal::make_control<Selector>::template type>(
			std::forward<ParseInput>(in),
			state))
	{
		return nullptr;
	}

	if (state.stack.size() != 1)
	{
		throw std::logic_error("Unexpected error parsing GraphQL");
	}

	return std::move(state.back());
}

} // namespace graphql_parse_tree

template <class ParseInput>
class [[nodiscard]] depth_limit_input : public ParseInput
{
public:
	template <typename... Args>
	explicit depth_limit_input(size_t depthLimit, Args&&... args) noexcept
		: ParseInput(std::forward<Args>(args)...)
		, _depthLimit(depthLimit)
	{
	}

	size_t depthLimit() const noexcept
	{
		return _depthLimit;
	}

	size_t selectionSetDepth = 0;

private:
	const size_t _depthLimit;
};

using ast_file = depth_limit_input<file_input<>>;
using ast_memory = depth_limit_input<memory_input<>>;

struct ast_string
{
	std::vector<char> input;
	std::unique_ptr<ast_memory> memory {};
};

struct ast_string_view
{
	std::string_view input;
	std::unique_ptr<memory_input<>> memory {};
};

struct [[nodiscard]] ast_input
{
	std::variant<ast_string, std::unique_ptr<ast_file>, ast_string_view> data;
};

ast parseSchemaString(std::string_view input, size_t depthLimit)
{
	ast result { std::make_shared<ast_input>(ast_string { { input.cbegin(), input.cend() } }), {} };
	auto& data = std::get<ast_string>(result.input->data);

	try
	{
		// Try a smaller grammar with only schema type definitions first.
		data.memory = std::make_unique<ast_memory>(depthLimit,
			data.input.data(),
			data.input.size(),
			"GraphQL"s);
		result.root =
			graphql_parse_tree::parse<schema_document, ast_action, schema_selector>(*data.memory);
	}
	catch (const peg::parse_error&)
	{
		// Try again with the full document grammar so validation can handle the unexepected
		// executable definitions if this is a mixed document.
		data.memory = std::make_unique<ast_memory>(depthLimit,
			data.input.data(),
			data.input.size(),
			"GraphQL"s);
		result.root =
			graphql_parse_tree::parse<mixed_document, ast_action, schema_selector>(*data.memory);
	}

	return result;
}

ast parseSchemaFile(std::string_view filename, size_t depthLimit)
{
	ast result;

	try
	{
		result.input = std::make_shared<ast_input>(
			ast_input { std::make_unique<ast_file>(depthLimit, filename) });

		auto& in = *std::get<std::unique_ptr<ast_file>>(result.input->data);

		// Try a smaller grammar with only schema type definitions first.
		result.root =
			graphql_parse_tree::parse<schema_document, ast_action, schema_selector>(std::move(in));
	}
	catch (const peg::parse_error&)
	{
		result.input = std::make_shared<ast_input>(
			ast_input { std::make_unique<ast_file>(depthLimit, filename) });

		auto& in = *std::get<std::unique_ptr<ast_file>>(result.input->data);

		// Try again with the full document grammar so validation can handle the unexepected
		// executable definitions if this is a mixed document.
		result.root =
			graphql_parse_tree::parse<mixed_document, ast_action, schema_selector>(std::move(in));
	}

	return result;
}

ast parseString(std::string_view input, size_t depthLimit)
{
	ast result { std::make_shared<ast_input>(ast_string { { input.cbegin(), input.cend() } }), {} };
	auto& data = std::get<ast_string>(result.input->data);

	try
	{
		// Try a smaller grammar with only executable definitions first.
		data.memory = std::make_unique<ast_memory>(depthLimit,
			data.input.data(),
			data.input.size(),
			"GraphQL"s);
		result.root =
			graphql_parse_tree::parse<executable_document, ast_action, executable_selector>(
				*data.memory);
	}
	catch (const peg::parse_error&)
	{
		// Try again with the full document grammar so validation can handle the unexepected type
		// definitions if this is a mixed document.
		data.memory = std::make_unique<ast_memory>(depthLimit,
			data.input.data(),
			data.input.size(),
			"GraphQL"s);
		result.root = graphql_parse_tree::parse<mixed_document, ast_action, executable_selector>(
			*data.memory);
	}

	return result;
}

ast parseFile(std::string_view filename, size_t depthLimit)
{
	ast result;

	try
	{
		result.input = std::make_shared<ast_input>(
			ast_input { std::make_unique<ast_file>(depthLimit, filename) });

		auto& in = *std::get<std::unique_ptr<ast_file>>(result.input->data);

		// Try a smaller grammar with only executable definitions first.
		result.root =
			graphql_parse_tree::parse<executable_document, ast_action, executable_selector>(
				std::move(in));
	}
	catch (const peg::parse_error&)
	{
		result.input = std::make_shared<ast_input>(
			ast_input { std::make_unique<ast_file>(depthLimit, filename) });

		auto& in = *std::get<std::unique_ptr<ast_file>>(result.input->data);

		// Try again with the full document grammar so validation can handle the unexepected type
		// definitions if this is a mixed document.
		result.root = graphql_parse_tree::parse<mixed_document, ast_action, executable_selector>(
			std::move(in));
	}

	return result;
}

} // namespace peg

peg::ast operator"" _graphql(const char* text, size_t size)
{
	peg::ast result { std::make_shared<peg::ast_input>(peg::ast_string_view { { text, size } }) };
	auto& data = std::get<peg::ast_string_view>(result.input->data);

	try
	{
		// Try a smaller grammar with only executable definitions first.
		data.memory =
			std::make_unique<peg::memory_input<>>(data.input.data(), data.input.size(), "GraphQL"s);
		result.root = peg::graphql_parse_tree::
			parse<peg::executable_document, peg::nothing, peg::executable_selector>(*data.memory);
	}
	catch (const peg::parse_error&)
	{
		// Try again with the full document grammar so validation can handle the unexepected type
		// definitions if this is a mixed document.
		data.memory =
			std::make_unique<peg::memory_input<>>(data.input.data(), data.input.size(), "GraphQL"s);
		result.root = peg::graphql_parse_tree::
			parse<peg::mixed_document, peg::nothing, peg::executable_selector>(*data.memory);
	}

	return result;
}

} // namespace graphql
