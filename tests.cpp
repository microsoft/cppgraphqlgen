// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "Today.h"
#include "GraphQLTree.h"
#include "GraphQLGrammar.h"

#include <tao/pegtl/analyze.hpp>

using namespace facebook::graphql;
using namespace facebook::graphql::peg;

using namespace tao::pegtl;

class TodayServiceCase : public ::testing::Test
{
protected:
	void SetUp() override
	{
		std::string fakeAppointmentId("fakeAppointmentId");
		_fakeAppointmentId.resize(fakeAppointmentId.size());
		std::copy(fakeAppointmentId.cbegin(), fakeAppointmentId.cend(), _fakeAppointmentId.begin());

		std::string fakeTaskId("fakeTaskId");
		_fakeTaskId.resize(fakeTaskId.size());
		std::copy(fakeTaskId.cbegin(), fakeTaskId.cend(), _fakeTaskId.begin());

		std::string fakeFolderId("fakeFolderId");
		_fakeFolderId.resize(fakeFolderId.size());
		std::copy(fakeFolderId.cbegin(), fakeFolderId.cend(), _fakeFolderId.begin());

		auto query = std::make_shared<today::Query>(
			[this]() -> std::vector<std::shared_ptr<today::Appointment>>
		{
			++_getAppointmentsCount;
			return { std::make_shared<today::Appointment>(std::vector<unsigned char>(_fakeAppointmentId), "tomorrow", "Lunch?", false) };
		}, [this]() -> std::vector<std::shared_ptr<today::Task>>
		{
			++_getTasksCount;
			return { std::make_shared<today::Task>(std::vector<unsigned char>(_fakeTaskId), "Don't forget", true) };
		}, [this]() -> std::vector<std::shared_ptr<today::Folder>>
		{
			++_getUnreadCountsCount;
			return { std::make_shared<today::Folder>(std::vector<unsigned char>(_fakeFolderId), "\"Fake\" Inbox", 3) };
		});
		auto mutation = std::make_shared<today::Mutation>(
			[](today::CompleteTaskInput&& input) -> std::shared_ptr<today::CompleteTaskPayload>
		{
			return std::make_shared<today::CompleteTaskPayload>(
				std::make_shared<today::Task>(std::move(input.id), "Mutated Task!", *(input.isComplete)),
				std::move(input.clientMutationId)
				);
		});
		auto subscription = std::make_shared<today::Subscription>();

		_service = std::make_shared<today::Operations>(query, mutation, subscription);
	}

	std::vector<unsigned char> _fakeAppointmentId;
	std::vector<unsigned char> _fakeTaskId;
	std::vector<unsigned char> _fakeFolderId;

	std::shared_ptr<today::Operations> _service;
	size_t _getAppointmentsCount = 0;
	size_t _getTasksCount = 0;
	size_t _getUnreadCountsCount = 0;
};

TEST_F(TodayServiceCase, QueryEverything)
{
	auto ast = peg::parseString(R"gql(
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
		})gql");
	auto result = _service->resolve(*ast, "Everything", web::json::value::object().as_object());
	EXPECT_EQ(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_EQ(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_EQ(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";

	try
	{
		ASSERT_TRUE(result.is_object());
		auto errorsItr = result.as_object().find(_XPLATSTR("errors"));
		if (errorsItr != result.as_object().cend())
		{
			utility::ostringstream_t errors;

			errors << errorsItr->second;
			FAIL() << utility::conversions::to_utf8string(errors.str());
		}
		auto data = service::ScalarArgument::require("data", result.as_object());

		auto appointmentEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges",
			service::ScalarArgument::require("appointments", data.as_object()).as_object());
		ASSERT_EQ(1, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].is_object()) << "appointment should be an object";
		auto appointmentNode = service::ScalarArgument::require("node", appointmentEdges[0].as_object());
		const web::json::object& appointment = appointmentNode.as_object();
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("id", appointment)) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointment)) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointment)) << "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointment)) << "isNow should match";

		auto taskEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges",
			service::ScalarArgument::require("tasks", data.as_object()).as_object());
		ASSERT_EQ(1, taskEdges.size()) << "tasks should have 1 entry";
		ASSERT_TRUE(taskEdges[0].is_object()) << "task should be an object";
		auto taskNode = service::ScalarArgument::require("node", taskEdges[0].as_object());
		const web::json::object& task = taskNode.as_object();
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("id", task)) << "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", task)) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", task)) << "isComplete should match";

		auto unreadCountEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges",
			service::ScalarArgument::require("unreadCounts", data.as_object()).as_object());
		ASSERT_EQ(1, unreadCountEdges.size()) << "unreadCounts should have 1 entry";
		ASSERT_TRUE(unreadCountEdges[0].is_object()) << "unreadCount should be an object";
		auto unreadCountNode = service::ScalarArgument::require("node", unreadCountEdges[0].as_object());
		const web::json::object& folder = unreadCountNode.as_object();
		EXPECT_EQ(_fakeFolderId, service::IdArgument::require("id", folder)) << "id should match in base64 encoding";
		EXPECT_EQ("\"Fake\" Inbox", service::StringArgument::require("name", folder)) << "name should match";
		EXPECT_EQ(3, service::IntArgument::require("unreadCount", folder)) << "isComplete should match";
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}
}

