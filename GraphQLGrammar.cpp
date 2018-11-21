#include "GraphQLGrammar.h"

#include <graphqlparser/Ast.h>

#include <memory>
#include <iostream>
#include <stack>
#include <tuple>
#include <codecvt>
#include <locale>

namespace facebook {
namespace graphql {
namespace service {

using namespace tao::pegtl;

struct parser_state
{
	std::unique_ptr<ast::Document>* document = nullptr;

	std::unique_ptr<char[], ast::CDeleter> operationType;
	std::unique_ptr<ast::NamedType> typeCondition;
	std::unique_ptr<ast::Type> type;
	std::unique_ptr<ast::NamedType> namedType;
	std::unique_ptr<ast::SelectionSet> selectionSet;

	std::string enumValue;
	std::ostringstream stringBuffer;

	std::unique_ptr<ast::Name> name;
	std::unique_ptr<ast::Name> aliasName;
	std::unique_ptr<ast::Name> fieldName;
	std::unique_ptr<ast::Name> objectFieldName;
	std::unique_ptr<ast::Name> variableName;
	std::unique_ptr<ast::Name> argumentName;
	std::unique_ptr<ast::Name> directiveName;
	std::unique_ptr<ast::Name> operationName;
	std::unique_ptr<ast::Name> fragmentName;
	std::unique_ptr<ast::Name> scalarName;
	std::unique_ptr<ast::Name> objectName;
	std::unique_ptr<ast::Name> interfaceName;
	std::unique_ptr<ast::Name> unionName;
	std::unique_ptr<ast::Name> enumName;

	std::unique_ptr<ast::Value> defaultValue;

	std::unique_ptr<std::vector<std::unique_ptr<ast::Definition>>> definitionList;
	std::unique_ptr<std::vector<std::unique_ptr<ast::VariableDefinition>>> variableDefinitionList;
	std::unique_ptr<std::vector<std::unique_ptr<ast::Directive>>> directiveList;
	std::unique_ptr<std::vector<std::unique_ptr<ast::Argument>>> argumentList;

	std::stack<std::pair<std::unique_ptr<ast::Name>, std::unique_ptr<ast::Name>>> fieldNames;
	std::stack<std::unique_ptr<std::vector<std::unique_ptr<ast::Argument>>>> argumentLists;
	std::stack<std::unique_ptr<ast::Value>> value;

	std::stack<std::unique_ptr<std::vector<std::unique_ptr<ast::Selection>>>> selectionList;
	std::stack<std::unique_ptr<std::vector<std::unique_ptr<ast::Value>>>> valueList;
	std::stack<std::unique_ptr<std::vector<std::unique_ptr<ast::ObjectField>>>> objectFieldList;

	// Schema types
	std::unique_ptr<ast::TypeExtensionDefinition> typeExtensionDefinition;

	std::unique_ptr<std::vector<std::unique_ptr<ast::OperationTypeDefinition>>> operationTypeDefinitionList;
	std::unique_ptr<std::vector<std::unique_ptr<ast::NamedType>>> typeNameList;
	std::unique_ptr<std::vector<std::unique_ptr<ast::InputValueDefinition>>> inputValueDefinitionList;
	std::unique_ptr<std::vector<std::unique_ptr<ast::FieldDefinition>>> fieldDefinitionList;
	std::unique_ptr<std::vector<std::unique_ptr<ast::Name>>> nameList;
	std::unique_ptr<std::vector<std::unique_ptr<ast::EnumValueDefinition>>> enumValueDefinitionList;
};

template <typename _Input>
static yy::location get_location(const _Input& in)
{
	position pos = in.position();
	yy::position begin(&pos.source, pos.line, pos.byte_in_line);
	yy::position end(&pos.source, in.input().line(), in.input().byte_in_line());

	return yy::location(begin, end);
}

template <typename _Input>
static std::unique_ptr<char[], ast::CDeleter> get_name(const _Input& in)
{
	auto parsedName = in.string();
	std::unique_ptr<char[], ast::CDeleter> name(reinterpret_cast<char*>(malloc(parsedName.size() + 1)));

	memmove(name.get(), parsedName.data(), parsedName.size());
	name[parsedName.size()] = '\0';
	return name;
}

template <typename _Rule>
struct build_ast
	: nothing<_Rule>
{
};

template <>
struct build_ast<grammar::operation_type>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.operationType = get_name(in);
	}
};

template <>
struct build_ast<grammar::begin_list>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.valueList.push(std::unique_ptr<std::vector<std::unique_ptr<ast::Value>>>(new std::vector<std::unique_ptr<ast::Value>>()));
	}
};

