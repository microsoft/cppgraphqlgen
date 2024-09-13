// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Introspection.h"

export module GraphQL.Internal.Introspection;

namespace included = graphql::introspection;

export namespace graphql::introspection {

namespace exported {

using Schema = included::Schema;
using Type = included::Type;
using Field = included::Field;
using InputValue = included::InputValue;
using EnumValue = included::EnumValue;
using Directive = included::Directive;

} // namespace exported

using namespace exported;

} // namespace graphql::introspection
