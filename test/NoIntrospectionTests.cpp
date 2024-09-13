// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "graphqlservice/JSONResponse.h"

#include <chrono>
#include <cstddef>

import GraphQL.Parse;
import GraphQL.Service;

import GraphQL.Today.Mock;

using namespace graphql;

using namespace std::literals;

// this is similar to TodayTests, will trust QueryEverything works
// and then check if introspection is disabled

class NoIntrospectionServiceCase : public ::testing::Test
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
	std::unique_ptr<today::TodayMockService> _mockService;
};

TEST_F(NoIntrospectionServiceCase, QueryEverything)
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
		_mockService->service
			->resolve({ query, "Everything"sv, std::move(variables), std::launch::async, state })
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
		auto errorsItr = result.find("errors");
		if (errorsItr != result.get<response::MapType>().cend())
		{
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointments = service::ScalarArgument::require("appointments", data);
		const auto appointmentEdges =
			service::ScalarArgument::require<service::TypeModifier::List>("edges", appointments);
		ASSERT_EQ(std::size_t { 1 }, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].type() == response::Type::Map)
			<< "appointment should be an object";
		const auto appointmentNode = service::ScalarArgument::require("node", appointmentEdges[0]);
		EXPECT_EQ(today::getFakeAppointmentId(),
			service::IdArgument::require("id", appointmentNode))
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
		ASSERT_EQ(std::size_t { 1 }, taskEdges.size()) << "tasks should have 1 entry";
		ASSERT_TRUE(taskEdges[0].type() == response::Type::Map) << "task should be an object";
		const auto taskNode = service::ScalarArgument::require("node", taskEdges[0]);
		EXPECT_EQ(today::getFakeTaskId(), service::IdArgument::require("id", taskNode))
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
		ASSERT_EQ(std::size_t { 1 }, unreadCountEdges.size()) << "unreadCounts should have 1 entry";
		ASSERT_TRUE(unreadCountEdges[0].type() == response::Type::Map)
			<< "unreadCount should be an object";
		const auto unreadCountNode = service::ScalarArgument::require("node", unreadCountEdges[0]);
		EXPECT_EQ(today::getFakeFolderId(), service::IdArgument::require("id", unreadCountNode))
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

TEST_F(NoIntrospectionServiceCase, NoSchema)
{
	auto query = R"(query {
			__schema {
				queryType { name }
			}
		})"_graphql;
	auto result = _mockService->service->resolve({ query }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		ASSERT_FALSE(errorsItr == result.get<response::MapType>().cend());
		auto errorsString = response::toJSON(response::Value(errorsItr->second));
		EXPECT_EQ(
			R"js([{"message":"Undefined field type: Query name: __schema","locations":[{"line":2,"column":4}]}])js",
			errorsString)
			<< "error should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}

TEST_F(NoIntrospectionServiceCase, NoType)
{
	auto query = R"(query {
			__type(name: "Query") {
				description
			}
		})"_graphql;
	auto result = _mockService->service->resolve({ query }).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		ASSERT_FALSE(errorsItr == result.get<response::MapType>().cend());
		auto errorsString = response::toJSON(response::Value(errorsItr->second));
		EXPECT_EQ(
			R"js([{"message":"Undefined field type: Query name: __type","locations":[{"line":2,"column":4}]}])js",
			errorsString)
			<< "error should match";
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}
}
