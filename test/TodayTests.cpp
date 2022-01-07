// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "TodayMock.h"

#include "graphqlservice/JSONResponse.h"

#include <chrono>

using namespace graphql;

using namespace std::literals;

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
	}

	void SetUp() override
	{
		auto query = std::make_shared<today::Query>(
			[this]() -> std::vector<std::shared_ptr<today::Appointment>> {
				++_getAppointmentsCount;
				return { std::make_shared<today::Appointment>(response::IdType(_fakeAppointmentId),
					"tomorrow",
					"Lunch?",
					false) };
			},
			[this]() -> std::vector<std::shared_ptr<today::Task>> {
				++_getTasksCount;
				return { std::make_shared<today::Task>(response::IdType(_fakeTaskId),
					"Don't forget",
					true) };
			},
			[this]() -> std::vector<std::shared_ptr<today::Folder>> {
				++_getUnreadCountsCount;
				return { std::make_shared<today::Folder>(response::IdType(_fakeFolderId),
					"\"Fake\" Inbox",
					3) };
			});
		auto mutation = std::make_shared<today::Mutation>(
			[](today::CompleteTaskInput&& input) -> std::shared_ptr<today::CompleteTaskPayload> {
				return std::make_shared<today::CompleteTaskPayload>(
					std::make_shared<today::Task>(std::move(input.id),
						"Mutated Task!",
						*(input.isComplete)),
					std::move(input.clientMutationId));
			});
		auto subscription = std::make_shared<today::NextAppointmentChange>(
			[](const std::shared_ptr<service::RequestState>&)
				-> std::shared_ptr<today::Appointment> {
				return { std::make_shared<today::Appointment>(response::IdType(_fakeAppointmentId),
					"tomorrow",
					"Lunch?",
					true) };
			});

		_service = std::make_shared<today::Operations>(query, mutation, subscription);
	}

	void TearDown() override
	{
		_service.reset();
	}

	static void TearDownTestCase()
	{
		_fakeAppointmentId.clear();
		_fakeTaskId.clear();
		_fakeFolderId.clear();
	}

protected:
	static response::IdType _fakeAppointmentId;
	static response::IdType _fakeTaskId;
	static response::IdType _fakeFolderId;

	std::shared_ptr<today::Operations> _service {};
	size_t _getAppointmentsCount {};
	size_t _getTasksCount {};
	size_t _getUnreadCountsCount {};
};

response::IdType TodayServiceCase::_fakeAppointmentId;
response::IdType TodayServiceCase::_fakeTaskId;
response::IdType TodayServiceCase::_fakeFolderId;

