// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

module;

#include "ProxySchema.h"

export module GraphQL.Proxy.ProxySchema;

namespace included = graphql::proxy;

export namespace graphql::proxy {

namespace exported {

using included::Operations;

using included::AddQueryDetails;

using included::GetSchema;

} // namespace exported

using namespace exported;

} // namespace graphql::proxy
