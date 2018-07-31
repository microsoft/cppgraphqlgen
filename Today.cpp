// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Today.h"

#include <iostream>
#include <algorithm>

namespace facebook {
namespace graphql {
namespace today {

Appointment::Appointment(std::vector<unsigned char> id, std::string when, std::string subject, bool isNow)
	: _id(std::move(id))
	, _when(std::move(when))
	, _subject(std::move(subject))
	, _isNow(isNow)
{
}

Task::Task(std::vector<unsigned char> id, std::string title, bool isComplete)
	: _id(std::move(id))
	, _title(std::move(title))
	, _isComplete(isComplete)
{
}

Folder::Folder(std::vector<unsigned char> id, std::string name, int unreadCount)
	: _id(std::move(id))
	, _name(std::move(name))
	, _unreadCount(unreadCount)
{
}

Query::Query(appointmentsLoader getAppointments, tasksLoader getTasks, unreadCountsLoader getUnreadCounts)
	: _getAppointments(std::move(getAppointments))
	, _getTasks(std::move(getTasks))
	, _getUnreadCounts(getUnreadCounts)
{
}

std::shared_ptr<service::Object> Query::getNode(std::vector<unsigned char> id) const
{
	if (_getAppointments)
	{
		_appointments = _getAppointments();
		_getAppointments = nullptr;
	}

	if (_getTasks)
	{
		_tasks = _getTasks();
		_getTasks = nullptr;
	}

	if (_getUnreadCounts)
	{
		_unreadCounts = _getUnreadCounts();
		_getUnreadCounts = nullptr;
	}

	for (const auto& appointment : _appointments)
	{
		auto appointmentId = appointment->getId();

		if (appointmentId == id)
		{
			return appointment;
		}
	}

	for (const auto& task : _tasks)
	{
		auto taskId = task->getId();

		if (taskId == id)
		{
			return task;
		}
	}

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

std::shared_ptr<object::AppointmentConnection> Query::getAppointments(std::unique_ptr<int> first, std::unique_ptr<web::json::value> after, std::unique_ptr<int> last, std::unique_ptr<web::json::value> before) const
{
	if (_getAppointments)
	{
		_appointments = _getAppointments();
		_getAppointments = nullptr;
	}

	auto itrFirst = _appointments.cbegin();
	auto itrLast = _appointments.cend();

	if (after)
	{
		auto value = web::json::value::object({
			{ _XPLATSTR("after"), *after }
		});

		auto afterId = service::IdArgument<>::require("after", value.as_object());
		auto itrAfter = std::find_if(itrFirst, itrLast,
			[&afterId](const std::shared_ptr<Appointment>& appointment)
		{
			return appointment->getId() == afterId;
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

		auto beforeId = service::IdArgument<>::require("before", value.as_object());
		auto itrBefore = std::find_if(itrFirst, itrLast,
			[&beforeId](const std::shared_ptr<Appointment>& appointment)
		{
			return appointment->getId() == beforeId;
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

	std::vector<std::shared_ptr<Appointment>> edges(itrLast - itrFirst);

	std::copy(itrFirst, itrLast, edges.begin());

	return std::static_pointer_cast<object::AppointmentConnection>(std::make_shared<AppointmentConnection>(itrLast < _appointments.cend(), itrFirst > _appointments.cbegin(), std::move(edges)));
}

std::shared_ptr<object::TaskConnection> Query::getTasks(std::unique_ptr<int> first, std::unique_ptr<web::json::value> after, std::unique_ptr<int> last, std::unique_ptr<web::json::value> before) const
{
	if (_getTasks)
	{
		_tasks = _getTasks();
		_getTasks = nullptr;
	}

	auto itrFirst = _tasks.cbegin();
	auto itrLast = _tasks.cend();

	if (after)
	{
		auto value = web::json::value::object({
			{ _XPLATSTR("after"), *after }
		});

		auto afterId = service::IdArgument<>::require("after", value.as_object());
		auto itrAfter = std::find_if(itrFirst, itrLast,
			[&afterId](const std::shared_ptr<Task>& task)
		{
			return task->getId() == afterId;
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

		auto beforeId = service::IdArgument<>::require("before", value.as_object());
		auto itrBefore = std::find_if(itrFirst, itrLast,
			[&beforeId](const std::shared_ptr<Task>& task)
		{
			return task->getId() == beforeId;
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

	std::vector<std::shared_ptr<Task>> edges(itrLast - itrFirst);

	std::copy(itrFirst, itrLast, edges.begin());

	return std::static_pointer_cast<object::TaskConnection>(std::make_shared<TaskConnection>(itrLast < _tasks.cend(), itrFirst > _tasks.cbegin(), std::move(edges)));
}

std::shared_ptr<object::FolderConnection> Query::getUnreadCounts(std::unique_ptr<int> first, std::unique_ptr<web::json::value> after, std::unique_ptr<int> last, std::unique_ptr<web::json::value> before) const
{
	if (_getUnreadCounts)
	{
		_unreadCounts = _getUnreadCounts();
		_getUnreadCounts = nullptr;
	}

	auto itrFirst = _unreadCounts.cbegin();
	auto itrLast = _unreadCounts.cend();

	if (after)
	{
		auto value = web::json::value::object({
			{ _XPLATSTR("after"), *after }
		});

		auto afterId = service::IdArgument<>::require("after", value.as_object());
		auto itrAfter = std::find_if(itrFirst, itrLast,
			[&afterId](const std::shared_ptr<Folder>& unreadCount)
		{
			return unreadCount->getId() == afterId;
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

		auto beforeId = service::IdArgument<>::require("before", value.as_object());
		auto itrBefore = std::find_if(itrFirst, itrLast,
			[&beforeId](const std::shared_ptr<Folder>& unreadCount)
		{
			return unreadCount->getId() == beforeId;
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

	std::vector<std::shared_ptr<Folder>> edges(itrLast - itrFirst);

	std::copy(itrFirst, itrLast, edges.begin());

	return std::static_pointer_cast<object::FolderConnection>(std::make_shared<FolderConnection>(itrLast < _unreadCounts.cend(), itrFirst > _unreadCounts.cbegin(), std::move(edges)));
}

std::vector<std::shared_ptr<object::Appointment>> Query::getAppointmentsById(std::vector<std::vector<unsigned char>> ids) const
{
	std::vector<std::shared_ptr<object::Appointment>> result(ids.size());

	if (_getAppointments)
	{
		_appointments = _getAppointments();
		_getAppointments = nullptr;
	}

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this](const std::vector<unsigned char>& id)
	{
		for (const auto& appointment : _appointments)
		{
			auto appointmentId = appointment->getId();

			if (appointmentId == id)
			{
				return std::static_pointer_cast<object::Appointment>(appointment);
			}
		}

		return std::shared_ptr<object::Appointment>(nullptr);
	});

	return result;
}

std::vector<std::shared_ptr<object::Task>> Query::getTasksById(std::vector<std::vector<unsigned char>> ids) const
{
	std::vector<std::shared_ptr<object::Task>> result(ids.size());

	if (_getTasks)
	{
		_tasks = _getTasks();
		_getTasks = nullptr;
	}

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this](const std::vector<unsigned char>& id)
	{
		for (const auto& task : _tasks)
		{
			auto taskId = task->getId();

			if (taskId == id)
			{
				return std::static_pointer_cast<object::Task>(task);
			}
		}

		return std::shared_ptr<object::Task>(nullptr);
	});

	return result;
}

std::vector<std::shared_ptr<object::Folder>> Query::getUnreadCountsById(std::vector<std::vector<unsigned char>> ids) const
{
	std::vector<std::shared_ptr<object::Folder>> result(ids.size());

	if (_getUnreadCounts)
	{
		_unreadCounts = _getUnreadCounts();
		_getUnreadCounts = nullptr;
	}

	std::transform(ids.cbegin(), ids.cend(), result.begin(),
		[this](const std::vector<unsigned char>& id)
	{
		for (const auto& folder : _unreadCounts)
		{
			auto folderId = folder->getId();

			if (folderId == id)
			{
				return std::static_pointer_cast<object::Folder>(folder);
			}
		}

		return std::shared_ptr<object::Folder>(nullptr);
	});

	return result;
}

Mutation::Mutation(completeTaskMutation mutateCompleteTask)
	: _mutateCompleteTask(std::move(mutateCompleteTask))
{
}

std::shared_ptr<object::CompleteTaskPayload> Mutation::getCompleteTask(CompleteTaskInput input) const
{
	return _mutateCompleteTask(std::move(input));
}

} /* namespace today */
} /* namespace graphql */
} /* namespace facebook */