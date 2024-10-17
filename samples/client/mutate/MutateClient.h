// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef MUTATECLIENT_H
#define MUTATECLIENT_H

#include "graphqlservice/GraphQLClient.h"
#include "graphqlservice/GraphQLParse.h"
#include "graphqlservice/GraphQLResponse.h"

#include "graphqlservice/internal/Version.h"

#include <optional>
#include <string>
#include <vector>

// Check if the library version is compatible with clientgen 5.0.0
static_assert(graphql::internal::MajorVersion == 5, "regenerate with clientgen: major version mismatch");
static_assert(graphql::internal::MinorVersion == 0, "regenerate with clientgen: minor version mismatch");

namespace graphql::mutate {

/// # Operation: mutation CompleteTaskMutation
/// ```graphql
/// # Copyright (c) Microsoft Corporation. All rights reserved.
/// # Licensed under the MIT License.
/// 
/// mutation CompleteTaskMutation($input: CompleteTaskInput = {id: "ZmFrZVRhc2tJZA==", isComplete: true, clientMutationId: "Hi There!", testTaskState: Unassigned}, $skipClientMutationId: Boolean!) {
///   completedTask: completeTask(input: $input) {
///     completedTask: task {
///       completedTaskId: id
///       title
///       isComplete
///     }
///     clientMutationId @skip(if: $skipClientMutationId)
///   }
/// }
/// ```
namespace client {

// Return the original text of the request document.
[[nodiscard("unnecessary call")]] const std::string& GetRequestText() noexcept;

// Return a pre-parsed, pre-validated request object.
[[nodiscard("unnecessary call")]] const peg::ast& GetRequestObject() noexcept;

} // namespace client

enum class [[nodiscard("unnecessary conversion")]] TaskState
{
	Unassigned,
	New,
	Started,
	Complete,
};

struct [[nodiscard("unnecessary construction")]] CompleteTaskInput
{
	explicit CompleteTaskInput() noexcept;
	explicit CompleteTaskInput(
		response::IdType idArg,
		std::optional<TaskState> testTaskStateArg,
		std::optional<bool> isCompleteArg,
		std::optional<std::string> clientMutationIdArg) noexcept;
	CompleteTaskInput(const CompleteTaskInput& other);
	CompleteTaskInput(CompleteTaskInput&& other) noexcept;
	~CompleteTaskInput();

	CompleteTaskInput& operator=(const CompleteTaskInput& other);
	CompleteTaskInput& operator=(CompleteTaskInput&& other) noexcept;

	response::IdType id;
	std::optional<TaskState> testTaskState;
	std::optional<bool> isComplete;
	std::optional<std::string> clientMutationId;
};

namespace client {

namespace mutation::CompleteTaskMutation {

using graphql::mutate::client::GetRequestText;
using graphql::mutate::client::GetRequestObject;

// Return the name of this operation in the shared request document.
[[nodiscard("unnecessary call")]] const std::string& GetOperationName() noexcept;

using graphql::mutate::TaskState;

using graphql::mutate::CompleteTaskInput;

struct [[nodiscard("unnecessary construction")]] Variables
{
	std::unique_ptr<CompleteTaskInput> input {};
	bool skipClientMutationId {};
};

[[nodiscard("unnecessary conversion")]] response::Value serializeVariables(Variables&& variables);

struct [[nodiscard("unnecessary construction")]] Response
{
	struct [[nodiscard("unnecessary construction")]] completedTask_CompleteTaskPayload
	{
		struct [[nodiscard("unnecessary construction")]] completedTask_Task
		{
			response::IdType completedTaskId {};
			std::optional<std::string> title {};
			bool isComplete {};
		};

		std::optional<completedTask_Task> completedTask {};
		std::optional<std::string> clientMutationId {};
	};

	completedTask_CompleteTaskPayload completedTask {};
};

class ResponseVisitor
	: public std::enable_shared_from_this<ResponseVisitor>
{
public:
	ResponseVisitor() noexcept;
	~ResponseVisitor();

	void add_value(std::shared_ptr<const response::Value>&&);
	void reserve(std::size_t count);
	void start_object();
	void add_member(std::string&& key);
	void end_object();
	void start_array();
	void end_array();
	void add_null();
	void add_string(std::string&& value);
	void add_enum(std::string&& value);
	void add_id(response::IdType&& value);
	void add_bool(bool value);
	void add_int(int value);
	void add_float(double value);
	void complete();

	Response response();

private:
	struct impl;

	std::unique_ptr<impl> _pimpl;
};

[[nodiscard("unnecessary conversion")]] Response parseResponse(response::Value&& response);

struct Traits
{
	[[nodiscard("unnecessary call")]] static const std::string& GetRequestText() noexcept;
	[[nodiscard("unnecessary call")]] static const peg::ast& GetRequestObject() noexcept;
	[[nodiscard("unnecessary call")]] static const std::string& GetOperationName() noexcept;

	using Variables = CompleteTaskMutation::Variables;

	[[nodiscard("unnecessary conversion")]] static response::Value serializeVariables(Variables&& variables);

	using Response = CompleteTaskMutation::Response;
	using ResponseVisitor = CompleteTaskMutation::ResponseVisitor;

	[[nodiscard("unnecessary conversion")]] static Response parseResponse(response::Value&& response);
};

} // namespace mutation::CompleteTaskMutation
} // namespace client
} // namespace graphql::mutate

#endif // MUTATECLIENT_H
