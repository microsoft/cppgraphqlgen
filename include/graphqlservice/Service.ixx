// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "GraphQLService.h"

export module GraphQL.Service;

export namespace graphql {

namespace schema {

using schema::Schema;

} // namespace schema

namespace service {

// clang-format off
using service::schema_location;
using service::path_segment;
using service::field_path;

using service::error_path;
using service::buildErrorPath;

using service::schema_error;
using service::buildErrorValues;

using service::schema_exception;
using service::unimplemented_method;

using service::RequestState;

namespace constants {

constexpr std::string_view strData = service::strData;
constexpr std::string_view strErrors = service::strErrors;
constexpr std::string_view strMessage = service::strMessage;
constexpr std::string_view strLocations = service::strLocations;
constexpr std::string_view strLine = service::strLine;
constexpr std::string_view strColumn = service::strColumn;
constexpr std::string_view strPath = service::strPath;
constexpr std::string_view strQuery = service::strQuery;
constexpr std::string_view strMutation = service::strMutation;
constexpr std::string_view strSubscription = service::strSubscription;
    
} // namespace constants

using namespace constants;

using service::ResolverContext;

using service::await_worker_thread;
using service::await_worker_queue;
using service::await_async;

using service::Directives;
using service::FragmentDefinitionDirectiveStack;
using service::FragmentSpreadDirectiveStack;

using service::SelectionSetParams;
using service::FieldParams;

using service::AwaitableScalar;
using service::AwaitableObject;

using service::Fragment;
using service::FragmentMap;

using service::ResolverParams;
using service::ResolverResult;
using service::AwaitableResolver;
using service::Resolver;
using service::ResolverMap;

using service::TypeModifier;
using service::Argument;

using service::ModifiedArgument;
using service::IntArgument;
using service::FloatArgument;
using service::StringArgument;
using service::BooleanArgument;
using service::IdArgument;
using service::ScalarArgument;

using service::TypeNames;
using service::Object;
using service::Result;

using service::ModifiedResult;
using service::IntResult;
using service::FloatResult;
using service::StringResult;
using service::BooleanResult;
using service::IdResult;
using service::ScalarResult;
using service::ObjectResult;

using service::SubscriptionCallback;
using service::SubscriptionKey;
using service::SubscriptionName;

using service::AwaitableSubscribe;
using service::AwaitableUnsubscribe;
using service::AwaitableDeliver;

using service::RequestResolveParams;
using service::RequestSubscribeParams;
using service::RequestUnsubscribeParams;

using service::SubscriptionArguments;
using service::SubscriptionArgumentFilterCallback;
using service::SubscriptionDirectiveFilterCallback;
using service::SubscriptionFilter;

using service::RequestDeliverFilter;
using service::RequestDeliverParams;

using service::TypeMap;
using service::OperationData;
using service::SubscriptionData;

using service::SubscriptionPlaceholder;

using service::Request;
// clang-format on

} // namespace service

} // namespace graphql
