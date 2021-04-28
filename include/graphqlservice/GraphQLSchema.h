// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLSCHEMA_H
#define GRAPHQLSCHEMA_H

#include "graphqlservice/GraphQLService.h"

#include <initializer_list>
#include <shared_mutex>

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
	GRAPHQLSERVICE_EXPORT const std::shared_ptr<const BaseType>& LookupType(
		std::string_view name) const;
	GRAPHQLSERVICE_EXPORT std::shared_ptr<const BaseType> WrapType(
		introspection::TypeKind kind, std::shared_ptr<const BaseType> ofType);
	GRAPHQLSERVICE_EXPORT void AddDirective(std::shared_ptr<Directive> directive);

	// Accessors
	GRAPHQLSERVICE_EXPORT bool supportsIntrospection() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::vector<
		std::pair<std::string_view, std::shared_ptr<const BaseType>>>&
	types() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::shared_ptr<const ObjectType>& queryType() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::shared_ptr<const ObjectType>& mutationType() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::shared_ptr<const ObjectType>& subscriptionType()
		const noexcept;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<const Directive>>& directives()
		const noexcept;

private:
	const bool _noIntrospection = false;

	std::shared_ptr<const ObjectType> _query;
	std::shared_ptr<const ObjectType> _mutation;
	std::shared_ptr<const ObjectType> _subscription;
	internal::string_view_map<size_t> _typeMap;
	std::vector<std::pair<std::string_view, std::shared_ptr<const BaseType>>> _types;
	std::vector<std::shared_ptr<const Directive>> _directives;
	std::shared_mutex _nonNullWrappersMutex;
	internal::sorted_map<std::shared_ptr<const BaseType>, std::shared_ptr<const BaseType>>
		_nonNullWrappers;
	std::shared_mutex _listWrappersMutex;
	internal::sorted_map<std::shared_ptr<const BaseType>, std::shared_ptr<const BaseType>>
		_listWrappers;
};

class BaseType : public std::enable_shared_from_this<BaseType>
{
public:
	// Accessors
	GRAPHQLSERVICE_EXPORT introspection::TypeKind kind() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual std::string_view name() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view description() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::vector<std::shared_ptr<const Field>>& fields()
		const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::vector<std::shared_ptr<const InterfaceType>>&
	interfaces() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::vector<std::weak_ptr<const BaseType>>& possibleTypes()
		const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::vector<std::shared_ptr<const EnumValue>>& enumValues()
		const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::vector<std::shared_ptr<const InputValue>>&
	inputFields() const noexcept;
	GRAPHQLSERVICE_EXPORT virtual const std::weak_ptr<const BaseType>& ofType() const noexcept;

protected:
	BaseType(introspection::TypeKind kind, std::string_view description);

private:
	const introspection::TypeKind _kind;
	const std::string_view _description;
};

