// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef INTROSPECTION_H
#define INTROSPECTION_H

#include <graphqlservice/IntrospectionSchema.h>

namespace graphql::introspection {

class Schema;
class Directive;
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

class Schema : public object::Schema
{
public:
	explicit Schema();

	void AddQueryType(std::shared_ptr<ObjectType> query);
	void AddMutationType(std::shared_ptr<ObjectType> mutation);
	void AddSubscriptionType(std::shared_ptr<ObjectType> subscription);
	void AddType(response::StringType&& name, std::shared_ptr<object::Type> type);
	const std::shared_ptr<object::Type>& LookupType(const response::StringType& name) const;
	const std::shared_ptr<object::Type>& WrapType(TypeKind kind, const std::shared_ptr<object::Type>& ofType);
	void AddDirective(std::shared_ptr<object::Directive> directive);

	// Accessors
	service::FieldResult<std::vector<std::shared_ptr<object::Type>>> getTypes(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getQueryType(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getMutationType(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getSubscriptionType(service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Directive>>> getDirectives(service::FieldParams&& params) const override;

private:
	std::shared_ptr<ObjectType> _query;
	std::shared_ptr<ObjectType> _mutation;
	std::shared_ptr<ObjectType> _subscription;
	std::unordered_map<response::StringType, size_t> _typeMap;
	std::vector<std::pair<response::StringType, std::shared_ptr<object::Type>>> _types;
	std::vector<std::shared_ptr<object::Directive>> _directives;
	std::unordered_map<std::shared_ptr<object::Type>, std::shared_ptr<object::Type>> _nonNullWrappers;
	std::unordered_map<std::shared_ptr<object::Type>, std::shared_ptr<object::Type>> _listWrappers;
};

class BaseType : public object::Type
{
public:
	// Accessors
	service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Field>>>> getFields(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> getInterfaces(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> getPossibleTypes(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::EnumValue>>>> getEnumValues(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::InputValue>>>> getInputFields(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getOfType(service::FieldParams&& params) const override;

protected:
	BaseType(response::StringType&& description);

private:
	const response::StringType _description;
};

class ScalarType : public BaseType
{
public:
	explicit ScalarType(response::StringType&& name, response::StringType&& description);

	// Accessors
	service::FieldResult<TypeKind> getKind(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;

private:
	const response::StringType _name;
};

class ObjectType : public BaseType
{
public:
	explicit ObjectType(response::StringType&& name, response::StringType&& description);

	void AddInterfaces(std::vector<std::shared_ptr<InterfaceType>> interfaces);
	void AddFields(std::vector<std::shared_ptr<Field>> fields);

	// Accessors
	service::FieldResult<TypeKind> getKind(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Field>>>> getFields(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> getInterfaces(service::FieldParams&& params) const override;

private:
	const response::StringType _name;

	std::vector<std::shared_ptr<InterfaceType>> _interfaces;
	std::vector<std::shared_ptr<Field>> _fields;
};

class InterfaceType : public BaseType
{
public:
	explicit InterfaceType(response::StringType&& name, response::StringType&& description);

	void AddFields(std::vector<std::shared_ptr<Field>> fields);

	// Accessors
	service::FieldResult<TypeKind> getKind(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Field>>>> getFields(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const override;

private:
	const response::StringType _name;

	std::vector<std::shared_ptr<Field>> _fields;
};

class UnionType : public BaseType
{
public:
	explicit UnionType(response::StringType&& name, response::StringType&& description);

	void AddPossibleTypes(std::vector<std::weak_ptr<object::Type>> possibleTypes);

	// Accessors
	service::FieldResult<TypeKind> getKind(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> getPossibleTypes(service::FieldParams&& params) const override;

private:
	const response::StringType _name;

	std::vector<std::weak_ptr<object::Type>> _possibleTypes;
};

struct EnumValueType
{
	response::StringType value;
	response::StringType description;
	std::optional<response::StringType> deprecationReason;
};

class EnumType : public BaseType
{
public:
	explicit EnumType(response::StringType&& name, response::StringType&& description);

	void AddEnumValues(std::vector<EnumValueType> enumValues);

	// Accessors
	service::FieldResult<TypeKind> getKind(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::EnumValue>>>> getEnumValues(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const override;

private:
	const response::StringType _name;

	std::vector<std::shared_ptr<object::EnumValue>> _enumValues;
};

class InputObjectType : public BaseType
{
public:
	explicit InputObjectType(response::StringType&& name, response::StringType&& description);

	void AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues);

	// Accessors
	service::FieldResult<TypeKind> getKind(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::InputValue>>>> getInputFields(service::FieldParams&& params) const override;

private:
	const response::StringType _name;

	std::vector<std::shared_ptr<InputValue>> _inputValues;
};

class WrapperType : public BaseType
{
public:
	explicit WrapperType(TypeKind kind, const std::shared_ptr<object::Type>& ofType);

	// Accessors
	service::FieldResult<TypeKind> getKind(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getOfType(service::FieldParams&& params) const override;

private:
	const TypeKind _kind;
	const std::weak_ptr<object::Type> _ofType;
};

class Field : public object::Field
{
public:
	explicit Field(response::StringType&& name, response::StringType&& description, std::optional<response::StringType>&& deprecationReason, std::vector<std::shared_ptr<InputValue>>&& args, const std::shared_ptr<object::Type>& type);

	// Accessors
	service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::InputValue>>> getArgs(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getType(service::FieldParams&& params) const override;
	service::FieldResult<response::BooleanType> getIsDeprecated(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDeprecationReason(service::FieldParams&& params) const override;

private:
	const response::StringType _name;
	const response::StringType _description;
	const std::optional<response::StringType> _deprecationReason;
	const std::vector<std::shared_ptr<InputValue>> _args;
	const std::weak_ptr<object::Type> _type;
};

class InputValue : public object::InputValue
{
public:
	explicit InputValue(response::StringType&& name, response::StringType&& description, const std::shared_ptr<object::Type>& type, response::StringType&& defaultValue);

	// Accessors
	service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::Type>> getType(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDefaultValue(service::FieldParams&& params) const override;

private:
	const response::StringType _name;
	const response::StringType _description;
	const std::weak_ptr<object::Type> _type;
	const response::StringType _defaultValue;
};

class EnumValue : public object::EnumValue
{
public:
	explicit EnumValue(response::StringType&& name, response::StringType&& description, std::optional<response::StringType>&& deprecationReason);

	// Accessors
	service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const override;
	service::FieldResult<response::BooleanType> getIsDeprecated(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDeprecationReason(service::FieldParams&& params) const override;

private:
	const response::StringType _name;
	const response::StringType _description;
	const std::optional<response::StringType> _deprecationReason;
};

class Directive : public object::Directive
{
public:
	explicit Directive(response::StringType&& name, response::StringType&& description, std::vector<response::StringType>&& locations, std::vector<std::shared_ptr<InputValue>>&& args);

	// Accessors
	service::FieldResult<response::StringType> getName(service::FieldParams&& params) const override;
	service::FieldResult<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const override;
	service::FieldResult<std::vector<DirectiveLocation>> getLocations(service::FieldParams&& params) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::InputValue>>> getArgs(service::FieldParams&& params) const override;

private:
	const response::StringType _name;
	const response::StringType _description;
	const std::vector<DirectiveLocation> _locations;
	const std::vector<std::shared_ptr<InputValue>> _args;
};

} /* namespace graphql::introspection */

#endif // INTROSPECTION_H
