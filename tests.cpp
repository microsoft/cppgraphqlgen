// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "Today.h"
#include "GraphQLTree.h"
#include "GraphQLGrammar.h"

#include <tao/pegtl/analyze.hpp>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace facebook::graphql;
using namespace facebook::graphql::peg;

using namespace tao::graphqlpeg;

class TodayServiceCase : public ::testing::Test
{
public:
	static void SetUpTestCase()
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
			[]() -> std::vector<std::shared_ptr<today::Appointment>>
		{
			++_getAppointmentsCount;
			return { std::make_shared<today::Appointment>(std::vector<uint8_t>(_fakeAppointmentId), "tomorrow", "Lunch?", false) };
		}, []() -> std::vector<std::shared_ptr<today::Task>>
		{
			++_getTasksCount;
			return { std::make_shared<today::Task>(std::vector<uint8_t>(_fakeTaskId), "Don't forget", true) };
		}, []() -> std::vector<std::shared_ptr<today::Folder>>
		{
			++_getUnreadCountsCount;
			return { std::make_shared<today::Folder>(std::vector<uint8_t>(_fakeFolderId), "\"Fake\" Inbox", 3) };
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

	static void TearDownTestCase()
	{
		_fakeAppointmentId.clear();
		_fakeTaskId.clear();
		_fakeFolderId.clear();
		_service.reset();
	}

protected:
	static std::vector<uint8_t> _fakeAppointmentId;
	static std::vector<uint8_t> _fakeTaskId;
	static std::vector<uint8_t> _fakeFolderId;

	static std::shared_ptr<today::Operations> _service;
	static size_t _getAppointmentsCount;
	static size_t _getTasksCount;
	static size_t _getUnreadCountsCount;
};

std::vector<uint8_t> TodayServiceCase::_fakeAppointmentId;
std::vector<uint8_t> TodayServiceCase::_fakeTaskId;
std::vector<uint8_t> TodayServiceCase::_fakeFolderId;

std::shared_ptr<today::Operations> TodayServiceCase::_service;
size_t TodayServiceCase::_getAppointmentsCount = 0;
size_t TodayServiceCase::_getTasksCount = 0;
size_t TodayServiceCase::_getUnreadCountsCount = 0;

TEST_F(TodayServiceCase, QueryEverything)
{
	auto ast = R"(
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
		})"_graphql;
	const rapidjson::Document variables(rapidjson::Type::kObjectType);
	auto result = _service->resolve(0, *ast->root, "Everything", variables.GetObject()).get();
	auto& allocator = result.GetAllocator();
	EXPECT_EQ(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_EQ(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_EQ(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";

	try
	{
		ASSERT_TRUE(result.IsObject());
		auto errorsItr = result.FindMember("errors");
		if (errorsItr != result.MemberEnd())
		{
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

			errorsItr->value.Accept(writer);

			FAIL() << buffer.GetString();
		}
		const auto data = service::ScalarArgument::require(allocator, "data", const_cast<const rapidjson::Document&>(result).GetObject());
		
		const auto appointments = service::ScalarArgument::require(allocator, "appointments", data.GetObject());
		const auto appointmentEdges = service::ScalarArgument::require<service::TypeModifier::List>(allocator, "edges", appointments.GetObject());
		ASSERT_EQ(1, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].IsObject()) << "appointment should be an object";
		const auto appointmentNode = service::ScalarArgument::require(allocator, "node", appointmentEdges[0].GetObject());
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require(allocator, "id", appointmentNode.GetObject())) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require(allocator, "subject", appointmentNode.GetObject())) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require(allocator, "when", appointmentNode.GetObject())) << "when should match";
		EXPECT_FALSE(service::BooleanArgument::require(allocator, "isNow", appointmentNode.GetObject())) << "isNow should match";

		const auto tasks = service::ScalarArgument::require(allocator, "tasks", data.GetObject());
		const auto taskEdges = service::ScalarArgument::require<service::TypeModifier::List>(allocator, "edges", tasks.GetObject());
		ASSERT_EQ(1, taskEdges.size()) << "tasks should have 1 entry";
		ASSERT_TRUE(taskEdges[0].IsObject()) << "task should be an object";
		const auto taskNode = service::ScalarArgument::require(allocator, "node", taskEdges[0].GetObject());
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require(allocator, "id", taskNode.GetObject())) << "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require(allocator, "title", taskNode.GetObject())) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require(allocator, "isComplete", taskNode.GetObject())) << "isComplete should match";

		const auto unreadCounts = service::ScalarArgument::require(allocator, "unreadCounts", data.GetObject());
		const auto unreadCountEdges = service::ScalarArgument::require<service::TypeModifier::List>(allocator, "edges", unreadCounts.GetObject());
		ASSERT_EQ(1, unreadCountEdges.size()) << "unreadCounts should have 1 entry";
		ASSERT_TRUE(unreadCountEdges[0].IsObject()) << "unreadCount should be an object";
		const auto unreadCountNode = service::ScalarArgument::require(allocator, "node", unreadCountEdges[0].GetObject());
		EXPECT_EQ(_fakeFolderId, service::IdArgument::require(allocator, "id", unreadCountNode.GetObject())) << "id should match in base64 encoding";
		EXPECT_EQ("\"Fake\" Inbox", service::StringArgument::require(allocator, "name", unreadCountNode.GetObject())) << "name should match";
		EXPECT_EQ(3, service::IntArgument::require(allocator, "unreadCount", unreadCountNode.GetObject())) << "unreadCount should match";
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
	}
}