TEST_F(TodayServiceCase, QueryAppointments)
{
	auto ast = peg::parseString(R"gql({
			appointments {
				edges {
					node {
						appointmentId: id
						subject
						when
						isNow
					}
				}
			}
		})gql");
	auto result = _service->resolve(*ast, "", web::json::value::object().as_object());
	EXPECT_EQ(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";

	try
	{
		ASSERT_TRUE(result.is_object());
		auto errorsItr = result.as_object().find(_XPLATSTR("errors"));
		if (errorsItr != result.as_object().cend())
		{
			utility::ostringstream_t errors;

			errors << errorsItr->second;
			FAIL() << utility::conversions::to_utf8string(errors.str());
		}
		auto data = service::ScalarArgument::require("data", result.as_object());

		auto appointmentEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges",
			service::ScalarArgument::require("appointments", data.as_object()).as_object());
		ASSERT_EQ(1, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].is_object()) << "appointment should be an object";
		auto appointmentNode = service::ScalarArgument::require("node", appointmentEdges[0].as_object());
		const web::json::object& appointment = appointmentNode.as_object();
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("appointmentId", appointment)) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointment)) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointment)) << "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointment)) << "isNow should match";
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}
}

TEST_F(TodayServiceCase, QueryTasks)
{
	auto ast = peg::parseString(R"gql({
			tasks {
				edges {
					node {
						taskId: id
						title
						isComplete
					}
				}
			}
		})gql");
	auto result = _service->resolve(*ast, "", web::json::value::object().as_object());
	EXPECT_GE(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_EQ(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";

	try
	{
		ASSERT_TRUE(result.is_object());
		auto errorsItr = result.as_object().find(_XPLATSTR("errors"));
		if (errorsItr != result.as_object().cend())
		{
			utility::ostringstream_t errors;

			errors << errorsItr->second;
			FAIL() << utility::conversions::to_utf8string(errors.str());
		}
		auto data = service::ScalarArgument::require("data", result.as_object());

		auto taskEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges",
			service::ScalarArgument::require("tasks", data.as_object()).as_object());
		ASSERT_EQ(1, taskEdges.size()) << "tasks should have 1 entry";
		ASSERT_TRUE(taskEdges[0].is_object()) << "task should be an object";
		auto taskNode = service::ScalarArgument::require("node", taskEdges[0].as_object());
		const web::json::object& task = taskNode.as_object();
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("taskId", task)) << "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", task)) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", task)) << "isComplete should match";
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}
}

TEST_F(TodayServiceCase, QueryUnreadCounts)
{
	auto ast = peg::parseString(R"gql({
			unreadCounts {
				edges {
					node {
						folderId: id
						name
						unreadCount
					}
				}
			}
		})gql");
	auto result = _service->resolve(*ast, "", web::json::value::object().as_object());
	EXPECT_GE(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_EQ(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";

	try
	{
		ASSERT_TRUE(result.is_object());
		auto errorsItr = result.as_object().find(_XPLATSTR("errors"));
		if (errorsItr != result.as_object().cend())
		{
			utility::ostringstream_t errors;

			errors << errorsItr->second;
			FAIL() << utility::conversions::to_utf8string(errors.str());
		}
		auto data = service::ScalarArgument::require("data", result.as_object());

		auto unreadCountEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges",
			service::ScalarArgument::require("unreadCounts", data.as_object()).as_object());
		ASSERT_EQ(1, unreadCountEdges.size()) << "unreadCounts should have 1 entry";
		ASSERT_TRUE(unreadCountEdges[0].is_object()) << "unreadCount should be an object";
		auto unreadCountNode = service::ScalarArgument::require("node", unreadCountEdges[0].as_object());
		const web::json::object& folder = unreadCountNode.as_object();
		EXPECT_EQ(_fakeFolderId, service::IdArgument::require("folderId", folder)) << "id should match in base64 encoding";
		EXPECT_EQ("\"Fake\" Inbox", service::StringArgument::require("name", folder)) << "name should match";
		EXPECT_EQ(3, service::IntArgument::require("unreadCount", folder)) << "isComplete should match";
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}
}

