// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "graphqlservice/GraphQLResponse.h"

#include <ranges>

using namespace graphql;

TEST(ResponseCase, ValueConstructorFromStringLiteral)
{
	auto expected = "Test String";
	auto actual = response::Value(expected);

	ASSERT_TRUE(response::Type::String == actual.type());
	ASSERT_EQ(expected, actual.release<std::string>());
}

TEST(ResponseCase, IdTypeCompareEqual)
{
	const auto fakeId = []() noexcept {
		std::string_view fakeIdString { "fakeId" };
		response::IdType result(fakeIdString.size());

		std::ranges::copy(fakeIdString, result.begin());

		return response::IdType { std::move(result) };
	}();

	EXPECT_TRUE(response::IdType { "" } < fakeId) << "empty string should compare as less";
	EXPECT_TRUE(fakeId < response::IdType { "invalid string" })
		<< "an invalid string should compare as greater";
	EXPECT_TRUE(fakeId == response::IdType { "ZmFrZUlk" })
		<< "actual string should compare as equal";
}
