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

using TypeKind = included::introspection::TypeKind;
using DirectiveLocation = included::introspection::DirectiveLocation;

} // namespace exported

using namespace exported;

} // namespace introspection

namespace schema {

namespace exported {

using Schema = included::schema::Schema;
using BaseType = included::schema::BaseType;
using ScalarType = included::schema::ScalarType;
using ObjectType = included::schema::ObjectType;
using InterfaceType = included::schema::InterfaceType;
using UnionType = included::schema::UnionType;
using EnumValueType = included::schema::EnumValueType;
using EnumType = included::schema::EnumType;
using InputObjectType = included::schema::InputObjectType;
using WrapperType = included::schema::WrapperType;
using Field = included::schema::Field;
using InputValue = included::schema::InputValue;
using EnumValue = included::schema::EnumValue;
using Directive = included::schema::Directive;

} // namespace exported

using namespace exported;

} // namespace schema

} // namespace graphql
