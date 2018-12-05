// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "graphqlservice/IntrospectionSchema.h"

namespace facebook {
namespace graphql {
namespace introspection {

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
	void AddType(std::string name, std::shared_ptr<object::__Type> type);
	std::shared_ptr<object::__Type> LookupType(const std::string& name) const;

	// Accessors
	std::future<std::vector<std::shared_ptr<object::__Type>>> getTypes(service::RequestId requestId) const override;
	std::future<std::shared_ptr<object::__Type>> getQueryType(service::RequestId requestId) const override;
	std::future<std::shared_ptr<object::__Type>> getMutationType(service::RequestId requestId) const override;
	std::future<std::shared_ptr<object::__Type>> getSubscriptionType(service::RequestId requestId) const override;
	std::future<std::vector<std::shared_ptr<object::__Directive>>> getDirectives(service::RequestId requestId) const override;

private:
	std::shared_ptr<ObjectType> _query;
	std::shared_ptr<ObjectType> _mutation;
	std::shared_ptr<ObjectType> _subscription;
	std::unordered_map<std::string, size_t> _typeMap;
	std::vector<std::pair<std::string, std::shared_ptr<object::__Type>>> _types;
};

class BaseType : public object::__Type
{
public:
	// Accessors
	std::future<std::unique_ptr<std::string>> getName(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getDescription(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> getFields(service::RequestId requestId, std::unique_ptr<bool>&& includeDeprecated) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> getInterfaces(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> getPossibleTypes(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> getEnumValues(service::RequestId requestId, std::unique_ptr<bool>&& includeDeprecated) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> getInputFields(service::RequestId requestId) const override;
	std::future<std::shared_ptr<object::__Type>> getOfType(service::RequestId requestId) const override;

protected:
	BaseType(std::string description);

private:
	const std::string _description;
};

class ScalarType : public BaseType
{
public:
	explicit ScalarType(std::string name, std::string description);

	// Accessors
	std::future<__TypeKind> getKind(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getName(service::RequestId requestId) const override;

private:
	const std::string _name;
};

class ObjectType : public BaseType
{
public:
	explicit ObjectType(std::string name, std::string description);

	void AddInterfaces(std::vector<std::shared_ptr<InterfaceType>> interfaces);
	void AddFields(std::vector<std::shared_ptr<Field>> fields);

	// Accessors
	std::future<__TypeKind> getKind(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getName(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> getFields(service::RequestId requestId, std::unique_ptr<bool>&& includeDeprecated) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> getInterfaces(service::RequestId requestId) const override;

private:
	const std::string _name;
	
	std::vector<std::shared_ptr<InterfaceType>> _interfaces;
	std::vector<std::shared_ptr<Field>> _fields;
};

class InterfaceType : public BaseType
{
public:
	explicit InterfaceType(std::string name, std::string description);

	void AddFields(std::vector<std::shared_ptr<Field>> fields);

	// Accessors
	std::future<__TypeKind> getKind(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getName(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> getFields(service::RequestId requestId, std::unique_ptr<bool>&& includeDeprecated) const override;

private:
	const std::string _name;

	std::vector<std::shared_ptr<Field>> _fields;
};

class UnionType : public BaseType
{
public:
	explicit UnionType(std::string name, std::string description);

	void AddPossibleTypes(std::vector<std::shared_ptr<object::__Type>> possibleTypes);

	// Accessors
	std::future<__TypeKind> getKind(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getName(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> getPossibleTypes(service::RequestId requestId) const override;

private:
	const std::string _name;

	std::vector<std::weak_ptr<object::__Type>> _possibleTypes;
};

struct EnumValueType
{
	std::string value;
	std::string description;
	const char* deprecationReason;
};

class EnumType : public BaseType
{
public:
	explicit EnumType(std::string name, std::string description);

	void AddEnumValues(std::vector<EnumValueType> enumValues);

	// Accessors
	std::future<__TypeKind> getKind(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getName(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> getEnumValues(service::RequestId requestId, std::unique_ptr<bool>&& includeDeprecated) const override;

private:
	const std::string _name;
	
	std::vector<std::shared_ptr<object::__EnumValue>> _enumValues;
};

class InputObjectType : public BaseType
{
public:
	explicit InputObjectType(std::string name, std::string description);

	void AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues);

	// Accessors
	std::future<__TypeKind> getKind(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getName(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> getInputFields(service::RequestId requestId) const override;

private:
	const std::string _name;
	
	std::vector<std::shared_ptr<InputValue>> _inputValues;
};

class WrapperType : public BaseType
{
public:
	explicit WrapperType(__TypeKind kind, std::shared_ptr<object::__Type> ofType);

	// Accessors
	std::future<__TypeKind> getKind(service::RequestId requestId) const override;
	std::future<std::shared_ptr<object::__Type>> getOfType(service::RequestId requestId) const override;

private:
	const __TypeKind _kind;
	const std::weak_ptr<object::__Type> _ofType;
};

class Field : public object::__Field
{
public:
	explicit Field(std::string name, std::string description, std::unique_ptr<std::string>&& deprecationReason, std::vector<std::shared_ptr<InputValue>> args, std::shared_ptr<object::__Type> type);

	// Accessors
	std::future<std::string> getName(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getDescription(service::RequestId requestId) const override;
	std::future<std::vector<std::shared_ptr<object::__InputValue>>> getArgs(service::RequestId requestId) const override;
	std::future<std::shared_ptr<object::__Type>> getType(service::RequestId requestId) const override;
	std::future<bool> getIsDeprecated(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getDeprecationReason(service::RequestId requestId) const override;

private:
	const std::string _name;
	const std::string _description;
	const std::unique_ptr<std::string> _deprecationReason;
	const std::vector<std::shared_ptr<InputValue>> _args;
	const std::weak_ptr<object::__Type> _type;
};

class InputValue : public object::__InputValue
{
public:
	explicit InputValue(std::string name, std::string description, std::shared_ptr<object::__Type> type, std::string defaultValue);

	// Accessors
	std::future<std::string> getName(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getDescription(service::RequestId requestId) const override;
	std::future<std::shared_ptr<object::__Type>> getType(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getDefaultValue(service::RequestId requestId) const override;

private:
	const std::string _name;
	const std::string _description;
	const std::weak_ptr<object::__Type> _type;
	const std::string _defaultValue;
};

class EnumValue : public object::__EnumValue
{
public:
	explicit EnumValue(std::string name, std::string description, std::unique_ptr<std::string>&& deprecationReason);

	// Accessors
	std::future<std::string> getName(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getDescription(service::RequestId requestId) const override;
	std::future<bool> getIsDeprecated(service::RequestId requestId) const override;
	std::future<std::unique_ptr<std::string>> getDeprecationReason(service::RequestId requestId) const override;

private:
	const std::string _name;
	const std::string _description;
	const std::unique_ptr<std::string> _deprecationReason;
};

} /* namespace facebook */
} /* namespace graphql */
} /* namespace introspection */