class ScalarType : public BaseType
{
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit ScalarType(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<ScalarType> Make(
		std::string_view name, std::string_view description);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;

private:
	const std::string_view _name;
};

class ObjectType : public BaseType
{
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit ObjectType(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<ObjectType> Make(
		std::string_view name, std::string_view description);

	GRAPHQLSERVICE_EXPORT void AddInterfaces(
		std::initializer_list<std::shared_ptr<const InterfaceType>> interfaces);
	GRAPHQLSERVICE_EXPORT void AddFields(std::initializer_list<std::shared_ptr<Field>> fields);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<const Field>>& fields()
		const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<const InterfaceType>>& interfaces()
		const noexcept final;

private:
	const std::string_view _name;

	std::vector<std::shared_ptr<const InterfaceType>> _interfaces;
	std::vector<std::shared_ptr<const Field>> _fields;
};

class InterfaceType : public BaseType
{
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit InterfaceType(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<InterfaceType> Make(
		std::string_view name, std::string_view description);

	GRAPHQLSERVICE_EXPORT void AddPossibleType(std::weak_ptr<ObjectType> possibleType);
	GRAPHQLSERVICE_EXPORT void AddFields(std::initializer_list<std::shared_ptr<Field>> fields);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<const Field>>& fields()
		const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::weak_ptr<const BaseType>>& possibleTypes()
		const noexcept final;

private:
	const std::string_view _name;

	std::vector<std::shared_ptr<const Field>> _fields;
	std::vector<std::weak_ptr<const BaseType>> _possibleTypes;
};

class UnionType : public BaseType
{
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit UnionType(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<UnionType> Make(
		std::string_view name, std::string_view description);

	GRAPHQLSERVICE_EXPORT void AddPossibleTypes(
		std::initializer_list<std::weak_ptr<const BaseType>> possibleTypes);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::weak_ptr<const BaseType>>& possibleTypes()
		const noexcept final;

private:
	const std::string_view _name;

	std::vector<std::weak_ptr<const BaseType>> _possibleTypes;
};

struct EnumValueType
{
	std::string_view value;
	std::string_view description;
	std::optional<std::string_view> deprecationReason;
};

class EnumType : public BaseType
{
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit EnumType(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<EnumType> Make(
		std::string_view name, std::string_view description);

	GRAPHQLSERVICE_EXPORT void AddEnumValues(std::initializer_list<EnumValueType> enumValues);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<const EnumValue>>& enumValues()
		const noexcept final;

private:
	const std::string_view _name;

	std::vector<std::shared_ptr<const EnumValue>> _enumValues;
};

class InputObjectType : public BaseType
{
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit InputObjectType(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<InputObjectType> Make(
		std::string_view name, std::string_view description);

	GRAPHQLSERVICE_EXPORT void AddInputValues(
		std::initializer_list<std::shared_ptr<InputValue>> inputValues);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept final;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<const InputValue>>& inputFields()
		const noexcept final;

private:
	const std::string_view _name;

	std::vector<std::shared_ptr<const InputValue>> _inputValues;
};

class WrapperType : public BaseType
{
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit WrapperType(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<WrapperType> Make(
		introspection::TypeKind kind, std::weak_ptr<const BaseType> ofType);

	// Accessors
	GRAPHQLSERVICE_EXPORT const std::weak_ptr<const BaseType>& ofType() const noexcept final;

private:
	const std::weak_ptr<const BaseType> _ofType;
};

class Field : public std::enable_shared_from_this<Field>
{
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit Field(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<Field> Make(std::string_view name,
		std::string_view description, std::optional<std::string_view> deprecationReason,
		std::weak_ptr<const BaseType> type,
		std::initializer_list<std::shared_ptr<InputValue>> args = {});

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view description() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<const InputValue>>& args()
		const noexcept;
	GRAPHQLSERVICE_EXPORT const std::weak_ptr<const BaseType>& type() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::optional<std::string_view>& deprecationReason() const noexcept;

private:
	const std::string_view _name;
	const std::string_view _description;
	const std::optional<std::string_view> _deprecationReason;
	const std::weak_ptr<const BaseType> _type;
	const std::vector<std::shared_ptr<const InputValue>> _args;
};

class InputValue : public std::enable_shared_from_this<InputValue>
{
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit InputValue(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<InputValue> Make(std::string_view name,
		std::string_view description, std::weak_ptr<const BaseType> type,
		std::string_view defaultValue);

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view description() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::weak_ptr<const BaseType>& type() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view defaultValue() const noexcept;

private:
	const std::string_view _name;
	const std::string_view _description;
	const std::weak_ptr<const BaseType> _type;
	const std::string_view _defaultValue;
};

class EnumValue : public std::enable_shared_from_this<EnumValue>
{
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit EnumValue(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<EnumValue> Make(std::string_view name,
		std::string_view description, std::optional<std::string_view> deprecationReason);

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
private:
	// Use a private constructor parameter type to enable std::make_shared inside of the static Make
	// method without exposing the constructor as part of the public interface.
	struct init;

public:
	explicit Directive(init&& params);

	GRAPHQLSERVICE_EXPORT static std::shared_ptr<Directive> Make(std::string_view name,
		std::string_view description,
		std::initializer_list<introspection::DirectiveLocation> locations,
		std::initializer_list<std::shared_ptr<InputValue>> args = {});

	// Accessors
	GRAPHQLSERVICE_EXPORT std::string_view name() const noexcept;
	GRAPHQLSERVICE_EXPORT std::string_view description() const noexcept;
	GRAPHQLSERVICE_EXPORT const std::vector<introspection::DirectiveLocation>& locations()
		const noexcept;
	GRAPHQLSERVICE_EXPORT const std::vector<std::shared_ptr<const InputValue>>& args()
		const noexcept;

private:
	const std::string_view _name;
	const std::string_view _description;
	const std::vector<introspection::DirectiveLocation> _locations;
	const std::vector<std::shared_ptr<const InputValue>> _args;
};

} // namespace schema
} // namespace graphql

#endif // GRAPHQLSCHEMA_H