TEST_F(TodayServiceCase, MutateCompleteTask)
{
	auto ast = peg::parseString(R"gql(mutation {
			completedTask: completeTask(input: {id: "ZmFrZVRhc2tJZA==", isComplete: true, clientMutationId: "Hi There!"}) {
				completedTask: task {
					completedTaskId: id
					title
					isComplete
				}
				clientMutationId
			}
		})gql");
	auto result = _service->resolve(*ast, "", web::json::value::object().as_object());

	try
	{
		ASSERT_TRUE(result.is_object());
		auto errorsItr = result.as_object().find(_XPLATSTR("errors"));
		if (errorsItr != result.as_object().cend())
		{
			utility::ostringstream_t errors;

			errors << errorsItr->second;
			FAIL() << utility::conversions::to_utf8string(errors.str());
		}
		auto data = service::ScalarArgument::require("data", result.as_object());

		auto completedTask = service::ScalarArgument::require("completedTask", data.as_object());
		ASSERT_TRUE(completedTask.is_object()) << "payload should be an object";

		auto task = service::ScalarArgument::require("completedTask", completedTask.as_object());
		EXPECT_TRUE(task.is_object()) << "should get back a task";
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("completedTaskId", task.as_object())) << "id should match in base64 encoding";
		EXPECT_EQ("Mutated Task!", service::StringArgument::require("title", task.as_object())) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", task.as_object())) << "isComplete should match";

		auto clientMutationId = service::StringArgument::require("clientMutationId", completedTask.as_object());
		EXPECT_EQ("Hi There!", clientMutationId) << "clientMutationId should match";
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}
}

TEST_F(TodayServiceCase, Introspection)
{
	auto ast = peg::parseString(R"gql({
			__schema {
				types {
					kind
					name
					description
					ofType
				}
				queryType {
					kind
					name
					fields {
						name
						args {
							name
							type {
								kind
								name
								ofType {
									kind
									name
								}
							}
						}
					}
				}
				mutationType {
					kind
					name
				}
			}
		})gql");
	auto result = _service->resolve(*ast, "", web::json::value::object().as_object());

	try
	{
		ASSERT_TRUE(result.is_object());
		auto errorsItr = result.as_object().find(_XPLATSTR("errors"));
		if (errorsItr != result.as_object().cend())
		{
			utility::ostringstream_t errors;

			errors << errorsItr->second;
			FAIL() << utility::conversions::to_utf8string(errors.str());
		}
		auto data = service::ScalarArgument::require("data", result.as_object());
		auto schema = service::ScalarArgument::require("__schema", data.as_object());
		auto types = service::ScalarArgument::require<service::TypeModifier::List>("types", schema.as_object());
		auto queryType = service::ScalarArgument::require("queryType", schema.as_object());
		auto mutationType = service::ScalarArgument::require("mutationType", schema.as_object());

		ASSERT_FALSE(types.empty());
		ASSERT_TRUE(queryType.is_object());
		ASSERT_TRUE(mutationType.is_object());
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}
}

TEST(ArgumentsCase, ListArgumentStrings)
{
	auto jsonListOfStrings = web::json::value::parse(_XPLATSTR(R"js({"value":[
		"string1",
		"string2",
		"string3"
	]})js"));
	std::vector<std::string> actual;

	try
	{
		actual = service::StringArgument::require<service::TypeModifier::List>("value", jsonListOfStrings.as_object());
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}

	ASSERT_EQ(3, actual.size()) << "should get 3 entries";
	EXPECT_EQ("string1", actual[0]) << "entry should match";
	EXPECT_EQ("string2", actual[1]) << "entry should match";
	EXPECT_EQ("string3", actual[2]) << "entry should match";
}

TEST(ArgumentsCase, ListArgumentStringsNonNullable)
{
	auto jsonListOfStrings = web::json::value::parse(_XPLATSTR(R"js({"value":[
		"string1",
		null,
		"string2",
		"string3"
	]})js"));
	bool caughtException = false;
	std::string exceptionWhat;

	try
	{
		service::StringArgument::require<service::TypeModifier::List>("value", jsonListOfStrings.as_object());
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		exceptionWhat = utility::conversions::to_utf8string(errors.str());
		caughtException = true;
	}

	ASSERT_TRUE(caughtException);
	EXPECT_EQ(R"js([{"message":"Invalid argument: value message: not a string"}])js", exceptionWhat) << "exception should match";
}

