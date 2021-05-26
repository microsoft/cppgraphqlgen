// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef INTROSPECTION_H
#define INTROSPECTION_H

#include "graphqlservice/GraphQLSchema.h"
#include "graphqlservice/introspection/IntrospectionSchema.h"

namespace graphql::introspection {

class Schema;
class Directive;
class Type;
class Field;
class InputValue;
class EnumValue;

class Schema : public object::Schema
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit Schema(const std::shared_ptr<schema::Schema>& schema);

	// Accessors
	service::FieldResult<std::vector<std::shared_ptr<object::Type>>> getTypes(
		service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getQueryType(
		service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getMutationType(
		service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getSubscriptionType(
		service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Directive>>> getDirectives(
		service::FieldParams&& params) const override;

private:
	const std::shared_ptr<schema::Schema> _schema;
};

class Type : public object::Type
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit Type(const std::shared_ptr<const schema::BaseType>& type);

	// Accessors
	service::FieldResult<TypeKind> getKind(service::FieldParams&&) const override;
	service::FieldResult<std::optional<response::StringType>> getName(
		service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDescription(
		service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Field>>>> getFields(
		service::FieldParams&& params,
		std::optional<response::BooleanType>&& includeDeprecatedArg) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> getInterfaces(
		service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>>
	getPossibleTypes(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::EnumValue>>>>
	getEnumValues(service::FieldParams&& params,
		std::optional<response::BooleanType>&& includeDeprecatedArg) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::InputValue>>>>
	getInputFields(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getOfType(
		service::FieldParams&& params) const override;

private:
	const std::shared_ptr<const schema::BaseType> _type;
};

class Field : public object::Field
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit Field(const std::shared_ptr<const schema::Field>& field);

	// Accessors
	service::FieldResult<response::StringType> getName(
		service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDescription(
		service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::InputValue>>> getArgs(
		service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getType(
		service::FieldParams&& params) const override;
	service::FieldResult<response::BooleanType> getIsDeprecated(
		service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDeprecationReason(
		service::FieldParams&& params) const override;

private:
	const std::shared_ptr<const schema::Field> _field;
};

class InputValue : public object::InputValue
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit InputValue(
		const std::shared_ptr<const schema::InputValue>& inputValue);

	// Accessors
	service::FieldResult<response::StringType> getName(
		service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDescription(
		service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getType(
		service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDefaultValue(
		service::FieldParams&& params) const override;

private:
	const std::shared_ptr<const schema::InputValue> _inputValue;
};

class EnumValue : public object::EnumValue
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit EnumValue(
		const std::shared_ptr<const schema::EnumValue>& enumValue);

	// Accessors
	service::FieldResult<response::StringType> getName(
		service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDescription(
		service::FieldParams&& params) const override;
	service::FieldResult<response::BooleanType> getIsDeprecated(
		service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDeprecationReason(
		service::FieldParams&& params) const override;

private:
	const std::shared_ptr<const schema::EnumValue> _enumValue;
};

class Directive : public object::Directive
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit Directive(
		const std::shared_ptr<const schema::Directive>& directive);

	// Accessors
	service::FieldResult<response::StringType> getName(
		service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDescription(
		service::FieldParams&& params) const override;
	service::FieldResult<std::vector<DirectiveLocation>> getLocations(
		service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::InputValue>>> getArgs(
		service::FieldParams&& params) const override;

private:
	const std::shared_ptr<const schema::Directive> _directive;
};

} // namespace graphql::introspection

#endif // INTROSPECTION_H
