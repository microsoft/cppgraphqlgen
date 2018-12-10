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

	explicit Query(appointmentsLoader&& getAppointments, tasksLoader&& getTasks, unreadCountsLoader&& getUnreadCounts);

	std::future<std::shared_ptr<service::Object>> getNode(service::RequestId requestId, std::vector<uint8_t>&& id) const override;
	std::future<std::shared_ptr<object::AppointmentConnection>> getAppointments(service::RequestId requestId, std::unique_ptr<response::Value::IntType>&& first, std::unique_ptr<response::Value>&& after, std::unique_ptr<response::Value::IntType>&& last, std::unique_ptr<response::Value>&& before) const override;
	std::future<std::shared_ptr<object::TaskConnection>> getTasks(service::RequestId requestId, std::unique_ptr<response::Value::IntType>&& first, std::unique_ptr<response::Value>&& after, std::unique_ptr<response::Value::IntType>&& last, std::unique_ptr<response::Value>&& before) const override;
	std::future<std::shared_ptr<object::FolderConnection>> getUnreadCounts(service::RequestId requestId, std::unique_ptr<response::Value::IntType>&& first, std::unique_ptr<response::Value>&& after, std::unique_ptr<response::Value::IntType>&& last, std::unique_ptr<response::Value>&& before) const override;
	std::future<std::vector<std::shared_ptr<object::Appointment>>> getAppointmentsById(service::RequestId requestId, std::vector<std::vector<uint8_t>>&& ids) const override;
	std::future<std::vector<std::shared_ptr<object::Task>>> getTasksById(service::RequestId requestId, std::vector<std::vector<uint8_t>>&& ids) const override;
	std::future<std::vector<std::shared_ptr<object::Folder>>> getUnreadCountsById(service::RequestId requestId, std::vector<std::vector<uint8_t>>&& ids) const override;

private:
	std::shared_ptr<Appointment> findAppointment(service::RequestId requestId, const std::vector<uint8_t>& id) const;
	std::shared_ptr<Task> findTask(service::RequestId requestId, const std::vector<uint8_t>& id) const;
	std::shared_ptr<Folder> findUnreadCount(service::RequestId requestId, const std::vector<uint8_t>& id) const;

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

	std::future<bool> getHasNextPage(service::RequestId) const override
	{
		std::promise<bool> promise;

		promise.set_value(_hasNextPage);

		return promise.get_future();
	}

	std::future<bool> getHasPreviousPage(service::RequestId) const override
	{
		std::promise<bool> promise;

		promise.set_value(_hasPreviousPage);

		return promise.get_future();
	}

private:
	const bool _hasNextPage;
	const bool _hasPreviousPage;
};

class Appointment : public object::Appointment
{
public:
	explicit Appointment(std::vector<uint8_t>&& id, std::string&& when, std::string&& subject, bool isNow);

	std::future<std::vector<uint8_t>> getId(service::RequestId) const override
	{
		std::promise<std::vector<uint8_t>> promise;

		promise.set_value(_id);

		return promise.get_future();
	}

	std::future<std::unique_ptr<response::Value>> getWhen(service::RequestId) const override
	{
		std::promise<std::unique_ptr<response::Value>> promise;

		promise.set_value(std::unique_ptr<response::Value>(new response::Value(std::string(_when))));

		return promise.get_future();
	}

	std::future<std::unique_ptr<response::Value::StringType>> getSubject(service::RequestId) const override
	{
		std::promise<std::unique_ptr<response::Value::StringType>> promise;

		promise.set_value(std::unique_ptr<response::Value::StringType>(new std::string(_subject)));

		return promise.get_future();
	}

	std::future<bool> getIsNow(service::RequestId) const override
	{
		std::promise<bool> promise;

		promise.set_value(_isNow);

		return promise.get_future();
	}

private:
	std::vector<uint8_t> _id;
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

	std::future<std::shared_ptr<object::Appointment>> getNode(service::RequestId) const override
	{
		std::promise<std::shared_ptr<object::Appointment>> promise;

		promise.set_value(std::static_pointer_cast<object::Appointment>(_appointment));

		return promise.get_future();
	}