TEST_F(TodayServiceCase, QueryAppointments)
{
	auto ast = R"({
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
		})"_graphql;
	const rapidjson::Document variables(rapidjson::Type::kObjectType);
	auto result = _service->resolve(1, *ast->root, "", variables.GetObject()).get();
	auto& allocator = result.GetAllocator();
	EXPECT_EQ(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";

	try
	{
		ASSERT_TRUE(result.IsObject());
		auto errorsItr = result.FindMember("errors");
		if (errorsItr != result.MemberEnd())
		{
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

			errorsItr->value.Accept(writer);

			FAIL() << buffer.GetString();
		}
		const auto data = service::ScalarArgument::require(allocator, "data", const_cast<const rapidjson::Document&>(result).GetObject());

		const auto appointments = service::ScalarArgument::require(allocator, "appointments", data.GetObject());
		const auto appointmentEdges = service::ScalarArgument::require<service::TypeModifier::List>(allocator, "edges", appointments.GetObject());
		ASSERT_EQ(1, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].IsObject()) << "appointment should be an object";
		const auto appointmentNode = service::ScalarArgument::require(allocator, "node", appointmentEdges[0].GetObject());
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require(allocator, "appointmentId", appointmentNode.GetObject())) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require(allocator, "subject", appointmentNode.GetObject())) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require(allocator, "when", appointmentNode.GetObject())) << "when should match";
		EXPECT_FALSE(service::BooleanArgument::require(allocator, "isNow", appointmentNode.GetObject())) << "isNow should match";
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
	}
}

TEST_F(TodayServiceCase, QueryTasks)
{
	auto ast = R"gql({
			tasks {
				edges {
					node {
						taskId: id
						title
						isComplete
					}
				}
			}
		})gql"_graphql;
	const rapidjson::Document variables(rapidjson::Type::kObjectType);
	auto result = _service->resolve(2, *ast->root, "", variables.GetObject()).get();
	auto& allocator = result.GetAllocator();
	EXPECT_GE(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_EQ(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";

	try
	{
		ASSERT_TRUE(result.IsObject());
		auto errorsItr = result.FindMember("errors");
		if (errorsItr != result.MemberEnd())
		{
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

			errorsItr->value.Accept(writer);

			FAIL() << buffer.GetString();
		}
		const auto data = service::ScalarArgument::require(allocator, "data", const_cast<const rapidjson::Document&>(result).GetObject());

		const auto tasks = service::ScalarArgument::require(allocator, "tasks", data.GetObject());
		const auto taskEdges = service::ScalarArgument::require<service::TypeModifier::List>(allocator, "edges", tasks.GetObject());
		ASSERT_EQ(1, taskEdges.size()) << "tasks should have 1 entry";
		ASSERT_TRUE(taskEdges[0].IsObject()) << "task should be an object";
		const auto taskNode = service::ScalarArgument::require(allocator, "node", taskEdges[0].GetObject());
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require(allocator, "taskId", taskNode.GetObject())) << "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require(allocator, "title", taskNode.GetObject())) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require(allocator, "isComplete", taskNode.GetObject())) << "isComplete should match";
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
	}
}

