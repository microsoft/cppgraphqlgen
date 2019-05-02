// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include <graphqlservice/GraphQLResponse.h>

using namespace graphql;


TEST(ResponseCase, ValueConstructorFromStringLiteral)
{
	auto expected = "Test String";
	auto actual = response::Value(expected);

	ASSERT_TRUE(response::Type::String == actual.type());
	ASSERT_EQ(expected, actual.release<response::StringType>());
}
