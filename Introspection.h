// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "IntrospectionSchema.h"

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
	std::vector<std::shared_ptr<object::__Type>> getTypes() const override;
	std::shared_ptr<object::__Type> getQueryType() const override;
	std::shared_ptr<object::__Type> getMutationType() const override;
	std::shared_ptr<object::__Type> getSubscriptionType() const override;
	std::vector<std::shared_ptr<object::__Directive>> getDirectives() const override;

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
	std::unique_ptr<std::string> getName() const override;
	std::unique_ptr<std::string> getDescription() const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> getFields(std::unique_ptr<bool> includeDeprecated) const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> getInterfaces() const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> getPossibleTypes() const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>> getEnumValues(std::unique_ptr<bool> includeDeprecated) const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>> getInputFields() const override;
	std::shared_ptr<object::__Type> getOfType() const override;

protected:
	BaseType() = default;
};

class ScalarType : public BaseType
{
public:
	explicit ScalarType(std::string name);

	// Accessors
	__TypeKind getKind() const override;
	std::unique_ptr<std::string> getName() const override;

private:
	const std::string _name;
};

class ObjectType : public BaseType
{
public:
	explicit ObjectType(std::string name);

	void AddInterfaces(std::vector<std::shared_ptr<InterfaceType>> interfaces);
	void AddFields(std::vector<std::shared_ptr<Field>> fields);

	// Accessors
	__TypeKind getKind() const override;
	std::unique_ptr<std::string> getName() const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> getFields(std::unique_ptr<bool> includeDeprecated) const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> getInterfaces() const override;

private:
	const std::string _name;
	
	std::vector<std::shared_ptr<InterfaceType>> _interfaces;
	std::vector<std::shared_ptr<Field>> _fields;
};

class InterfaceType : public BaseType
{
public:
	explicit InterfaceType(std::string name);

	void AddFields(std::vector<std::shared_ptr<Field>> fields);

	// Accessors
	__TypeKind getKind() const override;
	std::unique_ptr<std::string> getName() const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> getFields(std::unique_ptr<bool> includeDeprecated) const override;

private:
	const std::string _name;

	std::vector<std::shared_ptr<Field>> _fields;
};

class UnionType : public BaseType
{
public:
	explicit UnionType(std::string name);

	void AddPossibleTypes(std::vector<std::shared_ptr<object::__Type>> possibleTypes);

	// Accessors
	__TypeKind getKind() const override;
	std::unique_ptr<std::string> getName() const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> getPossibleTypes() const override;

private:
	const std::string _name;

	std::vector<std::shared_ptr<object::__Type>> _possibleTypes;
};

class EnumType : public BaseType
{
public:
	explicit EnumType(std::string name);

	void AddEnumValues(std::vector<std::string> enumValues);

	// Accessors
	__TypeKind getKind() const override;
	std::unique_ptr<std::string> getName() const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>> getEnumValues(std::unique_ptr<bool> includeDeprecated) const override;

private:
	const std::string _name;
	
	std::vector<std::shared_ptr<object::__EnumValue>> _enumValues;
};

class InputObjectType : public BaseType
{
public:
	explicit InputObjectType(std::string name);

	void AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues);

	// Accessors
	__TypeKind getKind() const override;
	std::unique_ptr<std::string> getName() const override;
	std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>> getInputFields() const override;

private:
	const std::string _name;
	
	std::vector<std::shared_ptr<InputValue>> _inputValues;
};

class WrapperType : public BaseType
{
public:
	explicit WrapperType(__TypeKind kind, std::shared_ptr<object::__Type> ofType);

	// Accessors
	__TypeKind getKind() const override;
	std::shared_ptr<object::__Type> getOfType() const override;

private:
	const __TypeKind _kind;
	const std::shared_ptr<object::__Type> _ofType;
};

class Field : public object::__Field
{
public:
	explicit Field(std::string name, std::vector<std::shared_ptr<InputValue>> args, std::shared_ptr<object::__Type> type);

	// Accessors
	std::string getName() const override;
	std::unique_ptr<std::string> getDescription() const override;
	std::vector<std::shared_ptr<object::__InputValue>> getArgs() const override;
	std::shared_ptr<object::__Type> getType() const override;
	bool getIsDeprecated() const override;
	std::unique_ptr<std::string> getDeprecationReason() const override;

private:
	const std::string _name;
	const std::vector<std::shared_ptr<InputValue>> _args;
	const std::shared_ptr<object::__Type> _type;
};

class InputValue : public object::__InputValue
{
public:
	explicit InputValue(std::string name, std::shared_ptr<object::__Type> type, const web::json::value& defaultValue);

	// Accessors
	std::string getName() const override;
	std::unique_ptr<std::string> getDescription() const override;
	std::shared_ptr<object::__Type> getType() const override;
	std::unique_ptr<std::string> getDefaultValue() const override;

private:
	static std::string formatDefaultValue(const web::json::value& defaultValue) noexcept;

	const std::string _name;
	const std::shared_ptr<object::__Type> _type;
	const std::string _defaultValue;
};

class EnumValue : public object::__EnumValue
{
public:
	explicit EnumValue(std::string name);

	// Accessors
	std::string getName() const override;
	std::unique_ptr<std::string> getDescription() const override;
	bool getIsDeprecated() const override;
	std::unique_ptr<std::string> getDeprecationReason() const override;

private:
	const std::string _name;
};

} /* namespace facebook */
} /* namespace graphql */
} /* namespace introspection */