// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLSchema.h"
#include "graphqlservice/IntrospectionSchema.h"

using namespace std::literals;

namespace graphql::schema {

Schema::Schema(bool noIntrospection)
	: _noIntrospection(noIntrospection)
{
}

void Schema::AddQueryType(std::shared_ptr<ObjectType> query)
{
	_query = std::move(query);
}

void Schema::AddMutationType(std::shared_ptr<ObjectType> mutation)
{
	_mutation = std::move(mutation);
}

void Schema::AddSubscriptionType(std::shared_ptr<ObjectType> subscription)
{
	_subscription = std::move(subscription);
}

void Schema::AddType(std::string_view name, std::shared_ptr<BaseType> type)
{
	_typeMap[name] = _types.size();
	_types.push_back({ name, std::move(type) });
}

bool Schema::supportsIntrospection() const noexcept
{
	return !_noIntrospection;
}

const std::shared_ptr<BaseType>& Schema::LookupType(std::string_view name) const
{
	auto itr = _typeMap.find(name);

	if (itr == _typeMap.cend())
	{
		std::ostringstream message;

		message << "Type not found";

		if (!name.empty())
		{
			message << " name: " << name;
		}

		throw service::schema_exception { { message.str() } };
	}

	return _types[itr->second].second;
}

const std::shared_ptr<BaseType>& Schema::WrapType(
	introspection::TypeKind kind, const std::shared_ptr<BaseType>& ofType)
{
	auto& wrappers = (kind == introspection::TypeKind::LIST) ? _listWrappers : _nonNullWrappers;
	auto itr = wrappers.find(ofType);

	if (itr == wrappers.cend())
	{
		std::tie(itr, std::ignore) =
			wrappers.insert({ ofType, std::make_shared<WrapperType>(kind, ofType) });
	}

	return itr->second;
}

void Schema::AddDirective(std::shared_ptr<Directive> directive)
{
	_directives.emplace_back(std::move(directive));
}

const std::vector<std::pair<std::string_view, std::shared_ptr<BaseType>>>& Schema::types()
	const noexcept
{
	return _types;
}

const std::shared_ptr<ObjectType>& Schema::queryType() const noexcept
{
	return _query;
}

const std::shared_ptr<ObjectType>& Schema::mutationType() const noexcept
{
	return _mutation;
}

const std::shared_ptr<ObjectType>& Schema::subscriptionType() const noexcept
{
	return _subscription;
}

const std::vector<std::shared_ptr<Directive>>& Schema::directives() const noexcept
{
	return _directives;
}

BaseType::BaseType(introspection::TypeKind kind, std::string_view description)
	: _kind(kind)
	, _description(description)
{
}

introspection::TypeKind BaseType::kind() const noexcept
{
	return _kind;
}

std::string_view BaseType::name() const noexcept
{
	return ""sv;
}

std::string_view BaseType::description() const noexcept
{
	return _description;
}

const std::vector<std::shared_ptr<Field>>& BaseType::fields() const noexcept
{
	static const std::vector<std::shared_ptr<Field>> defaultValue {};
	return defaultValue;
}

const std::vector<std::shared_ptr<InterfaceType>>& BaseType::interfaces() const noexcept
{
	static const std::vector<std::shared_ptr<InterfaceType>> defaultValue {};
	return defaultValue;
}

const std::vector<std::weak_ptr<BaseType>>& BaseType::possibleTypes() const noexcept
{
	static const std::vector<std::weak_ptr<BaseType>> defaultValue {};
	return defaultValue;
}

const std::vector<std::shared_ptr<EnumValue>>& BaseType::enumValues() const noexcept
{
	static const std::vector<std::shared_ptr<EnumValue>> defaultValue {};
	return defaultValue;
}

const std::vector<std::shared_ptr<InputValue>>& BaseType::inputFields() const noexcept
{
	static const std::vector<std::shared_ptr<InputValue>> defaultValue {};
	return defaultValue;
}

const std::weak_ptr<BaseType>& BaseType::ofType() const noexcept
{
	static const std::weak_ptr<BaseType> defaultValue;
	return defaultValue;
}

ScalarType::ScalarType(std::string_view name, std::string_view description)
	: BaseType(introspection::TypeKind::SCALAR, description)
	, _name(name)
{
}

std::string_view ScalarType::name() const noexcept
{
	return _name;
}

ObjectType::ObjectType(std::string_view name, std::string_view description)
	: BaseType(introspection::TypeKind::OBJECT, description)
	, _name(name)
{
}

void ObjectType::AddInterfaces(std::vector<std::shared_ptr<InterfaceType>> interfaces)
{
	_interfaces = std::move(interfaces);

	for (const auto& interface : _interfaces)
	{
		interface->AddPossibleType(std::static_pointer_cast<ObjectType>(shared_from_this()));
	}
}

void ObjectType::AddFields(std::vector<std::shared_ptr<Field>> fields)
{
	_fields = std::move(fields);
}

std::string_view ObjectType::name() const noexcept
{
	return _name;
}

const std::vector<std::shared_ptr<Field>>& ObjectType::fields() const noexcept
{
	return _fields;
}

const std::vector<std::shared_ptr<InterfaceType>>& ObjectType::interfaces() const noexcept
{
	return _interfaces;
}

InterfaceType::InterfaceType(std::string_view name, std::string_view description)
	: BaseType(introspection::TypeKind::INTERFACE, description)
	, _name(name)
{
}

void InterfaceType::AddPossibleType(std::weak_ptr<ObjectType> possibleType)
{
	_possibleTypes.push_back(possibleType);
}

void InterfaceType::AddFields(std::vector<std::shared_ptr<Field>> fields)
{
	_fields = std::move(fields);
}

std::string_view InterfaceType::name() const noexcept
{
	return _name;
}

const std::vector<std::shared_ptr<Field>>& InterfaceType::fields() const noexcept
{
	return _fields;
}

const std::vector<std::weak_ptr<BaseType>>& InterfaceType::possibleTypes() const noexcept
{
	return _possibleTypes;
}

UnionType::UnionType(std::string_view name, std::string_view description)
	: BaseType(introspection::TypeKind::UNION, description)
	, _name(name)
{
}

void UnionType::AddPossibleTypes(std::vector<std::weak_ptr<BaseType>> possibleTypes)
{
	_possibleTypes = std::move(possibleTypes);
}

std::string_view UnionType::name() const noexcept
{
	return _name;
}

const std::vector<std::weak_ptr<BaseType>>& UnionType::possibleTypes() const noexcept
{
	return _possibleTypes;
}

EnumType::EnumType(std::string_view name, std::string_view description)
	: BaseType(introspection::TypeKind::ENUM, description)
	, _name(name)
{
}

void EnumType::AddEnumValues(std::vector<EnumValueType> enumValues)
{
	_enumValues.reserve(_enumValues.size() + enumValues.size());

	for (auto& value : enumValues)
	{
		_enumValues.push_back(
			std::make_shared<EnumValue>(value.value, value.description, value.deprecationReason));
	}
}

std::string_view EnumType::name() const noexcept
{
	return _name;
}

const std::vector<std::shared_ptr<EnumValue>>& EnumType::enumValues() const noexcept
{
	return _enumValues;
}

InputObjectType::InputObjectType(std::string_view name, std::string_view description)
	: BaseType(introspection::TypeKind::INPUT_OBJECT, description)
	, _name(name)
{
}

void InputObjectType::AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues)
{
	_inputValues = std::move(inputValues);
}

