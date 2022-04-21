// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "graphqlservice/GraphQLParse.h"

#include "graphqlservice/internal/Grammar.h"

#include <tao/pegtl/contrib/analyze.hpp>

using namespace graphql;
using namespace graphql::peg;

using namespace tao::graphqlpeg;

TEST(PegtlExecutableCase, ParseKitchenSinkQuery)
{
	memory_input<> input(R"gql(
		# Copyright (c) 2015-present, Facebook, Inc.
		#
		# This source code is licensed under the MIT license found in the
		# LICENSE file in the root directory of this source tree.

		query queryName($foo: ComplexType, $site: Site = MOBILE) {
		  whoever123is: node(id: [123, 456]) {
			id ,
			... on User @defer {
			  field2 {
				id ,
				alias: field1(first:10, after:$foo,) @include(if: $foo) {
				  id,
				  ...frag
				}
			  }
			}
			... @skip(unless: $foo) {
			  id
			}
			... {
			  id
			}
		  }
		}

		mutation likeStory {
		  like(story: 123) @defer {
			story {
			  id
			}
		  }
		}

		subscription StoryLikeSubscription($input: StoryLikeSubscribeInput) {
		  storyLikeSubscribe(input: $input) {
			story {
			  likers {
				count
			  }
			  likeSentence {
				text
			  }
			}
		  }
		}

		fragment frag on Friend {
		  foo(size: $size, bar: $b, obj: {key: "value", block: """

			  block string uses \"""

		  """})
		}

		{
		  unnamed(truthy: true, falsey: false, nullish: null),
		  query
		})gql",
		"ParseKitchenSinkQuery");

	const bool result = parse<executable_document>(input);

	ASSERT_TRUE(result) << "we should be able to parse the doc";
}

TEST(PegtlExecutableCase, ParseTodayQuery)
{
	memory_input<> input(R"gql(
		query Everything {
			appointments {
				edges {
					node {
						id
						subject
						when
						isNow
					}
				}
			}
			tasks {
				edges {
					node {
						id
						title
						isComplete
					}
				}
			}
			unreadCounts {
				edges {
					node {
						id
						name
						unreadCount
					}
				}
			}
		})gql",
		"ParseTodayQuery");

	const bool result = parse<executable_document>(input);

	ASSERT_TRUE(result) << "we should be able to parse the doc";
}

TEST(PegtlExecutableCase, ParseVariableDefaultEmptyList)
{
	memory_input<> input(R"gql(
		query QueryWithEmptyListVariable($empty: [Boolean!]! = []) {
			fieldWithArg(arg: $empty)
		})gql",
		"ParseVariableDefaultEmptyList");

	const bool result = parse<executable_document>(input);

	ASSERT_TRUE(result) << "we should be able to parse the doc";
}

TEST(PegtlExecutableCase, AnalyzeExecutableGrammar)
{
	ASSERT_EQ(size_t { 0 }, analyze<executable_document>(true))
		<< "there shouldn't be any infinite loops in the PEG version of the grammar";
}

TEST(PegtlExecutableCase, InvalidStringEscapeSequence)
{
	bool parsedQuery = false;
	bool caughtException = false;

	try
	{
		memory_input<> input(R"gql(query { foo @something(arg: "\.") })gql",
			"InvalidStringEscapeSequence");
		parsedQuery = parse<executable_document>(input);
	}
	catch (const peg::parse_error& e)
	{
		ASSERT_NE(nullptr, e.what());

		using namespace std::literals;

		const std::string_view error { e.what() };
		constexpr auto c_start = "InvalidStringEscapeSequence:1:31: parse error matching "sv;
		constexpr auto c_end = " graphql::peg::string_escape_sequence_content"sv;

		ASSERT_TRUE(error.size() > c_start.size()) << "error message is too short";
		ASSERT_TRUE(error.size() > c_end.size()) << "error message is too short";
		EXPECT_TRUE(error.substr(0, c_start.size()) == c_start) << e.what();
		EXPECT_TRUE(error.substr(error.size() - c_end.size()) == c_end) << e.what();

		caughtException = true;
	}

	EXPECT_TRUE(caughtException) << "should catch a parse exception";
	EXPECT_FALSE(parsedQuery) << "should not successfully parse the query";
}

using namespace std::literals;

constexpr auto queryWithDepth3 = R"gql(query {
		foo {
			bar
		}
	})gql"sv;

TEST(PegtlExecutableCase, ParserDepthLimitNotExceeded)
{
	bool parsedQuery = false;

	try
	{
		auto query = peg::parseString(queryWithDepth3, 3);

		parsedQuery = query.root != nullptr;
	}
	catch (const peg::parse_error& ex)
	{
		FAIL() << ex.what();
	}

	EXPECT_TRUE(parsedQuery) << "should parse the query";
}

TEST(PegtlExecutableCase, ParserDepthLimitExceeded)
{
	bool parsedQuery = false;
	bool caughtException = false;

	try
	{
		auto query = peg::parseString(queryWithDepth3, 2);

		parsedQuery = query.root != nullptr;
	}
	catch (const peg::parse_error& ex)
	{
		ASSERT_NE(nullptr, ex.what());

		using namespace std::literals;

		const std::string_view error { ex.what() };
		constexpr auto expected =
			"GraphQL:4:3: Exceeded nested depth limit: 2 for https://spec.graphql.org/October2021/#SelectionSet"sv;

		EXPECT_TRUE(error == expected) << ex.what();

		caughtException = true;
	}

	EXPECT_TRUE(caughtException) << "should catch a parse exception";
	EXPECT_FALSE(parsedQuery) << "should not successfully parse the query";
}