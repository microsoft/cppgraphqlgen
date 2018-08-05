// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

namespace facebook {
namespace graphql {
namespace today {

class Appointment;
class Task;
class Folder;

class Query : public object::Query
{
public:
	using appointmentsLoader = std::function<std::vector<std::shared_ptr<Appointment>>()>;
	using tasksLoader = std::function<std::vector<std::shared_ptr<Task>>()>;
	using unreadCountsLoader = std::function<std::vector<std::shared_ptr<Folder>>()>;

	explicit Query(appointmentsLoader getAppointments, tasksLoader getTasks, unreadCountsLoader getUnreadCounts);

	std::shared_ptr<service::Object> getNode(std::vector<unsigned char> id) const override;
	std::shared_ptr<object::AppointmentConnection> getAppointments(std::unique_ptr<int> first, std::unique_ptr<web::json::value> after, std::unique_ptr<int> last, std::unique_ptr<web::json::value> before) const override;
	std::shared_ptr<object::TaskConnection> getTasks(std::unique_ptr<int> first, std::unique_ptr<web::json::value> after, std::unique_ptr<int> last, std::unique_ptr<web::json::value> before) const override;
	std::shared_ptr<object::FolderConnection> getUnreadCounts(std::unique_ptr<int> first, std::unique_ptr<web::json::value> after, std::unique_ptr<int> last, std::unique_ptr<web::json::value> before) const override;
	std::vector<std::shared_ptr<object::Appointment>> getAppointmentsById(std::vector<std::vector<unsigned char>> ids) const override;
	std::vector<std::shared_ptr<object::Task>> getTasksById(std::vector<std::vector<unsigned char>> ids) const override;
	std::vector<std::shared_ptr<object::Folder>> getUnreadCountsById(std::vector<std::vector<unsigned char>> ids) const override;

private:
	std::shared_ptr<Appointment> findAppointment(const std::vector<unsigned char>& id) const;
	std::shared_ptr<Task> findTask(const std::vector<unsigned char>& id) const;
	std::shared_ptr<Folder> findUnreadCount(const std::vector<unsigned char>& id) const;

	// Lazy load the fields in each query
	void loadAppointments() const;
	void loadTasks() const;
	void loadUnreadCounts() const;

	mutable appointmentsLoader _getAppointments;
	mutable tasksLoader _getTasks;
	mutable unreadCountsLoader _getUnreadCounts;

	mutable std::vector<std::shared_ptr<Appointment>> _appointments;
	mutable std::vector<std::shared_ptr<Task>> _tasks;
	mutable std::vector<std::shared_ptr<Folder>> _unreadCounts;
};

class PageInfo : public object::PageInfo
{
public:
	explicit PageInfo(bool hasNextPage, bool hasPreviousPage)
		: _hasNextPage(hasNextPage)
		, _hasPreviousPage(hasPreviousPage)
	{
	}

	bool getHasNextPage() const override
	{
		return _hasNextPage;
	}

	bool getHasPreviousPage() const override
	{
		return _hasPreviousPage;
	}

private:
	const bool _hasNextPage;
	const bool _hasPreviousPage;
};

class Appointment : public object::Appointment
{
public:
	explicit Appointment(std::vector<unsigned char> id, std::string when, std::string subject, bool isNow);

	std::vector<unsigned char> getId() const override { return _id; }
	std::unique_ptr<web::json::value> getWhen() const override{ return std::unique_ptr<web::json::value>(new web::json::value(web::json::value::string(utility::conversions::to_string_t(_when)))); }
	std::unique_ptr<std::string> getSubject() const override { return std::unique_ptr<std::string>(new std::string(_subject)); }
	bool getIsNow() const override { return _isNow; }

private:
	std::vector<unsigned char> _id;
	std::string _when;
	std::string _subject;
	bool _isNow;
};

class AppointmentEdge : public object::AppointmentEdge
{
public:
	explicit AppointmentEdge(std::shared_ptr<Appointment> appointment)
		: _appointment(std::move(appointment))
	{
	}

	std::shared_ptr<object::Appointment> getNode() const override
	{
		return std::static_pointer_cast<object::Appointment>(_appointment);
	}

	web::json::value getCursor() const override
	{
		return web::json::value::string(utility::conversions::to_base64(_appointment->getId()));
	}

private:
	std::shared_ptr<Appointment> _appointment;
};

class AppointmentConnection : public object::AppointmentConnection
{
public:
	explicit AppointmentConnection(bool hasNextPage, bool hasPreviousPage, std::vector<std::shared_ptr<Appointment>> appointments)
		: _pageInfo(std::make_shared<PageInfo>(hasNextPage, hasPreviousPage))
		, _appointments(std::move(appointments))
	{
	}

	std::shared_ptr<object::PageInfo> getPageInfo() const override
	{
		return _pageInfo;
	}

	std::unique_ptr<std::vector<std::shared_ptr<object::AppointmentEdge>>> getEdges() const override
	{
		auto result = std::unique_ptr<std::vector<std::shared_ptr<object::AppointmentEdge>>>(new std::vector<std::shared_ptr<object::AppointmentEdge>>(_appointments.size()));

		std::transform(_appointments.cbegin(), _appointments.cend(), result->begin(),
			[](const std::shared_ptr<Appointment>& node)
		{
			return std::make_shared<AppointmentEdge>(node);
		});

		return result;
	}

private:
	std::shared_ptr<PageInfo> _pageInfo;
	std::vector<std::shared_ptr<Appointment>> _appointments;
};

class Task : public object::Task
{
public:
	explicit Task(std::vector<unsigned char> id, std::string title, bool isComplete);

