// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef INTROSPECTION_H
#define INTROSPECTION_H

#include "graphqlservice/introspection/IntrospectionSchema.h"

#include "graphqlservice/internal/Schema.h"

namespace graphql::introspection {

class Schema;
class Directive;
class Type;
class Field;
class InputValue;
class EnumValue;

class Schema
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit Schema(const std::shared_ptr<schema::Schema>& schema);

	// Accessors
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLINTROSPECTION_EXPORT std::vector<std::shared_ptr<object::Type>> getTypes() const;
	GRAPHQLINTROSPECTION_EXPORT std::shared_ptr<object::Type> getQueryType() const;
	GRAPHQLINTROSPECTION_EXPORT std::shared_ptr<object::Type> getMutationType() const;
	GRAPHQLINTROSPECTION_EXPORT std::shared_ptr<object::Type> getSubscriptionType() const;
	GRAPHQLINTROSPECTION_EXPORT std::vector<std::shared_ptr<object::Directive>> getDirectives()
		const;

private:
	const std::shared_ptr<schema::Schema> _schema;
};

class Type
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit Type(const std::shared_ptr<const schema::BaseType>& type);

	// Accessors
	GRAPHQLINTROSPECTION_EXPORT TypeKind getKind() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getName() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::vector<std::shared_ptr<object::Field>>>
	getFields(std::optional<bool>&& includeDeprecatedArg) const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::vector<std::shared_ptr<object::Type>>>
	getInterfaces() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::vector<std::shared_ptr<object::Type>>>
	getPossibleTypes() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::vector<std::shared_ptr<object::EnumValue>>>
	getEnumValues(std::optional<bool>&& includeDeprecatedArg) const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::vector<std::shared_ptr<object::InputValue>>>
	getInputFields() const;
	GRAPHQLINTROSPECTION_EXPORT std::shared_ptr<object::Type> getOfType() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getSpecifiedByURL() const;

private:
	const std::shared_ptr<const schema::BaseType> _type;
};

class Field
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit Field(const std::shared_ptr<const schema::Field>& field);

	// Accessors
	GRAPHQLINTROSPECTION_EXPORT std::string getName() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLINTROSPECTION_EXPORT std::vector<std::shared_ptr<object::InputValue>> getArgs() const;
	GRAPHQLINTROSPECTION_EXPORT std::shared_ptr<object::Type> getType() const;
	GRAPHQLINTROSPECTION_EXPORT bool getIsDeprecated() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getDeprecationReason() const;

private:
	const std::shared_ptr<const schema::Field> _field;
};

class InputValue
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit InputValue(
		const std::shared_ptr<const schema::InputValue>& inputValue);

	// Accessors
	GRAPHQLINTROSPECTION_EXPORT std::string getName() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLINTROSPECTION_EXPORT std::shared_ptr<object::Type> getType() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getDefaultValue() const;

private:
	const std::shared_ptr<const schema::InputValue> _inputValue;
};

class EnumValue
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit EnumValue(
		const std::shared_ptr<const schema::EnumValue>& enumValue);

	// Accessors
	GRAPHQLINTROSPECTION_EXPORT std::string getName() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLINTROSPECTION_EXPORT bool getIsDeprecated() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getDeprecationReason() const;

private:
	const std::shared_ptr<const schema::EnumValue> _enumValue;
};

class Directive
{
public:
	GRAPHQLINTROSPECTION_EXPORT explicit Directive(
		const std::shared_ptr<const schema::Directive>& directive);

	// Accessors
	GRAPHQLINTROSPECTION_EXPORT std::string getName() const;
	GRAPHQLINTROSPECTION_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLINTROSPECTION_EXPORT std::vector<DirectiveLocation> getLocations() const;
	GRAPHQLINTROSPECTION_EXPORT std::vector<std::shared_ptr<object::InputValue>> getArgs() const;
	GRAPHQLINTROSPECTION_EXPORT bool getIsRepeatable() const;

private:
	const std::shared_ptr<const schema::Directive> _directive;
};

} // namespace graphql::introspection

#endif // INTROSPECTION_H