TEST_F(TodayServiceCase, QueryUnreadCounts)
{
	auto ast = R"({
			unreadCounts {
				edges {
					node {
						folderId: id
						name
						unreadCount
					}
				}
			}
		})"_graphql;
	const rapidjson::Document variables(rapidjson::Type::kObjectType);
	auto result = _service->resolve(3, *ast->root, "", variables.GetObject()).get();
	auto& allocator = result.GetAllocator();
	EXPECT_GE(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_EQ(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";

	try
	{
		ASSERT_TRUE(result.IsObject());
		auto errorsItr = result.FindMember("errors");
		if (errorsItr != result.MemberEnd())
		{
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

			errorsItr->value.Accept(writer);

			FAIL() << buffer.GetString();
		}
		const auto data = service::ScalarArgument::require(allocator, "data", const_cast<const rapidjson::Document&>(result).GetObject());

		const auto unreadCounts = service::ScalarArgument::require(allocator, "unreadCounts", data.GetObject());
		const auto unreadCountEdges = service::ScalarArgument::require<service::TypeModifier::List>(allocator, "edges", unreadCounts.GetObject());
		ASSERT_EQ(1, unreadCountEdges.size()) << "unreadCounts should have 1 entry";
		ASSERT_TRUE(unreadCountEdges[0].IsObject()) << "unreadCount should be an object";
		const auto unreadCountNode = service::ScalarArgument::require(allocator, "node", unreadCountEdges[0].GetObject());
		EXPECT_EQ(_fakeFolderId, service::IdArgument::require(allocator, "folderId", unreadCountNode.GetObject())) << "id should match in base64 encoding";
		EXPECT_EQ("\"Fake\" Inbox", service::StringArgument::require(allocator, "name", unreadCountNode.GetObject())) << "name should match";
		EXPECT_EQ(3, service::IntArgument::require(allocator, "unreadCount", unreadCountNode.GetObject())) << "unreadCount should match";
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
	}
}

TEST_F(TodayServiceCase, MutateCompleteTask)
{
	auto ast = R"(mutation {
			completedTask: completeTask(input: {id: "ZmFrZVRhc2tJZA==", isComplete: true, clientMutationId: "Hi There!"}) {
				completedTask: task {
					completedTaskId: id
					title
					isComplete
				}
				clientMutationId
			}
		})"_graphql;
	const rapidjson::Document variables(rapidjson::Type::kObjectType);
	auto result = _service->resolve(4, *ast->root, "", variables.GetObject()).get();
	auto& allocator = result.GetAllocator();

	try
	{
		ASSERT_TRUE(result.IsObject());
		auto errorsItr = result.FindMember("errors");
		if (errorsItr != result.MemberEnd())
		{
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

			errorsItr->value.Accept(writer);

			FAIL() << buffer.GetString();
		}
		const auto data = service::ScalarArgument::require(allocator, "data", const_cast<const rapidjson::Document&>(result).GetObject());

		const auto completedTask = service::ScalarArgument::require(allocator, "completedTask", data.GetObject());
		ASSERT_TRUE(completedTask.IsObject()) << "payload should be an object";

		const auto task = service::ScalarArgument::require(allocator, "completedTask", completedTask.GetObject());
		EXPECT_TRUE(task.IsObject()) << "should get back a task";
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require(allocator, "completedTaskId", task.GetObject())) << "id should match in base64 encoding";
		EXPECT_EQ("Mutated Task!", service::StringArgument::require(allocator, "title", task.GetObject())) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require(allocator, "isComplete", task.GetObject())) << "isComplete should match";

		const auto clientMutationId = service::StringArgument::require(allocator, "clientMutationId", completedTask.GetObject());
		EXPECT_EQ("Hi There!", clientMutationId) << "clientMutationId should match";
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
	}
}

TEST_F(TodayServiceCase, Introspection)
{
	auto ast = R"({
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
		})"_graphql;
	const rapidjson::Document variables(rapidjson::Type::kObjectType);
	auto result = _service->resolve(5, *ast->root, "", variables.GetObject()).get();
	auto& allocator = result.GetAllocator();

	try
	{
		ASSERT_TRUE(result.IsObject());
		auto errorsItr = result.FindMember("errors");
		if (errorsItr != result.MemberEnd())
		{
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

			errorsItr->value.Accept(writer);

			FAIL() << buffer.GetString();
		}
		const auto data = service::ScalarArgument::require(allocator, "data", const_cast<const rapidjson::Document&>(result).GetObject());
		const auto schema = service::ScalarArgument::require(allocator, "__schema", data.GetObject());
		const auto types = service::ScalarArgument::require<service::TypeModifier::List>(allocator, "types", schema.GetObject());
		const auto queryType = service::ScalarArgument::require(allocator, "queryType", schema.GetObject());
		const auto mutationType = service::ScalarArgument::require(allocator, "mutationType", schema.GetObject());

		ASSERT_FALSE(types.empty());
		ASSERT_TRUE(queryType.IsObject());
		ASSERT_TRUE(mutationType.IsObject());
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
	}
}