template <>
struct build_ast<grammar::list_entry>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.valueList.top()->push_back(std::unique_ptr<ast::Value>(state.value.top().release()));
		state.value.pop();
	}
};

template <>
struct build_ast<grammar::list_value>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.value.push(std::unique_ptr<ast::Value>(new ast::ListValue(get_location(in), state.valueList.top().release())));
		state.valueList.pop();
	}
};

template <>
struct build_ast<grammar::begin_object>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.objectFieldList.push(std::unique_ptr<std::vector<std::unique_ptr<ast::ObjectField>>>(new std::vector<std::unique_ptr<ast::ObjectField>>()));
	}
};

template <>
struct build_ast<grammar::object_field_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.objectFieldName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::object_field>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.objectFieldList.top()->push_back(std::unique_ptr<ast::ObjectField>(
			new ast::ObjectField(get_location(in),
				state.objectFieldName.release(),
				state.value.top().release())));

		state.value.pop();
	}
};

template <>
struct build_ast<grammar::object_value>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.value.push(std::unique_ptr<ast::Value>(new ast::ObjectValue(get_location(in), state.objectFieldList.top().release())));
		state.objectFieldList.pop();
	}
};

template <>
struct build_ast<grammar::variable_value>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		std::unique_ptr<ast::Name> variableName(new ast::Name(get_location(in), get_name(in).release()));

		state.value.push(std::unique_ptr<ast::Value>(new ast::Variable(get_location(in), variableName.release())));
	}
};

template <>
struct build_ast<grammar::integer_value>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.value.push(std::unique_ptr<ast::Value>(new ast::IntValue(get_location(in), get_name(in).release())));
	}
};

template <>
struct build_ast<grammar::float_value>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.value.push(std::unique_ptr<ast::Value>(new ast::FloatValue(get_location(in), get_name(in).release())));
	}
};

template <>
struct build_ast<grammar::escaped_unicode>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		std::wstring source;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8conv;
		std::istringstream encoded(in.string());
		uint32_t wch;

		// Skip past the first 'u' character
		encoded.seekg(1);
		encoded >> std::hex >> wch;
		source.push_back(static_cast<wchar_t>(wch));
		state.stringBuffer << utf8conv.to_bytes(source);
	}
};

template <>
struct build_ast<grammar::escaped_char>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		const char ch = in.peek_char();

		switch (ch)
		{
			case '"':
				state.stringBuffer << '"';
				break;

			case '\\':
				state.stringBuffer << '\\';
				break;

			case '/':
				state.stringBuffer << '/';
				break;

			case 'b':
				state.stringBuffer << '\b';
				break;

			case 'f':
				state.stringBuffer << '\f';
				break;

			case 'n':
				state.stringBuffer << '\n';
				break;

			case 'r':
				state.stringBuffer << '\r';
				break;

			case 't':
				state.stringBuffer << '\t';
				break;

			default:
				state.stringBuffer << '\\' << ch;
				break;
		}
	}
};

template <>
struct build_ast<grammar::string_quote_character>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.stringBuffer << in.peek_char();
	}
};


template <>
struct build_ast<grammar::block_escape_sequence>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.stringBuffer << R"bq(""")bq";
	}
};

template <>
struct build_ast<grammar::block_quote_character>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.stringBuffer << in.peek_char();
	}
};

