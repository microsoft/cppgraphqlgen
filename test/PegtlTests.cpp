// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "graphqlservice/GraphQLGrammar.h"

#include <tao/pegtl/contrib/analyze.hpp>

using namespace graphql;
using namespace graphql::peg;

using namespace tao::graphqlpeg;

TEST(PegtlCase, ParseKitchenSinkQuery)
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

TEST(PegtlCase, ParseKitchenSinkSchema)
{
	memory_input<> input(R"gql(
		# Copyright (c) 2015-present, Facebook, Inc.
		#
		# This source code is licensed under the MIT license found in the
		# LICENSE file in the root directory of this source tree.

		# (this line is padding to maintain test line numbers)

		schema {
		  query: QueryType
		  mutation: MutationType
		}

		type Foo implements Bar {
		  one: Type
		  two(argument: InputType!): Type
		  three(argument: InputType, other: String): Int
		  four(argument: String = "string"): String
		  five(argument: [String] = ["string", "string"]): String
		  six(argument: InputType = {key: "value"}): Type
		  seven(argument: Int = null): Type
		}

		type AnnotatedObject @onObject(arg: "value") {
		  annotatedField(arg: Type = "default" @onArg): Type @onField
		}

		interface Bar {
		  one: Type
		  four(argument: String = "string"): String
		}

		interface AnnotatedInterface @onInterface {
		  annotatedField(arg: Type @onArg): Type @onField
		}

		union Feed = Story | Article | Advert

		union AnnotatedUnion @onUnion = A | B

		scalar CustomScalar

		scalar AnnotatedScalar @onScalar

		enum Site {
		  DESKTOP
		  MOBILE
		}

		enum AnnotatedEnum @onEnum {
		  ANNOTATED_VALUE @onEnumValue
		  OTHER_VALUE
		}

		input InputType {
		  key: String!
		  answer: Int = 42
		}

		input AnnotatedInput @onInputObjectType {
		  annotatedField: Type @onField
		}

		extend type Foo {
		  seven(argument: [String]): Type
		}

		# NOTE: out-of-spec test cases commented out until the spec is clarified; see
		# https://github.com/graphql/graphql-js/issues/650 .
		# extend type Foo @onType {}

		#type NoFields {}

		directive @skip(if: Boolean!) on FIELD | FRAGMENT_SPREAD | INLINE_FRAGMENT

		directive @include(if: Boolean!)
		  on FIELD
		   | FRAGMENT_SPREAD
		   | INLINE_FRAGMENT)gql",
		"ParseKitchenSinkSchema");

	const bool result = parse<schema_document>(input);

	ASSERT_TRUE(result) << "we should be able to parse the doc";
}

TEST(PegtlCase, ParseTodayQuery)
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

TEST(PegtlCase, ParseTodaySchema)
{
	memory_input<> input(R"gql(
		# Copyright (c) Microsoft Corporation. All rights reserved.
		# Licensed under the MIT License.

		schema {
			query: Query
			mutation: Mutation
			subscription: Subscription
		}

		scalar ItemCursor

		type Query {
			node(id: ID!) : Node

			appointments(first: Int, after: ItemCursor, last: Int, before: ItemCursor): AppointmentConnection!
			tasks(first: Int, after: ItemCursor, last: Int, before: ItemCursor): TaskConnection!
			unreadCounts(first: Int, after: ItemCursor, last: Int, before: ItemCursor): FolderConnection!

			appointmentsById(ids: [ID!]!) : [Appointment]!
			tasksById(ids: [ID!]!): [Task]!
			unreadCountsById(ids: [ID!]!): [Folder]!
		}

		interface Node {
			id: ID!
		}

		type PageInfo {
			hasNextPage: Boolean!
			hasPreviousPage: Boolean!
		}

		type AppointmentEdge {
			node: Appointment
			cursor: ItemCursor!
		}

		type AppointmentConnection {
			pageInfo: PageInfo!
			edges: [AppointmentEdge]
		}

		type TaskEdge {
			node: Task
			cursor: ItemCursor!
		}

		type TaskConnection {
			pageInfo: PageInfo!
			edges: [TaskEdge]
		}

		type FolderEdge {
			node: Folder
			cursor: ItemCursor!
		}

		type FolderConnection {
			pageInfo: PageInfo!
			edges: [FolderEdge]
		}

		input CompleteTaskInput {
			id: ID!
			isComplete: Boolean = true
			clientMutationId: String
		}

		type CompleteTaskPayload {
			task: Task
			clientMutationId: String
		}

		type Mutation {
			completeTask(input: CompleteTaskInput!) : CompleteTaskPayload!
		}

		type Subscription {
			nextAppointmentChange : Appointment
		}

		scalar DateTime

		enum TaskState {
			New
			Started
			Complete
		}

		type Appointment implements Node {
			id: ID!
			when: DateTime
			subject: String
			isNow: Boolean!
		}

		type Task implements Node {
			id: ID!
			title: String
			isComplete: Boolean!
		}

		type Folder implements Node {
			id: ID!
			name: String
			unreadCount: Int!
		})gql",
		"ParseTodaySchema");

	const bool result = parse<schema_document>(input);

	ASSERT_TRUE(result) << "we should be able to parse the doc";
}

TEST(PegtlCase, ParseVariableDefaultEmptyList)
{
	memory_input<> input(R"gql(
		query QueryWithEmptyListVariable($empty: [Boolean!]! = []) {
			fieldWithArg(arg: $empty)
		})gql",
		"ParseVariableDefaultEmptyList");

	const bool result = parse<executable_document>(input);

	ASSERT_TRUE(result) << "we should be able to parse the doc";
}

TEST(PegtlCase, AnalyzeMixedGrammar)
{
	ASSERT_EQ(0, analyze<mixed_document>(true))
		<< "there shouldn't be any infinite loops in the PEG version of the grammar";
}

TEST(PegtlCase, AnalyzeExecutableGrammar)
{
	ASSERT_EQ(0, analyze<executable_document>(true))
		<< "there shouldn't be any infinite loops in the PEG version of the grammar";
}

TEST(PegtlCase, AnalyzeSchemaGrammar)
{
	ASSERT_EQ(0, analyze<schema_document>(true))
		<< "there shouldn't be any infinite loops in the PEG version of the grammar";
}
