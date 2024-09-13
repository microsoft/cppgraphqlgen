// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Awaitable.h"

export module GraphQL.Internal.Awaitable;

namespace included = graphql::internal;

export namespace graphql::internal {

namespace exported {

template <typename T>
using Awaitable = included::Awaitable<T>;

} // namespace exported

using namespace exported;

} // namespace graphql::internal
