// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "MutateClient.h"
#include "QueryClient.h"
#include "SubscribeClient.h"
#include "TodayMock.h"

#include <chrono>

using namespace graphql;

using namespace std::literals;

class ClientCase : public ::testing::Test
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
			[]() -> std::vector<std::shared_ptr<today::Appointment>> {
				++_getAppointmentsCount;
				return { std::make_shared<today::Appointment>(response::IdType(_fakeAppointmentId),
					"tomorrow",
					"Lunch?",
					false) };
			},
			[]() -> std::vector<std::shared_ptr<today::Task>> {
				++_getTasksCount;
				return { std::make_shared<today::Task>(response::IdType(_fakeTaskId),
					"Don't forget",
					true) };
			},
			[]() -> std::vector<std::shared_ptr<today::Folder>> {
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

	static void TearDownTestCase()
	{
		_fakeAppointmentId.clear();
		_fakeTaskId.clear();
		_fakeFolderId.clear();
		_service.reset();
	}

protected:
	static response::IdType _fakeAppointmentId;
	static response::IdType _fakeTaskId;
	static response::IdType _fakeFolderId;

	static std::shared_ptr<today::Operations> _service;
	static size_t _getAppointmentsCount;
	static size_t _getTasksCount;
	static size_t _getUnreadCountsCount;
};

response::IdType ClientCase::_fakeAppointmentId;
response::IdType ClientCase::_fakeTaskId;
response::IdType ClientCase::_fakeFolderId;

std::shared_ptr<today::Operations> ClientCase::_service;
size_t ClientCase::_getAppointmentsCount = 0;
size_t ClientCase::_getTasksCount = 0;
size_t ClientCase::_getUnreadCountsCount = 0;

TEST_F(ClientCase, QueryEverything)
{
	using namespace client::query::Query;

	auto query = GetRequestObject();

	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(1);
	auto result =
		_service->resolve(std::launch::async, state, query, "", std::move(variables)).get();
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
			FAIL() << "service returned errors";
		}

		const auto response = parseResponse(service::ScalarArgument::require("data", result));

		ASSERT_TRUE(response.appointments.edges.has_value()) << "appointments should be set";
		ASSERT_EQ(1, response.appointments.edges->size()) << "appointments should have 1 entry";
		ASSERT_TRUE((*response.appointments.edges)[0].has_value()) << "edge should be set";
		const auto& appointmentNode = (*response.appointments.edges)[0]->node;
		ASSERT_TRUE(appointmentNode.has_value()) << "node should be set";
		EXPECT_EQ(_fakeAppointmentId, appointmentNode->id) << "id should match in base64 encoding";
		ASSERT_TRUE(appointmentNode->subject.has_value()) << "subject should be set";
		EXPECT_EQ("Lunch?", *(appointmentNode->subject)) << "subject should match";
		ASSERT_TRUE(appointmentNode->when.has_value()) << "when should be set";
		EXPECT_EQ("tomorrow", appointmentNode->when->get<response::StringType>())
			<< "when should match";
		EXPECT_FALSE(appointmentNode->isNow) << "isNow should match";
		EXPECT_EQ("Appointment", appointmentNode->_typename) << "__typename should match";

		ASSERT_TRUE(response.tasks.edges.has_value()) << "tasks should be set";
		ASSERT_EQ(1, response.tasks.edges->size()) << "tasks should have 1 entry";
		ASSERT_TRUE((*response.tasks.edges)[0].has_value()) << "edge should be set";
		const auto& taskNode = (*response.tasks.edges)[0]->node;
		ASSERT_TRUE(taskNode.has_value()) << "node should be set";
		EXPECT_EQ(_fakeTaskId, taskNode->id) << "id should match in base64 encoding";
		ASSERT_TRUE(taskNode->title.has_value()) << "subject should be set";
		EXPECT_EQ("Don't forget", *(taskNode->title)) << "title should match";
		EXPECT_TRUE(taskNode->isComplete) << "isComplete should match";
		EXPECT_EQ("Task", taskNode->_typename) << "__typename should match";

		ASSERT_TRUE(response.unreadCounts.edges.has_value()) << "unreadCounts should be set";
		ASSERT_EQ(1, response.unreadCounts.edges->size()) << "unreadCounts should have 1 entry";
		ASSERT_TRUE((*response.unreadCounts.edges)[0].has_value()) << "edge should be set";
		const auto& unreadCountNode = (*response.unreadCounts.edges)[0]->node;
		ASSERT_TRUE(unreadCountNode.has_value()) << "node should be set";
		EXPECT_EQ(_fakeFolderId, unreadCountNode->id) << "id should match in base64 encoding";
		ASSERT_TRUE(unreadCountNode->name.has_value()) << "name should be set";
		EXPECT_EQ("\"Fake\" Inbox", *(unreadCountNode->name)) << "name should match";
		EXPECT_EQ(3, unreadCountNode->unreadCount) << "unreadCount should match";
		EXPECT_EQ("Folder", unreadCountNode->_typename) << "__typename should match";

		ASSERT_TRUE(response.testTaskState.has_value()) << "testTaskState should be set";
		EXPECT_EQ(client::query::Query::TaskState::Unassigned, *response.testTaskState)
			<< "testTaskState should match";

		ASSERT_EQ(1, response.anyType.size()) << "anyType should have 1 entry";
		ASSERT_TRUE(response.anyType[0].has_value()) << "appointment should be set";
		const auto& anyType = *response.anyType[0];
		EXPECT_EQ("Appointment", anyType._typename) << "__typename should match";
		EXPECT_EQ(_fakeAppointmentId, anyType.id) << "id should match in base64 encoding";
		EXPECT_FALSE(anyType.title.has_value()) << "appointment should not have a title";
		EXPECT_FALSE(anyType.isComplete) << "appointment should not set isComplete";
		ASSERT_TRUE(anyType.subject.has_value()) << "subject should be set";
		EXPECT_EQ("Lunch?", *(anyType.subject)) << "subject should match";
		ASSERT_TRUE(anyType.when.has_value()) << "when should be set";
		EXPECT_EQ("tomorrow", anyType.when->get<response::StringType>()) << "when should match";
		EXPECT_FALSE(anyType.isNow) << "isNow should match";
	}
	catch (const std::logic_error& ex)
	{
		FAIL() << ex.what();
	}
}

