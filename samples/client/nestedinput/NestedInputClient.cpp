// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "NestedInputClient.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <string_view>

using namespace std::literals;

namespace graphql::client {

using namespace query::testQuery;

template <>
constexpr bool isInputType<Variables::InputA>() noexcept
{
	return true;
}

template <>
constexpr bool isInputType<Variables::InputB>() noexcept
{
	return true;
}

template <>
constexpr bool isInputType<Variables::InputABCD>() noexcept
{
	return true;
}

template <>
constexpr bool isInputType<Variables::InputBC>() noexcept
{
	return true;
}

template <>
response::Value ModifiedVariable<Variables::InputA>::serialize(Variables::InputA&& inputValue)
{
	response::Value result { response::Type::Map };

	result.emplace_back(R"js(a)js"s, ModifiedVariable<bool>::serialize(std::move(inputValue.a)));

	return result;
}

template <>
response::Value ModifiedVariable<Variables::InputB>::serialize(Variables::InputB&& inputValue)
{
	response::Value result { response::Type::Map };

	result.emplace_back(R"js(b)js"s, ModifiedVariable<double>::serialize(std::move(inputValue.b)));

	return result;
}

template <>
response::Value ModifiedVariable<Variables::InputABCD>::serialize(Variables::InputABCD&& inputValue)
{
	response::Value result { response::Type::Map };

	result.emplace_back(R"js(d)js"s, ModifiedVariable<std::string>::serialize(std::move(inputValue.d)));
	result.emplace_back(R"js(a)js"s, ModifiedVariable<InputA>::serialize(std::move(inputValue.a)));
	result.emplace_back(R"js(b)js"s, ModifiedVariable<InputB>::serialize(std::move(inputValue.b)));
	result.emplace_back(R"js(bc)js"s, ModifiedVariable<InputBC>::serialize<TypeModifier::List>(std::move(inputValue.bc)));

	return result;
}

template <>
response::Value ModifiedVariable<Variables::InputBC>::serialize(Variables::InputBC&& inputValue)
{
	response::Value result { response::Type::Map };

	result.emplace_back(R"js(c)js"s, ModifiedVariable<response::IdType>::serialize(std::move(inputValue.c)));
	result.emplace_back(R"js(b)js"s, ModifiedVariable<InputB>::serialize(std::move(inputValue.b)));

	return result;
}

template <>
Response::control_Control::test_Output ModifiedResponse<Response::control_Control::test_Output>::parse(response::Value&& response)
{
	Response::control_Control::test_Output result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == R"js(id)js"sv)
			{
				result.id = ModifiedResponse<bool>::parse<TypeModifier::Nullable>(std::move(member.second));
				continue;
			}
		}
	}

	return result;
}

template <>
Response::control_Control ModifiedResponse<Response::control_Control>::parse(response::Value&& response)
{
	Response::control_Control result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == R"js(test)js"sv)
			{
				result.test = ModifiedResponse<Response::control_Control::test_Output>::parse<TypeModifier::Nullable>(std::move(member.second));
				continue;
			}
		}
	}

	return result;
}

namespace query::testQuery {

const std::string& GetRequestText() noexcept
{
	static const auto s_request = R"gql(
		query testQuery($stream: InputABCD!) {
		  control {
		    test(new: $stream) {
		      id
		    }
		  }
		}
	)gql"s;

	return s_request;
}

const peg::ast& GetRequestObject() noexcept
{
	static const auto s_request = []() noexcept {
		auto ast = peg::parseString(GetRequestText());

		// This has already been validated against the schema by clientgen.
		ast.validated = true;

		return ast;
	}();

	return s_request;
}

response::Value serializeVariables(Variables&& variables)
{
	response::Value result { response::Type::Map };

	result.emplace_back(R"js(stream)js"s, ModifiedVariable<Variables::InputABCD>::serialize(std::move(variables.stream)));

	return result;
}

Response parseResponse(response::Value&& response)
{
	Response result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == R"js(control)js"sv)
			{
				result.control = ModifiedResponse<Response::control_Control>::parse(std::move(member.second));
				continue;
			}
		}
	}

	return result;
}

} // namespace query::testQuery
} // namespace graphql::client