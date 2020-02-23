// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "UnifiedToday.h"

#include <graphqlservice/JSONResponse.h>

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

		auto query = std::make_shared<today::Query>(
			[]() -> std::vector<std::shared_ptr<today::Appointment>>
		{
			++_getAppointmentsCount;
			return { std::make_shared<today::Appointment>(response::IdType(_fakeAppointmentId), "tomorrow", "Lunch?", false) };
		}, []() -> std::vector<std::shared_ptr<today::Task>>
		{
			++_getTasksCount;
			return { std::make_shared<today::Task>(response::IdType(_fakeTaskId), "Don't forget", true) };
		}, []() -> std::vector<std::shared_ptr<today::Folder>>
		{
			++_getUnreadCountsCount;
			return { std::make_shared<today::Folder>(response::IdType(_fakeFolderId), "\"Fake\" Inbox", 3) };
		});
		auto mutation = std::make_shared<today::Mutation>(
			[](today::CompleteTaskInput&& input) -> std::shared_ptr<today::CompleteTaskPayload>
		{
			return std::make_shared<today::CompleteTaskPayload>(
				std::make_shared<today::Task>(std::move(input.id), "Mutated Task!", *(input.isComplete)),
				std::move(input.clientMutationId)
				);
		});
		auto subscription = std::make_shared<today::NextAppointmentChange>(
			[](const std::shared_ptr<service::RequestState>&) -> std::shared_ptr<today::Appointment>
		{
			return { std::make_shared<today::Appointment>(response::IdType(_fakeAppointmentId), "tomorrow", "Lunch?", true) };
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

response::IdType TodayServiceCase::_fakeAppointmentId;
response::IdType TodayServiceCase::_fakeTaskId;
response::IdType TodayServiceCase::_fakeFolderId;

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
	auto result = _service->resolve(state, *ast.root, "Everything", std::move(variables)).get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_EQ(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_EQ(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(1), state->appointmentsRequestId) << "today service passed the same RequestState";
	EXPECT_EQ(size_t(1), state->tasksRequestId) << "today service passed the same RequestState";
	EXPECT_EQ(size_t(1), state->unreadCountsRequestId) << "today service passed the same RequestState";
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
		const auto appointmentEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges", appointments);
		ASSERT_EQ(1, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].type() == response::Type::Map) << "appointment should be an object";
		const auto appointmentNode = service::ScalarArgument::require("node", appointmentEdges[0]);
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("id", appointmentNode)) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode)) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode)) << "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentNode)) << "isNow should match";
		EXPECT_EQ("Appointment", service::StringArgument::require("__typename", appointmentNode)) << "__typename should match";

		const auto tasks = service::ScalarArgument::require("tasks", data);
		const auto taskEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges", tasks);
		ASSERT_EQ(1, taskEdges.size()) << "tasks should have 1 entry";
		ASSERT_TRUE(taskEdges[0].type() == response::Type::Map) << "task should be an object";
		const auto taskNode = service::ScalarArgument::require("node", taskEdges[0]);
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("id", taskNode)) << "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", taskNode)) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", taskNode)) << "isComplete should match";
		EXPECT_EQ("Task", service::StringArgument::require("__typename", taskNode)) << "__typename should match";

		const auto unreadCounts = service::ScalarArgument::require("unreadCounts", data);
		const auto unreadCountEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges", unreadCounts);
		ASSERT_EQ(1, unreadCountEdges.size()) << "unreadCounts should have 1 entry";
		ASSERT_TRUE(unreadCountEdges[0].type() == response::Type::Map) << "unreadCount should be an object";
		const auto unreadCountNode = service::ScalarArgument::require("node", unreadCountEdges[0]);
		EXPECT_EQ(_fakeFolderId, service::IdArgument::require("id", unreadCountNode)) << "id should match in base64 encoding";
		EXPECT_EQ("\"Fake\" Inbox", service::StringArgument::require("name", unreadCountNode)) << "name should match";
		EXPECT_EQ(3, service::IntArgument::require("unreadCount", unreadCountNode)) << "unreadCount should match";
		EXPECT_EQ("Folder", service::StringArgument::require("__typename", unreadCountNode)) << "__typename should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
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
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(2);
	auto result = _service->resolve(state, *ast.root, "", std::move(variables)).get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(2), state->appointmentsRequestId) << "today service passed the same RequestState";
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
		const auto appointmentEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges", appointments);
		ASSERT_EQ(1, appointmentEdges.size()) << "appointments should have 1 entry";
		ASSERT_TRUE(appointmentEdges[0].type() == response::Type::Map) << "appointment should be an object";
		const auto appointmentNode = service::ScalarArgument::require("node", appointmentEdges[0]);
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("appointmentId", appointmentNode)) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode)) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode)) << "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentNode)) << "isNow should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
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
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(3);
	auto result = _service->resolve(state, *ast.root, "", std::move(variables)).get();
	EXPECT_GE(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_EQ(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";
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
		const auto taskEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges", tasks);
		ASSERT_EQ(1, taskEdges.size()) << "tasks should have 1 entry";
		ASSERT_TRUE(taskEdges[0].type() == response::Type::Map) << "task should be an object";
		const auto taskNode = service::ScalarArgument::require("node", taskEdges[0]);
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("taskId", taskNode)) << "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", taskNode)) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", taskNode)) << "isComplete should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
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
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(4);
	auto result = _service->resolve(state, *ast.root, "", std::move(variables)).get();
	EXPECT_GE(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_EQ(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(0), state->appointmentsRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(0), state->tasksRequestId) << "today service did not call the loader";
	EXPECT_EQ(size_t(4), state->unreadCountsRequestId) << "today service passed the same RequestState";
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
		const auto unreadCountEdges = service::ScalarArgument::require<service::TypeModifier::List>("edges", unreadCounts);
		ASSERT_EQ(1, unreadCountEdges.size()) << "unreadCounts should have 1 entry";
		ASSERT_TRUE(unreadCountEdges[0].type() == response::Type::Map) << "unreadCount should be an object";
		const auto unreadCountNode = service::ScalarArgument::require("node", unreadCountEdges[0]);
		EXPECT_EQ(_fakeFolderId, service::IdArgument::require("folderId", unreadCountNode)) << "id should match in base64 encoding";
		EXPECT_EQ("\"Fake\" Inbox", service::StringArgument::require("name", unreadCountNode)) << "name should match";
		EXPECT_EQ(3, service::IntArgument::require("unreadCount", unreadCountNode)) << "unreadCount should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
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
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(5);
	auto result = _service->resolve(state, *ast.root, "", std::move(variables)).get();

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
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("completedTaskId", task)) << "id should match in base64 encoding";
		EXPECT_EQ("Mutated Task!", service::StringArgument::require("title", task)) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", task)) << "isComplete should match";

		const auto clientMutationId = service::StringArgument::require("clientMutationId", completedTask);
		EXPECT_EQ("Hi There!", clientMutationId) << "clientMutationId should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, SubscribeNextAppointmentChangeDefault)
{
	auto ast = peg::parseString(R"(subscription TestSubscription {
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
	auto key = _service->subscribe(service::SubscriptionParams { state, std::move(ast), "TestSubscription", std::move(std::move(variables)) },
		[&result](std::future<response::Value> response)
		{
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
			FAIL() << response::toJSON(response::Value(errorsItr->second));
		}
		const auto data = service::ScalarArgument::require("data", result);

		const auto appointmentNode = service::ScalarArgument::require("nextAppointment", data);
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("nextAppointmentId", appointmentNode)) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode)) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode)) << "when should match";
		EXPECT_TRUE(service::BooleanArgument::require("isNow", appointmentNode)) << "isNow should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, SubscribeNextAppointmentChangeOverride)
{
	auto ast = peg::parseString(R"(subscription TestSubscription {
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
		[this](const std::shared_ptr<service::RequestState>& state) -> std::shared_ptr<today::Appointment>
	{
		EXPECT_EQ(7, std::static_pointer_cast<today::RequestState>(state)->requestId) << "should pass the RequestState to the subscription resolvers";
		return std::make_shared<today::Appointment>(response::IdType(_fakeAppointmentId), "today", "Dinner Time!", true);
	});
	response::Value result;
	auto key = _service->subscribe(service::SubscriptionParams { state, std::move(ast), "TestSubscription", std::move(std::move(variables)) },
		[&result](std::future<response::Value> response)
	{
		result = response.get();
	});
	_service->deliver("nextAppointmentChange", std::static_pointer_cast<service::Object>(subscriptionObject));
	_service->unsubscribe(key);

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
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("nextAppointmentId", appointmentNode)) << "id should match in base64 encoding";
		EXPECT_EQ("Dinner Time!", service::StringArgument::require("subject", appointmentNode)) << "subject should match";
		EXPECT_EQ("today", service::StringArgument::require("when", appointmentNode)) << "when should match";
		EXPECT_TRUE(service::BooleanArgument::require("isNow", appointmentNode)) << "isNow should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
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
	response::Value variables(response::Type::Map);
	auto state = std::make_shared<today::RequestState>(8);
	auto result = _service->resolve(state, *ast.root, "", std::move(variables)).get();

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
		const auto types = service::ScalarArgument::require<service::TypeModifier::List>("types", schema);
		const auto queryType = service::ScalarArgument::require("queryType", schema);
		const auto mutationType = service::ScalarArgument::require("mutationType", schema);

		ASSERT_FALSE(types.empty());
		ASSERT_TRUE(queryType.type() == response::Type::Map);
		ASSERT_TRUE(mutationType.type() == response::Type::Map);
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, SkipDirective)
{
	auto ast = R"({
			__schema {
				types {
					kind
					name
					description
					ofType
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
	auto result = _service->resolve(state, *ast.root, "", std::move(variables)).get();

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
		const auto types = service::ScalarArgument::require<service::TypeModifier::List>("types", schema);
		const auto queryType = service::ScalarArgument::require("queryType", schema);
		const auto mutationType = service::ScalarArgument::find("mutationType", schema);

		ASSERT_FALSE(types.empty());
		ASSERT_TRUE(queryType.type() == response::Type::Map);
		ASSERT_FALSE(mutationType.second);
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, IncludeDirective)
{
	auto ast = R"({
			__schema {
				types {
					kind
					name
					description
					ofType
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
	auto result = _service->resolve(state, *ast.root, "", std::move(variables)).get();

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
		const auto types = service::ScalarArgument::require<service::TypeModifier::List>("types", schema);
		const auto queryType = service::ScalarArgument::find("queryType", schema);
		const auto mutationType = service::ScalarArgument::require("mutationType", schema);

		ASSERT_FALSE(types.empty());
		ASSERT_FALSE(queryType.second);
		ASSERT_TRUE(mutationType.type() == response::Type::Map);
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, NestedFragmentDirectives)
{
	auto ast = R"(
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
							inlineFragmentNested: nested @fieldTag(field: "nested4") {
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
	auto result = _service->resolve(state, *ast.root, "", std::move(variables)).get();

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
		const auto queryTag1 = service::ScalarArgument::require("queryTag", params1.operationDirectives);
		const auto query1 = service::StringArgument::require("query", queryTag1);
		const auto fragmentDefinitionCount1 = params1.fragmentDefinitionDirectives.size();
		const auto fragmentSpreadCount1 = params1.fragmentSpreadDirectives.size();
		const auto inlineFragmentCount1 = params1.inlineFragmentDirectives.size();
		const auto fieldTag1 = service::ScalarArgument::require("fieldTag", params1.fieldDirectives);
		const auto field1 = service::StringArgument::require("field", fieldTag1);
		const auto queryTag2 = service::ScalarArgument::require("queryTag", params2.operationDirectives);
		const auto query2 = service::StringArgument::require("query", queryTag2);
		const auto fragmentDefinitionTag2 = service::ScalarArgument::require("fragmentDefinitionTag", params2.fragmentDefinitionDirectives);
		const auto fragmentDefinition2 = service::StringArgument::require("fragmentDefinition", fragmentDefinitionTag2);
		const auto fragmentSpreadTag2 = service::ScalarArgument::require("fragmentSpreadTag", params2.fragmentSpreadDirectives);
		const auto fragmentSpread2 = service::StringArgument::require("fragmentSpread", fragmentSpreadTag2);
		const auto inlineFragmentCount2 = params2.inlineFragmentDirectives.size();
		const auto fieldTag2 = service::ScalarArgument::require("fieldTag", params2.fieldDirectives);
		const auto field2 = service::StringArgument::require("field", fieldTag2);
		const auto queryTag3 = service::ScalarArgument::require("queryTag", params3.operationDirectives);
		const auto query3 = service::StringArgument::require("query", queryTag3);
		const auto fragmentDefinitionTag3 = service::ScalarArgument::require("fragmentDefinitionTag", params3.fragmentDefinitionDirectives);
		const auto fragmentDefinition3 = service::StringArgument::require("fragmentDefinition", fragmentDefinitionTag3);
		const auto fragmentSpreadTag3 = service::ScalarArgument::require("fragmentSpreadTag", params3.fragmentSpreadDirectives);
		const auto fragmentSpread3 = service::StringArgument::require("fragmentSpread", fragmentSpreadTag3);
		const auto inlineFragmentTag3 = service::ScalarArgument::require("inlineFragmentTag", params3.inlineFragmentDirectives);
		const auto inlineFragment3 = service::StringArgument::require("inlineFragment", inlineFragmentTag3);
		const auto fieldTag3 = service::ScalarArgument::require("fieldTag", params3.fieldDirectives);
		const auto field3 = service::StringArgument::require("field", fieldTag3);
		const auto queryTag4 = service::ScalarArgument::require("queryTag", params4.operationDirectives);
		const auto query4 = service::StringArgument::require("query", queryTag4);
		const auto fragmentDefinitionCount4 = params4.fragmentDefinitionDirectives.size();
		const auto fragmentSpreadCount4 = params4.fragmentSpreadDirectives.size();
		const auto inlineFragmentTag4 = service::ScalarArgument::require("inlineFragmentTag", params4.inlineFragmentDirectives);
		const auto inlineFragment4 = service::StringArgument::require("inlineFragment", inlineFragmentTag4);
		const auto fieldTag4 = service::ScalarArgument::require("fieldTag", params4.fieldDirectives);
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
		ASSERT_EQ("fragmentDefinition1", fragmentDefinition2) << "remember the directives from the fragment definition";
		ASSERT_EQ("fragmentSpread1", fragmentSpread2) << "remember the directives from the fragment spread";
		ASSERT_EQ(size_t(0), inlineFragmentCount2);
		ASSERT_EQ("nested2", field2) << "remember the field directives";
		ASSERT_EQ("nested", query3) << "remember the operation directives";
		ASSERT_EQ("fragmentDefinition2", fragmentDefinition3) << "outer fragement definition directives are preserved with inline fragments";
		ASSERT_EQ("fragmentSpread2", fragmentSpread3) << "outer fragement spread directives are preserved with inline fragments";
		ASSERT_EQ("inlineFragment3", inlineFragment3) << "remember the directives from the inline fragment";
		ASSERT_EQ("nested3", field3) << "remember the field directives";
		ASSERT_EQ("nested", query4) << "remember the operation directives";
		ASSERT_EQ(size_t(0), fragmentDefinitionCount4) << "traversing a field to a nested object SelectionSet resets the fragment directives";
		ASSERT_EQ(size_t(0), fragmentSpreadCount4) << "traversing a field to a nested object SelectionSet resets the fragment directives";
		ASSERT_EQ("inlineFragment5", inlineFragment4) << "nested inline fragments don't reset, but do overwrite on collision";
		ASSERT_EQ("nested4", field4) << "remember the field directives";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, QueryAppointmentsById)
{
	auto ast = R"(query SpecificAppointment($appointmentId: ID!) {
			appointmentsById(ids: [$appointmentId]) {
				appointmentId: id
				subject
				when
				isNow
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	variables.emplace_back("appointmentId", response::Value(std::string("ZmFrZUFwcG9pbnRtZW50SWQ=")));
	auto state = std::make_shared<today::RequestState>(12);
	auto result = _service->resolve(state, *ast.root, "", std::move(variables)).get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(12), state->appointmentsRequestId) << "today service passed the same RequestState";
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

		const auto appointmentsById = service::ScalarArgument::require<service::TypeModifier::List>("appointmentsById", data);
		ASSERT_EQ(size_t(1), appointmentsById.size());
		const auto& appointmentEntry = appointmentsById.front();
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("appointmentId", appointmentEntry)) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentEntry)) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentEntry)) << "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentEntry)) << "isNow should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, UnimplementedFieldError)
{
	auto ast = R"(query {
			unimplemented
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto result = _service->resolve(nullptr, *ast.root, "", std::move(variables)).get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		const auto& errors = result["errors"];
		ASSERT_TRUE(errors.type() == response::Type::List);
		ASSERT_EQ(size_t(1), errors.size());
		const auto& error = errors[0];
		ASSERT_TRUE(error.type() == response::Type::Map);
		const auto& message = error[std::string{ service::strMessage }];
		ASSERT_TRUE(message.type() == response::Type::String);
		ASSERT_EQ(R"e(Field error name: unimplemented unknown error: Query::getUnimplemented is not implemented)e", message.get<response::StringType>());
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, SubscribeNodeChangeMatchingId)
{
	auto ast = peg::parseString(R"(subscription TestSubscription {
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
	auto subscriptionObject = std::make_shared<today::NodeChange>(
		[this](const std::shared_ptr<service::RequestState>& state, response::IdType&& idArg) -> std::shared_ptr<service::Object>
	{
		EXPECT_EQ(13, std::static_pointer_cast<today::RequestState>(state)->requestId) << "should pass the RequestState to the subscription resolvers";
		EXPECT_EQ(_fakeTaskId, idArg);
		return std::static_pointer_cast<service::Object>(std::make_shared<today::Task>(response::IdType(_fakeTaskId), "Don't forget", true));
	});
	response::Value result;
	auto key = _service->subscribe(service::SubscriptionParams { state, std::move(ast), "TestSubscription", std::move(std::move(variables)) },
		[&result](std::future<response::Value> response)
	{
		result = response.get();
	});
	_service->deliver("nodeChange", { {"id", response::Value(std::string("ZmFrZVRhc2tJZA==")) } }, std::static_pointer_cast<service::Object>(subscriptionObject));
	_service->unsubscribe(key);

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
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("changedId", taskNode)) << "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", taskNode)) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", taskNode)) << "isComplete should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, SubscribeNodeChangeMismatchedId)
{
	auto ast = peg::parseString(R"(subscription TestSubscription {
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
		[this, &calledResolver](const std::shared_ptr<service::RequestState>& state, response::IdType&& idArg) -> std::shared_ptr<service::Object>
	{
		calledResolver = true;
		return nullptr;
	});
	bool calledGet = false;
	auto key = _service->subscribe(service::SubscriptionParams { nullptr, std::move(ast), "TestSubscription", std::move(std::move(variables)) },
		[&calledGet](std::future<response::Value>)
	{
		calledGet = true;
	});
	_service->deliver("nodeChange", { {"id", response::Value(std::string("ZmFrZUFwcG9pbnRtZW50SWQ=")) } }, std::static_pointer_cast<service::Object>(subscriptionObject));
	_service->unsubscribe(key);

	try
	{
		ASSERT_FALSE(calledResolver);
		ASSERT_FALSE(calledGet);
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, SubscribeNodeChangeFuzzyComparator)
{
	auto ast = peg::parseString(R"(subscription TestSubscription {
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
	auto filterCallback = [&filterCalled](response::MapType::const_reference fuzzy) noexcept -> bool
	{
		EXPECT_FALSE(filterCalled);
		EXPECT_EQ("id", fuzzy.first) << "should only get called once for the id argument";
		EXPECT_EQ("ZmFr", fuzzy.second.get<response::StringType>());
		filterCalled = true;
		return true;
	};
	auto subscriptionObject = std::make_shared<today::NodeChange>(
		[this](const std::shared_ptr<service::RequestState>& state, response::IdType&& idArg) -> std::shared_ptr<service::Object>
		{
			const response::IdType fuzzyId { 'f', 'a', 'k' };

			EXPECT_EQ(14, std::static_pointer_cast<today::RequestState>(state)->requestId) << "should pass the RequestState to the subscription resolvers";
			EXPECT_EQ(fuzzyId, idArg);
			return std::static_pointer_cast<service::Object>(std::make_shared<today::Task>(response::IdType(_fakeTaskId), "Don't forget", true));
		});
	response::Value result;
	auto key = _service->subscribe(service::SubscriptionParams { state, std::move(ast), "TestSubscription", std::move(std::move(variables)) },
		[&result](std::future<response::Value> response)
		{
			result = response.get();
		});
	_service->deliver("nodeChange", filterCallback, std::static_pointer_cast<service::Object>(subscriptionObject));
	_service->unsubscribe(key);

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
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("changedId", taskNode)) << "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", taskNode)) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", taskNode)) << "isComplete should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, SubscribeNodeChangeFuzzyMismatch)
{
	auto ast = peg::parseString(R"(subscription TestSubscription {
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
	auto filterCallback = [&filterCalled](response::MapType::const_reference fuzzy) noexcept -> bool
	{
		EXPECT_FALSE(filterCalled);
		EXPECT_EQ("id", fuzzy.first) << "should only get called once for the id argument";
		EXPECT_EQ("ZmFrZVRhc2tJZA==", fuzzy.second.get<response::StringType>());
		filterCalled = true;
		return false;
	};
	bool calledResolver = false;
	auto subscriptionObject = std::make_shared<today::NodeChange>(
		[this, &calledResolver](const std::shared_ptr<service::RequestState>& state, response::IdType&& idArg) -> std::shared_ptr<service::Object>
		{
			calledResolver = true;
			return nullptr;
		});
	bool calledGet = false;
	auto key = _service->subscribe(service::SubscriptionParams { nullptr, std::move(ast), "TestSubscription", std::move(std::move(variables)) },
		[&calledGet](std::future<response::Value>)
		{
			calledGet = true;
		});
	_service->deliver("nodeChange", filterCallback, std::static_pointer_cast<service::Object>(subscriptionObject));
	_service->unsubscribe(key);

	try
	{
		ASSERT_TRUE(filterCalled) << "should not match the id parameter in the subscription";
		ASSERT_FALSE(calledResolver);
		ASSERT_FALSE(calledGet);
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, SubscribeNodeChangeMatchingVariable)
{
	auto ast = peg::parseString(R"(subscription TestSubscription($taskId: ID!) {
			changedNode: nodeChange(id: $taskId) {
				changedId: id
				...on Task {
					title
					isComplete
				}
			}
		})");
	response::Value variables(response::Type::Map);
	variables.emplace_back("taskId", response::Value(std::string("ZmFrZVRhc2tJZA==")));
	auto state = std::make_shared<today::RequestState>(14);
	auto subscriptionObject = std::make_shared<today::NodeChange>(
		[this](const std::shared_ptr<service::RequestState>& state, response::IdType&& idArg) -> std::shared_ptr<service::Object>
	{
		EXPECT_EQ(14, std::static_pointer_cast<today::RequestState>(state)->requestId) << "should pass the RequestState to the subscription resolvers";
		EXPECT_EQ(_fakeTaskId, idArg);
		return std::static_pointer_cast<service::Object>(std::make_shared<today::Task>(response::IdType(_fakeTaskId), "Don't forget", true));
	});
	response::Value result;
	auto key = _service->subscribe(service::SubscriptionParams { state, std::move(ast), "TestSubscription", std::move(std::move(variables)) },
		[&result](std::future<response::Value> response)
	{
		result = response.get();
	});
	_service->deliver("nodeChange", { {"id", response::Value(std::string("ZmFrZVRhc2tJZA==")) } }, std::static_pointer_cast<service::Object>(subscriptionObject));
	_service->unsubscribe(key);

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
		EXPECT_EQ(_fakeTaskId, service::IdArgument::require("changedId", taskNode)) << "id should match in base64 encoding";
		EXPECT_EQ("Don't forget", service::StringArgument::require("title", taskNode)) << "title should match";
		EXPECT_TRUE(service::BooleanArgument::require("isComplete", taskNode)) << "isComplete should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, DeferredQueryAppointmentsById)
{
	auto ast = R"(query SpecificAppointment($appointmentId: ID!) {
			appointmentsById(ids: [$appointmentId]) {
				appointmentId: id
				subject
				when
				isNow
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	variables.emplace_back("appointmentId", response::Value(std::string("ZmFrZUFwcG9pbnRtZW50SWQ=")));
	auto state = std::make_shared<today::RequestState>(15);
	auto future = _service->resolve(std::launch::deferred, state, *ast.root, "", std::move(variables));
	ASSERT_EQ(std::future_status::deferred, future.wait_for(0s)) << "should be deferred";
	EXPECT_EQ(size_t(0), state->loadAppointmentsCount) << "today service should not call the loader until we block";
	ASSERT_EQ(std::future_status::deferred, future.wait_for(0s)) << "should stay deferred until we block";
	EXPECT_EQ(size_t(0), state->loadAppointmentsCount) << "today service should not call the loader until we block";
	auto result = future.get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(15), state->appointmentsRequestId) << "today service passed the same RequestState";
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

		const auto appointmentsById = service::ScalarArgument::require<service::TypeModifier::List>("appointmentsById", data);
		ASSERT_EQ(size_t(1), appointmentsById.size());
		const auto& appointmentEntry = appointmentsById.front();
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("appointmentId", appointmentEntry)) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentEntry)) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentEntry)) << "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentEntry)) << "isNow should match";
	}
	catch (const service::schema_exception & ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, NonBlockingQueryAppointmentsById)
{
	auto ast = R"(query SpecificAppointment($appointmentId: ID!) {
			appointmentsById(ids: [$appointmentId]) {
				appointmentId: id
				subject
				when
				isNow
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	variables.emplace_back("appointmentId", response::Value(std::string("ZmFrZUFwcG9pbnRtZW50SWQ=")));
	auto state = std::make_shared<today::RequestState>(16);
	auto future = _service->resolve(std::launch::async, state, *ast.root, "", std::move(variables));
	ASSERT_NE(std::future_status::deferred, future.wait_for(0s)) << "should not start out deferred";
	auto result = future.get();
	EXPECT_EQ(size_t(1), _getAppointmentsCount) << "today service lazy loads the appointments and caches the result";
	EXPECT_GE(size_t(1), _getTasksCount) << "today service lazy loads the tasks and caches the result";
	EXPECT_GE(size_t(1), _getUnreadCountsCount) << "today service lazy loads the unreadCounts and caches the result";
	EXPECT_EQ(size_t(16), state->appointmentsRequestId) << "today service passed the same RequestState";
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

		const auto appointmentsById = service::ScalarArgument::require<service::TypeModifier::List>("appointmentsById", data);
		ASSERT_EQ(size_t(1), appointmentsById.size());
		const auto& appointmentEntry = appointmentsById.front();
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("appointmentId", appointmentEntry)) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentEntry)) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentEntry)) << "when should match";
		EXPECT_FALSE(service::BooleanArgument::require("isNow", appointmentEntry)) << "isNow should match";
	}
	catch (const service::schema_exception & ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, NonExistentTypeIntrospection)
{
	auto ast = R"(query {
			__type(name: "NonExistentType") {
				description
			}
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto future = _service->resolve(std::launch::deferred, nullptr, *ast.root, "", std::move(variables));
	auto result = future.get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		ASSERT_FALSE(errorsItr == result.get<response::MapType>().cend());
		auto errorsString = response::toJSON(response::Value(errorsItr->second));
		EXPECT_EQ(R"js([{"message":"Type not found name: NonExistentType"}])js", errorsString) << "error should match";
	}
	catch (const service::schema_exception & ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, DuplicateFragments)
{
	auto ast = R"(query {
			__type(name: "Appointment") {
				...DuplicateFragment
			}
		}
		fragment DuplicateFragment on __Type {
			name
		}		
		fragment DuplicateFragment on __Type {
			name
		})"_graphql;
	response::Value variables(response::Type::Map);
	auto future = _service->resolve(std::launch::deferred, nullptr, *ast.root, "", std::move(variables));
	auto result = future.get();

	try
	{
		ASSERT_TRUE(result.type() == response::Type::Map);
		auto errorsItr = result.find("errors");
		ASSERT_FALSE(errorsItr == result.get<response::MapType>().cend());
		auto errorsString = response::toJSON(response::Value(errorsItr->second));
		EXPECT_EQ(R"js([{"message":"Duplicate fragment name: DuplicateFragment","locations":[{"line":9,"column":12}]}])js", errorsString) << "error should match";
	}
	catch (const service::schema_exception & ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}

TEST_F(TodayServiceCase, SubscribeNextAppointmentChangeAsync)
{
	auto ast = peg::parseString(R"(subscription TestSubscription {
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
	auto key = _service->subscribe(service::SubscriptionParams { state, std::move(ast), "TestSubscription", std::move(std::move(variables)) },
		[&result](std::future<response::Value> response)
		{
			result = response.get();
		});
	_service->deliver(std::launch::async, "nextAppointmentChange", nullptr);
	_service->unsubscribe(key);
	
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
		EXPECT_EQ(_fakeAppointmentId, service::IdArgument::require("nextAppointmentId", appointmentNode)) << "id should match in base64 encoding";
		EXPECT_EQ("Lunch?", service::StringArgument::require("subject", appointmentNode)) << "subject should match";
		EXPECT_EQ("tomorrow", service::StringArgument::require("when", appointmentNode)) << "when should match";
		EXPECT_TRUE(service::BooleanArgument::require("isNow", appointmentNode)) << "isNow should match";
	}
	catch (const service::schema_exception& ex)
	{
		FAIL() << response::toJSON(response::Value(ex.getErrors()));
	}
}