std::string_view InputObjectType::name() const noexcept
{
	return _name;
}

const std::vector<std::shared_ptr<InputValue>>& InputObjectType::inputFields() const noexcept
{
	return _inputValues;
}

WrapperType::WrapperType(introspection::TypeKind kind, const std::shared_ptr<BaseType>& ofType)
	: BaseType(kind, std::string_view())
	, _ofType(ofType)
{
}

const std::weak_ptr<BaseType>& WrapperType::ofType() const noexcept
{
	return _ofType;
}

Field::Field(std::string_view name, std::string_view description,
	std::optional<std::string_view> deprecationReason,
	std::vector<std::shared_ptr<InputValue>>&& args, const std::shared_ptr<BaseType>& type)
	: _name(name)
	, _description(description)
	, _deprecationReason(deprecationReason)
	, _args(std::move(args))
	, _type(type)
{
}

std::string_view Field::name() const noexcept
{
	return _name;
}

std::string_view Field::description() const noexcept
{
	return _description;
}

const std::vector<std::shared_ptr<InputValue>>& Field::args() const noexcept
{
	return _args;
}

const std::weak_ptr<BaseType>& Field::type() const noexcept
{
	return _type;
}

const std::optional<std::string_view>& Field::deprecationReason() const noexcept
{
	return _deprecationReason;
}

InputValue::InputValue(std::string_view name, std::string_view description,
	const std::shared_ptr<BaseType>& type, std::string_view defaultValue)
	: _name(name)
	, _description(description)
	, _type(type)
	, _defaultValue(defaultValue)
{
}

std::string_view InputValue::name() const noexcept
{
	return _name;
}

std::string_view InputValue::description() const noexcept
{
	return _description;
}

const std::weak_ptr<BaseType>& InputValue::type() const noexcept
{
	return _type;
}

std::string_view InputValue::defaultValue() const noexcept
{
	return _defaultValue;
}

EnumValue::EnumValue(std::string_view name, std::string_view description,
	std::optional<std::string_view> deprecationReason)
	: _name(name)
	, _description(description)
	, _deprecationReason(deprecationReason)
{
}

std::string_view EnumValue::name() const noexcept
{
	return _name;
}

std::string_view EnumValue::description() const noexcept
{
	return _description;
}

const std::optional<std::string_view>& EnumValue::deprecationReason() const noexcept
{
	return _deprecationReason;
}

Directive::Directive(std::string_view name, std::string_view description,
	std::vector<introspection::DirectiveLocation>&& locations,
	std::vector<std::shared_ptr<InputValue>>&& args)
	: _name(name)
	, _description(description)
	, _locations(std::move(locations))
	, _args(std::move(args))
{
}

std::string_view Directive::name() const noexcept
{
	return _name;
}

std::string_view Directive::description() const noexcept
{
	return _description;
}

const std::vector<introspection::DirectiveLocation>& Directive::locations() const noexcept
{
	return _locations;
}

const std::vector<std::shared_ptr<InputValue>>& Directive::args() const noexcept
{
	return _args;
}

} // namespace graphql::schema