template <>
struct build_ast<grammar::string_value>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		auto parsedValue = state.stringBuffer.str();
		std::unique_ptr<char[], ast::CDeleter> value(reinterpret_cast<char*>(malloc(parsedValue.size() + 1)));

		memmove(value.get(), parsedValue.data(), parsedValue.size());
		value[parsedValue.size()] = '\0';
		state.value.push(std::unique_ptr<ast::Value>(new ast::StringValue(get_location(in), value.release())));
		state.stringBuffer.str("");
	}
};

template <>
struct build_ast<grammar::true_keyword>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.value.push(std::unique_ptr<ast::Value>(new ast::BooleanValue(get_location(in), true)));
	}
};

template <>
struct build_ast<grammar::false_keyword>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.value.push(std::unique_ptr<ast::Value>(new ast::BooleanValue(get_location(in), false)));
	}
};

template <>
struct build_ast<grammar::null_keyword>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.value.push(std::unique_ptr<ast::Value>(new ast::NullValue(get_location(in))));
	}
};

template <>
struct build_ast<grammar::enum_value>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.value.push(std::unique_ptr<ast::Value>(new ast::EnumValue(get_location(in), get_name(in).release())));
		state.enumValue = in.string();
	}
};

template <>
struct build_ast<grammar::variable_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.variableName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::alias_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.name.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::alias>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.aliasName = std::move(state.name);
	}
};

template <>
struct build_ast<grammar::argument_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.argumentName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::named_type>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		std::unique_ptr<ast::Name> namedTypeName(new ast::Name(get_location(in), get_name(in).release()));

		state.namedType.reset(new ast::NamedType(get_location(in), namedTypeName.release()));
	}
};

template <>
struct build_ast<grammar::directive_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.directiveName.reset(new ast::Name(get_location(in), get_name(in).release()));

		state.argumentLists.push(std::move(state.argumentList));
	}
};

template <>
struct build_ast<grammar::field_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.fieldName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::operation_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.operationName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::fragment_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.fragmentName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::scalar_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.scalarName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::list_type>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (state.namedType)
		{
			state.type = std::move(state.namedType);
		}

		state.type.reset(new ast::ListType(get_location(in), state.type.release()));
	}
};

template <>
struct build_ast<grammar::nonnull_type>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (state.namedType)
		{
			state.type = std::move(state.namedType);
		}

		state.type.reset(new ast::NonNullType(get_location(in), state.type.release()));
	}
};

template <>
struct build_ast<grammar::default_value>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.defaultValue = std::move(state.value.top());
		state.value.pop();
	}
};

template <>
struct build_ast<grammar::variable>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		auto parsedName = in.string();
		std::unique_ptr<ast::Variable> variable(new ast::Variable(get_location(in), state.variableName.release()));

		if (state.namedType)
		{
			state.type = std::move(state.namedType);
		}

		if (!state.variableDefinitionList)
		{
			state.variableDefinitionList.reset(new std::vector<std::unique_ptr<ast::VariableDefinition>>());
		}

		state.variableDefinitionList->push_back(std::unique_ptr<ast::VariableDefinition>(
			new ast::VariableDefinition(get_location(in),
				variable.release(),
				state.type.release(),
				state.defaultValue.release())));
	}
};

template <>
struct build_ast<grammar::object_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.objectName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::interface_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.interfaceName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::union_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.unionName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::enum_name>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.enumName.reset(new ast::Name(get_location(in), get_name(in).release()));
	}
};

template <>
struct build_ast<grammar::argument>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.argumentList)
		{
			state.argumentList.reset(new std::vector<std::unique_ptr<ast::Argument>>());
		}

		state.argumentList->push_back(std::unique_ptr<ast::Argument>(
			new ast::Argument(get_location(in),
				state.argumentName.release(),
				state.value.top().release())));

		state.value.pop();
	}
};

