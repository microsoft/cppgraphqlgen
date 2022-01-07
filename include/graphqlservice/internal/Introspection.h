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
	GRAPHQLSERVICE_EXPORT explicit Schema(const std::shared_ptr<schema::Schema>& schema);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLSERVICE_EXPORT std::vector<std::shared_ptr<object::Type>> getTypes() const;
	GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type> getQueryType() const;
	GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type> getMutationType() const;
	GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type> getSubscriptionType() const;
	GRAPHQLSERVICE_EXPORT std::vector<std::shared_ptr<object::Directive>> getDirectives()
		const;

private:
	const std::shared_ptr<schema::Schema> _schema;
};

class Type
{
public:
	GRAPHQLSERVICE_EXPORT explicit Type(const std::shared_ptr<const schema::BaseType>& type);

	// Accessors
	GRAPHQLSERVICE_EXPORT TypeKind getKind() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getName() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::vector<std::shared_ptr<object::Field>>>
	getFields(std::optional<bool>&& includeDeprecatedArg) const;
	GRAPHQLSERVICE_EXPORT std::optional<std::vector<std::shared_ptr<object::Type>>>
	getInterfaces() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::vector<std::shared_ptr<object::Type>>>
	getPossibleTypes() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::vector<std::shared_ptr<object::EnumValue>>>
	getEnumValues(std::optional<bool>&& includeDeprecatedArg) const;
	GRAPHQLSERVICE_EXPORT std::optional<std::vector<std::shared_ptr<object::InputValue>>>
	getInputFields() const;
	GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type> getOfType() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getSpecifiedByURL() const;

private:
	const std::shared_ptr<const schema::BaseType> _type;
};

class Field
{
public:
	GRAPHQLSERVICE_EXPORT explicit Field(const std::shared_ptr<const schema::Field>& field);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string getName() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLSERVICE_EXPORT std::vector<std::shared_ptr<object::InputValue>> getArgs() const;
	GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type> getType() const;
	GRAPHQLSERVICE_EXPORT bool getIsDeprecated() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getDeprecationReason() const;

private:
	const std::shared_ptr<const schema::Field> _field;
};

class InputValue
{
public:
	GRAPHQLSERVICE_EXPORT explicit InputValue(
		const std::shared_ptr<const schema::InputValue>& inputValue);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string getName() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type> getType() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getDefaultValue() const;

private:
	const std::shared_ptr<const schema::InputValue> _inputValue;
};

class EnumValue
{
public:
	GRAPHQLSERVICE_EXPORT explicit EnumValue(
		const std::shared_ptr<const schema::EnumValue>& enumValue);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string getName() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLSERVICE_EXPORT bool getIsDeprecated() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getDeprecationReason() const;

private:
	const std::shared_ptr<const schema::EnumValue> _enumValue;
};

class Directive
{
public:
	GRAPHQLSERVICE_EXPORT explicit Directive(
		const std::shared_ptr<const schema::Directive>& directive);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string getName() const;
	GRAPHQLSERVICE_EXPORT std::optional<std::string> getDescription() const;
	GRAPHQLSERVICE_EXPORT std::vector<DirectiveLocation> getLocations() const;
	GRAPHQLSERVICE_EXPORT std::vector<std::shared_ptr<object::InputValue>> getArgs() const;
	GRAPHQLSERVICE_EXPORT bool getIsRepeatable() const;

private:
	const std::shared_ptr<const schema::Directive> _directive;
};

} // namespace graphql::introspection

#endif // INTROSPECTION_H
