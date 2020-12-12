// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLSCHEMA_H
#define GRAPHQLSCHEMA_H

#include "graphqlservice/GraphQLService.h"

namespace graphql {
namespace introspection {

enum class TypeKind;
enum class DirectiveLocation;

} // namespace introspection

namespace schema {

class Schema;
class Directive;
class BaseType;
class ScalarType;
class ObjectType;
class InterfaceType;
class UnionType;
class EnumType;
class InputObjectType;
class WrapperType;
class Field;
class InputValue;
class EnumValue;

class Schema : public std::enable_shared_from_this<Schema>
{
public:
	GRAPHQLSERVICE_EXPORT explicit Schema(bool noIntrospection = false);

	GRAPHQLSERVICE_EXPORT void AddQueryType(std::shared_ptr<ObjectType> query);
	GRAPHQLSERVICE_EXPORT void AddMutationType(std::shared_ptr<ObjectType> mutation);
	GRAPHQLSERVICE_EXPORT void AddSubscriptionType(std::shared_ptr<ObjectType> subscription);
	GRAPHQLSERVICE_EXPORT void AddType(std::string_view name, std::shared_ptr<BaseType> type);
	GRAPHQLSERVICE_EXPORT const std::shared_ptr<BaseType>& LookupType(std::string_view name) const;
	GRAPHQLSERVICE_EXPORT const std::shared_ptr<BaseType>& WrapType(
		introspection::TypeKind kind, const std::shared_ptr<BaseType>& ofType);
	GRAPHQLSERVICE_EXPORT void AddDirective(std::shared_ptr<Directive> directive);

	// Accessors
	GRAPHQLSERVICE_EXPORT bool supportsIntrospection() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::vector<std::pair<std::string_view, std::shared_ptr<BaseType>>>&
	types()
		const noexcept;
	GRAPHQLSERVICE_EXPORT const std::shared_ptr<ObjectType>& queryType() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::shared_ptr<ObjectType>& mutationType() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::shared_ptr<ObjectType>& subscriptionType() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<Directive>>& directives()
		const noexcept;

private:
	const bool _noIntrospection = false;

	std::shared_ptr<ObjectType> _query;
	std::shared_ptr<ObjectType> _mutation;
	std::shared_ptr<ObjectType> _subscription;
	std::unordered_map<std::string_view, size_t> _typeMap;
	std::vector<std::pair<std::string_view, std::shared_ptr<BaseType>>> _types;
	std::vector<std::shared_ptr<Directive>> _directives;
	std::unordered_map<std::shared_ptr<BaseType>, std::shared_ptr<BaseType>> _nonNullWrappers;
	std::unordered_map<std::shared_ptr<BaseType>, std::shared_ptr<BaseType>> _listWrappers;
};

class BaseType : public std::enable_shared_from_this<BaseType>
{
public:
	// Accessors
	GRAPHQLSERVICE_EXPORT introspection::TypeKind kind() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual std::string_view name() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view description() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::vector<std::shared_ptr<Field>>& fields() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::vector<std::shared_ptr<InterfaceType>>& interfaces() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::vector<std::weak_ptr<BaseType>>& possibleTypes() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::vector<std::shared_ptr<EnumValue>>& enumValues() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::vector<std::shared_ptr<InputValue>>& inputFields() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::weak_ptr<BaseType>& ofType() const noexcept;

protected:
	BaseType(introspection::TypeKind kind, std::string_view description);

private:
	const introspection::TypeKind _kind;
	const std::string_view _description;
};

class ScalarType : public BaseType
{
public:
	GRAPHQLSERVICE_EXPORT explicit ScalarType(std::string_view name, std::string_view description);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;

private:
	const std::string_view _name;
};

class ObjectType : public BaseType
{
public:
	GRAPHQLSERVICE_EXPORT explicit ObjectType(std::string_view name, std::string_view description);

	GRAPHQLSERVICE_EXPORT void AddInterfaces(
		std::vector<std::shared_ptr<InterfaceType>> interfaces);
	GRAPHQLSERVICE_EXPORT void AddFields(std::vector<std::shared_ptr<Field>> fields);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<Field>>& fields() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<InterfaceType>>& interfaces() const noexcept final;

private:
	const std::string_view _name;

	std::vector<std::shared_ptr<InterfaceType>> _interfaces;
	std::vector<std::shared_ptr<Field>> _fields;
};

class InterfaceType : public BaseType
{
public:
	GRAPHQLSERVICE_EXPORT explicit InterfaceType(
		std::string_view name, std::string_view description);