	std::future<response::Value> getCursor(service::RequestId requestId) const override
	{
		std::promise<response::Value> promise;

		promise.set_value(response::Value(service::Base64::toBase64(_appointment->getId(requestId).get())));

		return promise.get_future();
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

	std::future<std::shared_ptr<object::PageInfo>> getPageInfo(service::RequestId) const override
	{
		std::promise<std::shared_ptr<object::PageInfo>> promise;

		promise.set_value(_pageInfo);

		return promise.get_future();
	}

	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::AppointmentEdge>>>> getEdges(service::RequestId) const override
	{
		std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::AppointmentEdge>>>> promise;
		auto result = std::unique_ptr<std::vector<std::shared_ptr<object::AppointmentEdge>>>(new std::vector<std::shared_ptr<object::AppointmentEdge>>(_appointments.size()));

		std::transform(_appointments.cbegin(), _appointments.cend(), result->begin(),
			[](const std::shared_ptr<Appointment>& node)
		{
			return std::make_shared<AppointmentEdge>(node);
		});
		promise.set_value(std::move(result));

		return promise.get_future();
	}

private:
	std::shared_ptr<PageInfo> _pageInfo;
	std::vector<std::shared_ptr<Appointment>> _appointments;
};

class Task : public object::Task
{
public:
	explicit Task(std::vector<uint8_t>&& id, std::string&& title, bool isComplete);

	std::future<std::vector<uint8_t>> getId(service::RequestId) const override
	{
		std::promise<std::vector<uint8_t>> promise;

		promise.set_value(_id);

		return promise.get_future();
	}

	std::future<std::unique_ptr<response::Value::StringType>> getTitle(service::RequestId) const override
	{
		std::promise<std::unique_ptr<response::Value::StringType>> promise;

		promise.set_value(std::unique_ptr<response::Value::StringType>(new std::string(_title)));

		return promise.get_future();
	}

	std::future<bool> getIsComplete(service::RequestId) const override
	{
		std::promise<bool> promise;

		promise.set_value(_isComplete);

		return promise.get_future();
	}

private:
	std::vector<uint8_t> _id;
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

	std::future<std::shared_ptr<object::Task>> getNode(service::RequestId) const override
	{
		std::promise<std::shared_ptr<object::Task>> promise;

		promise.set_value(std::static_pointer_cast<object::Task>(_task));

		return promise.get_future();
	}

	std::future<response::Value> getCursor(service::RequestId requestId) const override
	{
		std::promise<response::Value> promise;

		promise.set_value(response::Value(service::Base64::toBase64(_task->getId(requestId).get())));

		return promise.get_future();
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

	std::future<std::shared_ptr<object::PageInfo>> getPageInfo(service::RequestId) const override
	{
		std::promise<std::shared_ptr<object::PageInfo>> promise;

		promise.set_value(_pageInfo);

		return promise.get_future();
	}

	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::TaskEdge>>>> getEdges(service::RequestId) const override
	{
		std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::TaskEdge>>>> promise;
		auto result = std::unique_ptr<std::vector<std::shared_ptr<object::TaskEdge>>>(new std::vector<std::shared_ptr<object::TaskEdge>>(_tasks.size()));

		std::transform(_tasks.cbegin(), _tasks.cend(), result->begin(),
			[](const std::shared_ptr<Task>& node)
		{
			return std::make_shared<TaskEdge>(node);
		});
		promise.set_value(std::move(result));

		return promise.get_future();
	}

private:
	std::shared_ptr<PageInfo> _pageInfo;
	std::vector<std::shared_ptr<Task>> _tasks;
};

class Folder : public object::Folder
{
public:
	explicit Folder(std::vector<uint8_t>&& id, std::string&& name, int unreadCount);

	std::future<std::vector<uint8_t>> getId(service::RequestId) const override
	{
		std::promise<std::vector<uint8_t>> promise;

		promise.set_value(_id);

		return promise.get_future();
	}

