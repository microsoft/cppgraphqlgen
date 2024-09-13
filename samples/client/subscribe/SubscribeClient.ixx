// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

module;

#include "SubscribeClient.h"

export module GraphQL.Subscribe.SubscribeClient;

namespace included = graphql::client;

export namespace graphql::client {

namespace exported {

namespace subscribe {

using included::subscribe::GetRequestText;
using included::subscribe::GetRequestObject;

} // namespace subscribe

namespace subscription::TestSubscription {

using included::subscribe::GetRequestText;
using included::subscribe::GetRequestObject;
using included::subscription::TestSubscription::GetOperationName;

using included::subscription::TestSubscription::Response;
using included::subscription::TestSubscription::parseResponse;

using included::subscription::TestSubscription::Traits;

} // namespace subscription::TestSubscription

} // namespace exported

using namespace exported;

} // namespace graphql::client
