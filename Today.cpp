// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Today.h"

#include <iostream>
#include <algorithm>

namespace facebook {
namespace graphql {
namespace today {

Appointment::Appointment(std::vector<uint8_t>&& id, std::string&& when, std::string&& subject, bool isNow)
	: _id(std::move(id))
	, _when(std::move(when))
	, _subject(std::move(subject))
	, _isNow(isNow)
{
}

Task::Task(std::vector<uint8_t>&& id, std::string&& title, bool isComplete)
	: _id(std::move(id))
	, _title(std::move(title))
	, _isComplete(isComplete)
{
}

Folder::Folder(std::vector<uint8_t>&& id, std::string&& name, int unreadCount)
	: _id(std::move(id))
	, _name(std::move(name))
	, _unreadCount(unreadCount)
{
}

Query::Query(appointmentsLoader&& getAppointments, tasksLoader&& getTasks, unreadCountsLoader&& getUnreadCounts)
	: _getAppointments(std::move(getAppointments))
	, _getTasks(std::move(getTasks))
	, _getUnreadCounts(getUnreadCounts)
{
}

void Query::loadAppointments(const std::shared_ptr<service::RequestState>& state) const
{
	if (state)
	{
		auto todayState = std::static_pointer_cast<RequestState>(state);

		todayState->appointmentsRequestId = todayState->requestId;
		todayState->loadAppointmentsCount++;
	}

	if (_getAppointments)
	{
		_appointments = _getAppointments();
		_getAppointments = nullptr;
	}
}

std::shared_ptr<Appointment> Query::findAppointment(const std::shared_ptr<service::RequestState>& state, const std::vector<uint8_t>& id) const
{
	loadAppointments(state);

	for (const auto& appointment : _appointments)
	{
		auto appointmentId = appointment->getId(state).get();

		if (appointmentId == id)
		{
			return appointment;
		}
	}

	return nullptr;
}

void Query::loadTasks(const std::shared_ptr<service::RequestState>& state) const
{
	if (state)
	{
		auto todayState = std::static_pointer_cast<RequestState>(state);

		todayState->tasksRequestId = todayState->requestId;
		todayState->loadTasksCount++;
	}

	if (_getTasks)
	{
		_tasks = _getTasks();
		_getTasks = nullptr;
	}
}

std::shared_ptr<Task> Query::findTask(const std::shared_ptr<service::RequestState>& state, const std::vector<uint8_t>& id) const
{
	loadTasks(state);

	for (const auto& task : _tasks)
	{
		auto taskId = task->getId(state).get();

		if (taskId == id)
		{
			return task;
		}
	}

	return nullptr;
}

void Query::loadUnreadCounts(const std::shared_ptr<service::RequestState>& state) const
{
	if (state)
	{
		auto todayState = std::static_pointer_cast<RequestState>(state);

		todayState->unreadCountsRequestId = todayState->requestId;
		todayState->loadUnreadCountsCount++;
	}

	if (_getUnreadCounts)
	{
		_unreadCounts = _getUnreadCounts();
		_getUnreadCounts = nullptr;
	}
}

std::shared_ptr<Folder> Query::findUnreadCount(const std::shared_ptr<service::RequestState>& state, const std::vector<uint8_t>& id) const
{
	loadUnreadCounts(state);

	for (const auto& folder : _unreadCounts)
	{
		auto folderId = folder->getId(state).get();

		if (folderId == id)
		{
			return folder;
		}
	}

	return nullptr;
}

std::future<std::shared_ptr<service::Object>> Query::getNode(const std::shared_ptr<service::RequestState>& state, std::vector<uint8_t>&& id) const
{
	std::promise<std::shared_ptr<service::Object>> promise;
	auto appointment = findAppointment(state, id);

	if (appointment)
	{
		promise.set_value(appointment);
		return promise.get_future();
	}

	auto task = findTask(state, id);

	if (task)
	{
		promise.set_value(task);
		return promise.get_future();
	}

	auto folder = findUnreadCount(state, id);

	if (folder)
	{
		promise.set_value(folder);
		return promise.get_future();
	}

	promise.set_value(nullptr);
	return promise.get_future();
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

	std::shared_ptr<_Connection> operator()(const int* first, const response::Value* after, const int* last, const response::Value* before) const
	{
		auto itrFirst = _objects.cbegin();
		auto itrLast = _objects.cend();

		if (after)
		{
			const auto& encoded = after->get<const response::StringType&>();
			auto afterId = service::Base64::fromBase64(encoded.c_str(), encoded.size());
			auto itrAfter = std::find_if(itrFirst, itrLast,
				[this, &afterId](const std::shared_ptr<_Object>& entry)
			{
				return entry->getId(_state).get() == afterId;
			});

			if (itrAfter != itrLast)
			{
				itrFirst = itrAfter;
			}
		}

		if (before)
		{
			const auto& encoded = before->get<const response::StringType&>();
			auto beforeId = service::Base64::fromBase64(encoded.c_str(), encoded.size());
			auto itrBefore = std::find_if(itrFirst, itrLast,
				[this, &beforeId](const std::shared_ptr<_Object>& entry)
			{
				return entry->getId(_state).get() == beforeId;
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
				throw service::schema_exception({ error.str() });
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
				throw service::schema_exception({ error.str() });
			}

			if (itrLast - itrFirst > *last)
			{
				itrFirst = itrLast - *last;
			}
		}

		std::vector<std::shared_ptr<_Object>> edges(itrLast - itrFirst);

		std::copy(itrFirst, itrLast, edges.begin());

		return std::make_shared<_Connection>(itrLast < _objects.cend(), itrFirst > _objects.cbegin(), std::move(edges));
	}

private:
	std::shared_ptr<service::RequestState> _state;
	const vec_type& _objects;
};

std::future<std::shared_ptr<object::AppointmentConnection>> Query::getAppointments(const std::shared_ptr<service::RequestState>& state, std::unique_ptr<int>&& first, std::unique_ptr<response::Value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<response::Value>&& before) const
{
	auto spThis = shared_from_this();
	return std::async(std::launch::async,
		[this, spThis](const std::shared_ptr<service::RequestState>& stateWrapped, std::unique_ptr<int>&& firstWrapped, std::unique_ptr<response::Value>&& afterWrapped, std::unique_ptr<int>&& lastWrapped, std::unique_ptr<response::Value>&& beforeWrapped)
	{
		loadAppointments(stateWrapped);

		EdgeConstraints<Appointment, AppointmentConnection> constraints(stateWrapped, _appointments);
		auto connection = constraints(firstWrapped.get(), afterWrapped.get(), lastWrapped.get(), beforeWrapped.get());

		return std::static_pointer_cast<object::AppointmentConnection>(connection);
	}, std::move(state), std::move(first), std::move(after), std::move(last), std::move(before));
}

std::future<std::shared_ptr<object::TaskConnection>> Query::getTasks(const std::shared_ptr<service::RequestState>& state, std::unique_ptr<int>&& first, std::unique_ptr<response::Value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<response::Value>&& before) const
{
	auto spThis = shared_from_this();
	return std::async(std::launch::async,
		[this, spThis](const std::shared_ptr<service::RequestState>& stateWrapped, std::unique_ptr<int>&& firstWrapped, std::unique_ptr<response::Value>&& afterWrapped, std::unique_ptr<int>&& lastWrapped, std::unique_ptr<response::Value>&& beforeWrapped)
	{
		loadTasks(stateWrapped);

		EdgeConstraints<Task, TaskConnection> constraints(stateWrapped, _tasks);
		auto connection = constraints(firstWrapped.get(), afterWrapped.get(), lastWrapped.get(), beforeWrapped.get());

		return std::static_pointer_cast<object::TaskConnection>(connection);
	}, std::move(state), std::move(first), std::move(after), std::move(last), std::move(before));
}

std::future<std::shared_ptr<object::FolderConnection>> Query::getUnreadCounts(const std::shared_ptr<service::RequestState>& state, std::unique_ptr<int>&& first, std::unique_ptr<response::Value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<response::Value>&& before) const
{
	auto spThis = shared_from_this();
	return std::async(std::launch::async,
		[this, spThis](const std::shared_ptr<service::RequestState>& stateWrapped, std::unique_ptr<int>&& firstWrapped, std::unique_ptr<response::Value>&& afterWrapped, std::unique_ptr<int>&& lastWrapped, std::unique_ptr<response::Value>&& beforeWrapped)
	{
		loadUnreadCounts(stateWrapped);

		EdgeConstraints<Folder, FolderConnection> constraints(stateWrapped, _unreadCounts);
		auto connection = constraints(firstWrapped.get(), afterWrapped.get(), lastWrapped.get(), beforeWrapped.get());

		return std::static_pointer_cast<object::FolderConnection>(connection);
	}, std::move(state), std::move(first), std::move(after), std::move(last), std::move(before));
}

std::future<std::vector<std::shared_ptr<object::Appointment>>> Query::getAppointmentsById(const std::shared_ptr<service::RequestState>& state, std::vector<std::vector<uint8_t>>&& ids) const
{
	std::promise<std::vector<std::shared_ptr<object::Appointment>>> promise;
	std::vector<std::shared_ptr<object::Appointment>> result(ids.size());

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this, &state](const std::vector<uint8_t>& id)
	{
		return std::static_pointer_cast<object::Appointment>(findAppointment(state, id));
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::Task>>> Query::getTasksById(const std::shared_ptr<service::RequestState>& state, std::vector<std::vector<uint8_t>>&& ids) const
{
	std::promise<std::vector<std::shared_ptr<object::Task>>> promise;
	std::vector<std::shared_ptr<object::Task>> result(ids.size());

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this, &state](const std::vector<uint8_t>& id)
	{
		return std::static_pointer_cast<object::Task>(findTask(state, id));
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::Folder>>> Query::getUnreadCountsById(const std::shared_ptr<service::RequestState>& state, std::vector<std::vector<uint8_t>>&& ids) const
{
	std::promise<std::vector<std::shared_ptr<object::Folder>>> promise;
	std::vector<std::shared_ptr<object::Folder>> result(ids.size());

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this, &state](const std::vector<uint8_t>& id)
	{
		return std::static_pointer_cast<object::Folder>(findUnreadCount(state, id));
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

Mutation::Mutation(completeTaskMutation&& mutateCompleteTask)
	: _mutateCompleteTask(std::move(mutateCompleteTask))
{
}

std::future<std::shared_ptr<object::CompleteTaskPayload>> Mutation::getCompleteTask(const std::shared_ptr<service::RequestState>& state, CompleteTaskInput&& input) const
{
	std::promise<std::shared_ptr<object::CompleteTaskPayload>> promise;

	promise.set_value(_mutateCompleteTask(std::move(input)));

	return promise.get_future();
}

} /* namespace today */
} /* namespace graphql */
} /* namespace facebook */