	std::future<std::unique_ptr<response::Value::StringType>> getName(service::RequestId) const override
	{
		std::promise<std::unique_ptr<response::Value::StringType>> promise;

		promise.set_value(std::unique_ptr<response::Value::StringType>(new std::string(_name)));

		return promise.get_future();
	}

	std::future<int> getUnreadCount(service::RequestId) const override
	{
		std::promise<int> promise;

		promise.set_value(_unreadCount);

		return promise.get_future();
	}

private:
	std::vector<uint8_t> _id;
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

	std::future<std::shared_ptr<object::Folder>> getNode(service::RequestId) const override
	{
		std::promise<std::shared_ptr<object::Folder>> promise;

		promise.set_value(std::static_pointer_cast<object::Folder>(_folder));

		return promise.get_future();
	}

	std::future<response::Value> getCursor(service::RequestId requestId) const override
	{
		std::promise<response::Value> promise;

		promise.set_value(response::Value(service::Base64::toBase64(_folder->getId(requestId).get())));

		return promise.get_future();
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

	std::future<std::shared_ptr<object::PageInfo>> getPageInfo(service::RequestId) const override
	{
		std::promise<std::shared_ptr<object::PageInfo>> promise;

		promise.set_value(_pageInfo);

		return promise.get_future();
	}

	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::FolderEdge>>>> getEdges(service::RequestId) const override
	{
		std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::FolderEdge>>>> promise;
		auto result = std::unique_ptr<std::vector<std::shared_ptr<object::FolderEdge>>>(new std::vector<std::shared_ptr<object::FolderEdge>>(_folders.size()));

		std::transform(_folders.cbegin(), _folders.cend(), result->begin(),
			[](const std::shared_ptr<Folder>& node)
		{
			return std::make_shared<FolderEdge>(node);
		});
		promise.set_value(std::move(result));

		return promise.get_future();
	}

private:
	std::shared_ptr<PageInfo> _pageInfo;
	std::vector<std::shared_ptr<Folder>> _folders;
};

class CompleteTaskPayload : public object::CompleteTaskPayload
{
public:
	explicit CompleteTaskPayload(std::shared_ptr<Task> task, std::unique_ptr<response::Value::StringType>&& clientMutationId)
		: _task(std::move(task))
		, _clientMutationId(std::move(clientMutationId))
	{
	}

	std::future<std::shared_ptr<object::Task>> getTask(service::RequestId) const override
	{
		std::promise<std::shared_ptr<object::Task>> promise;

		promise.set_value(std::static_pointer_cast<object::Task>(_task));

		return promise.get_future();
	}

	std::future<std::unique_ptr<response::Value::StringType>> getClientMutationId(service::RequestId) const override
	{
		std::promise<std::unique_ptr<response::Value::StringType>> promise;

		promise.set_value(std::unique_ptr<response::Value::StringType>(_clientMutationId
			? new std::string(*_clientMutationId)
			: nullptr));

		return promise.get_future();
	}

private:
	std::shared_ptr<Task> _task;
	std::unique_ptr<response::Value::StringType> _clientMutationId;
};

class Mutation : public object::Mutation
{
public:
	using completeTaskMutation = std::function<std::shared_ptr<CompleteTaskPayload>(CompleteTaskInput&&)>;

	explicit Mutation(completeTaskMutation&& mutateCompleteTask);

	std::future<std::shared_ptr<object::CompleteTaskPayload>> getCompleteTask(service::RequestId requestId, CompleteTaskInput&& input) const override;

private:
	completeTaskMutation _mutateCompleteTask;
};

class Subscription : public object::Subscription
{
public:
	explicit Subscription() = default;

	std::future<std::shared_ptr<object::Appointment>> getNextAppointmentChange(service::RequestId) const override
	{
		std::promise<std::shared_ptr<object::Appointment>> promise;

		promise.set_value(nullptr);

		return promise.get_future();
	}
};

} /* namespace today */
} /* namespace graphql */
} /* namespace facebook */