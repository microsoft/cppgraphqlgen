// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <future>

import GraphQL.Parse;
import GraphQL.Client;
import GraphQL.Service;

import GraphQL.Mutate.MutateClient;
import GraphQL.Query.QueryClient;
import GraphQL.Subscribe.SubscribeClient;

import GraphQL.Today.Mock;

using namespace graphql;

using namespace std::literals;

class ClientCase : public ::testing::Test
{
public:
	void SetUp() override
	{
		_mockService = today::mock_service();
	}

	void TearDown() override
	{
		_mockService.reset();
	}

protected:
	std::shared_ptr<today::TodayMockService> _mockService;
};

TEST_F(ClientCase, QueryEverything)
{
	using namespace query::client::query::Query;

	auto query = GetRequestObject();

	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(1);
	auto result = _mockService->service
					  ->resolve({ query, {}, std::move(variables), std::launch::async, state })
					  .get();
	EXPECT_EQ(std::size_t { 1 }, _mockService->getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_EQ(std::size_t { 1 }, _mockService->getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_EQ(std::size_t { 1 }, _mockService->getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(std::size_t { 1 }, state->appointmentsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(std::size_t { 1 }, state->tasksRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(std::size_t { 1 }, state->unreadCountsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(std::size_t { 1 }, state->loadAppointmentsCount)
		<< "today service called the loader once";
	EXPECT_EQ(std::size_t { 1 }, state->loadTasksCount) << "today service called the loader once";
	EXPECT_EQ(std::size_t { 1 }, state->loadUnreadCountsCount)
		<< "today service called the loader once";

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto serviceResponse = client::parseServiceResponse(std::move(result));
		const auto response = parseResponse(std::move(serviceResponse.data));

		EXPECT_EQ(std::size_t { 0 }, serviceResponse.errors.size()) << "no errors expected";

		ASSERT_TRUE(response.appointments.edges.has_value()) << "appointments should be set";
		ASSERT_EQ(std::size_t { 1 }, response.appointments.edges->size())
			<< "appointments should have 1 entry";
		ASSERT_TRUE((*response.appointments.edges)[0].has_value()) << "edge should be set";
		const auto& appointmentNode = (*response.appointments.edges)[0]->node;
		ASSERT_TRUE(appointmentNode.has_value()) << "node should be set";
		EXPECT_EQ(today::getFakeAppointmentId(), appointmentNode->id)
			<< "id should match in base64 encoding";
		ASSERT_TRUE(appointmentNode->subject.has_value()) << "subject should be set";
		EXPECT_EQ("Lunch?", *(appointmentNode->subject)) << "subject should match";
		ASSERT_TRUE(appointmentNode->when.has_value()) << "when should be set";
		EXPECT_EQ("tomorrow", appointmentNode->when->get<std::string>()) << "when should match";
		EXPECT_FALSE(appointmentNode->isNow) << "isNow should match";
		EXPECT_EQ("Appointment", appointmentNode->_typename) << "__typename should match";

		ASSERT_TRUE(response.tasks.edges.has_value()) << "tasks should be set";
		ASSERT_EQ(std::size_t { 1 }, response.tasks.edges->size()) << "tasks should have 1 entry";
		ASSERT_TRUE((*response.tasks.edges)[0].has_value()) << "edge should be set";
		const auto& taskNode = (*response.tasks.edges)[0]->node;
		ASSERT_TRUE(taskNode.has_value()) << "node should be set";
		EXPECT_EQ(today::getFakeTaskId(), taskNode->id) << "id should match in base64 encoding";
		ASSERT_TRUE(taskNode->title.has_value()) << "subject should be set";
		EXPECT_EQ("Don't forget", *(taskNode->title)) << "title should match";
		EXPECT_TRUE(taskNode->isComplete) << "isComplete should match";
		EXPECT_EQ("Task", taskNode->_typename) << "__typename should match";

		ASSERT_TRUE(response.unreadCounts.edges.has_value()) << "unreadCounts should be set";
		ASSERT_EQ(std::size_t { 1 }, response.unreadCounts.edges->size())
			<< "unreadCounts should have 1 entry";
		ASSERT_TRUE((*response.unreadCounts.edges)[0].has_value()) << "edge should be set";
		const auto& unreadCountNode = (*response.unreadCounts.edges)[0]->node;
		ASSERT_TRUE(unreadCountNode.has_value()) << "node should be set";
		EXPECT_EQ(today::getFakeFolderId(), unreadCountNode->id)
			<< "id should match in base64 encoding";
		ASSERT_TRUE(unreadCountNode->name.has_value()) << "name should be set";
		EXPECT_EQ("\"Fake\" Inbox", *(unreadCountNode->name)) << "name should match";
		EXPECT_EQ(3, unreadCountNode->unreadCount) << "unreadCount should match";
		EXPECT_EQ("Folder", unreadCountNode->_typename) << "__typename should match";

		EXPECT_EQ(query::client::query::Query::TaskState::Unassigned, response.testTaskState)
			<< "testTaskState should match";

		ASSERT_EQ(std::size_t { 1 }, response.anyType.size()) << "anyType should have 1 entry";
		ASSERT_TRUE(response.anyType[0].has_value()) << "appointment should be set";
		const auto& anyType = *response.anyType[0];
		EXPECT_EQ("Appointment", anyType._typename) << "__typename should match";
		EXPECT_EQ(today::getFakeAppointmentId(), anyType.id)
			<< "id should match in base64 encoding";
		EXPECT_FALSE(anyType.title.has_value()) << "appointment should not have a title";
		EXPECT_FALSE(anyType.isComplete) << "appointment should not set isComplete";
		ASSERT_TRUE(anyType.subject.has_value()) << "subject should be set";
		EXPECT_EQ("Lunch?", *(anyType.subject)) << "subject should match";
		ASSERT_TRUE(anyType.when.has_value()) << "when should be set";
		EXPECT_EQ("tomorrow", anyType.when->get<std::string>()) << "when should match";
		EXPECT_FALSE(anyType.isNow) << "isNow should match";
	}
	catch (const std::logic_error& ex)
	{
		FAIL() << ex.what();
	}
}

TEST_F(ClientCase, QueryEverythingWithVisitor)
{
	using namespace query::client::query::Query;

	auto query = GetRequestObject();

	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(1);
	auto result =
		_mockService->service->visit({ query, {}, std::move(variables), std::launch::async, state })
			.get();
	EXPECT_EQ(std::size_t { 1 }, _mockService->getAppointmentsCount)
		<< "today service lazy loads the appointments and caches the result";
	EXPECT_EQ(std::size_t { 1 }, _mockService->getTasksCount)
		<< "today service lazy loads the tasks and caches the result";
	EXPECT_EQ(std::size_t { 1 }, _mockService->getUnreadCountsCount)
		<< "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(std::size_t { 1 }, state->appointmentsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(std::size_t { 1 }, state->tasksRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(std::size_t { 1 }, state->unreadCountsRequestId)
		<< "today service passed the same RequestState";
	EXPECT_EQ(std::size_t { 1 }, state->loadAppointmentsCount)
		<< "today service called the loader once";
	EXPECT_EQ(std::size_t { 1 }, state->loadTasksCount) << "today service called the loader once";
	EXPECT_EQ(std::size_t { 1 }, state->loadUnreadCountsCount)
		<< "today service called the loader once";

	try
	{
		auto visitor = std::make_shared<ResponseVisitor>();
		auto responseVisitor = std::make_shared<response::ValueVisitor>(visitor);
		std::move(result.data).visit(responseVisitor);
		const auto response = visitor->response();

		EXPECT_EQ(std::size_t { 0 }, result.errors.size()) << "no errors expected";

		ASSERT_TRUE(response.appointments.edges.has_value()) << "appointments should be set";
		ASSERT_EQ(std::size_t { 1 }, response.appointments.edges->size())
			<< "appointments should have 1 entry";
		ASSERT_TRUE((*response.appointments.edges)[0].has_value()) << "edge should be set";
		const auto& appointmentNode = (*response.appointments.edges)[0]->node;
		ASSERT_TRUE(appointmentNode.has_value()) << "node should be set";
		EXPECT_EQ(today::getFakeAppointmentId(), appointmentNode->id)
			<< "id should match in base64 encoding";
		ASSERT_TRUE(appointmentNode->subject.has_value()) << "subject should be set";
		EXPECT_EQ("Lunch?", *(appointmentNode->subject)) << "subject should match";
		ASSERT_TRUE(appointmentNode->when.has_value()) << "when should be set";
		EXPECT_EQ("tomorrow", appointmentNode->when->get<std::string>()) << "when should match";
		EXPECT_FALSE(appointmentNode->isNow) << "isNow should match";
		EXPECT_EQ("Appointment", appointmentNode->_typename) << "__typename should match";

		ASSERT_TRUE(response.tasks.edges.has_value()) << "tasks should be set";
		ASSERT_EQ(std::size_t { 1 }, response.tasks.edges->size()) << "tasks should have 1 entry";
		ASSERT_TRUE((*response.tasks.edges)[0].has_value()) << "edge should be set";
		const auto& taskNode = (*response.tasks.edges)[0]->node;
		ASSERT_TRUE(taskNode.has_value()) << "node should be set";
		EXPECT_EQ(today::getFakeTaskId(), taskNode->id) << "id should match in base64 encoding";
		ASSERT_TRUE(taskNode->title.has_value()) << "subject should be set";
		EXPECT_EQ("Don't forget", *(taskNode->title)) << "title should match";
		EXPECT_TRUE(taskNode->isComplete) << "isComplete should match";
		EXPECT_EQ("Task", taskNode->_typename) << "__typename should match";

		ASSERT_TRUE(response.unreadCounts.edges.has_value()) << "unreadCounts should be set";
		ASSERT_EQ(std::size_t { 1 }, response.unreadCounts.edges->size())
			<< "unreadCounts should have 1 entry";
		ASSERT_TRUE((*response.unreadCounts.edges)[0].has_value()) << "edge should be set";
		const auto& unreadCountNode = (*response.unreadCounts.edges)[0]->node;
		ASSERT_TRUE(unreadCountNode.has_value()) << "node should be set";
		EXPECT_EQ(today::getFakeFolderId(), unreadCountNode->id)
			<< "id should match in base64 encoding";
		ASSERT_TRUE(unreadCountNode->name.has_value()) << "name should be set";
		EXPECT_EQ("\"Fake\" Inbox", *(unreadCountNode->name)) << "name should match";
		EXPECT_EQ(3, unreadCountNode->unreadCount) << "unreadCount should match";
		EXPECT_EQ("Folder", unreadCountNode->_typename) << "__typename should match";

		EXPECT_EQ(query::client::query::Query::TaskState::Unassigned, response.testTaskState)
			<< "testTaskState should match";

		ASSERT_EQ(std::size_t { 1 }, response.anyType.size()) << "anyType should have 1 entry";
		ASSERT_TRUE(response.anyType[0].has_value()) << "appointment should be set";
		const auto& anyType = *response.anyType[0];
		EXPECT_EQ("Appointment", anyType._typename) << "__typename should match";
		EXPECT_EQ(today::getFakeAppointmentId(), anyType.id)
			<< "id should match in base64 encoding";
		EXPECT_FALSE(anyType.title.has_value()) << "appointment should not have a title";
		EXPECT_FALSE(anyType.isComplete) << "appointment should not set isComplete";
		ASSERT_TRUE(anyType.subject.has_value()) << "subject should be set";
		EXPECT_EQ("Lunch?", *(anyType.subject)) << "subject should match";
		ASSERT_TRUE(anyType.when.has_value()) << "when should be set";
		EXPECT_EQ("tomorrow", anyType.when->get<std::string>()) << "when should match";
		EXPECT_FALSE(anyType.isNow) << "isNow should match";
	}
	catch (const std::logic_error& ex)
	{
		FAIL() << ex.what();
	}
}

TEST_F(ClientCase, MutateCompleteTask)
{
	using namespace mutate::client::mutation::CompleteTaskMutation;

	auto query = GetRequestObject();
	auto variables = serializeVariables(
		{ std::make_unique<CompleteTaskInput>(CompleteTaskInput { today::getFakeTaskId(),
			std::nullopt,
			std::make_optional(true),
			std::make_optional("Hi There!"s) }) });

	auto state = std::make_shared<today::RequestState>(5);
	auto result =
		_mockService->service->resolve({ query, {}, std::move(variables), {}, state }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto serviceResponse = client::parseServiceResponse(std::move(result));
		const auto response = parseResponse(std::move(serviceResponse.data));

		EXPECT_EQ(std::size_t { 0 }, serviceResponse.errors.size()) << "no errors expected";

		const auto& completedTask = response.completedTask;
		const auto& task = completedTask.completedTask;
		ASSERT_TRUE(task.has_value()) << "should get back a task";
		EXPECT_EQ(today::getFakeTaskId(), task->completedTaskId)
			<< "id should match in base64 encoding";
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
	using namespace subscribe::client::subscription::TestSubscription;

	auto query = GetRequestObject();

	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(6);
	response::Value result;
	auto key = _mockService->service
				   ->subscribe({ [&result](response::Value&& response) {
									result = std::move(response);
								},
					   std::move(query),
					   "TestSubscription"s,
					   std::move(variables),
					   {},
					   state })
				   .get();
	_mockService->service->deliver({ "nextAppointmentChange"sv }).get();
	_mockService->service->unsubscribe({ key }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto serviceResponse = client::parseServiceResponse(std::move(result));
		const auto response = parseResponse(std::move(serviceResponse.data));

		EXPECT_EQ(std::size_t { 0 }, serviceResponse.errors.size()) << "no errors expected";

		const auto& appointmentNode = response.nextAppointment;
		ASSERT_TRUE(appointmentNode.has_value()) << "should get back a task";
		EXPECT_EQ(today::getFakeAppointmentId(), appointmentNode->nextAppointmentId)
			<< "id should match in base64 encoding";
		ASSERT_TRUE(appointmentNode->subject.has_value()) << "subject should be set";
		EXPECT_EQ("Lunch?", *(appointmentNode->subject)) << "subject should match";
		ASSERT_TRUE(appointmentNode->when.has_value()) << "when should be set";
		EXPECT_EQ("tomorrow", appointmentNode->when->get<std::string>()) << "when should match";
		EXPECT_TRUE(appointmentNode->isNow) << "isNow should match";
	}
	catch (const std::logic_error& ex)
	{
		FAIL() << ex.what();
	}
}
