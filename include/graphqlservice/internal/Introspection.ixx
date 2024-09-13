// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Introspection.h"

export module GraphQL.Internal.Introspection;

namespace included = graphql::introspection;

export namespace graphql::introspection {

namespace exported {

// clang-format off
using included::Schema;
using included::Type;
using included::Field;
using included::InputValue;
using included::EnumValue;
using included::Directive;
// clang-format on

} // namespace exported

using namespace exported;

} // namespace graphql::introspection
