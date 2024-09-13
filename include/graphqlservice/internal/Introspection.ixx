// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Introspection.h"

export module GraphQL.Internal.Introspection;

namespace included = graphql::introspection;

export namespace graphql::introspection {

namespace object {

namespace exported {

using Schema = included::object::Schema;
using Type = included::object::Type;
using Field = included::object::Field;
using InputValue = included::object::InputValue;
using EnumValue = included::object::EnumValue;
using Directive = included::object::Directive;

} // namespace exported

using namespace exported;

} // namespace object

namespace exported {

using TypeKind = included::TypeKind;

constexpr auto getTypeKindNames() noexcept
{
	return included::getTypeKindNames();
}

constexpr auto getTypeKindValues() noexcept
{
	return included::getTypeKindValues();
}

using DirectiveLocation = included::DirectiveLocation;

constexpr auto getDirectiveLocationNames() noexcept
{
	return included::getDirectiveLocationNames();
}

constexpr auto getDirectiveLocationValues() noexcept
{
	return included::getDirectiveLocationValues();
}

using Schema = included::Schema;
using Type = included::Type;
using Field = included::Field;
using InputValue = included::InputValue;
using EnumValue = included::EnumValue;
using Directive = included::Directive;

void AddSchemaDetails(const std::shared_ptr<schema::ObjectType>& typeSchema, const std::shared_ptr<schema::Schema>& schema)
{
    included::AddSchemaDetails(typeSchema, schema);
}

void AddTypeDetails(const std::shared_ptr<schema::ObjectType>& typeType, const std::shared_ptr<schema::Schema>& schema)
{
    included::AddTypeDetails(typeType, schema);
}

void AddFieldDetails(const std::shared_ptr<schema::ObjectType>& typeField, const std::shared_ptr<schema::Schema>& schema)
{
    included::AddFieldDetails(typeField, schema);
}

void AddInputValueDetails(const std::shared_ptr<schema::ObjectType>& typeInputValue, const std::shared_ptr<schema::Schema>& schema)
{
    included::AddInputValueDetails(typeInputValue, schema);
}

void AddEnumValueDetails(const std::shared_ptr<schema::ObjectType>& typeEnumValue, const std::shared_ptr<schema::Schema>& schema)
{
    included::AddEnumValueDetails(typeEnumValue, schema);
}

void AddDirectiveDetails(const std::shared_ptr<schema::ObjectType>& typeDirective, const std::shared_ptr<schema::Schema>& schema)
{
    included::AddDirectiveDetails(typeDirective, schema);
}

void AddTypesToSchema(const std::shared_ptr<schema::Schema>& schema){
    included::AddTypesToSchema(schema);
}

} // namespace exported

using namespace exported;

} // namespace graphql::introspection