TEST_F(TodayServiceCase, QueryEverything)
{
	auto query = R"(
		query Everything {
			appointments {
				edges {
					node {
						id
						subject
						when
						isNow
						__typename
					}
				}
			}
			tasks {
				edges {
					node {
						id
						title
						isComplete
						__typename
					}
				}
			}
			unreadCounts {
				edges {
					node {
						id
						name
						unreadCount
						__typename
					}
				}
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(1);
	auto result =
		_service->resolve(
					{ query, "Everything"sv, std::move(variables), std::launch::async, state })
			.get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_EQ(size_t(1), _getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_EQ(size_t(1), _getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(1), state->appointmentsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(size_t(1), state->tasksRequestId) << "today service passed the same RequestState";
	EXPECT_EQ(size_t(1), state->unreadCountsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(size_t(1), state->loadAppointmentsCount) << "today service called the loader once";
	EXPECT_EQ(size_t(1), state->loadTasksCount) << "today service called the loader once";
	EXPECT_EQ(size_t(1), state->loadUnreadCountsCount) << "today service called the loader once";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointments = service::ScalarArgument::require("appointments", data);
		const auto appointmentEdges =
			service::ScalarArgument::require<service::TypeModifier::List>("edges", appointments);
		ASSERT_EQ(size_t { 1 }, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].type() == response::Type::Map)
			<< "appointment should be an object";
		const auto appointmentNode = service::ScalarArgument::require("node", appointmentEdges[0]);
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("id", appointmentNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode))
			<< "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode))
			<< "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentNode))
			<< "isNow should match";
		EXPECT_EQ("Appointment", service::StringArgument::require("__typename", appointmentNode))
			<< "__typename should match";

		const auto tasks = service::ScalarArgument::require("tasks", data);
		const auto taskEdges =
			service::ScalarArgument::require<service::TypeModifier::List>("edges", tasks);
		ASSERT_EQ(size_t { 1 }, taskEdges.size()) << "tasks should have 1 entry";
		ASSERT_TRUE(taskEdges[0].type() == response::Type::Map) << "task should be an object";
		const auto taskNode = service::ScalarArgument::require("node", taskEdges[0]);
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("id", taskNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", taskNode))
			<< "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", taskNode))
			<< "isComplete should match";
		EXPECT_EQ("Task", service::StringArgument::require("__typename", taskNode))
			<< "__typename should match";

		const auto unreadCounts = service::ScalarArgument::require("unreadCounts", data);
		const auto unreadCountEdges =
			service::ScalarArgument::require<service::TypeModifier::List>("edges", unreadCounts);
		ASSERT_EQ(size_t { 1 }, unreadCountEdges.size()) << "unreadCounts should have 1 entry";
		ASSERT_TRUE(unreadCountEdges[0].type() == response::Type::Map)
			<< "unreadCount should be an object";
		const auto unreadCountNode = service::ScalarArgument::require("node", unreadCountEdges[0]);
		EXPECT_EQ(_fakeFolderId, service::IdArgument::require("id", unreadCountNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("\"Fake\" Inbox", service::StringArgument::require("name", unreadCountNode))
			<< "name should match";
		EXPECT_EQ(3, service::IntArgument::require("unreadCount", unreadCountNode))
			<< "unreadCount should match";
		EXPECT_EQ("Folder", service::StringArgument::require("__typename", unreadCountNode))
			<< "__typename should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, QueryAppointments)
{
	auto query = R"({
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
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(2);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(2), state->appointmentsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(size_t(0), state->tasksRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->unreadCountsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(1), state->loadAppointmentsCount) << "today service called the loader once";
	EXPECT_EQ(size_t(0), state->loadTasksCount) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->loadUnreadCountsCount) << "today service did not call the loader";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointments = service::ScalarArgument::require("appointments", data);
		const auto appointmentEdges =
			service::ScalarArgument::require<service::TypeModifier::List>("edges", appointments);
		ASSERT_EQ(size_t { 1 }, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].type() == response::Type::Map)
			<< "appointment should be an object";
		const auto appointmentNode = service::ScalarArgument::require("node", appointmentEdges[0]);
		EXPECT_EQ(_fakeAppointmentId,
			service::IdArgument::require("appointmentId", appointmentNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode))
			<< "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode))
			<< "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentNode))
			<< "isNow should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, QueryAppointmentsWithForceError)
{
	auto query = R"({
			appointments {
				edges {
					node {
						appointmentId: id
						subject
						when
						isNow
						forceError
					}
				}
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(2);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(2), state->appointmentsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(size_t(0), state->tasksRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->unreadCountsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(1), state->loadAppointmentsCount) << "today service called the loader once";
	EXPECT_EQ(size_t(0), state->loadTasksCount) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->loadUnreadCountsCount) << "today service did not call the loader";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr == result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(result)) << "no errors returned";
		}

		auto errorsString = response::toJSON(response::Value(errorsItr->second));
		EXPECT_EQ(
			R"js([{"message":"Field error name: forceError unknown error: this error was forced","locations":[{"line":9,"column":7}],"path":["appointments","edges",0,"node","forceError"]}])js",
			errorsString)
			<< "error should match";

		const auto data = service::ScalarArgument::require("data", result);

		const auto appointments = service::ScalarArgument::require("appointments", data);
		const auto appointmentEdges =
			service::ScalarArgument::require<service::TypeModifier::List>("edges", appointments);
		ASSERT_EQ(size_t { 1 }, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].type() == response::Type::Map)
			<< "appointment should be an object";
		const auto appointmentNode = service::ScalarArgument::require("node", appointmentEdges[0]);
		EXPECT_EQ(_fakeAppointmentId,
			service::IdArgument::require("appointmentId", appointmentNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode))
			<< "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode))
			<< "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentNode))
			<< "isNow should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, QueryAppointmentsWithForceErrorAsync)
{
	auto query = R"({
			appointments {
				edges {
					node {
						appointmentId: id
						subject
						when
						isNow
						forceError
					}
				}
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(2);
	auto result =
		_service->resolve({ query, {}, std::move(variables), std::launch::async, state }).get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(2), state->appointmentsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(size_t(0), state->tasksRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->unreadCountsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(1), state->loadAppointmentsCount) << "today service called the loader once";
	EXPECT_EQ(size_t(0), state->loadTasksCount) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->loadUnreadCountsCount) << "today service did not call the loader";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr == result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(result)) << "no errors returned";
		}

		auto errorsString = response::toJSON(response::Value(errorsItr->second));
		EXPECT_EQ(
			R"js([{"message":"Field error name: forceError unknown error: this error was forced","locations":[{"line":9,"column":7}],"path":["appointments","edges",0,"node","forceError"]}])js",
			errorsString)
			<< "error should match";

		const auto data = service::ScalarArgument::require("data", result);

		const auto appointments = service::ScalarArgument::require("appointments", data);
		const auto appointmentEdges =
			service::ScalarArgument::require<service::TypeModifier::List>("edges", appointments);
		ASSERT_EQ(size_t { 1 }, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].type() == response::Type::Map)
			<< "appointment should be an object";
		const auto appointmentNode = service::ScalarArgument::require("node", appointmentEdges[0]);
		EXPECT_EQ(_fakeAppointmentId,
			service::IdArgument::require("appointmentId", appointmentNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode))
			<< "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode))
			<< "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentNode))
			<< "isNow should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, QueryTasks)
{
	auto query = R"gql({
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
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(3);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();
	EXPECT_GE(size_t(1), _getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_EQ(size_t(1), _getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(0), state->appointmentsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(3), state->tasksRequestId) << "today service passed the same RequestState";
	EXPECT_EQ(size_t(0), state->unreadCountsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->loadAppointmentsCount) << "today service did not call the loader";
	EXPECT_EQ(size_t(1), state->loadTasksCount) << "today service called the loader once";
	EXPECT_EQ(size_t(0), state->loadUnreadCountsCount) << "today service did not call the loader";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto tasks = service::ScalarArgument::require("tasks", data);
		const auto taskEdges =
			service::ScalarArgument::require<service::TypeModifier::List>("edges", tasks);
		ASSERT_EQ(size_t { 1 }, taskEdges.size()) << "tasks should have 1 entry";
		ASSERT_TRUE(taskEdges[0].type() == response::Type::Map) << "task should be an object";
		const auto taskNode = service::ScalarArgument::require("node", taskEdges[0]);
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("taskId", taskNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", taskNode))
			<< "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", taskNode))
			<< "isComplete should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, QueryUnreadCounts)
{
	auto query = R"({
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
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(4);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();
	EXPECT_GE(size_t(1), _getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_EQ(size_t(1), _getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(0), state->appointmentsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->tasksRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(4), state->unreadCountsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(size_t(0), state->loadAppointmentsCount) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->loadTasksCount) << "today service did not call the loader";
	EXPECT_EQ(size_t(1), state->loadUnreadCountsCount) << "today service called the loader once";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto unreadCounts = service::ScalarArgument::require("unreadCounts", data);
		const auto unreadCountEdges =
			service::ScalarArgument::require<service::TypeModifier::List>("edges", unreadCounts);
		ASSERT_EQ(size_t { 1 }, unreadCountEdges.size()) << "unreadCounts should have 1 entry";
		ASSERT_TRUE(unreadCountEdges[0].type() == response::Type::Map)
			<< "unreadCount should be an object";
		const auto unreadCountNode = service::ScalarArgument::require("node", unreadCountEdges[0]);
		EXPECT_EQ(_fakeFolderId, service::IdArgument::require("folderId", unreadCountNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("\"Fake\" Inbox", service::StringArgument::require("name", unreadCountNode))
			<< "name should match";
		EXPECT_EQ(3, service::IntArgument::require("unreadCount", unreadCountNode))
			<< "unreadCount should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, MutateCompleteTask)
{
	auto query = R"(mutation {
			completedTask: completeTask(input: {id: "ZmFrZVRhc2tJZA==", isComplete: true, clientMutationId: "Hi There!"}) {
				completedTask: task {
					completedTaskId: id
					title
					isComplete
				}
				clientMutationId
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(5);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto completedTask = service::ScalarArgument::require("completedTask", data);
		ASSERT_TRUE(completedTask.type() == response::Type::Map) << "payload should be an object";

		const auto task = service::ScalarArgument::require("completedTask", completedTask);
		EXPECT_TRUE(task.type() == response::Type::Map) << "should get back a task";
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("completedTaskId", task))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Mutated Task!", service::StringArgument::require("title", task))
			<< "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", task))
			<< "isComplete should match";

		const auto clientMutationId =
			service::StringArgument::require("clientMutationId", completedTask);
		EXPECT_EQ("Hi There!", clientMutationId) << "clientMutationId should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, SubscribeNextAppointmentChangeDefault)
{
	auto query = peg::parseString(R"(subscription TestSubscription {
			nextAppointment: nextAppointmentChange {
				nextAppointmentId: id
				when
				subject
				isNow
			}
		})");
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(6);
	response::Value result;
	auto key = _service
				   ->subscribe({ [&result](response::Value&& response) {
									result = std::move(response);
								},
					   std::move(query),
					   "TestSubscription"s,
					   std::move(variables),
					   {},
					   state })
				   .get();
	_service->deliver({ "nextAppointmentChange"sv }).get();
	_service->unsubscribe({ key }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointmentNode = service::ScalarArgument::require("nextAppointment", data);
		EXPECT_EQ(_fakeAppointmentId,
			service::IdArgument::require("nextAppointmentId", appointmentNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode))
			<< "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode))
			<< "when should match";
		EXPECT_TRUE(service::BooleanArgument::require("isNow", appointmentNode))
			<< "isNow should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, SubscribeNextAppointmentChangeOverride)
{
	auto query = peg::parseString(R"(subscription TestSubscription {
			nextAppointment: nextAppointmentChange {
				nextAppointmentId: id
				when
				subject
				isNow
			}
		})");
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(7);
	auto subscriptionObject = std::make_shared<today::NextAppointmentChange>(
		[](const std::shared_ptr<service::RequestState>& state)
			-> std::shared_ptr<today::Appointment> {
			EXPECT_EQ(size_t { 7 }, std::static_pointer_cast<today::RequestState>(state)->requestId)
				<< "should pass the RequestState to the subscription resolvers";
			return std::make_shared<today::Appointment>(response::IdType(_fakeAppointmentId),
				"today",
				"Dinner Time!",
				true);
		});
	response::Value result;
	auto key = _service
				   ->subscribe({ [&result](response::Value&& response) {
									result = std::move(response);
								},
					   std::move(query),
					   "TestSubscription"s,
					   std::move(variables),
					   {},
					   state })
				   .get();
	_service
		->deliver({ "nextAppointmentChange"sv,
			{}, // filter
			{}, // launch
			std::make_shared<today::object::Subscription>(std::move(subscriptionObject)) })
		.get();
	_service->unsubscribe({ key }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointmentNode = service::ScalarArgument::require("nextAppointment", data);
		EXPECT_EQ(_fakeAppointmentId,
			service::IdArgument::require("nextAppointmentId", appointmentNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Dinner Time!", service::StringArgument::require("subject", appointmentNode))
			<< "subject should match";
		EXPECT_EQ("today", service::StringArgument::require("when", appointmentNode))
			<< "when should match";
		EXPECT_TRUE(service::BooleanArgument::require("isNow", appointmentNode))
			<< "isNow should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, DeliverNextAppointmentChangeNoSubscriptionObject)
{
	auto service = std::make_shared<today::Operations>(nullptr, nullptr, nullptr);
	bool exception = false;

	try
	{
		service->deliver({ "nextAppointmentChange"sv }).get();
	}
	catch (const std::invalid_argument& ex)
	{
		EXPECT_TRUE(ex.what() == "Missing subscriptionObject"sv) << "exception should match";
		exception = true;
	}

	ASSERT_TRUE(exception) << "expected an exception";
}

TEST_F(TodayServiceCase, DeliverNextAppointmentChangeNoSubscriptionSupport)
{
	auto service = std::make_shared<today::EmptyOperations>();
	bool exception = false;

	try
	{
		service->deliver({ "nextAppointmentChange"sv }).get();
	}
	catch (const std::logic_error& ex)
	{
		EXPECT_TRUE(ex.what() == "Subscriptions not supported"sv) << "exception should match";
		exception = true;
	}

	ASSERT_TRUE(exception) << "expected an exception";
}
TEST_F(TodayServiceCase, Introspection)
{
	auto query = R"({
			__schema {
				types {
					kind
					name
					description
					ofType {
						kind
					}
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
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(8);
	auto result =
		_service->resolve({ query, {}, std::move(variables), std::launch::async, state }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);
		const auto schema = service::ScalarArgument::require("__schema", data);
		const auto types =
			service::ScalarArgument::require<service::TypeModifier::List>("types", schema);
		const auto queryType = service::ScalarArgument::require("queryType", schema);
		const auto mutationType = service::ScalarArgument::require("mutationType", schema);

		ASSERT_FALSE(types.empty());
		ASSERT_TRUE(queryType.type() == response::Type::Map);
		ASSERT_TRUE(mutationType.type() == response::Type::Map);
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, SkipDirective)
{
	auto query = R"({
			__schema {
				types {
					kind
					name
					description
					ofType {
						kind
					}
				}
				queryType @skip(if: false) {
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
				mutationType @skip(if: true) {
					kind
					name
				}
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(9);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);
		const auto schema = service::ScalarArgument::require("__schema", data);
		const auto types =
			service::ScalarArgument::require<service::TypeModifier::List>("types", schema);
		const auto queryType = service::ScalarArgument::require("queryType", schema);
		const auto mutationType = service::ScalarArgument::find("mutationType", schema);

		ASSERT_FALSE(types.empty());
		ASSERT_TRUE(queryType.type() == response::Type::Map);
		ASSERT_FALSE(mutationType.second);
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, IncludeDirective)
{
	auto query = R"({
			__schema {
				types {
					kind
					name
					description
					ofType {
						kind
					}
				}
				queryType @include(if: false) {
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
				mutationType @include(if: true) {
					kind
					name
				}
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(10);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);
		const auto schema = service::ScalarArgument::require("__schema", data);
		const auto types =
			service::ScalarArgument::require<service::TypeModifier::List>("types", schema);
		const auto queryType = service::ScalarArgument::find("queryType", schema);
		const auto mutationType = service::ScalarArgument::require("mutationType", schema);

		ASSERT_FALSE(types.empty());
		ASSERT_FALSE(queryType.second);
		ASSERT_TRUE(mutationType.type() == response::Type::Map);
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, NestedFragmentDirectives)
{
	auto query = R"(
		query NestedFragmentsQuery @queryTag(query: "nested") {
			nested @fieldTag(field: "nested1") {
				...Fragment1 @fragmentSpreadTag(fragmentSpread: "fragmentSpread1")
			}
		}
		fragment Fragment1 on NestedType @fragmentDefinitionTag(fragmentDefinition: "fragmentDefinition1") {
			fragmentDefinitionNested: nested @fieldTag(field: "nested2") {
				...Fragment2 @fragmentSpreadTag(fragmentSpread: "fragmentSpread2")
			}
			depth @fieldTag(field: "depth1")
		}
		fragment Fragment2 on NestedType @fragmentDefinitionTag(fragmentDefinition: "fragmentDefinition2") {
			...on NestedType @inlineFragmentTag(inlineFragment: "inlineFragment3") {
				inlineFragmentNested: nested @fieldTag(field: "nested3") {
					...on NestedType @inlineFragmentTag(inlineFragment: "inlineFragment4") {
						...on NestedType @inlineFragmentTag(inlineFragment: "inlineFragment5") {
							inlineFragmentNested: nested @repeatableOnField @fieldTag(field: "nested4") @repeatableOnField {
								depth @fieldTag(field: "depth4")
							}
						}
					}
					depth @fieldTag(field: "depth3")
				}
			}
			depth @fieldTag(field: "depth2")
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(11);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);
		const auto nested1 = service::ScalarArgument::require("nested", data);
		const auto depth1 = service::IntArgument::require("depth", nested1);
		const auto nested2 = service::ScalarArgument::require("fragmentDefinitionNested", nested1);
		const auto depth2 = service::IntArgument::require("depth", nested2);
		const auto nested3 = service::ScalarArgument::require("inlineFragmentNested", nested2);
		const auto depth3 = service::IntArgument::require("depth", nested3);
		const auto nested4 = service::ScalarArgument::require("inlineFragmentNested", nested3);
		const auto depth4 = service::IntArgument::require("depth", nested4);
		auto capturedParams = today::NestedType::getCapturedParams();
		const auto params4 = std::move(capturedParams.top());
		capturedParams.pop();
		const auto params3 = std::move(capturedParams.top());
		capturedParams.pop();
		const auto params2 = std::move(capturedParams.top());
		capturedParams.pop();
		const auto params1 = std::move(capturedParams.top());
		capturedParams.pop();
		ASSERT_EQ(size_t(1), params1.operationDirectives.size()) << "missing operation directive";
		const auto itrQueryTag1 = params1.operationDirectives.cbegin();
		ASSERT_TRUE(itrQueryTag1->first == "queryTag"sv) << "missing required directive";
		const auto& queryTag1 = itrQueryTag1->second;
		const auto query1 = service::StringArgument::require("query", queryTag1);
		const auto fragmentDefinitionCount1 = params1.fragmentDefinitionDirectives.size();
		const auto fragmentSpreadCount1 = params1.fragmentSpreadDirectives.size();
		const auto inlineFragmentCount1 = params1.inlineFragmentDirectives.size();
		ASSERT_EQ(size_t(1), params1.fieldDirectives.size()) << "missing operation directive";
		const auto itrFieldTag1 = params1.fieldDirectives.cbegin();
		ASSERT_TRUE(itrFieldTag1->first == "fieldTag"sv) << "missing required directive";
		const auto& fieldTag1 = itrFieldTag1->second;
		const auto field1 = service::StringArgument::require("field", fieldTag1);
		ASSERT_EQ(size_t(1), params2.operationDirectives.size()) << "missing operation directive";
		const auto itrQueryTag2 = params2.operationDirectives.cbegin();
		ASSERT_TRUE(itrQueryTag2->first == "queryTag"sv) << "missing required directive";
		const auto& queryTag2 = itrQueryTag2->second;
		const auto query2 = service::StringArgument::require("query", queryTag2);
		ASSERT_EQ(size_t(1), params2.fragmentDefinitionDirectives.size())
			<< "missing fragment definition directive";
		const auto itrFragmentDefinitionTag2 = params2.fragmentDefinitionDirectives.cbegin();
		ASSERT_TRUE(itrFragmentDefinitionTag2->first == "fragmentDefinitionTag"sv)
			<< "missing fragment definition directive";
		const auto& fragmentDefinitionTag2 = itrFragmentDefinitionTag2->second;
		const auto fragmentDefinition2 =
			service::StringArgument::require("fragmentDefinition", fragmentDefinitionTag2);
		ASSERT_EQ(size_t(1), params2.fragmentSpreadDirectives.size())
			<< "missing fragment spread directive";
		const auto itrFragmentSpreadTag2 = params2.fragmentSpreadDirectives.cbegin();
		ASSERT_TRUE(itrFragmentSpreadTag2->first == "fragmentSpreadTag"sv)
			<< "missing fragment spread directive";
		const auto& fragmentSpreadTag2 = itrFragmentSpreadTag2->second;
		const auto fragmentSpread2 =
			service::StringArgument::require("fragmentSpread", fragmentSpreadTag2);
		const auto inlineFragmentCount2 = params2.inlineFragmentDirectives.size();
		ASSERT_EQ(size_t(1), params2.fieldDirectives.size()) << "missing field directive";
		const auto itrFieldTag2 = params2.fieldDirectives.cbegin();
		ASSERT_TRUE(itrFieldTag2->first == "fieldTag"sv) << "missing field directive";
		const auto& fieldTag2 = itrFieldTag2->second;
		const auto field2 = service::StringArgument::require("field", fieldTag2);
		ASSERT_EQ(size_t(1), params3.operationDirectives.size()) << "missing operation directive";
		const auto itrQueryTag3 = params3.operationDirectives.cbegin();
		ASSERT_TRUE(itrQueryTag3->first == "queryTag"sv) << "missing required directive";
		const auto& queryTag3 = itrQueryTag3->second;
		const auto query3 = service::StringArgument::require("query", queryTag3);
		ASSERT_EQ(size_t(1), params3.fragmentDefinitionDirectives.size())
			<< "missing fragment definition directive";
		const auto itrFragmentDefinitionTag3 = params3.fragmentDefinitionDirectives.cbegin();
		ASSERT_TRUE(itrFragmentDefinitionTag3->first == "fragmentDefinitionTag"sv)
			<< "missing fragment definition directive";
		const auto& fragmentDefinitionTag3 = itrFragmentDefinitionTag3->second;
		const auto fragmentDefinition3 =
			service::StringArgument::require("fragmentDefinition", fragmentDefinitionTag3);
		ASSERT_EQ(size_t(1), params3.fragmentSpreadDirectives.size())
			<< "missing fragment spread directive";
		const auto itrFragmentSpreadTag3 = params3.fragmentSpreadDirectives.cbegin();
		ASSERT_TRUE(itrFragmentSpreadTag3->first == "fragmentSpreadTag"sv)
			<< "missing fragment spread directive";
		const auto& fragmentSpreadTag3 = itrFragmentSpreadTag3->second;
		const auto fragmentSpread3 =
			service::StringArgument::require("fragmentSpread", fragmentSpreadTag3);
		ASSERT_EQ(size_t(1), params3.inlineFragmentDirectives.size())
			<< "missing inline fragment directive";
		const auto itrInlineFragmentTag3 = params3.inlineFragmentDirectives.cbegin();
		ASSERT_TRUE(itrInlineFragmentTag3->first == "inlineFragmentTag"sv);
		const auto& inlineFragmentTag3 = itrInlineFragmentTag3->second;
		const auto inlineFragment3 =
			service::StringArgument::require("inlineFragment", inlineFragmentTag3);
		ASSERT_EQ(size_t(1), params3.fieldDirectives.size()) << "missing field directive";
		const auto itrFieldTag3 = params3.fieldDirectives.cbegin();
		ASSERT_TRUE(itrFieldTag3->first == "fieldTag"sv) << "missing field directive";
		const auto& fieldTag3 = itrFieldTag3->second;
		const auto field3 = service::StringArgument::require("field", fieldTag3);
		ASSERT_EQ(size_t(1), params4.operationDirectives.size()) << "missing operation directive";
		const auto itrQueryTag4 = params4.operationDirectives.cbegin();
		ASSERT_TRUE(itrQueryTag4->first == "queryTag"sv) << "missing required directive";
		const auto& queryTag4 = itrQueryTag4->second;
		const auto query4 = service::StringArgument::require("query", queryTag4);
		const auto fragmentDefinitionCount4 = params4.fragmentDefinitionDirectives.size();
		const auto fragmentSpreadCount4 = params4.fragmentSpreadDirectives.size();
		ASSERT_EQ(size_t(1), params4.inlineFragmentDirectives.size())
			<< "missing inline fragment directive";
		const auto itrInlineFragmentTag4 = params4.inlineFragmentDirectives.cbegin();
		ASSERT_TRUE(itrInlineFragmentTag4->first == "inlineFragmentTag"sv);
		const auto& inlineFragmentTag4 = itrInlineFragmentTag4->second;
		const auto inlineFragment4 =
			service::StringArgument::require("inlineFragment", inlineFragmentTag4);
		ASSERT_EQ(size_t(3), params4.fieldDirectives.size()) << "missing field directive";
		const auto itrRepeatable1 = params4.fieldDirectives.cbegin();
		ASSERT_TRUE(itrRepeatable1->first == "repeatableOnField"sv) << "missing field directive";
		EXPECT_TRUE(response::Type::Map == itrRepeatable1->second.type())
			<< "unexpected arguments type directive";
		EXPECT_EQ(size_t(0), itrRepeatable1->second.size()) << "extra arguments on directive";
		const auto itrFieldTag4 = itrRepeatable1 + 1;
		ASSERT_TRUE(itrFieldTag4->first == "fieldTag"sv) << "missing field directive";
		const auto& fieldTag4 = itrFieldTag4->second;
		const auto itrRepeatable2 = itrFieldTag4 + 1;
		ASSERT_TRUE(itrRepeatable2->first == "repeatableOnField"sv) << "missing field directive";
		EXPECT_TRUE(response::Type::Map == itrRepeatable2->second.type())
			<< "unexpected arguments type directive";
		EXPECT_EQ(size_t(0), itrRepeatable2->second.size()) << "extra arguments on directive";
		const auto field4 = service::StringArgument::require("field", fieldTag4);

		ASSERT_EQ(1, depth1);
		ASSERT_EQ(2, depth2);
		ASSERT_EQ(3, depth3);
		ASSERT_EQ(4, depth4);
		ASSERT_TRUE(capturedParams.empty());
		ASSERT_EQ("nested", query1) << "remember the operation directives";
		ASSERT_EQ(size_t(0), fragmentDefinitionCount1);
		ASSERT_EQ(size_t(0), fragmentSpreadCount1);
		ASSERT_EQ(size_t(0), inlineFragmentCount1);
		ASSERT_EQ("nested1", field1) << "remember the field directives";
		ASSERT_EQ("nested", query2) << "remember the operation directives";
		ASSERT_EQ("fragmentDefinition1", fragmentDefinition2)
			<< "remember the directives from the fragment definition";
		ASSERT_EQ("fragmentSpread1", fragmentSpread2)
			<< "remember the directives from the fragment spread";
		ASSERT_EQ(size_t(0), inlineFragmentCount2);
		ASSERT_EQ("nested2", field2) << "remember the field directives";
		ASSERT_EQ("nested", query3) << "remember the operation directives";
		ASSERT_EQ("fragmentDefinition2", fragmentDefinition3)
			<< "outer fragement definition directives are preserved with inline fragments";
		ASSERT_EQ("fragmentSpread2", fragmentSpread3)
			<< "outer fragement spread directives are preserved with inline fragments";
		ASSERT_EQ("inlineFragment3", inlineFragment3)
			<< "remember the directives from the inline fragment";
		ASSERT_EQ("nested3", field3) << "remember the field directives";
		ASSERT_EQ("nested", query4) << "remember the operation directives";
		ASSERT_EQ(size_t(0), fragmentDefinitionCount4)
			<< "traversing a field to a nested object SelectionSet resets the fragment "
			   "directives";
		ASSERT_EQ(size_t(0), fragmentSpreadCount4)
			<< "traversing a field to a nested object SelectionSet resets the fragment "
			   "directives";
		ASSERT_EQ("inlineFragment5", inlineFragment4)
			<< "nested inline fragments don't reset, but do overwrite on collision";
		ASSERT_EQ("nested4", field4) << "remember the field directives";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, QueryAppointmentsById)
{
	auto query = R"(query SpecificAppointment($appointmentId: ID!) {
			appointmentsById(ids: [$appointmentId]) {
				appointmentId: id
				subject
				when
				isNow
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	variables.emplace_back("appointmentId", response::Value("ZmFrZUFwcG9pbnRtZW50SWQ="s));
	auto state = std::make_shared<today::RequestState>(12);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(12), state->appointmentsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(size_t(0), state->tasksRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->unreadCountsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(1), state->loadAppointmentsCount) << "today service called the loader once";
	EXPECT_EQ(size_t(0), state->loadTasksCount) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->loadUnreadCountsCount) << "today service did not call the loader";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointmentsById =
			service::ScalarArgument::require<service::TypeModifier::List>("appointmentsById", data);
		ASSERT_EQ(size_t(1), appointmentsById.size());
		const auto& appointmentEntry = appointmentsById.front();
		EXPECT_EQ(_fakeAppointmentId,
			service::IdArgument::require("appointmentId", appointmentEntry))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentEntry))
			<< "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentEntry))
			<< "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentEntry))
			<< "isNow should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, UnimplementedFieldError)
{
	auto query = R"(query {
			unimplemented
		})"_graphql;
	auto result = _service->resolve({ query }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		const auto& errors = result["errors"];
		ASSERT_TRUE(errors.type() == response::Type::List);
		ASSERT_EQ(size_t(1), errors.size());
		response::Value error { errors[0] };
		ASSERT_TRUE(error.type() == response::Type::Map);
		ASSERT_EQ(
			R"e({"message":"Field error name: unimplemented unknown error: Query::getUnimplemented is not implemented","locations":[{"line":2,"column":4}],"path":["unimplemented"]})e",
			response::toJSON(std::move(error)));
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, SubscribeNodeChangeMatchingId)
{
	auto query = peg::parseString(R"(subscription TestSubscription {
			changedNode: nodeChange(id: "ZmFrZVRhc2tJZA==") {
				changedId: id
				...on Task {
					title
					isComplete
				}
			}
		})");
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(13);
	auto subscriptionObject = std::make_shared<
		today::NodeChange>([](const std::shared_ptr<service::RequestState>& state,
							   response::IdType&& idArg) -> std::shared_ptr<today::object::Node> {
		EXPECT_EQ(size_t { 13 }, std::static_pointer_cast<today::RequestState>(state)->requestId)
			<< "should pass the RequestState to the subscription resolvers";
		EXPECT_EQ(_fakeTaskId, idArg);
		return std::make_shared<today::object::Node>(std::make_shared<today::object::Task>(
			std::make_shared<today::Task>(response::IdType(_fakeTaskId), "Don't forget", true)));
	});
	response::Value result;
	auto key = _service
				   ->subscribe({ [&result](response::Value&& response) {
									result = std::move(response);
								},
					   std::move(query),
					   "TestSubscription",
					   std::move(variables),
					   {},
					   state })
				   .get();
	_service
		->deliver({ "nodeChange"sv,
			{ service::SubscriptionFilter { { service::SubscriptionArguments {
				{ "id", response::Value("ZmFrZVRhc2tJZA=="s) } } } } },
			{}, // launch
			std::make_shared<today::object::Subscription>(std::move(subscriptionObject)) })
		.get();
	_service->unsubscribe({ key }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto taskNode = service::ScalarArgument::require("changedNode", data);
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("changedId", taskNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", taskNode))
			<< "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", taskNode))
			<< "isComplete should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, SubscribeNodeChangeMismatchedId)
{
	auto query = peg::parseString(R"(subscription TestSubscription {
			changedNode: nodeChange(id: "ZmFrZVRhc2tJZA==") {
				changedId: id
				...on Task {
					title
					isComplete
				}
			}
		})");
	response::Value variables(response::Type::Map);
	bool calledResolver = false;
	auto subscriptionObject = std::make_shared<today::NodeChange>(
		[&calledResolver](const std::shared_ptr<service::RequestState>& /* state */,
			response::IdType&& /* idArg */) -> std::shared_ptr<today::object::Node> {
			calledResolver = true;
			return nullptr;
		});
	bool calledGet = false;
	auto key = _service
				   ->subscribe({ [&calledGet](response::Value&&) {
									calledGet = true;
								},
					   std::move(query),
					   "TestSubscription"s,
					   std::move(variables) })
				   .get();
	_service
		->deliver({ "nodeChange"sv,
			{ service::SubscriptionFilter { { service::SubscriptionArguments {
				{ "id", response::Value("ZmFrZUFwcG9pbnRtZW50SWQ="s) } } } } },
			{}, // launch
			std::make_shared<today::object::Subscription>(std::move(subscriptionObject)) })
		.get();
	_service->unsubscribe({ key }).get();

	try
	{
		ASSERT_FALSE(calledResolver);
		ASSERT_FALSE(calledGet);
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, SubscribeNodeChangeFuzzyComparator)
{
	auto query = peg::parseString(R"(subscription TestSubscription {
			changedNode: nodeChange(id: "ZmFr") {
				changedId: id
				...on Task {
					title
					isComplete
				}
			}
		})");
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(14);
	bool filterCalled = false;
	auto filterCallback = [&filterCalled](
							  response::MapType::const_reference fuzzy) noexcept -> bool {
		EXPECT_FALSE(filterCalled);
		EXPECT_EQ("id", fuzzy.first) << "should only get called once for the id argument";
		EXPECT_EQ("ZmFr", fuzzy.second.get<std::string>());
		filterCalled = true;
		return true;
	};
	auto subscriptionObject = std::make_shared<
		today::NodeChange>([](const std::shared_ptr<service::RequestState>& state,
							   response::IdType&& idArg) -> std::shared_ptr<today::object::Node> {
		const response::IdType fuzzyId { 'f', 'a', 'k' };

		EXPECT_EQ(size_t { 14 }, std::static_pointer_cast<today::RequestState>(state)->requestId)
			<< "should pass the RequestState to the subscription resolvers";
		EXPECT_EQ(fuzzyId, idArg);
		return std::make_shared<today::object::Node>(std::make_shared<today::object::Task>(
			std::make_shared<today::Task>(response::IdType(_fakeTaskId), "Don't forget", true)));
	});
	response::Value result;
	auto key = _service
				   ->subscribe({ [&result](response::Value&& response) {
									result = std::move(response);
								},
					   std::move(query),
					   "TestSubscription"s,
					   std::move(variables),
					   {},
					   state })
				   .get();
	_service
		->deliver({ "nodeChange"sv,
			{ service::SubscriptionFilter {
				{ service::SubscriptionArgumentFilterCallback { std::move(filterCallback) } } } },
			{}, // launch
			std::make_shared<today::object::Subscription>(std::move(subscriptionObject)) })
		.get();
	_service->unsubscribe({ key }).get();

	try
	{
		ASSERT_TRUE(filterCalled) << "should match the id parameter in the subscription";
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto taskNode = service::ScalarArgument::require("changedNode", data);
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("changedId", taskNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", taskNode))
			<< "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", taskNode))
			<< "isComplete should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, SubscribeNodeChangeFuzzyMismatch)
{
	auto query = peg::parseString(R"(subscription TestSubscription {
			changedNode: nodeChange(id: "ZmFrZVRhc2tJZA==") {
				changedId: id
				...on Task {
					title
					isComplete
				}
			}
		})");
	response::Value variables(response::Type::Map);
	bool filterCalled = false;
	auto filterCallback = [&filterCalled](
							  response::MapType::const_reference fuzzy) noexcept -> bool {
		EXPECT_FALSE(filterCalled);
		EXPECT_EQ("id", fuzzy.first) << "should only get called once for the id argument";
		EXPECT_EQ("ZmFrZVRhc2tJZA==", fuzzy.second.get<std::string>());
		filterCalled = true;
		return false;
	};
	bool calledResolver = false;
	auto subscriptionObject = std::make_shared<today::NodeChange>(
		[&calledResolver](const std::shared_ptr<service::RequestState>& /* state */,
			response::IdType&& /* idArg */) -> std::shared_ptr<today::object::Node> {
			calledResolver = true;
			return nullptr;
		});
	bool calledGet = false;
	auto key = _service
				   ->subscribe({ [&calledGet](response::Value&&) {
									calledGet = true;
								},
					   std::move(query),
					   "TestSubscription"s,
					   std::move(variables) })
				   .get();
	_service
		->deliver({ "nodeChange"sv,
			{ service::SubscriptionFilter {
				{ service::SubscriptionArgumentFilterCallback { std::move(filterCallback) } } } },
			{}, // launch
			std::make_shared<today::object::Subscription>(std::move(subscriptionObject)) })
		.get();
	_service->unsubscribe({ key }).get();

	try
	{
		ASSERT_TRUE(filterCalled) << "should not match the id parameter in the subscription";
		ASSERT_FALSE(calledResolver);
		ASSERT_FALSE(calledGet);
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, SubscribeNodeChangeMatchingVariable)
{
	auto query = peg::parseString(R"(subscription TestSubscription($taskId: ID!) {
			changedNode: nodeChange(id: $taskId) {
				changedId: id
				...on Task {
					title
					isComplete
				}
			}
		})");
	response::Value variables(response::Type::Map);
	variables.emplace_back("taskId", response::Value("ZmFrZVRhc2tJZA=="s));
	auto state = std::make_shared<today::RequestState>(14);
	auto subscriptionObject = std::make_shared<
		today::NodeChange>([](const std::shared_ptr<service::RequestState>& state,
							   response::IdType&& idArg) -> std::shared_ptr<today::object::Node> {
		EXPECT_EQ(size_t { 14 }, std::static_pointer_cast<today::RequestState>(state)->requestId)
			<< "should pass the RequestState to the subscription resolvers";
		EXPECT_EQ(_fakeTaskId, idArg);
		return std::make_shared<today::object::Node>(std::make_shared<today::object::Task>(
			std::make_shared<today::Task>(response::IdType(_fakeTaskId), "Don't forget", true)));
	});
	response::Value result;
	auto key = _service
				   ->subscribe({ [&result](response::Value&& response) {
									result = std::move(response);
								},
					   std::move(query),
					   "TestSubscription",
					   std::move(variables),
					   {},
					   state })
				   .get();
	_service
		->deliver({ "nodeChange"sv,
			{ service::SubscriptionFilter { { service::SubscriptionArguments {
				{ "id", response::Value("ZmFrZVRhc2tJZA=="s) } } } } },
			{}, // launch
			std::make_shared<today::object::Subscription>(std::move(subscriptionObject)) })
		.get();
	_service->unsubscribe({ key }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto taskNode = service::ScalarArgument::require("changedNode", data);
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("changedId", taskNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", taskNode))
			<< "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", taskNode))
			<< "isComplete should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, DeferredQueryAppointmentsById)
{
	auto query = R"(query SpecificAppointment($appointmentId: ID!) {
			appointmentsById(ids: [$appointmentId]) {
				appointmentId: id
				subject
				when
				isNow
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	variables.emplace_back("appointmentId", response::Value("ZmFrZUFwcG9pbnRtZW50SWQ="s));
	auto state = std::make_shared<today::RequestState>(15);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(15), state->appointmentsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(size_t(0), state->tasksRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->unreadCountsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(1), state->loadAppointmentsCount) << "today service called the loader once";
	EXPECT_EQ(size_t(0), state->loadTasksCount) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->loadUnreadCountsCount) << "today service did not call the loader";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointmentsById =
			service::ScalarArgument::require<service::TypeModifier::List>("appointmentsById", data);
		ASSERT_EQ(size_t(1), appointmentsById.size());
		const auto& appointmentEntry = appointmentsById.front();
		EXPECT_EQ(_fakeAppointmentId,
			service::IdArgument::require("appointmentId", appointmentEntry))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentEntry))
			<< "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentEntry))
			<< "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentEntry))
			<< "isNow should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, NonBlockingQueryAppointmentsById)
{
	auto query = R"(query SpecificAppointment($appointmentId: ID!) {
			appointmentsById(ids: [$appointmentId]) {
				appointmentId: id
				subject
				when
				isNow
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	variables.emplace_back("appointmentId", response::Value("ZmFrZUFwcG9pbnRtZW50SWQ="s));
	auto state = std::make_shared<today::RequestState>(16);
	auto result =
		_service->resolve({ query, {}, std::move(variables), std::launch::async, state }).get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(16), state->appointmentsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(size_t(0), state->tasksRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->unreadCountsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(1), state->loadAppointmentsCount) << "today service called the loader once";
	EXPECT_EQ(size_t(0), state->loadTasksCount) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->loadUnreadCountsCount) << "today service did not call the loader";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointmentsById =
			service::ScalarArgument::require<service::TypeModifier::List>("appointmentsById", data);
		ASSERT_EQ(size_t(1), appointmentsById.size());
		const auto& appointmentEntry = appointmentsById.front();
		EXPECT_EQ(_fakeAppointmentId,
			service::IdArgument::require("appointmentId", appointmentEntry))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentEntry))
			<< "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentEntry))
			<< "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentEntry))
			<< "isNow should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, NonExistentTypeIntrospection)
{
	auto query = R"(query {
			__type(name: "NonExistentType") {
				description
			}
		})"_graphql;
	auto result = _service->resolve({ query }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		ASSERT_FALSE(errorsItr == result.get<response::MapType>().cend());
		auto errorsString = response::toJSON(response::Value(errorsItr->second));
		EXPECT_EQ(
			R"js([{"message":"Type not found name: NonExistentType","locations":[{"line":2,"column":4}],"path":["__type"]}])js",
			errorsString)
			<< "error should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, SubscribeNextAppointmentChangeAsync)
{
	auto query = peg::parseString(R"(subscription TestSubscription {
			nextAppointment: nextAppointmentChange {
				nextAppointmentId: id
				when
				subject
				isNow
			}
		})");
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(17);
	response::Value result;
	auto key = _service
				   ->subscribe({ [&result](response::Value&& response) {
									result = std::move(response);
								},
					   std::move(query),
					   "TestSubscription"s,
					   std::move(variables),
					   {},
					   state })
				   .get();
	_service->deliver({ "nextAppointmentChange"sv, {}, std::launch::async }).get();
	_service->unsubscribe({ key }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointmentNode = service::ScalarArgument::require("nextAppointment", data);
		EXPECT_EQ(_fakeAppointmentId,
			service::IdArgument::require("nextAppointmentId", appointmentNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode))
			<< "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode))
			<< "when should match";
		EXPECT_TRUE(service::BooleanArgument::require("isNow", appointmentNode))
			<< "isNow should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, NonblockingDeferredExpensive)
{
	auto query = R"(query NonblockingDeferredExpensive {
			expensive {
				order
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(18);
	std::unique_lock testLock(today::Expensive::testMutex);
	auto result =
		_service
			->resolve({ query, "NonblockingDeferredExpensive"sv, std::move(variables), {}, state })
			.get();

	try
	{
		ASSERT_TRUE(today::Expensive::Reset()) << "there should be no remaining instances";
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		ASSERT_TRUE(errorsItr == result.get<response::MapType>().cend());
		auto response = response::toJSON(response::Value(result));
		EXPECT_EQ(
			R"js({"data":{"expensive":[{"order":1},{"order":2},{"order":3},{"order":4},{"order":5}]}})js",
			response)
			<< "output should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, BlockingAsyncExpensive)
{
	auto query = R"(query BlockingAsyncExpensive {
			expensive {
				order
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(19);
	std::unique_lock testLock(today::Expensive::testMutex);
	auto result = _service
					  ->resolve({ query,
						  "BlockingAsyncExpensive"sv,
						  std::move(variables),
						  std::launch::async,
						  state })
					  .get();

	try
	{
		ASSERT_TRUE(today::Expensive::Reset()) << "there should be no remaining instances";
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		ASSERT_TRUE(errorsItr == result.get<response::MapType>().cend());
		auto response = response::toJSON(response::Value(result));
		EXPECT_EQ(
			R"js({"data":{"expensive":[{"order":1},{"order":2},{"order":3},{"order":4},{"order":5}]}})js",
			response)
			<< "output should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, QueryAppointmentsThroughUnionTypeFragment)
{
	auto query = R"({
			appointments {
				edges {
					node {
						...AppointmentUnionFragment
					}
				}
			}
		}

		fragment AppointmentUnionFragment on UnionType {
			...on Appointment {
				appointmentId: id
				subject
				when
				isNow
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(20);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(20), state->appointmentsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(size_t(0), state->tasksRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->unreadCountsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(1), state->loadAppointmentsCount) << "today service called the loader once";
	EXPECT_EQ(size_t(0), state->loadTasksCount) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->loadUnreadCountsCount) << "today service did not call the loader";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointments = service::ScalarArgument::require("appointments", data);
		const auto appointmentEdges =
			service::ScalarArgument::require<service::TypeModifier::List>("edges", appointments);
		ASSERT_EQ(size_t { 1 }, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].type() == response::Type::Map)
			<< "appointment should be an object";
		const auto appointmentNode = service::ScalarArgument::require("node", appointmentEdges[0]);
		EXPECT_EQ(_fakeAppointmentId,
			service::IdArgument::require("appointmentId", appointmentNode))
			<< "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode))
			<< "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode))
			<< "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentNode))
			<< "isNow should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

size_t today::NextAppointmentChange::_notifySubscribeCount = 0;
size_t today::NextAppointmentChange::_subscriptionCount = 0;
size_t today::NextAppointmentChange::_notifyUnsubscribeCount = 0;

TEST_F(TodayServiceCase, SubscribeUnsubscribeNotificationsAsync)
{
	auto query = peg::parseString(R"(subscription TestSubscription {
			nextAppointment: nextAppointmentChange {
				nextAppointmentId: id
				when
				subject
				isNow
			}
		})");
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(21);
	bool calledCallback = false;
	const auto notifySubscribeBegin =
		today::NextAppointmentChange::getCount(service::ResolverContext::NotifySubscribe);
	const auto subscriptionBegin =
		today::NextAppointmentChange::getCount(service::ResolverContext::Subscription);
	const auto notifyUnsubscribeBegin =
		today::NextAppointmentChange::getCount(service::ResolverContext::NotifyUnsubscribe);
	auto key = _service
				   ->subscribe({
					   [&calledCallback](response::Value&& /* response */) {
						   calledCallback = true;
					   },
					   std::move(query),
					   "TestSubscription"s,
					   std::move(variables),
					   std::launch::async,
					   state,
				   })
				   .get();
	_service->unsubscribe({ key, std::launch::async }).get();
	const auto notifySubscribeEnd =
		today::NextAppointmentChange::getCount(service::ResolverContext::NotifySubscribe);
	const auto subscriptionEnd =
		today::NextAppointmentChange::getCount(service::ResolverContext::Subscription);
	const auto notifyUnsubscribeEnd =
		today::NextAppointmentChange::getCount(service::ResolverContext::NotifyUnsubscribe);

	try
	{
		EXPECT_FALSE(calledCallback);
		EXPECT_EQ(notifySubscribeBegin + 1, notifySubscribeEnd)
			<< "should pass NotifySubscribe once";
		EXPECT_EQ(subscriptionBegin, subscriptionEnd) << "should not pass Subscription";
		EXPECT_EQ(notifyUnsubscribeBegin + 1, notifyUnsubscribeEnd)
			<< "should pass NotifyUnsubscribe once";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, SubscribeUnsubscribeNotificationsDeferred)
{
	auto query = peg::parseString(R"(subscription TestSubscription {
			nextAppointment: nextAppointmentChange {
				nextAppointmentId: id
				when
				subject
				isNow
			}
		})");
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(21);
	bool calledCallback = false;
	const auto notifySubscribeBegin =
		today::NextAppointmentChange::getCount(service::ResolverContext::NotifySubscribe);
	const auto subscriptionBegin =
		today::NextAppointmentChange::getCount(service::ResolverContext::Subscription);
	const auto notifyUnsubscribeBegin =
		today::NextAppointmentChange::getCount(service::ResolverContext::NotifyUnsubscribe);
	auto key = _service
				   ->subscribe({ [&calledCallback](response::Value&& /* response */) {
									calledCallback = true;
								},
					   std::move(query),
					   "TestSubscription"s,
					   std::move(variables),
					   {},
					   state })
				   .get();
	_service->unsubscribe({ key }).get();
	const auto notifySubscribeEnd =
		today::NextAppointmentChange::getCount(service::ResolverContext::NotifySubscribe);
	const auto subscriptionEnd =
		today::NextAppointmentChange::getCount(service::ResolverContext::Subscription);
	const auto notifyUnsubscribeEnd =
		today::NextAppointmentChange::getCount(service::ResolverContext::NotifyUnsubscribe);

	try
	{
		EXPECT_FALSE(calledCallback);
		EXPECT_EQ(notifySubscribeBegin + 1, notifySubscribeEnd)
			<< "should pass NotifySubscribe once";
		EXPECT_EQ(subscriptionBegin, subscriptionEnd) << "should not pass Subscription";
		EXPECT_EQ(notifyUnsubscribeBegin + 1, notifyUnsubscribeEnd)
			<< "should pass NotifyUnsubscribe once";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, MutateSetFloat)
{
	auto query = R"(mutation {
			setFloat(value: 0.1)
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(22);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);
		ASSERT_TRUE(data.type() == response::Type::Map);
		const auto setFloat = service::FloatArgument::require("setFloat", data);
		ASSERT_EQ(0.1, setFloat) << "should return the value that was set";
		ASSERT_EQ(0.1, today::Mutation::getFloat())
			<< "should save the value in the Mutation static member";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(TodayServiceCase, MutateCoerceSetFloat)
{
	auto query = R"(mutation {
			coerceFloat: setFloat(value: 1)
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(22);
	auto result = _service->resolve({ query, {}, std::move(variables), {}, state }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);
		ASSERT_TRUE(data.type() == response::Type::Map);
		const auto coerceFloat = service::FloatArgument::require("coerceFloat", data);
		ASSERT_EQ(1.0, coerceFloat) << "should return the value that was coerced from an int";
		ASSERT_EQ(1.0, today::Mutation::getFloat())
			<< "should save the value in the Mutation static member";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}
