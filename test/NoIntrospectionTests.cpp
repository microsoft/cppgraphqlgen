// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "TodayMock.h"

#include "graphqlservice/JSONResponse.h"

#include <chrono>

using namespace graphql;

using namespace std::literals;

// this is similar to TodayTests, will trust QueryEverything works
// and then check if introspection is disabled

class NoIntrospectionServiceCase : public ::testing::Test
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

response::IdType NoIntrospectionServiceCase::_fakeAppointmentId;
response::IdType NoIntrospectionServiceCase::_fakeTaskId;
response::IdType NoIntrospectionServiceCase::_fakeFolderId;

std::shared_ptr<today::Operations> NoIntrospectionServiceCase::_service;
size_t NoIntrospectionServiceCase::_getAppointmentsCount = 0;
size_t NoIntrospectionServiceCase::_getTasksCount = 0;
size_t NoIntrospectionServiceCase::_getUnreadCountsCount = 0;
size_t today::NextAppointmentChange::_notifySubscribeCount = 0;
size_t today::NextAppointmentChange::_subscriptionCount = 0;
size_t today::NextAppointmentChange::_notifyUnsubscribeCount = 0;

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

TEST_F(NoIntrospectionServiceCase, NoSchema)
{
	auto query = R"(query {
			__schema {
				queryType { name }
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
	auto result = _service->resolve({ query }).get();

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