TEST_F(ClientCase, MutateCompleteTask)
{
	using namespace client::mutation::CompleteTaskMutation;

	auto query = GetRequestObject();
	auto variables = serializeVariables({ { _fakeTaskId,
		std::nullopt,
		std::make_optional(true),
		std::make_optional("Hi There!"s) } });

	auto state = std::make_shared<today::RequestState>(5);
	auto result = _service->resolve(state, query, "", std::move(variables)).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << "service returned errors";
		}

		const auto response = parseResponse(service::ScalarArgument::require("data", result));

		const auto& completedTask = response.completedTask;
		const auto& task = completedTask.completedTask;
		ASSERT_TRUE(task.has_value()) << "should get back a task";
		EXPECT_EQ(_fakeTaskId, task->completedTaskId) << "id should match in base64 encoding";
		ASSERT_TRUE(task->title.has_value()) << "title should be set";
		EXPECT_EQ("Mutated Task!", *(task->title)) << "title should match";
		EXPECT_TRUE(task->isComplete) << "isComplete should match";

		const auto& clientMutationId = completedTask.clientMutationId;
		ASSERT_TRUE(clientMutationId.has_value()) << "clientMutationId should be set";
		EXPECT_EQ("Hi There!", *clientMutationId) << "clientMutationId should match";
	}
	catch (const std::logic_error& ex)
	{
		FAIL() << ex.what();
	}
}

TEST_F(ClientCase, SubscribeNextAppointmentChangeDefault)
{
	using namespace client::subscription::TestSubscription;

	auto query = GetRequestObject();

	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(6);
	response::Value result;
	auto key = _service->subscribe(service::SubscriptionParams { state,
									   std::move(query),
									   "TestSubscription",
									   std::move(std::move(variables)) },
		[&result](std::future<response::Value> response) {
			result = response.get();
		});
	_service->deliver("nextAppointmentChange", nullptr);
	_service->unsubscribe(key);

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << "service returned errors";
		}

		const auto response = parseResponse(service::ScalarArgument::require("data", result));

		const auto& appointmentNode = response.nextAppointment;
		ASSERT_TRUE(appointmentNode.has_value()) << "should get back a task";
		EXPECT_EQ(_fakeAppointmentId, appointmentNode->nextAppointmentId)
			<< "id should match in base64 encoding";
		ASSERT_TRUE(appointmentNode->subject.has_value()) << "subject should be set";
		EXPECT_EQ("Lunch?", *(appointmentNode->subject)) << "subject should match";
		ASSERT_TRUE(appointmentNode->when.has_value()) << "when should be set";
		EXPECT_EQ("tomorrow", appointmentNode->when->get<response::StringType>())
			<< "when should match";
		EXPECT_TRUE(appointmentNode->isNow) << "isNow should match";
	}
	catch (const std::logic_error& ex)
	{
		FAIL() << ex.what();
	}
}

size_t today::NextAppointmentChange::_notifySubscribeCount = 0;
size_t today::NextAppointmentChange::_subscriptionCount = 0;
size_t today::NextAppointmentChange::_notifyUnsubscribeCount = 0;
