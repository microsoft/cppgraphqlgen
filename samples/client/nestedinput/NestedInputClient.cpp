// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#include "NestedInputClient.h"

#include "graphqlservice/internal/SortedMap.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace std::literals;

namespace graphql::client {
namespace nestedinput {

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

InputA::InputA(
		bool aArg) noexcept
	: a { std::move(aArg) }
{
}

InputA::InputA(const InputA& other)
	: a { ModifiedVariable<bool>::duplicate(other.a) }
{
}

InputA::InputA(InputA&& other) noexcept
	: a { std::move(other.a) }
{
}

InputA& InputA::operator=(const InputA& other)
{
	InputA value { other };

	std::swap(*this, value);

	return *this;
}

InputA& InputA::operator=(InputA&& other) noexcept
{
	a = std::move(other.a);

	return *this;
}

InputB::InputB(
		double bArg) noexcept
	: b { std::move(bArg) }
{
}

InputB::InputB(const InputB& other)
	: b { ModifiedVariable<double>::duplicate(other.b) }
{
}

InputB::InputB(InputB&& other) noexcept
	: b { std::move(other.b) }
{
}

InputB& InputB::operator=(const InputB& other)
{
	InputB value { other };

	std::swap(*this, value);

	return *this;
}

InputB& InputB::operator=(InputB&& other) noexcept
{
	b = std::move(other.b);

	return *this;
}

InputABCD::InputABCD(
		std::string dArg,
		InputA aArg,
		InputB bArg,
		std::vector<InputBC> bcArg) noexcept
	: d { std::move(dArg) }
	, a { std::move(aArg) }
	, b { std::move(bArg) }
	, bc { std::move(bcArg) }
{
}

InputABCD::InputABCD(const InputABCD& other)
	: d { ModifiedVariable<std::string>::duplicate(other.d) }
	, a { ModifiedVariable<InputA>::duplicate(other.a) }
	, b { ModifiedVariable<InputB>::duplicate(other.b) }
	, bc { ModifiedVariable<InputBC>::duplicate<TypeModifier::List>(other.bc) }
{
}

InputABCD::InputABCD(InputABCD&& other) noexcept
	: d { std::move(other.d) }
	, a { std::move(other.a) }
	, b { std::move(other.b) }
	, bc { std::move(other.bc) }
{
}

InputABCD& InputABCD::operator=(const InputABCD& other)
{
	InputABCD value { other };

	std::swap(*this, value);

	return *this;
}

InputABCD& InputABCD::operator=(InputABCD&& other) noexcept
{
	d = std::move(other.d);
	a = std::move(other.a);
	b = std::move(other.b);
	bc = std::move(other.bc);

	return *this;
}

InputBC::InputBC(
		response::IdType cArg,
		InputB bArg) noexcept
	: c { std::move(cArg) }
	, b { std::move(bArg) }
{
}

InputBC::InputBC(const InputBC& other)
	: c { ModifiedVariable<response::IdType>::duplicate(other.c) }
	, b { ModifiedVariable<InputB>::duplicate(other.b) }
{
}

InputBC::InputBC(InputBC&& other) noexcept
	: c { std::move(other.c) }
	, b { std::move(other.b) }
{
}

InputBC& InputBC::operator=(const InputBC& other)
{
	InputBC value { other };

	std::swap(*this, value);

	return *this;
}

InputBC& InputBC::operator=(InputBC&& other) noexcept
{
	c = std::move(other.c);
	b = std::move(other.b);

	return *this;
}

} // namespace nestedinput

using namespace nestedinput;

template <>
response::Value ModifiedVariable<InputA>::serialize(InputA&& inputValue)
{
	response::Value result { response::Type::Map };

	result.emplace_back(R"js(a)js"s, ModifiedVariable<bool>::serialize(std::move(inputValue.a)));

	return result;
}

template <>
response::Value ModifiedVariable<InputB>::serialize(InputB&& inputValue)
{
	response::Value result { response::Type::Map };

	result.emplace_back(R"js(b)js"s, ModifiedVariable<double>::serialize(std::move(inputValue.b)));

	return result;
}

template <>
response::Value ModifiedVariable<InputABCD>::serialize(InputABCD&& inputValue)
{
	response::Value result { response::Type::Map };

	result.emplace_back(R"js(d)js"s, ModifiedVariable<std::string>::serialize(std::move(inputValue.d)));
	result.emplace_back(R"js(a)js"s, ModifiedVariable<InputA>::serialize(std::move(inputValue.a)));
	result.emplace_back(R"js(b)js"s, ModifiedVariable<InputB>::serialize(std::move(inputValue.b)));
	result.emplace_back(R"js(bc)js"s, ModifiedVariable<InputBC>::serialize<TypeModifier::List>(std::move(inputValue.bc)));

	return result;
}

template <>
response::Value ModifiedVariable<InputBC>::serialize(InputBC&& inputValue)
{
	response::Value result { response::Type::Map };

	result.emplace_back(R"js(c)js"s, ModifiedVariable<response::IdType>::serialize(std::move(inputValue.c)));
	result.emplace_back(R"js(b)js"s, ModifiedVariable<InputB>::serialize(std::move(inputValue.b)));

	return result;
}

template <>
query::testQuery::Response::control_Control::test_Output ModifiedResponse<query::testQuery::Response::control_Control::test_Output>::parse(response::Value&& response)
{
	query::testQuery::Response::control_Control::test_Output result;

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
query::testQuery::Response::control_Control ModifiedResponse<query::testQuery::Response::control_Control>::parse(response::Value&& response)
{
	query::testQuery::Response::control_Control result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == R"js(test)js"sv)
			{
				result.test = ModifiedResponse<query::testQuery::Response::control_Control::test_Output>::parse<TypeModifier::Nullable>(std::move(member.second));
				continue;
			}
		}
	}

	return result;
}

namespace query::testQuery {

const std::string& GetOperationName() noexcept
{
	static const auto s_name = R"gql(testQuery)gql"s;

	return s_name;
}

response::Value serializeVariables(Variables&& variables)
{
	response::Value result { response::Type::Map };

	result.emplace_back(R"js(stream)js"s, ModifiedVariable<InputABCD>::serialize(std::move(variables.stream)));

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
				result.control = ModifiedResponse<query::testQuery::Response::control_Control>::parse(std::move(member.second));
				continue;
			}
		}
	}

	return result;
}

} // namespace query::testQuery
} // namespace graphql::client
