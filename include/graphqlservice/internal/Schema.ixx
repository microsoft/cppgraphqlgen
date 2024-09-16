// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Schema.h"

export module GraphQL.Internal.Schema;

export namespace graphql {

namespace introspection {

// clang-format off
using introspection::TypeKind;
using introspection::DirectiveLocation;
// clang-format on

} // namespace introspection

namespace schema {

// clang-format off
using schema::Schema;
using schema::BaseType;
using schema::ScalarType;
using schema::ObjectType;
using schema::InterfaceType;
using schema::UnionType;
using schema::EnumValueType;
using schema::EnumType;
using schema::InputObjectType;
using schema::WrapperType;
using schema::Field;
using schema::InputValue;
using schema::EnumValue;
using schema::Directive;
// clang-format on

} // namespace schema

} // namespace graphql
