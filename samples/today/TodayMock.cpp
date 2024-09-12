// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayMock.h"

#include "AppointmentConnectionObject.h"
#include "CompleteTaskPayloadObject.h"
#include "ExpensiveObject.h"
#include "FolderConnectionObject.h"
#include "NestedTypeObject.h"
#include "TaskConnectionObject.h"
#include "UnionTypeObject.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <mutex>

namespace graphql::today {

const response::IdType& getFakeAppointmentId() noexcept
{
	static const auto s_fakeId = []() noexcept {
		std::string_view fakeIdString { "fakeAppointmentId" };
		response::IdType result(fakeIdString.size());

		std::copy(fakeIdString.cbegin(), fakeIdString.cend(), result.begin());

		return response::IdType { std::move(result) };
	}();

	return s_fakeId;
}

const response::IdType& getFakeTaskId() noexcept
{
	static const auto s_fakeId = []() noexcept {
		std::string_view fakeIdString { "fakeTaskId" };
		response::IdType result(fakeIdString.size());

		std::copy(fakeIdString.cbegin(), fakeIdString.cend(), result.begin());

		return response::IdType { std::move(result) };
	}();

	return s_fakeId;
}

const response::IdType& getFakeFolderId() noexcept
{
	static const auto s_fakeId = []() noexcept {
		std::string_view fakeIdString { "fakeFolderId" };
		response::IdType result(fakeIdString.size());

		std::copy(fakeIdString.cbegin(), fakeIdString.cend(), result.begin());

		return response::IdType { std::move(result) };
	}();

	return s_fakeId;
}

std::unique_ptr<TodayMockService> mock_service() noexcept
{
	auto result = std::make_unique<TodayMockService>();

	auto query = std::make_shared<Query>(
		[mockService = result.get()]() -> std::vector<std::shared_ptr<Appointment>> {
			++mockService->getAppointmentsCount;
			return { std::make_shared<Appointment>(response::IdType(getFakeAppointmentId()),
				"tomorrow",
				"Lunch?",
				false) };
		},
		[mockService = result.get()]() -> std::vector<std::shared_ptr<Task>> {
			++mockService->getTasksCount;
			return {
				std::make_shared<Task>(response::IdType(getFakeTaskId()), "Don't forget", true)
			};
		},
		[mockService = result.get()]() -> std::vector<std::shared_ptr<Folder>> {
			++mockService->getUnreadCountsCount;
			return {
				std::make_shared<Folder>(response::IdType(getFakeFolderId()), "\"Fake\" Inbox", 3)
			};
		});
	auto mutation = std::make_shared<Mutation>(
		[](CompleteTaskInput&& input) -> std::shared_ptr<CompleteTaskPayload> {
			return std::make_shared<CompleteTaskPayload>(
				std::make_shared<Task>(std::move(input.id), "Mutated Task!", *(input.isComplete)),
				std::move(input.clientMutationId));
		});
	auto subscription = std::make_shared<NextAppointmentChange>(
		[](const std::shared_ptr<service::RequestState>&) -> std::shared_ptr<Appointment> {
			return { std::make_shared<Appointment>(response::IdType(getFakeAppointmentId()),
				"tomorrow",
				"Lunch?",
				true) };
		});

	result->service = std::make_shared<Operations>(std::move(query),
		std::move(mutation),
		std::move(subscription));

	return result;
}

RequestState::RequestState(std::size_t id)
	: requestId(id)
{
}

PageInfo::PageInfo(bool hasNextPage, bool hasPreviousPage)
	: _hasNextPage(hasNextPage)
	, _hasPreviousPage(hasPreviousPage)
{
}

bool PageInfo::getHasNextPage() const noexcept
{
	return _hasNextPage;
}

bool PageInfo::getHasPreviousPage() const noexcept
{
	return _hasPreviousPage;
}

Appointment::Appointment(
	response::IdType&& id, std::string&& when, std::string&& subject, bool isNow)
	: _id(std::move(id))
	, _when(std::make_shared<response::Value>(std::move(when)))
	, _subject(std::make_shared<response::Value>(std::move(subject)))
	, _isNow(isNow)
{
}

const response::IdType& Appointment::id() const noexcept
{
	return _id;
}

service::AwaitableScalar<response::IdType> Appointment::getId() const noexcept
{
	return _id;
}

std::shared_ptr<const response::Value> Appointment::getWhen() const noexcept
{
	return _when;
}

std::shared_ptr<const response::Value> Appointment::getSubject() const noexcept
{
	return _subject;
}

bool Appointment::getIsNow() const noexcept
{
	return _isNow;
}

std::optional<std::string> Appointment::getForceError() const
{
	throw std::runtime_error(R"ex(this error was forced)ex");
}

AppointmentEdge::AppointmentEdge(std::shared_ptr<Appointment> appointment)
	: _appointment(std::move(appointment))
{
}

std::shared_ptr<object::Appointment> AppointmentEdge::getNode() const noexcept
{
	return std::make_shared<object::Appointment>(_appointment);
}

service::AwaitableScalar<response::Value> AppointmentEdge::getCursor() const
{
	co_return response::Value(co_await _appointment->getId());
}

AppointmentConnection::AppointmentConnection(
	bool hasNextPage, bool hasPreviousPage, std::vector<std::shared_ptr<Appointment>> appointments)
	: _pageInfo(std::make_shared<PageInfo>(hasNextPage, hasPreviousPage))
	, _appointments(std::move(appointments))
{
}

std::shared_ptr<object::PageInfo> AppointmentConnection::getPageInfo() const noexcept
{
	return std::make_shared<object::PageInfo>(_pageInfo);
}

std::optional<std::vector<std::shared_ptr<object::AppointmentEdge>>> AppointmentConnection::
	getEdges() const noexcept
{
	auto result = std::make_optional<std::vector<std::shared_ptr<object::AppointmentEdge>>>(
		_appointments.size());

	std::transform(_appointments.cbegin(),
		_appointments.cend(),
		result->begin(),
		[](const std::shared_ptr<Appointment>& node) {
			return std::make_shared<object::AppointmentEdge>(
				std::make_shared<AppointmentEdge>(node));
		});

	return result;
}

Task::Task(response::IdType&& id, std::string&& title, bool isComplete)
	: _id(std::move(id))
	, _title(std::make_shared<response::Value>(std::move(title)))
	, _isComplete(isComplete)
{
}

const response::IdType& Task::id() const
{
	return _id;
}

service::AwaitableScalar<response::IdType> Task::getId() const noexcept
{
	return _id;
}

std::shared_ptr<const response::Value> Task::getTitle() const noexcept
{
	return _title;
}

bool Task::getIsComplete() const noexcept
{
	return _isComplete;
}

TaskEdge::TaskEdge(std::shared_ptr<Task> task)
	: _task(std::move(task))
{
}

std::shared_ptr<object::Task> TaskEdge::getNode() const noexcept
{
	return std::make_shared<object::Task>(_task);
}

service::AwaitableScalar<response::Value> TaskEdge::getCursor() const noexcept
{
	co_return response::Value(co_await _task->getId());
}

TaskConnection::TaskConnection(
	bool hasNextPage, bool hasPreviousPage, std::vector<std::shared_ptr<Task>> tasks)
	: _pageInfo(std::make_shared<PageInfo>(hasNextPage, hasPreviousPage))
	, _tasks(std::move(tasks))
{
}

std::shared_ptr<object::PageInfo> TaskConnection::getPageInfo() const noexcept
{
	return std::make_shared<object::PageInfo>(_pageInfo);
}

std::optional<std::vector<std::shared_ptr<object::TaskEdge>>> TaskConnection::getEdges()
	const noexcept
{
	auto result = std::make_optional<std::vector<std::shared_ptr<object::TaskEdge>>>(_tasks.size());

	std::transform(_tasks.cbegin(),
		_tasks.cend(),
		result->begin(),
		[](const std::shared_ptr<Task>& node) {
			return std::make_shared<object::TaskEdge>(std::make_shared<TaskEdge>(node));
		});

	return result;
}

Folder::Folder(response::IdType&& id, std::string&& name, int unreadCount)
	: _id(std::move(id))
	, _name(std::make_shared<response::Value>(std::move(name)))
	, _unreadCount(unreadCount)
{
}

const response::IdType& Folder::id() const noexcept
{
	return _id;
}

service::AwaitableScalar<response::IdType> Folder::getId() const noexcept
{
	return _id;
}

std::shared_ptr<const response::Value> Folder::getName() const noexcept
{
	return _name;
}

int Folder::getUnreadCount() const noexcept
{
	return _unreadCount;
}

FolderEdge::FolderEdge(std::shared_ptr<Folder> folder)
	: _folder(std::move(folder))
{
}

std::shared_ptr<object::Folder> FolderEdge::getNode() const noexcept
{
	return std::make_shared<object::Folder>(_folder);
}

service::AwaitableScalar<response::Value> FolderEdge::getCursor() const noexcept
{
	co_return response::Value(co_await _folder->getId());
}

FolderConnection::FolderConnection(
	bool hasNextPage, bool hasPreviousPage, std::vector<std::shared_ptr<Folder>> folders)
	: _pageInfo(std::make_shared<PageInfo>(hasNextPage, hasPreviousPage))
	, _folders(std::move(folders))
{
}

std::shared_ptr<object::PageInfo> FolderConnection::getPageInfo() const noexcept
{
	return std::make_shared<object::PageInfo>(_pageInfo);
}

std::optional<std::vector<std::shared_ptr<object::FolderEdge>>> FolderConnection::getEdges()
	const noexcept
{
	auto result =
		std::make_optional<std::vector<std::shared_ptr<object::FolderEdge>>>(_folders.size());

	std::transform(_folders.cbegin(),
		_folders.cend(),
		result->begin(),
		[](const std::shared_ptr<Folder>& node) {
			return std::make_shared<object::FolderEdge>(std::make_shared<FolderEdge>(node));
		});

	return result;
}

Query::Query(appointmentsLoader&& getAppointments, tasksLoader&& getTasks,
	unreadCountsLoader&& getUnreadCounts)
	: _getAppointments(std::move(getAppointments))
	, _getTasks(std::move(getTasks))
	, _getUnreadCounts(getUnreadCounts)
{
}

void Query::loadAppointments(const std::shared_ptr<service::RequestState>& state)
{
	static std::mutex s_loaderMutex {};
	std::lock_guard lock { s_loaderMutex };

	if (_getAppointments)
	{
		if (state)
		{
			auto todayState = std::static_pointer_cast<RequestState>(state);

			todayState->appointmentsRequestId = todayState->requestId;
			todayState->loadAppointmentsCount++;
		}

		_appointments = _getAppointments();
		_getAppointments = nullptr;
	}
}

std::shared_ptr<Appointment> Query::findAppointment(
	const service::FieldParams& params, const response::IdType& id)
{
	loadAppointments(params.state);

	for (const auto& appointment : _appointments)
	{
		if (appointment->id() == id)
		{
			return appointment;
		}
	}

	return nullptr;
}

void Query::loadTasks(const std::shared_ptr<service::RequestState>& state)
{
	static std::mutex s_loaderMutex {};
	std::lock_guard lock { s_loaderMutex };

	if (_getTasks)
	{
		if (state)
		{
			auto todayState = std::static_pointer_cast<RequestState>(state);

			todayState->tasksRequestId = todayState->requestId;
			todayState->loadTasksCount++;
		}

		_tasks = _getTasks();
		_getTasks = nullptr;
	}
}

std::shared_ptr<Task> Query::findTask(
	const service::FieldParams& params, const response::IdType& id)
{
	loadTasks(params.state);

	for (const auto& task : _tasks)
	{
		if (task->id() == id)
		{
			return task;
		}
	}

	return nullptr;
}

void Query::loadUnreadCounts(const std::shared_ptr<service::RequestState>& state)
{
	static std::mutex s_loaderMutex {};
	std::lock_guard lock { s_loaderMutex };

	if (_getUnreadCounts)
	{
		if (state)
		{
			auto todayState = std::static_pointer_cast<RequestState>(state);

			todayState->unreadCountsRequestId = todayState->requestId;
			todayState->loadUnreadCountsCount++;
		}

		_unreadCounts = _getUnreadCounts();
		_getUnreadCounts = nullptr;
	}
}

std::shared_ptr<Folder> Query::findUnreadCount(
	const service::FieldParams& params, const response::IdType& id)
{
	loadUnreadCounts(params.state);

	for (const auto& folder : _unreadCounts)
	{
		if (folder->id() == id)
		{
			return folder;
		}
	}

	return nullptr;
}

template <class _Rep, class _Period>
auto operator co_await(std::chrono::duration<_Rep, _Period> delay)
{
	struct awaiter
	{
		const std::chrono::duration<_Rep, _Period> delay;

		constexpr bool await_ready() const
		{
			return true;
		}

		void await_suspend(std::coroutine_handle<> h) noexcept
		{
			h.resume();
		}

		void await_resume()
		{
			std::this_thread::sleep_for(delay);
		}
	};

	return awaiter { delay };
}

service::AwaitableObject<std::shared_ptr<object::Node>> Query::getNode(
	service::FieldParams params, response::IdType id)
{
	// query { node(id: "ZmFrZVRhc2tJZA==") { ...on Task { title } } }
	using namespace std::literals;
	co_await 100ms;

	auto appointment = findAppointment(params, id);

	if (appointment)
	{
		co_return std::make_shared<object::Node>(
			std::make_shared<object::Appointment>(std::move(appointment)));
	}

	auto task = findTask(params, id);

	if (task)
	{
		co_return std::make_shared<object::Node>(std::make_shared<object::Task>(std::move(task)));
	}

	auto folder = findUnreadCount(params, id);

	if (folder)
	{
		co_return std::make_shared<object::Node>(
			std::make_shared<object::Folder>(std::move(folder)));
	}

	co_return nullptr;
}

template <class _Object, class _Connection>
struct EdgeConstraints
{
	using vec_type = std::vector<std::shared_ptr<_Object>>;
	using itr_type = typename vec_type::const_iterator;

