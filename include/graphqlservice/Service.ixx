// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "GraphQLService.h"

export module GraphQL.Service;

namespace included = graphql::service;

export namespace graphql {

namespace schema {

namespace exported {

using schema::Schema;

} // namespace exported

using namespace exported;

} // namespace schema

namespace service {

namespace exported {

// clang-format off
using included::schema_location;
using included::path_segment;
using included::field_path;

using included::error_path;
using included::buildErrorPath;

using included::schema_error;
using included::buildErrorValues;

using included::schema_exception;
using included::unimplemented_method;

using included::RequestState;

constexpr std::string_view strData = included::strData;
constexpr std::string_view strErrors = included::strErrors;
constexpr std::string_view strMessage = included::strMessage;
constexpr std::string_view strLocations = included::strLocations;
constexpr std::string_view strLine = included::strLine;
constexpr std::string_view strColumn = included::strColumn;
constexpr std::string_view strPath = included::strPath;
constexpr std::string_view strQuery = included::strQuery;
constexpr std::string_view strMutation = included::strMutation;
constexpr std::string_view strSubscription = included::strSubscription;

using included::ResolverContext;

using included::await_worker_thread;
using included::await_worker_queue;
using included::await_async;

using included::Directives;
using included::FragmentDefinitionDirectiveStack;
using included::FragmentSpreadDirectiveStack;

using included::SelectionSetParams;
using included::FieldParams;

using included::AwaitableScalar;
using included::AwaitableObject;

using included::Fragment;
using included::FragmentMap;

using included::ResolverParams;
using included::ResolverResult;
using included::AwaitableResolver;
using included::Resolver;
using included::ResolverMap;

using included::TypeModifier;
using included::Argument;

using included::ModifiedArgument;
using included::IntArgument;
using included::FloatArgument;
using included::StringArgument;
using included::BooleanArgument;
using included::IdArgument;
using included::ScalarArgument;

using included::TypeNames;
using included::Object;
using included::Result;

using included::ModifiedResult;
using included::IntResult;
using included::FloatResult;
using included::StringResult;
using included::BooleanResult;
using included::IdResult;
using included::ScalarResult;
using included::ObjectResult;

using included::SubscriptionCallback;
using included::SubscriptionKey;
using included::SubscriptionName;

using included::AwaitableSubscribe;
using included::AwaitableUnsubscribe;
using included::AwaitableDeliver;

using included::RequestResolveParams;
using included::RequestSubscribeParams;
using included::RequestUnsubscribeParams;

using included::SubscriptionArguments;
using included::SubscriptionArgumentFilterCallback;
using included::SubscriptionDirectiveFilterCallback;
using included::SubscriptionFilter;

using included::RequestDeliverFilter;
using included::RequestDeliverParams;

using included::TypeMap;
using included::OperationData;
using included::SubscriptionData;

using included::SubscriptionPlaceholder;

using included::Request;
// clang-format on

} // namespace exported

using namespace exported;

} // namespace service

} // namespace graphql
