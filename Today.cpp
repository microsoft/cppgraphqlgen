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

void Query::loadAppointments() const
{
	if (_getAppointments)
	{
		_appointments = _getAppointments();
		_getAppointments = nullptr;
	}
}

std::shared_ptr<Appointment> Query::findAppointment(service::RequestId requestId, const std::vector<uint8_t>& id) const
{
	loadAppointments();

	for (const auto& appointment : _appointments)
	{
		auto appointmentId = appointment->getId(requestId).get();

		if (appointmentId == id)
		{
			return appointment;
		}
	}

	return nullptr;
}

void Query::loadTasks() const
{
	if (_getTasks)
	{
		_tasks = _getTasks();
		_getTasks = nullptr;
	}
}

std::shared_ptr<Task> Query::findTask(service::RequestId requestId, const std::vector<uint8_t>& id) const
{
	loadTasks();

	for (const auto& task : _tasks)
	{
		auto taskId = task->getId(requestId).get();

		if (taskId == id)
		{
			return task;
		}
	}

	return nullptr;
}

void Query::loadUnreadCounts() const
{
	if (_getUnreadCounts)
	{
		_unreadCounts = _getUnreadCounts();
		_getUnreadCounts = nullptr;
	}
}

std::shared_ptr<Folder> Query::findUnreadCount(service::RequestId requestId, const std::vector<uint8_t>& id) const
{
	loadUnreadCounts();

	for (const auto& folder : _unreadCounts)
	{
		auto folderId = folder->getId(requestId).get();

		if (folderId == id)
		{
			return folder;
		}
	}

	return nullptr;
}

std::future<std::shared_ptr<service::Object>> Query::getNode(service::RequestId requestId, std::vector<uint8_t>&& id) const
{
	std::promise<std::shared_ptr<service::Object>> promise;
	auto appointment = findAppointment(requestId, id);

	if (appointment)
	{
		promise.set_value(appointment);
		return promise.get_future();
	}

	auto task = findTask(requestId, id);

	if (task)
	{
		promise.set_value(task);
		return promise.get_future();
	}

	auto folder = findUnreadCount(requestId, id);

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

	EdgeConstraints(service::RequestId requestId, const vec_type& objects)
		: _requestId(requestId)
		, _objects(objects)
	{
	}

	std::shared_ptr<_Connection> operator()(const int* first, const rapidjson::Value* after, const int* last, const rapidjson::Value* before) const
	{
		auto itrFirst = _objects.cbegin();
		auto itrLast = _objects.cend();

		if (after)
		{
			auto afterId = service::Base64::fromBase64(after->GetString(), after->GetStringLength());
			auto itrAfter = std::find_if(itrFirst, itrLast,
				[this, &afterId](const std::shared_ptr<_Object>& entry)
			{
				return entry->getId(_requestId).get() == afterId;
			});

			if (itrAfter != itrLast)
			{
				itrFirst = itrAfter;
			}
		}

		if (before)
		{
			auto beforeId = service::Base64::fromBase64(before->GetString(), before->GetStringLength());
			auto itrBefore = std::find_if(itrFirst, itrLast,
				[this, &beforeId](const std::shared_ptr<_Object>& entry)
			{
				return entry->getId(_requestId).get() == beforeId;
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
	const service::RequestId _requestId;
	const vec_type& _objects;
};

std::future<std::shared_ptr<object::AppointmentConnection>> Query::getAppointments(service::RequestId requestId, std::unique_ptr<int>&& first, std::unique_ptr<rapidjson::Value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<rapidjson::Value>&& before) const
{
	loadAppointments();

	std::promise<std::shared_ptr<object::AppointmentConnection>> promise;
	EdgeConstraints<Appointment, AppointmentConnection> constraints(requestId, _appointments);
	auto connection = constraints(first.get(), after.get(), last.get(), before.get());

	promise.set_value(std::static_pointer_cast<object::AppointmentConnection>(connection));

	return promise.get_future();
}

std::future<std::shared_ptr<object::TaskConnection>> Query::getTasks(service::RequestId requestId, std::unique_ptr<int>&& first, std::unique_ptr<rapidjson::Value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<rapidjson::Value>&& before) const
{
	loadTasks();

	std::promise<std::shared_ptr<object::TaskConnection>> promise;
	EdgeConstraints<Task, TaskConnection> constraints(requestId, _tasks);
	auto connection = constraints(first.get(), after.get(), last.get(), before.get());

	promise.set_value(std::static_pointer_cast<object::TaskConnection>(connection));

	return promise.get_future();
}

std::future<std::shared_ptr<object::FolderConnection>> Query::getUnreadCounts(service::RequestId requestId, std::unique_ptr<int>&& first, std::unique_ptr<rapidjson::Value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<rapidjson::Value>&& before) const
{
	loadUnreadCounts();

	std::promise<std::shared_ptr<object::FolderConnection>> promise;
	EdgeConstraints<Folder, FolderConnection> constraints(requestId, _unreadCounts);
	auto connection = constraints(first.get(), after.get(), last.get(), before.get());

	promise.set_value(std::static_pointer_cast<object::FolderConnection>(connection));

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::Appointment>>> Query::getAppointmentsById(service::RequestId requestId, std::vector<std::vector<uint8_t>>&& ids) const
{
	std::promise<std::vector<std::shared_ptr<object::Appointment>>> promise;
	std::vector<std::shared_ptr<object::Appointment>> result(ids.size());

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this, requestId](const std::vector<uint8_t>& id)
	{
		return std::static_pointer_cast<object::Appointment>(findAppointment(requestId, id));
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::Task>>> Query::getTasksById(service::RequestId requestId, std::vector<std::vector<uint8_t>>&& ids) const
{
	std::promise<std::vector<std::shared_ptr<object::Task>>> promise;
	std::vector<std::shared_ptr<object::Task>> result(ids.size());

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this, requestId](const std::vector<uint8_t>& id)
	{
		return std::static_pointer_cast<object::Task>(findTask(requestId, id));
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::Folder>>> Query::getUnreadCountsById(service::RequestId requestId, std::vector<std::vector<uint8_t>>&& ids) const
{
	std::promise<std::vector<std::shared_ptr<object::Folder>>> promise;
	std::vector<std::shared_ptr<object::Folder>> result(ids.size());

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this, requestId](const std::vector<uint8_t>& id)
	{
		return std::static_pointer_cast<object::Folder>(findUnreadCount(requestId, id));
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

Mutation::Mutation(completeTaskMutation&& mutateCompleteTask)
	: _mutateCompleteTask(std::move(mutateCompleteTask))
{
}

std::future<std::shared_ptr<object::CompleteTaskPayload>> Mutation::getCompleteTask(service::RequestId requestId, CompleteTaskInput&& input) const
{
	std::promise<std::shared_ptr<object::CompleteTaskPayload>> promise;

	promise.set_value(_mutateCompleteTask(std::move(input)));

	return promise.get_future();
}

} /* namespace today */
} /* namespace graphql */
} /* namespace facebook */