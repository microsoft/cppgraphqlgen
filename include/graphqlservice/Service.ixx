// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "GraphQLService.h"

export module GraphQL.Service;

export import GraphQL.Parse;
export import GraphQL.Response;

export import GraphQL.Internal.Awaitable;
export import GraphQL.Internal.SortedMap;

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
using service::visitErrorValues;

using service::schema_exception;
using service::unimplemented_method;

using service::RequestState;

constexpr std::string_view strData = keywords::strData;
constexpr std::string_view strErrors = keywords::strErrors;
constexpr std::string_view strMessage = keywords::strMessage;
constexpr std::string_view strLocations = keywords::strLocations;
constexpr std::string_view strLine = keywords::strLine;
constexpr std::string_view strColumn = keywords::strColumn;
constexpr std::string_view strPath = keywords::strPath;
constexpr std::string_view strQuery = keywords::strQuery;
constexpr std::string_view strMutation = keywords::strMutation;
constexpr std::string_view strSubscription = keywords::strSubscription;

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

using modified_argument::ModifiedArgument;
using modified_argument::IntArgument;
using modified_argument::FloatArgument;
using modified_argument::StringArgument;
using modified_argument::BooleanArgument;
using modified_argument::IdArgument;
using modified_argument::ScalarArgument;

using service::TypeNames;
using service::Object;
using service::Result;

using modified_result::ModifiedResult;
using modified_result::IntResult;
using modified_result::FloatResult;
using modified_result::StringResult;
using modified_result::BooleanResult;
using modified_result::IdResult;
using modified_result::ScalarResult;
using modified_result::ObjectResult;

using service::SubscriptionCallback;
using service::SubscriptionVisitor;
using service::SubscriptionCallbackOrVisitor;
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