template <>
struct build_ast<grammar::directive>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.directiveList)
		{
			state.directiveList.reset(new std::vector<std::unique_ptr<ast::Directive>>());
		}

		state.directiveList->push_back(std::unique_ptr<ast::Directive>(
			new ast::Directive(get_location(in),
				state.directiveName.release(),
				state.argumentList.release())));

		if (!state.argumentLists.empty())
		{
			state.argumentList = std::move(state.argumentLists.top());
			state.argumentLists.pop();
		}
	}
};

template <>
struct build_ast<grammar::begin_selection_set>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.selectionList.push(std::unique_ptr<std::vector<std::unique_ptr<ast::Selection>>>(new std::vector<std::unique_ptr<ast::Selection>>()));

		if (state.fieldName)
		{
			state.fieldNames.push({
				std::move(state.aliasName),
				std::move(state.fieldName)
				});

			state.argumentLists.push(std::move(state.argumentList));
		}
	}
};

template <>
struct build_ast<grammar::field>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.selectionList.top()->push_back(std::unique_ptr<ast::Field>(
			new ast::Field(get_location(in),
				state.aliasName.release(),
				state.fieldName.release(),
				state.argumentList.release(),
				state.directiveList.release(),
				state.selectionSet.release())));
	}
};

template <>
struct build_ast<grammar::fragment_spread>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.selectionList.top()->push_back(std::unique_ptr<ast::FragmentSpread>(
			new ast::FragmentSpread(get_location(in),
				state.fragmentName.release(),
				state.directiveList.release())));
	}
};

template <>
struct build_ast<grammar::inline_fragment>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.selectionList.top()->push_back(std::unique_ptr<ast::InlineFragment>(
			new ast::InlineFragment(get_location(in),
				state.typeCondition.release(),
				state.directiveList.release(),
				state.selectionSet.release())));
	}
};

template <>
struct build_ast<grammar::selection_set>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.selectionSet.reset(new ast::SelectionSet(get_location(in), state.selectionList.top().release()));
		state.selectionList.pop();

		if (!state.fieldNames.empty())
		{
			state.aliasName = std::move(state.fieldNames.top().first);
			state.fieldName = std::move(state.fieldNames.top().second);
			state.fieldNames.pop();
		}

		if (!state.argumentLists.empty())
		{
			state.argumentList = std::move(state.argumentLists.top());
			state.argumentLists.pop();
		}
	}
};

template <>
struct build_ast<grammar::operation_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.operationType)
		{
			static const char defaultOperation[] = "query";
			std::unique_ptr<char[], ast::CDeleter> name(reinterpret_cast<char*>(malloc(sizeof(defaultOperation))));

			memmove(name.get(), defaultOperation, sizeof(defaultOperation));
			state.operationType = std::move(name);
		}

		if (!state.definitionList)
		{
			state.definitionList.reset(new std::vector<std::unique_ptr<ast::Definition>>());
		}

		state.definitionList->push_back(std::unique_ptr<ast::Definition>(
			new ast::OperationDefinition(get_location(in),
				state.operationType.release(),
				state.operationName.release(),
				state.variableDefinitionList.release(),
				state.directiveList.release(),
				state.selectionSet.release())));
	}
};

template <>
struct build_ast<grammar::type_condition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.typeCondition = std::move(state.namedType);
	}
};

template <>
struct build_ast<grammar::fragment_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.definitionList)
		{
			state.definitionList.reset(new std::vector<std::unique_ptr<ast::Definition>>());
		}

		state.definitionList->push_back(std::unique_ptr<ast::Definition>(
			new ast::FragmentDefinition(get_location(in),
				state.fragmentName.release(),
				state.typeCondition.release(),
				state.directiveList.release(),
				state.selectionSet.release())));
	}
};

template <>
struct build_ast<grammar::root_operation_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.operationTypeDefinitionList)
		{
			state.operationTypeDefinitionList.reset(new std::vector<std::unique_ptr<ast::OperationTypeDefinition>>());
		}

		state.operationTypeDefinitionList->push_back(std::unique_ptr<ast::OperationTypeDefinition>(
			new ast::OperationTypeDefinition(get_location(in),
				state.operationType.release(),
				state.namedType.release())));
	}
};

