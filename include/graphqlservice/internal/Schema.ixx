// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Schema.h"

export module GraphQL.Internal.Schema;

namespace included {

namespace introspection = graphql::introspection;
namespace schema = graphql::schema;

} // namespace included

export namespace graphql {

namespace introspection {

namespace exported {

// clang-format off
using included::introspection::TypeKind;
using included::introspection::DirectiveLocation;
// clang-format on

} // namespace exported

using namespace exported;

} // namespace introspection

namespace schema {

namespace exported {

// clang-format off
using included::schema::Schema;
using included::schema::BaseType;
using included::schema::ScalarType;
using included::schema::ObjectType;
using included::schema::InterfaceType;
using included::schema::UnionType;
using included::schema::EnumValueType;
using included::schema::EnumType;
using included::schema::InputObjectType;
using included::schema::WrapperType;
using included::schema::Field;
using included::schema::InputValue;
using included::schema::EnumValue;
using included::schema::Directive;
// clang-format on

} // namespace exported

using namespace exported;

} // namespace schema

} // namespace graphql
