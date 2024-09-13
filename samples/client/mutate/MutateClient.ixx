// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

module;

#include "MutateClient.h"

export module GraphQL.Mutate.MutateClient;

namespace included = graphql::client;

export namespace graphql::client {

namespace exported {

namespace mutate {

using included::mutate::GetRequestText;
using included::mutate::GetRequestObject;

using included::mutate::TaskState;

using included::mutate::CompleteTaskInput;

} // namespace mutate

namespace mutation::CompleteTaskMutation {

using included::mutate::GetRequestText;
using included::mutate::GetRequestObject;
using included::mutation::CompleteTaskMutation::GetOperationName;

using included::mutate::TaskState;

using included::mutate::CompleteTaskInput;

using included::mutation::CompleteTaskMutation::Variables;
using included::mutation::CompleteTaskMutation::serializeVariables;

using included::mutation::CompleteTaskMutation::Response;
using included::mutation::CompleteTaskMutation::parseResponse;

using included::mutation::CompleteTaskMutation::Traits;

} // namespace mutation::CompleteTaskMutation

} // namespace exported

using namespace exported;

} // namespace graphql::client