template <>
struct build_ast<grammar::schema_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.definitionList)
		{
			state.definitionList.reset(new std::vector<std::unique_ptr<ast::Definition>>());
		}

		state.definitionList->push_back(std::unique_ptr<ast::Definition>(
			new ast::SchemaDefinition(get_location(in),
				state.directiveList.release(),
				state.operationTypeDefinitionList.release())));
	}
};

template <>
struct build_ast<grammar::scalar_type_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.definitionList)
		{
			state.definitionList.reset(new std::vector<std::unique_ptr<ast::Definition>>());
		}

		state.definitionList->push_back(std::unique_ptr<ast::Definition>(
			new ast::ScalarTypeDefinition(get_location(in),
				state.scalarName.release(),
				state.directiveList.release())));
	}
};

template <>
struct build_ast<grammar::interface_type>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.typeNameList)
		{
			state.typeNameList.reset(new std::vector<std::unique_ptr<ast::NamedType>>());
		}

		std::unique_ptr<ast::Name> namedTypeName(new ast::Name(get_location(in), get_name(in).release()));

		state.typeNameList->push_back(std::unique_ptr<ast::NamedType>(new ast::NamedType(get_location(in), namedTypeName.release())));
	}
};

template <>
struct build_ast<grammar::input_field_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.inputValueDefinitionList)
		{
			state.inputValueDefinitionList.reset(new std::vector<std::unique_ptr<ast::InputValueDefinition>>());
		}

		if (state.namedType)
		{
			state.type = std::move(state.namedType);
		}

		state.inputValueDefinitionList->push_back(std::unique_ptr<ast::InputValueDefinition>(
			new ast::InputValueDefinition(get_location(in),
				state.argumentName.release(),
				state.type.release(),
				state.defaultValue.release(),
				state.directiveList.release())));
	}
};

template <>
struct build_ast<grammar::field_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.fieldDefinitionList)
		{
			state.fieldDefinitionList.reset(new std::vector<std::unique_ptr<ast::FieldDefinition>>());
		}

		if (state.namedType)
		{
			state.type = std::move(state.namedType);
		}

		state.fieldDefinitionList->push_back(std::unique_ptr<ast::FieldDefinition>(
			new ast::FieldDefinition(get_location(in),
				state.fieldName.release(),
				state.inputValueDefinitionList.release(),
				state.type.release(),
				state.directiveList.release())));
	}
};

template <>
struct build_ast<grammar::object_type_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.definitionList)
		{
			state.definitionList.reset(new std::vector<std::unique_ptr<ast::Definition>>());
		}

		state.definitionList->push_back(std::unique_ptr<ast::Definition>(
			new ast::ObjectTypeDefinition(get_location(in),
				state.objectName.release(),
				state.typeNameList.release(),
				state.directiveList.release(),
				state.fieldDefinitionList.release())));
	}
};

template <>
struct build_ast<grammar::interface_type_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.definitionList)
		{
			state.definitionList.reset(new std::vector<std::unique_ptr<ast::Definition>>());
		}

		state.definitionList->push_back(std::unique_ptr<ast::Definition>(
			new ast::InterfaceTypeDefinition(get_location(in),
				state.interfaceName.release(),
				state.directiveList.release(),
				state.fieldDefinitionList.release())));
	}
};

template <>
struct build_ast<grammar::union_type>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.typeNameList)
		{
			state.typeNameList.reset(new std::vector<std::unique_ptr<ast::NamedType>>());
		}

		std::unique_ptr<ast::Name> namedTypeName(new ast::Name(get_location(in), get_name(in).release()));

		state.typeNameList->push_back(std::unique_ptr<ast::NamedType>(new ast::NamedType(get_location(in), namedTypeName.release())));
	}
};

template <>
struct build_ast<grammar::union_type_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.definitionList)
		{
			state.definitionList.reset(new std::vector<std::unique_ptr<ast::Definition>>());
		}

		state.definitionList->push_back(std::unique_ptr<ast::Definition>(
			new ast::UnionTypeDefinition(get_location(in),
				state.unionName.release(),
				state.directiveList.release(),
				state.typeNameList.release())));
	}
};

