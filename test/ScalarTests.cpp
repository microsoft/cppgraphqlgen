// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "BigIntScalar.h"
#include "ScalarSchema.h"

#include "QueryObject.h"

#include "graphqlservice/JSONResponse.h"

#include <cstdint>
#include <optional>

using namespace graphql;

using namespace std::literals;

namespace {

// A minimal implementation of the generated Query interface. The resolvers work directly with the
// custom bigint::BigInt C++ type, never with response::Value, demonstrating that a custom scalar's
// storage is no longer limited to response::Value.
class Query
{
public:
	Query() = default;

	bigint::BigInt getEcho(bigint::BigInt&& valueArg) const
	{
		return std::move(valueArg);
	}

	bigint::BigInt getAdd(bigint::BigInt&& leftArg, bigint::BigInt&& rightArg) const
	{
		return bigint::BigInt { leftArg.value + rightArg.value };
	}

	std::optional<bigint::BigInt> getMaybe(std::optional<bigint::BigInt>&& valueArg) const
	{
		return std::move(valueArg);
	}
};

[[nodiscard]] std::shared_ptr<scalar::Operations> makeService()
{
	return std::make_shared<scalar::Operations>(std::make_shared<Query>());
}

[[nodiscard]] response::Value runQuery(std::string_view query, response::Value&& variables)
{
	auto service = makeService();
	auto parsed = peg::parseString(query);

	return service->resolve({ parsed, ""sv, std::move(variables), std::launch::async }).get();
}

[[nodiscard]] const response::Value& requireData(const response::Value& result)
{
	auto errorsItr = result.find("errors");

	EXPECT_TRUE(
		errorsItr == result.get<response::MapType>().cend() || errorsItr->second.size() == 0)
		<< response::toJSON(response::Value { result });

	return result["data"];
}

} // namespace

TEST(BigIntScalarCase, EchoRoundTripsValueLargerThan32Bits)
{
	// 9000000000 does not fit in the 32-bit response::IntType, so it must round-trip through the
	// custom bigint::BigInt type and its String wire representation.
	constexpr std::int64_t largeValue = 9'000'000'000;

	response::Value variables { response::Type::Map };
	variables.emplace_back("value"s, response::Value { std::to_string(largeValue) });

	auto result =
		runQuery(R"(query($value: BigInt!) { echo(value: $value) })"sv, std::move(variables));
	const auto& data = requireData(result);

	ASSERT_EQ(response::Type::String, data["echo"].type());
	EXPECT_EQ(std::to_string(largeValue), data["echo"].get<response::StringType>());
}

TEST(BigIntScalarCase, AddComputesWithCustomCppType)
{
	response::Value variables { response::Type::Map };
	variables.emplace_back("left"s, response::Value { "4000000000"s });
	variables.emplace_back("right"s, response::Value { "5000000001"s });

	auto result = runQuery(R"(query($left: BigInt!, $right: BigInt!) {
			add(left: $left right: $right)
		})"sv,
		std::move(variables));
	const auto& data = requireData(result);

	ASSERT_EQ(response::Type::String, data["add"].type());
	EXPECT_EQ("9000000001"s, data["add"].get<response::StringType>());
}

TEST(BigIntScalarCase, EchoAcceptsInlineStringLiteral)
{
	auto result = runQuery(R"(query { echo(value: "12345678901234") })"sv,
		response::Value { response::Type::Map });
	const auto& data = requireData(result);

	ASSERT_EQ(response::Type::String, data["echo"].type());
	EXPECT_EQ("12345678901234"s, data["echo"].get<response::StringType>());
}

TEST(BigIntScalarCase, NullableArgumentReturnsNullWhenOmitted)
{
	auto result = runQuery(R"(query { maybe })"sv, response::Value { response::Type::Map });
	const auto& data = requireData(result);

	EXPECT_EQ(response::Type::Null, data["maybe"].type());
}

TEST(BigIntScalarCase, NullableArgumentRoundTripsValue)
{
	response::Value variables { response::Type::Map };
	variables.emplace_back("value"s, response::Value { "42"s });

	auto result =
		runQuery(R"(query($value: BigInt) { maybe(value: $value) })"sv, std::move(variables));
	const auto& data = requireData(result);

	ASSERT_EQ(response::Type::String, data["maybe"].type());
	EXPECT_EQ("42"s, data["maybe"].get<response::StringType>());
}

TEST(BigIntScalarCase, InvalidValueProducesError)
{
	response::Value variables { response::Type::Map };
	variables.emplace_back("value"s, response::Value { "not-a-number"s });

	auto result =
		runQuery(R"(query($value: BigInt!) { echo(value: $value) })"sv, std::move(variables));

	auto errorsItr = result.find("errors");

	ASSERT_NE(errorsItr, result.get<response::MapType>().cend());
	EXPECT_GT(errorsItr->second.size(), size_t { 0 });
}
