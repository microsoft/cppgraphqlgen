// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

module;

#include "MultipleQueriesClient.h"

export module GraphQL.MultipleQueries.MultipleQueriesClient;

namespace included = graphql::client;

export namespace graphql::client {

namespace exported {

namespace multiple {

using included::multiple::GetRequestText;
using included::multiple::GetRequestObject;

using included::multiple::TaskState;

using included::multiple::CompleteTaskInput;

} // namespace multiple

namespace query::Appointments {

using included::multiple::GetRequestText;
using included::multiple::GetRequestObject;
using included::query::Appointments::GetOperationName;

using included::query::Appointments::Response;
using included::query::Appointments::parseResponse;

using included::query::Appointments::Traits;

} // namespace query::Appointments

namespace query::Tasks {

using included::multiple::GetRequestText;
using included::multiple::GetRequestObject;
using included::query::Tasks::GetOperationName;

using included::query::Tasks::Response;
using included::query::Tasks::parseResponse;

using included::query::Tasks::Traits;

} // namespace query::Tasks

namespace query::UnreadCounts {

using included::multiple::GetRequestText;
using included::multiple::GetRequestObject;
using included::query::UnreadCounts::GetOperationName;

using included::query::UnreadCounts::Response;
using included::query::UnreadCounts::parseResponse;

using included::query::UnreadCounts::Traits;

} // namespace query::UnreadCounts

namespace query::Miscellaneous {

using included::multiple::GetRequestText;
using included::multiple::GetRequestObject;
using included::query::Miscellaneous::GetOperationName;

using included::multiple::TaskState;

using included::query::Miscellaneous::Response;
using included::query::Miscellaneous::parseResponse;

using included::query::Miscellaneous::Traits;

} // namespace query::Miscellaneous

namespace mutation::CompleteTaskMutation {

using included::multiple::GetRequestText;
using included::multiple::GetRequestObject;
using included::mutation::CompleteTaskMutation::GetOperationName;

using included::multiple::TaskState;

using included::multiple::CompleteTaskInput;

using included::mutation::CompleteTaskMutation::Variables;
using included::mutation::CompleteTaskMutation::serializeVariables;

using included::mutation::CompleteTaskMutation::Response;
using included::mutation::CompleteTaskMutation::parseResponse;

using included::mutation::CompleteTaskMutation::Traits;

} // namespace mutation::CompleteTaskMutation

} // namespace exported

using namespace exported;

} // namespace graphql::client