	GRAPHQLSERVICE_EXPORT void AddPossibleType(std::weak_ptr<ObjectType> possibleType);
	GRAPHQLSERVICE_EXPORT void AddFields(std::vector<std::shared_ptr<Field>> fields);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<Field>>& fields() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::weak_ptr<BaseType>>& possibleTypes() const noexcept final;

private:
	const std::string_view _name;

	std::vector<std::shared_ptr<Field>> _fields;
	std::vector<std::weak_ptr<BaseType>> _possibleTypes;
};

class UnionType : public BaseType
{
public:
	GRAPHQLSERVICE_EXPORT explicit UnionType(std::string_view name, std::string_view description);

	GRAPHQLSERVICE_EXPORT void AddPossibleTypes(std::vector<std::weak_ptr<BaseType>> possibleTypes);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::weak_ptr<BaseType>>& possibleTypes() const noexcept final;

private:
	const std::string_view _name;

	std::vector<std::weak_ptr<BaseType>> _possibleTypes;
};

struct EnumValueType
{
	std::string_view value;
	std::string_view description;
	std::optional<std::string_view> deprecationReason;
};

class EnumType : public BaseType
{
public:
	GRAPHQLSERVICE_EXPORT explicit EnumType(std::string_view name, std::string_view description);

	GRAPHQLSERVICE_EXPORT void AddEnumValues(std::vector<EnumValueType> enumValues);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<EnumValue>>& enumValues() const noexcept final;

private:
	const std::string_view _name;

	std::vector<std::shared_ptr<EnumValue>> _enumValues;
};

class InputObjectType : public BaseType
{
public:
	GRAPHQLSERVICE_EXPORT explicit InputObjectType(
		std::string_view name, std::string_view description);

	GRAPHQLSERVICE_EXPORT void AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<InputValue>>& inputFields() const noexcept final;

private:
	const std::string_view _name;

	std::vector<std::shared_ptr<InputValue>> _inputValues;
};

class WrapperType : public BaseType
{
public:
	GRAPHQLSERVICE_EXPORT explicit WrapperType(
		introspection::TypeKind kind, const std::shared_ptr<BaseType>& ofType);

	// Accessors
	GRAPHQLSERVICE_EXPORT const std::weak_ptr<BaseType>& ofType() const noexcept final;

private:
	const std::weak_ptr<BaseType> _ofType;
};

class Field : public std::enable_shared_from_this<Field>
{
public:
	GRAPHQLSERVICE_EXPORT explicit Field(std::string_view name, std::string_view description,
		std::optional<std::string_view> deprecationReason,
		std::vector<std::shared_ptr<InputValue>>&& args, const std::shared_ptr<BaseType>& type);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view description() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<InputValue>>& args() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::weak_ptr<BaseType>& type() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::optional<std::string_view>& deprecationReason() const noexcept;

private:
	const std::string_view _name;
	const std::string_view _description;
	const std::optional<std::string_view> _deprecationReason;
	const std::vector<std::shared_ptr<InputValue>> _args;
	const std::weak_ptr<BaseType> _type;
};

class InputValue : public std::enable_shared_from_this<InputValue>
{
public:
	GRAPHQLSERVICE_EXPORT explicit InputValue(std::string_view name, std::string_view description,
		const std::shared_ptr<BaseType>& type, std::string_view defaultValue);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view description() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::weak_ptr<BaseType>& type() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view defaultValue() const noexcept;

private:
	const std::string_view _name;
	const std::string_view _description;
	const std::weak_ptr<BaseType> _type;
	const std::string_view _defaultValue;
};

class EnumValue : public std::enable_shared_from_this<EnumValue>
{
public:
	GRAPHQLSERVICE_EXPORT explicit EnumValue(std::string_view name, std::string_view description,
		std::optional<std::string_view> deprecationReason);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view description() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::optional<std::string_view>& deprecationReason() const noexcept;

private:
	const std::string_view _name;
	const std::string_view _description;
	const std::optional<std::string_view> _deprecationReason;
};

class Directive : public std::enable_shared_from_this<Directive>
{
public:
	GRAPHQLSERVICE_EXPORT explicit Directive(std::string_view name, std::string_view description,
		std::vector<introspection::DirectiveLocation>&& locations,
		std::vector<std::shared_ptr<InputValue>>&& args);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view description() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::vector<introspection::DirectiveLocation>& locations() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<InputValue>>& args() const noexcept;

private:
	const std::string_view _name;
	const std::string_view _description;
	const std::vector<introspection::DirectiveLocation> _locations;
	const std::vector<std::shared_ptr<InputValue>> _args;
};

} // namespace schema
} // namespace graphql

#endif // GRAPHQLSCHEMA_H
