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

class [[nodiscard("unnecessary construction")]] Schema
{
public:
	GRAPHQLSERVICE_EXPORT explicit Schema(const std::shared_ptr<schema::Schema>& schema);

	// Accessors
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string>
	getDescription() const;
	[[nodiscard(
		"unnecessary call")]] GRAPHQLSERVICE_EXPORT std::vector<std::shared_ptr<object::Type>>
	getTypes() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type>
	getQueryType() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type>
	getMutationType() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type>
	getSubscriptionType() const;
	[[nodiscard(
		"unnecessary call")]] GRAPHQLSERVICE_EXPORT std::vector<std::shared_ptr<object::Directive>>
	getDirectives() const;

private:
	const std::shared_ptr<schema::Schema> _schema;
};

class [[nodiscard("unnecessary construction")]] Type
{
public:
	GRAPHQLSERVICE_EXPORT explicit Type(const std::shared_ptr<const schema::BaseType>& type);

	// Accessors
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT TypeKind getKind() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string> getName()
		const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string>
	getDescription() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<
		std::vector<std::shared_ptr<object::Field>>>
	getFields(std::optional<bool>&& includeDeprecatedArg) const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<
		std::vector<std::shared_ptr<object::Type>>>
	getInterfaces() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<
		std::vector<std::shared_ptr<object::Type>>>
	getPossibleTypes() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<
		std::vector<std::shared_ptr<object::EnumValue>>>
	getEnumValues(std::optional<bool>&& includeDeprecatedArg) const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<
		std::vector<std::shared_ptr<object::InputValue>>>
	getInputFields() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type>
	getOfType() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string>
	getSpecifiedByURL() const;

private:
	const std::shared_ptr<const schema::BaseType> _type;
};

class [[nodiscard("unnecessary construction")]] Field
{
public:
	GRAPHQLSERVICE_EXPORT explicit Field(const std::shared_ptr<const schema::Field>& field);

	// Accessors
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::string getName() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string>
	getDescription() const;
	[[nodiscard(
		"unnecessary call")]] GRAPHQLSERVICE_EXPORT std::vector<std::shared_ptr<object::InputValue>>
	getArgs() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type> getType()
		const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT bool getIsDeprecated() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string>
	getDeprecationReason() const;

private:
	const std::shared_ptr<const schema::Field> _field;
};

class [[nodiscard("unnecessary construction")]] InputValue
{
public:
	GRAPHQLSERVICE_EXPORT explicit InputValue(
		const std::shared_ptr<const schema::InputValue>& inputValue);

	// Accessors
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::string getName() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string>
	getDescription() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::shared_ptr<object::Type> getType()
		const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string>
	getDefaultValue() const;

private:
	const std::shared_ptr<const schema::InputValue> _inputValue;
};

class [[nodiscard("unnecessary construction")]] EnumValue
{
public:
	GRAPHQLSERVICE_EXPORT explicit EnumValue(
		const std::shared_ptr<const schema::EnumValue>& enumValue);

	// Accessors
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::string getName() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string>
	getDescription() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT bool getIsDeprecated() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string>
	getDeprecationReason() const;

private:
	const std::shared_ptr<const schema::EnumValue> _enumValue;
};

class [[nodiscard("unnecessary construction")]] Directive
{
public:
	GRAPHQLSERVICE_EXPORT explicit Directive(
		const std::shared_ptr<const schema::Directive>& directive);

	// Accessors
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::string getName() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::optional<std::string>
	getDescription() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT std::vector<DirectiveLocation>
	getLocations() const;
	[[nodiscard(
		"unnecessary call")]] GRAPHQLSERVICE_EXPORT std::vector<std::shared_ptr<object::InputValue>>
	getArgs() const;
	[[nodiscard("unnecessary call")]] GRAPHQLSERVICE_EXPORT bool getIsRepeatable() const;

private:
	const std::shared_ptr<const schema::Directive> _directive;
};

} // namespace graphql::introspection

#endif // INTROSPECTION_H
