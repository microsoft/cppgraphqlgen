// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <graphqlservice/IntrospectionSchema.h>

namespace facebook::graphql::introspection {

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

class Schema : public object::__Schema
{
public:
	explicit Schema();

	void AddQueryType(std::shared_ptr<ObjectType> query);
	void AddMutationType(std::shared_ptr<ObjectType> mutation);
	void AddSubscriptionType(std::shared_ptr<ObjectType> subscription);
	void AddType(response::StringType&& name, std::shared_ptr<object::__Type> type);
	const std::shared_ptr<object::__Type>& LookupType(const response::StringType& name) const;
	const std::shared_ptr<object::__Type>& WrapType(__TypeKind kind, const std::shared_ptr<object::__Type>& ofType);
	void AddDirective(std::shared_ptr<object::__Directive> directive);

	// Accessors
	std::future<std::vector<std::shared_ptr<object::__Type>>> getTypes(service::FieldParams&& params) const override;
	std::future<std::shared_ptr<object::__Type>> getQueryType(service::FieldParams&& params) const override;
	std::future<std::shared_ptr<object::__Type>> getMutationType(service::FieldParams&& params) const override;
	std::future<std::shared_ptr<object::__Type>> getSubscriptionType(service::FieldParams&& params) const override;
	std::future<std::vector<std::shared_ptr<object::__Directive>>> getDirectives(service::FieldParams&& params) const override;

private:
	std::shared_ptr<ObjectType> _query;
	std::shared_ptr<ObjectType> _mutation;
	std::shared_ptr<ObjectType> _subscription;
	std::unordered_map<response::StringType, size_t> _typeMap;
	std::vector<std::pair<response::StringType, std::shared_ptr<object::__Type>>> _types;
	std::vector<std::shared_ptr<object::__Directive>> _directives;
	std::unordered_map<std::shared_ptr<object::__Type>, std::shared_ptr<object::__Type>> _nonNullWrappers;
	std::unordered_map<std::shared_ptr<object::__Type>, std::shared_ptr<object::__Type>> _listWrappers;
};

class BaseType : public object::__Type
{
public:
	// Accessors
	std::future<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__Field>>>> getFields(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__Type>>>> getInterfaces(service::FieldParams&& params) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__Type>>>> getPossibleTypes(service::FieldParams&& params) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__EnumValue>>>> getEnumValues(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__InputValue>>>> getInputFields(service::FieldParams&& params) const override;
	std::future<std::shared_ptr<object::__Type>> getOfType(service::FieldParams&& params) const override;

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
	std::future<__TypeKind> getKind(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;

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
	std::future<__TypeKind> getKind(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__Field>>>> getFields(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__Type>>>> getInterfaces(service::FieldParams&& params) const override;

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
	std::future<__TypeKind> getKind(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__Field>>>> getFields(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const override;

private:
	const response::StringType _name;

	std::vector<std::shared_ptr<Field>> _fields;
};

class UnionType : public BaseType
{
public:
	explicit UnionType(response::StringType&& name, response::StringType&& description);

	void AddPossibleTypes(std::vector<std::weak_ptr<object::__Type>> possibleTypes);

	// Accessors
	std::future<__TypeKind> getKind(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__Type>>>> getPossibleTypes(service::FieldParams&& params) const override;

private:
	const response::StringType _name;

	std::vector<std::weak_ptr<object::__Type>> _possibleTypes;
};

struct EnumValueType
{
	response::StringType value;
	response::StringType description;
	const char* deprecationReason;
};

class EnumType : public BaseType
{
public:
	explicit EnumType(response::StringType&& name, response::StringType&& description);

	void AddEnumValues(std::vector<EnumValueType> enumValues);

	// Accessors
	std::future<__TypeKind> getKind(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__EnumValue>>>> getEnumValues(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const override;

private:
	const response::StringType _name;
	
	std::vector<std::shared_ptr<object::__EnumValue>> _enumValues;
};

class InputObjectType : public BaseType
{
public:
	explicit InputObjectType(response::StringType&& name, response::StringType&& description);

	void AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues);

	// Accessors
	std::future<__TypeKind> getKind(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getName(service::FieldParams&& params) const override;
	std::future<std::optional<std::vector<std::shared_ptr<object::__InputValue>>>> getInputFields(service::FieldParams&& params) const override;

private:
	const response::StringType _name;
	
	std::vector<std::shared_ptr<InputValue>> _inputValues;
};

class WrapperType : public BaseType
{
public:
	explicit WrapperType(__TypeKind kind, const std::shared_ptr<object::__Type>& ofType);

	// Accessors
	std::future<__TypeKind> getKind(service::FieldParams&& params) const override;
	std::future<std::shared_ptr<object::__Type>> getOfType(service::FieldParams&& params) const override;

private:
	const __TypeKind _kind;
	const std::weak_ptr<object::__Type> _ofType;
};

class Field : public object::__Field
{
public:
	explicit Field(response::StringType&& name, response::StringType&& description, std::optional<response::StringType>&& deprecationReason, std::vector<std::shared_ptr<InputValue>>&& args, const std::shared_ptr<object::__Type>& type);

	// Accessors
	std::future<response::StringType> getName(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const override;
	std::future<std::vector<std::shared_ptr<object::__InputValue>>> getArgs(service::FieldParams&& params) const override;
	std::future<std::shared_ptr<object::__Type>> getType(service::FieldParams&& params) const override;
	std::future<response::BooleanType> getIsDeprecated(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getDeprecationReason(service::FieldParams&& params) const override;

private:
	const response::StringType _name;
	const response::StringType _description;
	const std::optional<response::StringType> _deprecationReason;
	const std::vector<std::shared_ptr<InputValue>> _args;
	const std::weak_ptr<object::__Type> _type;
};

class InputValue : public object::__InputValue
{
public:
	explicit InputValue(response::StringType&& name, response::StringType&& description, const std::shared_ptr<object::__Type>& type, response::StringType&& defaultValue);

	// Accessors
	std::future<response::StringType> getName(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const override;
	std::future<std::shared_ptr<object::__Type>> getType(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getDefaultValue(service::FieldParams&& params) const override;

private:
	const response::StringType _name;
	const response::StringType _description;
	const std::weak_ptr<object::__Type> _type;
	const response::StringType _defaultValue;
};

class EnumValue : public object::__EnumValue
{
public:
	explicit EnumValue(response::StringType&& name, response::StringType&& description, std::optional<response::StringType>&& deprecationReason);

	// Accessors
	std::future<response::StringType> getName(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const override;
	std::future<response::BooleanType> getIsDeprecated(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getDeprecationReason(service::FieldParams&& params) const override;

private:
	const response::StringType _name;
	const response::StringType _description;
	const std::optional<response::StringType> _deprecationReason;
};

class Directive : public object::__Directive
{
public:
	explicit Directive(response::StringType&& name, response::StringType&& description, std::vector<response::StringType>&& locations, std::vector<std::shared_ptr<InputValue>>&& args);

	// Accessors
	std::future<response::StringType> getName(service::FieldParams&& params) const override;
	std::future<std::optional<response::StringType>> getDescription(service::FieldParams&& params) const override;
	std::future<std::vector<__DirectiveLocation>> getLocations(service::FieldParams&& params) const override;
	std::future<std::vector<std::shared_ptr<object::__InputValue>>> getArgs(service::FieldParams&& params) const override;

private:
	const response::StringType _name;
	const response::StringType _description;
	const std::vector<__DirectiveLocation> _locations;
	const std::vector<std::shared_ptr<InputValue>> _args;
};

} /* namespace facebook::graphql::introspection */
