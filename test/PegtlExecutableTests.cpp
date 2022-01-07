// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

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
	ASSERT_EQ(size_t {}, analyze<executable_document>(true))
		<< "there shouldn't be any infinite loops in the PEG version of the grammar";
}
