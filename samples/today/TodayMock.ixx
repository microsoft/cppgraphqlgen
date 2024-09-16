// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "TodayMock.h"

export module GraphQL.Today.Mock;

export import GraphQL.Today.TodaySchema;

export namespace graphql::today {

// clang-format off
using today::getFakeAppointmentId;
using today::getFakeTaskId;
using today::getFakeFolderId;

using today::TodayMockService;
using today::mock_service;

using today::RequestState;

using today::Query;

using today::PageInfo;
using today::Appointment;
using today::AppointmentEdge;
using today::Task;
using today::TaskEdge;
using today::TaskConnection;
using today::Folder;
using today::FolderEdge;
using today::FolderConnection;
using today::CompleteTaskPayload;
using today::Mutation;
using today::Subscription;
using today::NextAppointmentChange;
using today::NodeChange;
using today::CapturedParams;
using today::NestedType;
using today::Expensive;
using today::EmptyOperations;
// clang-format on

} // namespace graphql::today