TEST(ArgumentsCase, ListArgumentStringsNullable)
{
	auto jsonListOfStrings = web::json::value::parse(_XPLATSTR(R"js({"value":[
		"string1",
		"string2",
		null,
		"string3"
	]})js"));
	std::vector<std::unique_ptr<std::string>> actual;

	try
	{
		actual = service::StringArgument::require<
			service::TypeModifier::List,
			service::TypeModifier::Nullable
		>("value", jsonListOfStrings.as_object());
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}

	ASSERT_EQ(4, actual.size()) << "should get 4 entries";
	ASSERT_NE(nullptr, actual[0]) << "should not be null";
	EXPECT_EQ("string1", *actual[0]) << "entry should match";
	ASSERT_NE(nullptr, actual[1]) << "should not be null";
	EXPECT_EQ("string2", *actual[1]) << "entry should match";
	EXPECT_EQ(nullptr, actual[2]) << "should be null";
	ASSERT_NE(nullptr, actual[3]) << "should not be null";
	EXPECT_EQ("string3", *actual[3]) << "entry should match";
}

TEST(ArgumentsCase, ListArgumentListArgumentStrings)
{
	auto jsonListOfListOfStrings = web::json::value::parse(_XPLATSTR(R"js({"value":[
		["list1string1", "list1string2"],
		["list2string1", "list2string2"]
	]})js"));
	std::vector<std::vector<std::string>> actual;

	try
	{
		actual = service::StringArgument::require<
			service::TypeModifier::List,
			service::TypeModifier::List
		>("value", jsonListOfListOfStrings.as_object());
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}

	ASSERT_EQ(2, actual.size()) << "should get 2 entries";
	ASSERT_EQ(2, actual[0].size()) << "should get 2 entries";
	EXPECT_EQ("list1string1", actual[0][0]) << "entry should match";
	EXPECT_EQ("list1string2", actual[0][1]) << "entry should match";
	ASSERT_EQ(2, actual[1].size()) << "should get 2 entries";
	EXPECT_EQ("list2string1", actual[1][0]) << "entry should match";
	EXPECT_EQ("list2string2", actual[1][1]) << "entry should match";
}

TEST(ArgumentsCase, ListArgumentNullableListArgumentStrings)
{
	auto jsonListOfListOfStrings = web::json::value::parse(_XPLATSTR(R"js({"value":[
		null,
		["list2string1", "list2string2"]
	]})js"));
	std::vector<std::unique_ptr<std::vector<std::string>>> actual;

	try
	{
		actual = service::StringArgument::require<
			service::TypeModifier::List,
			service::TypeModifier::Nullable,
			service::TypeModifier::List
		>("value", jsonListOfListOfStrings.as_object());
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}

	ASSERT_EQ(2, actual.size()) << "should get 2 entries";
	EXPECT_EQ(nullptr, actual[0].get()) << "should be null";
	ASSERT_EQ(2, actual[1]->size()) << "should get 2 entries";
	EXPECT_EQ("list2string1", (*actual[1])[0]) << "entry should match";
	EXPECT_EQ("list2string2", (*actual[1])[1]) << "entry should match";
}

TEST(ArgumentsCase, TaskStateEnum)
{
	auto jsonTaskState = web::json::value::parse(_XPLATSTR(R"js({"status":"Started"})js"));
	today::TaskState actual = static_cast<today::TaskState>(-1);

	try
	{
		actual = service::ModifiedArgument<today::TaskState>::require("status", jsonTaskState.as_object());
	}
	catch (const service::schema_exception& ex)
	{
		utility::ostringstream_t errors;

		errors << ex.getErrors();
		FAIL() << errors.str();
	}

	EXPECT_EQ(today::TaskState::Started, actual) << "should parse the enum";
}

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
		})gql", "ParseKitchenSinkQuery");

	const bool result = parse<document>(input);

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
		   | INLINE_FRAGMENT)gql", "ParseKitchenSinkSchema");

	const bool result = parse<document>(input);

	ASSERT_TRUE(result) << "we should be able to parse the doc";
}

TEST(PegtlCase, ParseKitchenSink)
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
		})gql", "ParseKitchenSinkSchema");

	const bool result = parse<document>(input);

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
		})gql", "ParseTodayQuery");

	const bool result = parse<document>(input);

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
		})gql", "ParseTodaySchema");

	const bool result = parse<document>(input);

	ASSERT_TRUE(result) << "we should be able to parse the doc";
}

TEST(PegtlCase, AnalyzeGrammar)
{
	ASSERT_EQ(0, analyze<document>(true)) << "there shuldn't be any infinite loops in the PEG version of the grammar";
}