	std::vector<unsigned char> getId() const override { return _id; }
	std::unique_ptr<std::string> getTitle() const override { return std::unique_ptr<std::string>(new std::string(_title)); }
	bool getIsComplete() const override { return _isComplete; }

private:
	std::vector<unsigned char> _id;
	std::string _title;
	bool _isComplete;
	TaskState _state = TaskState::New;
};

class TaskEdge : public object::TaskEdge
{
public:
	explicit TaskEdge(std::shared_ptr<Task> task)
		: _task(std::move(task))
	{
	}

	std::shared_ptr<object::Task> getNode() const override
	{
		return std::static_pointer_cast<object::Task>(_task);
	}

	web::json::value getCursor() const override
	{
		return web::json::value::string(utility::conversions::to_base64(_task->getId()));
	}

private:
	std::shared_ptr<Task> _task;
};

class TaskConnection : public object::TaskConnection
{
public:
	explicit TaskConnection(bool hasNextPage, bool hasPreviousPage, std::vector<std::shared_ptr<Task>> tasks)
		: _pageInfo(std::make_shared<PageInfo>(hasNextPage, hasPreviousPage))
		, _tasks(std::move(tasks))
	{
	}

	std::shared_ptr<object::PageInfo> getPageInfo() const override
	{
		return _pageInfo;
	}

	std::unique_ptr<std::vector<std::shared_ptr<object::TaskEdge>>> getEdges() const override
	{
		auto result = std::unique_ptr<std::vector<std::shared_ptr<object::TaskEdge>>>(new std::vector<std::shared_ptr<object::TaskEdge>>(_tasks.size()));

		std::transform(_tasks.cbegin(), _tasks.cend(), result->begin(),
			[](const std::shared_ptr<Task>& node)
		{
			return std::make_shared<TaskEdge>(node);
		});

		return result;
	}

private:
	std::shared_ptr<PageInfo> _pageInfo;
	std::vector<std::shared_ptr<Task>> _tasks;
};

class Folder : public object::Folder
{
public:
	explicit Folder(std::vector<unsigned char> id, std::string name, int unreadCount);

	std::vector<unsigned char> getId() const override { return _id; }
	std::unique_ptr<std::string> getName() const override { return std::unique_ptr<std::string>(new std::string(_name)); }
	int getUnreadCount() const override { return _unreadCount; }

private:
	std::vector<unsigned char> _id;
	std::string _name;
	int _unreadCount;
};

class FolderEdge : public object::FolderEdge
{
public:
	explicit FolderEdge(std::shared_ptr<Folder> folder)
		: _folder(std::move(folder))
	{
	}

	std::shared_ptr<object::Folder> getNode() const override
	{
		return std::static_pointer_cast<object::Folder>(_folder);
	}

	web::json::value getCursor() const override
	{
		return web::json::value::string(utility::conversions::to_base64(_folder->getId()));
	}

private:
	std::shared_ptr<Folder> _folder;
};

class FolderConnection : public object::FolderConnection
{
public:
	explicit FolderConnection(bool hasNextPage, bool hasPreviousPage, std::vector<std::shared_ptr<Folder>> folders)
		: _pageInfo(std::make_shared<PageInfo>(hasNextPage, hasPreviousPage))
		, _folders(std::move(folders))
	{
	}

	std::shared_ptr<object::PageInfo> getPageInfo() const override
	{
		return _pageInfo;
	}

	std::unique_ptr<std::vector<std::shared_ptr<object::FolderEdge>>> getEdges() const override
	{
		auto result = std::unique_ptr<std::vector<std::shared_ptr<object::FolderEdge>>>(new std::vector<std::shared_ptr<object::FolderEdge>>(_folders.size()));

		std::transform(_folders.cbegin(), _folders.cend(), result->begin(),
			[](const std::shared_ptr<Folder>& node)
		{
			return std::make_shared<FolderEdge>(node);
		});

		return result;
	}

private:
	std::shared_ptr<PageInfo> _pageInfo;
	std::vector<std::shared_ptr<Folder>> _folders;
};

class CompleteTaskPayload : public object::CompleteTaskPayload
{
public:
	explicit CompleteTaskPayload(std::shared_ptr<Task> task, std::unique_ptr<std::string> clientMutationId)
		: _task(std::move(task))
		, _clientMutationId(std::move(clientMutationId))
	{
	}

	std::shared_ptr<object::Task> getTask() const override
	{
		return std::static_pointer_cast<object::Task>(_task);
	}

	std::unique_ptr<std::string> getClientMutationId() const override
	{
		return std::unique_ptr<std::string>(_clientMutationId
			? new std::string(*_clientMutationId)
			: nullptr);
	}

private:
	std::shared_ptr<Task> _task;
	std::unique_ptr<std::string> _clientMutationId;
};

class Mutation : public object::Mutation
{
public:
	using completeTaskMutation = std::function<std::shared_ptr<CompleteTaskPayload>(CompleteTaskInput)>;

	explicit Mutation(completeTaskMutation mutateCompleteTask);

	std::shared_ptr<object::CompleteTaskPayload> getCompleteTask(CompleteTaskInput input) const override;

private:
	completeTaskMutation _mutateCompleteTask;
};

class Subscription : public object::Subscription
{
public:
	explicit Subscription() = default;

	std::shared_ptr<object::Appointment> getNextAppointmentChange() const override
	{
		return nullptr;
	}
};

} /* namespace today */
} /* namespace graphql */
} /* namespace facebook */