	EdgeConstraints(const std::shared_ptr<service::RequestState>& state, const vec_type& objects)
		: _state(state)
		, _objects(objects)
	{
	}

	std::shared_ptr<_Connection> operator()(const std::optional<int>& first,
		std::optional<response::Value>&& after, const std::optional<int>& last,
		std::optional<response::Value>&& before) const
	{
		auto itrFirst = _objects.cbegin();
		auto itrLast = _objects.cend();

		if (after)
		{
			auto afterId = after->release<response::IdType>();
			auto itrAfter =
				std::find_if(itrFirst, itrLast, [&afterId](const std::shared_ptr<_Object>& entry) {
					return entry->id() == afterId;
				});

			if (itrAfter != itrLast)
			{
				itrFirst = itrAfter;
			}
		}

		if (before)
		{
			auto beforeId = before->release<response::IdType>();
			auto itrBefore =
				std::find_if(itrFirst, itrLast, [&beforeId](const std::shared_ptr<_Object>& entry) {
					return entry->id() == beforeId;
				});

			if (itrBefore != itrLast)
			{
				itrLast = itrBefore;
				++itrLast;
			}
		}

		if (first)
		{
			if (*first < 0)
			{
				std::ostringstream error;

				error << "Invalid argument: first value: " << *first;
				throw service::schema_exception { { service::schema_error { error.str() } } };
			}

			if (itrLast - itrFirst > *first)
			{
				itrLast = itrFirst + *first;
			}
		}

		if (last)
		{
			if (*last < 0)
			{
				std::ostringstream error;

				error << "Invalid argument: last value: " << *last;
				throw service::schema_exception { { service::schema_error { error.str() } } };
			}

			if (itrLast - itrFirst > *last)
			{
				itrFirst = itrLast - *last;
			}
		}

		std::vector<std::shared_ptr<_Object>> edges(itrLast - itrFirst);

		std::copy(itrFirst, itrLast, edges.begin());

		return std::make_shared<_Connection>(itrLast<_objects.cend(), itrFirst> _objects.cbegin(),
			std::move(edges));
	}

private:
	const std::shared_ptr<service::RequestState>& _state;
	const vec_type& _objects;
};

std::future<std::shared_ptr<object::AppointmentConnection>> Query::getAppointments(
	const service::FieldParams& params, std::optional<int> first,
	std::optional<response::Value>&& after, std::optional<int> last,
	std::optional<response::Value>&& before)
{
	auto spThis = shared_from_this();
	auto state = params.state;
	return std::async(
		std::launch::async,
		[this, spThis, state](std::optional<int>&& firstWrapped,
			std::optional<response::Value>&& afterWrapped,
			std::optional<int>&& lastWrapped,
			std::optional<response::Value>&& beforeWrapped) {
			loadAppointments(state);

			EdgeConstraints<Appointment, AppointmentConnection> constraints(state, _appointments);
			auto connection = constraints(firstWrapped,
				std::move(afterWrapped),
				lastWrapped,
				std::move(beforeWrapped));

			return std::make_shared<object::AppointmentConnection>(connection);
		},
		std::move(first),
		std::move(after),
		std::move(last),
		std::move(before));
}

std::future<std::shared_ptr<object::TaskConnection>> Query::getTasks(
	const service::FieldParams& params, std::optional<int> first,
	std::optional<response::Value>&& after, std::optional<int> last,
	std::optional<response::Value>&& before)
{
	auto spThis = shared_from_this();
	auto state = params.state;
	return std::async(
		std::launch::async,
		[this, spThis, state](std::optional<int>&& firstWrapped,
			std::optional<response::Value>&& afterWrapped,
			std::optional<int>&& lastWrapped,
			std::optional<response::Value>&& beforeWrapped) {
			loadTasks(state);

			EdgeConstraints<Task, TaskConnection> constraints(state, _tasks);
			auto connection = constraints(firstWrapped,
				std::move(afterWrapped),
				lastWrapped,
				std::move(beforeWrapped));

			return std::make_shared<object::TaskConnection>(connection);
		},
		std::move(first),
		std::move(after),
		std::move(last),
		std::move(before));
}

std::future<std::shared_ptr<object::FolderConnection>> Query::getUnreadCounts(
	const service::FieldParams& params, std::optional<int> first,
	std::optional<response::Value>&& after, std::optional<int> last,
	std::optional<response::Value>&& before)
{
	auto spThis = shared_from_this();
	auto state = params.state;
	return std::async(
		std::launch::async,
		[this, spThis, state](std::optional<int>&& firstWrapped,
			std::optional<response::Value>&& afterWrapped,
			std::optional<int>&& lastWrapped,
			std::optional<response::Value>&& beforeWrapped) {
			loadUnreadCounts(state);

			EdgeConstraints<Folder, FolderConnection> constraints(state, _unreadCounts);
			auto connection = constraints(firstWrapped,
				std::move(afterWrapped),
				lastWrapped,
				std::move(beforeWrapped));

			return std::make_shared<object::FolderConnection>(connection);
		},
		std::move(first),
		std::move(after),
		std::move(last),
		std::move(before));
}

std::vector<std::shared_ptr<object::Appointment>> Query::getAppointmentsById(
	const service::FieldParams& params, const std::vector<response::IdType>& ids)
{
	std::vector<std::shared_ptr<object::Appointment>> result(ids.size());

	std::transform(ids.cbegin(),
		ids.cend(),
		result.begin(),
		[this, &params](const response::IdType& id) {
			return std::make_shared<object::Appointment>(findAppointment(params, id));
		});

	return result;
}

std::vector<std::shared_ptr<object::Task>> Query::getTasksById(
	const service::FieldParams& params, const std::vector<response::IdType>& ids)
{
	std::vector<std::shared_ptr<object::Task>> result(ids.size());

	std::transform(ids.cbegin(),
		ids.cend(),
		result.begin(),
		[this, &params](const response::IdType& id) {
			return std::make_shared<object::Task>(findTask(params, id));
		});

	return result;
}

std::vector<std::shared_ptr<object::Folder>> Query::getUnreadCountsById(
	const service::FieldParams& params, const std::vector<response::IdType>& ids)
{
	std::vector<std::shared_ptr<object::Folder>> result(ids.size());

	std::transform(ids.cbegin(),
		ids.cend(),
		result.begin(),
		[this, &params](const response::IdType& id) {
			return std::make_shared<object::Folder>(findUnreadCount(params, id));
		});

	return result;
}

std::shared_ptr<object::NestedType> Query::getNested(service::FieldParams&& params)
{
	return std::make_shared<object::NestedType>(std::make_shared<NestedType>(std::move(params), 1));
}

std::vector<std::shared_ptr<object::Expensive>> Query::getExpensive()
{
	std::vector<std::shared_ptr<object::Expensive>> result(Expensive::count);

	for (auto& entry : result)
	{
		entry = std::make_shared<object::Expensive>(std::make_shared<Expensive>());
	}

	return result;
}

TaskState Query::getTestTaskState()
{
	return TaskState::Unassigned;
}

std::vector<std::shared_ptr<object::UnionType>> Query::getAnyType(
	const service::FieldParams& params, const std::vector<response::IdType>&)
{
	loadAppointments(params.state);

	std::vector<std::shared_ptr<object::UnionType>> result(_appointments.size());

	std::transform(_appointments.cbegin(),
		_appointments.cend(),
		result.begin(),
		[](const auto& appointment) noexcept {
			return std::make_shared<object::UnionType>(
				std::make_shared<object::Appointment>(appointment));
		});

	return result;
}

std::optional<std::string> Query::getDefault() const noexcept
{
	return std::nullopt;
}

CompleteTaskPayload::CompleteTaskPayload(
	std::shared_ptr<Task> task, std::optional<std::string>&& clientMutationId)
	: _task(std::move(task))
	, _clientMutationId(std::move(clientMutationId))
{
}

std::shared_ptr<object::Task> CompleteTaskPayload::getTask() const noexcept
{
	return std::make_shared<object::Task>(_task);
}

const std::optional<std::string>& CompleteTaskPayload::getClientMutationId() const noexcept
{
	return _clientMutationId;
}

Mutation::Mutation(completeTaskMutation&& mutateCompleteTask)
	: _mutateCompleteTask(std::move(mutateCompleteTask))
{
}

std::shared_ptr<object::CompleteTaskPayload> Mutation::applyCompleteTask(
	CompleteTaskInput&& input) noexcept
{
	return std::make_shared<object::CompleteTaskPayload>(_mutateCompleteTask(std::move(input)));
}

std::optional<double> Mutation::_setFloat = std::nullopt;

double Mutation::getFloat() noexcept
{
	return *_setFloat;
}

double Mutation::applySetFloat(double valueArg) noexcept
{
	_setFloat = std::make_optional(valueArg);
	return valueArg;
}

std::shared_ptr<object::Appointment> Subscription::getNextAppointmentChange() const
{
	throw std::runtime_error("Unexpected call to getNextAppointmentChange");
}

std::shared_ptr<object::Node> Subscription::getNodeChange(const response::IdType&) const
{
	throw std::runtime_error("Unexpected call to getNodeChange");
}

std::size_t NextAppointmentChange::_notifySubscribeCount = 0;
std::size_t NextAppointmentChange::_subscriptionCount = 0;
std::size_t NextAppointmentChange::_notifyUnsubscribeCount = 0;

NextAppointmentChange::NextAppointmentChange(nextAppointmentChange&& changeNextAppointment)
	: _changeNextAppointment(std::move(changeNextAppointment))
{
}

std::size_t NextAppointmentChange::getCount(service::ResolverContext resolverContext)
{
	switch (resolverContext)
	{
		case service::ResolverContext::NotifySubscribe:
			return _notifySubscribeCount;

		case service::ResolverContext::Subscription:
			return _subscriptionCount;

		case service::ResolverContext::NotifyUnsubscribe:
			return _notifyUnsubscribeCount;

		default:
			throw std::runtime_error("Unexpected ResolverContext");
	}
}

std::shared_ptr<object::Appointment> NextAppointmentChange::getNextAppointmentChange(
	const service::FieldParams& params) const
{
	switch (params.resolverContext)
	{
		case service::ResolverContext::NotifySubscribe:
		{
			++_notifySubscribeCount;
			break;
		}

		case service::ResolverContext::Subscription:
		{
			++_subscriptionCount;
			break;
		}

		case service::ResolverContext::NotifyUnsubscribe:
		{
			++_notifyUnsubscribeCount;
			break;
		}

		default:
			throw std::runtime_error("Unexpected ResolverContext");
	}

	return std::make_shared<object::Appointment>(_changeNextAppointment(params.state));
}

std::shared_ptr<object::Node> NextAppointmentChange::getNodeChange(const response::IdType&) const
{
	throw std::runtime_error("Unexpected call to getNodeChange");
}

NodeChange::NodeChange(nodeChange&& changeNode)
	: _changeNode(std::move(changeNode))
{
}

std::shared_ptr<object::Appointment> NodeChange::getNextAppointmentChange() const
{
	throw std::runtime_error("Unexpected call to getNextAppointmentChange");
}

std::shared_ptr<object::Node> NodeChange::getNodeChange(
	const service::FieldParams& params, response::IdType&& idArg) const
{
	return _changeNode(params.resolverContext, params.state, std::move(idArg));
}

std::stack<CapturedParams> NestedType::_capturedParams;

NestedType::NestedType(service::FieldParams&& params, int depth)
	: depth(depth)
{
	_capturedParams.push({ { params.operationDirectives },
		params.fragmentDefinitionDirectives->empty()
			? service::Directives {}
			: service::Directives { params.fragmentDefinitionDirectives->front().get() },
		params.fragmentSpreadDirectives->empty()
			? service::Directives {}
			: service::Directives { params.fragmentSpreadDirectives->front() },
		params.inlineFragmentDirectives->empty()
			? service::Directives {}
			: service::Directives { params.inlineFragmentDirectives->front() },
		std::move(params.fieldDirectives) });
}

int NestedType::getDepth() const noexcept
{
	return depth;
}

std::shared_ptr<object::NestedType> NestedType::getNested(
	service::FieldParams&& params) const noexcept
{
	return std::make_shared<object::NestedType>(
		std::make_shared<NestedType>(std::move(params), depth + 1));
}

std::stack<CapturedParams> NestedType::getCapturedParams() noexcept
{
	auto result = std::move(_capturedParams);

	return result;
}

std::mutex Expensive::testMutex {};
std::mutex Expensive::pendingExpensiveMutex {};
std::condition_variable Expensive::pendingExpensiveCondition {};
std::size_t Expensive::pendingExpensive = 0;

std::atomic<std::size_t> Expensive::instances = 0;

bool Expensive::Reset() noexcept
{
	std::unique_lock pendingExpensiveLock(pendingExpensiveMutex);

	pendingExpensive = 0;
	pendingExpensiveLock.unlock();

	return instances == 0;
}

Expensive::Expensive()
	: order(++instances)
{
}

Expensive::~Expensive()
{
	--instances;
}

std::future<int> Expensive::getOrder(const service::FieldParams& params) const noexcept
{
	return std::async(
		params.launch.await_ready() ? std::launch::deferred : std::launch::async,
		[](bool blockAsync, int instanceOrder) noexcept {
			if (blockAsync)
			{
				// Block all of the Expensive objects in async mode until the count is reached.
				std::unique_lock pendingExpensiveLock(pendingExpensiveMutex);

				if (++pendingExpensive < count)
				{
					pendingExpensiveCondition.wait(pendingExpensiveLock, []() {
						return pendingExpensive == count;
					});
				}

				// Wake up the next Expensive object.
				pendingExpensiveLock.unlock();
				pendingExpensiveCondition.notify_one();
			}

			return instanceOrder;
		},
		!params.launch.await_ready(),
		static_cast<int>(order));
}

EmptyOperations::EmptyOperations()
	: service::Request({}, GetSchema())
{
}

} // namespace graphql::today
