// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Today.h"

#include <iostream>
#include <algorithm>

namespace facebook {
namespace graphql {
namespace today {

Appointment::Appointment(std::vector<unsigned char>&& id, std::string&& when, std::string&& subject, bool isNow)
	: _id(std::move(id))
	, _when(std::move(when))
	, _subject(std::move(subject))
	, _isNow(isNow)
{
}

Task::Task(std::vector<unsigned char>&& id, std::string&& title, bool isComplete)
	: _id(std::move(id))
	, _title(std::move(title))
	, _isComplete(isComplete)
{
}

Folder::Folder(std::vector<unsigned char>&& id, std::string&& name, int unreadCount)
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

std::shared_ptr<Appointment> Query::findAppointment(const std::vector<unsigned char>& id) const
{
	loadAppointments();

	for (const auto& appointment : _appointments)
	{
		auto appointmentId = appointment->getId();

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

std::shared_ptr<Task> Query::findTask(const std::vector<unsigned char>& id) const
{
	loadTasks();

	for (const auto& task : _tasks)
	{
		auto taskId = task->getId();

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

std::shared_ptr<Folder> Query::findUnreadCount(const std::vector<unsigned char>& id) const
{
	loadUnreadCounts();

	for (const auto& folder : _unreadCounts)
	{
		auto folderId = folder->getId();

		if (folderId == id)
		{
			return folder;
		}
	}

	return nullptr;
}

std::shared_ptr<service::Object> Query::getNode(std::vector<unsigned char>&& id) const
{
	auto appointment = findAppointment(id);

	if (appointment)
	{
		return appointment;
	}

	auto task = findTask(id);

	if (task)
	{
		return task;
	}

	auto folder = findUnreadCount(id);

	if (folder)
	{
		return folder;
	}

	return nullptr;
}

template <class _Object, class _Connection>
struct EdgeConstraints
{
	using vec_type = std::vector<std::shared_ptr<_Object>>;
	using itr_type = typename vec_type::const_iterator;

	EdgeConstraints(const vec_type& objects)
		: _objects(objects)
	{
	}

	std::shared_ptr<_Connection> operator()(const int* first, const web::json::value* after, const int* last, const web::json::value* before) const
	{
		auto itrFirst = _objects.cbegin();
		auto itrLast = _objects.cend();

		if (after)
		{
			auto value = web::json::value::object({
				{ _XPLATSTR("after"), *after }
				});

			auto afterId = service::IdArgument::require("after", value.as_object());
			auto itrAfter = std::find_if(itrFirst, itrLast,
				[&afterId](const std::shared_ptr<_Object>& entry)
			{
				return entry->getId() == afterId;
			});

			if (itrAfter != itrLast)
			{
				itrFirst = itrAfter;
			}
		}

		if (before)
		{
			auto value = web::json::value::object({
				{ _XPLATSTR("before"), *before }
				});

			auto beforeId = service::IdArgument::require("before", value.as_object());
			auto itrBefore = std::find_if(itrFirst, itrLast,
				[&beforeId](const std::shared_ptr<_Object>& entry)
			{
				return entry->getId() == beforeId;
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
	const vec_type& _objects;
};

std::shared_ptr<object::AppointmentConnection> Query::getAppointments(std::unique_ptr<int>&& first, std::unique_ptr<web::json::value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<web::json::value>&& before) const
{
	loadAppointments();

	EdgeConstraints<Appointment, AppointmentConnection> constraints(_appointments);
	auto connection = constraints(first.get(), after.get(), last.get(), before.get());

	return std::static_pointer_cast<object::AppointmentConnection>(connection);
}

std::shared_ptr<object::TaskConnection> Query::getTasks(std::unique_ptr<int>&& first, std::unique_ptr<web::json::value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<web::json::value>&& before) const
{
	loadTasks();

	EdgeConstraints<Task, TaskConnection> constraints(_tasks);
	auto connection = constraints(first.get(), after.get(), last.get(), before.get());

	return std::static_pointer_cast<object::TaskConnection>(connection);
}

std::shared_ptr<object::FolderConnection> Query::getUnreadCounts(std::unique_ptr<int>&& first, std::unique_ptr<web::json::value>&& after, std::unique_ptr<int>&& last, std::unique_ptr<web::json::value>&& before) const
{
	loadUnreadCounts();

	EdgeConstraints<Folder, FolderConnection> constraints(_unreadCounts);
	auto connection = constraints(first.get(), after.get(), last.get(), before.get());

	return std::static_pointer_cast<object::FolderConnection>(connection);
}

std::vector<std::shared_ptr<object::Appointment>> Query::getAppointmentsById(std::vector<std::vector<unsigned char>>&& ids) const
{
	std::vector<std::shared_ptr<object::Appointment>> result(ids.size());

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this](const std::vector<unsigned char>& id)
	{
		return std::static_pointer_cast<object::Appointment>(findAppointment(id));
	});

	return result;
}

std::vector<std::shared_ptr<object::Task>> Query::getTasksById(std::vector<std::vector<unsigned char>>&& ids) const
{
	std::vector<std::shared_ptr<object::Task>> result(ids.size());

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this](const std::vector<unsigned char>& id)
	{
		return std::static_pointer_cast<object::Task>(findTask(id));
	});

	return result;
}

std::vector<std::shared_ptr<object::Folder>> Query::getUnreadCountsById(std::vector<std::vector<unsigned char>>&& ids) const
{
	std::vector<std::shared_ptr<object::Folder>> result(ids.size());

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this](const std::vector<unsigned char>& id)
	{
		return std::static_pointer_cast<object::Folder>(findUnreadCount(id));
	});

	return result;
}

Mutation::Mutation(completeTaskMutation&& mutateCompleteTask)
	: _mutateCompleteTask(std::move(mutateCompleteTask))
{
}

std::shared_ptr<object::CompleteTaskPayload> Mutation::getCompleteTask(CompleteTaskInput&& input) const
{
	return _mutateCompleteTask(std::move(input));
}

} /* namespace today */
} /* namespace graphql */
} /* namespace facebook */