TEST(ArgumentsCase, ListArgumentStrings)
{
	rapidjson::Document parsed;
	parsed.Parse(R"js({"value":[
		"string1",
		"string2",
		"string3"
	]})js");
	std::vector<std::string> actual;

	try
	{
		actual = service::StringArgument::require<service::TypeModifier::List>(parsed.GetAllocator(), "value", const_cast<const rapidjson::Document&>(parsed).GetObject());
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
	}

	ASSERT_EQ(3, actual.size()) << "should get 3 entries";
	EXPECT_EQ("string1", actual[0]) << "entry should match";
	EXPECT_EQ("string2", actual[1]) << "entry should match";
	EXPECT_EQ("string3", actual[2]) << "entry should match";
}

TEST(ArgumentsCase, ListArgumentStringsNonNullable)
{
	rapidjson::Document parsed;
	parsed.Parse(R"js({"value":[
		"string1",
		null,
		"string2",
		"string3"
	]})js");
	bool caughtException = false;
	std::string exceptionWhat;

	try
	{
		service::StringArgument::require<service::TypeModifier::List>(parsed.GetAllocator(), "value", const_cast<const rapidjson::Document&>(parsed).GetObject());
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		exceptionWhat = buffer.GetString();
		caughtException = true;
	}

	ASSERT_TRUE(caughtException);
	EXPECT_EQ(R"js([{"message":"Invalid argument: value message: not a string"}])js", exceptionWhat) << "exception should match";
}

TEST(ArgumentsCase, ListArgumentStringsNullable)
{
	rapidjson::Document parsed;
	parsed.Parse(R"js({"value":[
		"string1",
		"string2",
		null,
		"string3"
	]})js");
	std::vector<std::unique_ptr<std::string>> actual;

	try
	{
		actual = service::StringArgument::require<
			service::TypeModifier::List,
			service::TypeModifier::Nullable
		>(parsed.GetAllocator(), "value", const_cast<const rapidjson::Document&>(parsed).GetObject());
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
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
	rapidjson::Document parsed;
	parsed.Parse(R"js({"value":[
		["list1string1", "list1string2"],
		["list2string1", "list2string2"]
	]})js");
	std::vector<std::vector<std::string>> actual;

	try
	{
		actual = service::StringArgument::require<
			service::TypeModifier::List,
			service::TypeModifier::List
		>(parsed.GetAllocator(), "value", const_cast<const rapidjson::Document&>(parsed).GetObject());
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
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
	rapidjson::Document parsed;
	parsed.Parse(R"js({"value":[
		null,
		["list2string1", "list2string2"]
	]})js");
	std::vector<std::unique_ptr<std::vector<std::string>>> actual;

	try
	{
		actual = service::StringArgument::require<
			service::TypeModifier::List,
			service::TypeModifier::Nullable,
			service::TypeModifier::List
		>(parsed.GetAllocator(), "value", const_cast<const rapidjson::Document&>(parsed).GetObject());
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
	}

	ASSERT_EQ(2, actual.size()) << "should get 2 entries";
	EXPECT_EQ(nullptr, actual[0].get()) << "should be null";
	ASSERT_EQ(2, actual[1]->size()) << "should get 2 entries";
	EXPECT_EQ("list2string1", (*actual[1])[0]) << "entry should match";
	EXPECT_EQ("list2string2", (*actual[1])[1]) << "entry should match";
}

TEST(ArgumentsCase, TaskStateEnum)
{
	rapidjson::Document parsed;
	parsed.Parse(R"js({"status":"Started"})js");
	today::TaskState actual = static_cast<today::TaskState>(-1);

	try
	{
		actual = service::ModifiedArgument<today::TaskState>::require(parsed.GetAllocator(), "status", const_cast<const rapidjson::Document&>(parsed).GetObject());
	}
	catch (const service::schema_exception& ex)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		ex.getErrors().Accept(writer);

		FAIL() << buffer.GetString();
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