// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Base64.h"

export module GraphQL.Internal:Base64;

namespace included = graphql::internal;

export namespace graphql::internal {

namespace exported {

using Base64 = included::Base64;

} // namespace exported

using namespace exported;

} // namespace graphql::internal