template <>
struct build_ast<grammar::enum_value_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.enumValueDefinitionList)
		{
			state.enumValueDefinitionList.reset(new std::vector<std::unique_ptr<ast::EnumValueDefinition>>());
		}

		std::unique_ptr<ast::Name> enumValueName;
		std::unique_ptr<char[], ast::CDeleter> value(reinterpret_cast<char*>(malloc(state.enumValue.size() + 1)));

		memmove(value.get(), state.enumValue.data(), state.enumValue.size());
		value[state.enumValue.size()] = '\0';
		enumValueName.reset(new ast::Name(get_location(in), value.release()));

		state.enumValueDefinitionList->push_back(std::unique_ptr<ast::EnumValueDefinition>(
			new ast::EnumValueDefinition(get_location(in),
				enumValueName.release(),
				state.directiveList.release())));
	}
};

template <>
struct build_ast<grammar::enum_type_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.definitionList)
		{
			state.definitionList.reset(new std::vector<std::unique_ptr<ast::Definition>>());
		}

		state.definitionList->push_back(std::unique_ptr<ast::Definition>(
			new ast::EnumTypeDefinition(get_location(in),
				state.enumName.release(),
				state.directiveList.release(),
				state.enumValueDefinitionList.release())));
	}
};

template <>
struct build_ast<grammar::input_object_type_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.definitionList)
		{
			state.definitionList.reset(new std::vector<std::unique_ptr<ast::Definition>>());
		}

		state.definitionList->push_back(std::unique_ptr<ast::Definition>(
			new ast::InputObjectTypeDefinition(get_location(in),
				state.objectName.release(),
				state.directiveList.release(),
				state.inputValueDefinitionList.release())));
	}
};

template <>
struct build_ast<grammar::directive_location>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.nameList)
		{
			state.nameList.reset(new std::vector<std::unique_ptr<ast::Name>>());
		}

		state.nameList->push_back(std::unique_ptr<ast::Name>(new ast::Name(get_location(in), get_name(in).release())));
	}
};

template <>
struct build_ast<grammar::directive_definition>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		if (!state.definitionList)
		{
			state.definitionList.reset(new std::vector<std::unique_ptr<ast::Definition>>());
		}

		state.definitionList->push_back(std::unique_ptr<ast::Definition>(
			new ast::DirectiveDefinition(get_location(in),
				state.directiveName.release(),
				state.inputValueDefinitionList.release(),
				state.nameList.release())));
	}
};

// TODO: The GraphQLParser AST only has support for TypeExtensionDefinition, so we can't implement all of the
// extension types in the grammar's type_system_extension. I'm going to wait until I implement my own AST to
// generate them, I don't support them in the SchemaGenerator anyway. Alternatively, maybe I can bypass the AST
// entirely and drive the whole service based on pegtl actions.

template <>
struct build_ast<grammar::document>
{
	template <typename _Input>
	static void apply(const _Input& in, parser_state& state)
	{
		state.document->reset(new ast::Document(get_location(in),
			state.definitionList.release()));
	}
};

std::unique_ptr<ast::Node> parseString(const char* text)
{
	parser_state state;
	std::unique_ptr<ast::Document> document;
	memory_input<> in(text, "GraphQL");

	state.document = &document;
	parse<grammar::document, build_ast>(std::move(in), std::move(state));

	return std::unique_ptr<ast::Node>(document.release());
}

std::unique_ptr<ast::Node> parseFile(FILE* file)
{
	parser_state state;
	std::unique_ptr<ast::Document> document;
	file_input<> in(file, "GraphQL");

	state.document = &document;
	parse<grammar::document, build_ast>(std::move(in), std::move(state));

	return std::unique_ptr<ast::Node>(document.release());
}

} /* namespace service */
} /* namespace graphql */
} /* namespace facebook */