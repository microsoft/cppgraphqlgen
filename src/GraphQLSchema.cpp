// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLSchema.h"
#include "graphqlservice/introspection/IntrospectionSchema.h"

using namespace std::literals;

namespace graphql::schema {

Schema::Schema(bool noIntrospection)
	: _noIntrospection(noIntrospection)
{
}

void Schema::AddQueryType(std::shared_ptr<ObjectType> query)
{
	_query = query;
}

void Schema::AddMutationType(std::shared_ptr<ObjectType> mutation)
{
	_mutation = mutation;
}

void Schema::AddSubscriptionType(std::shared_ptr<ObjectType> subscription)
{
	_subscription = subscription;
}

void Schema::AddType(std::string_view name, std::shared_ptr<BaseType> type)
{
	_typeMap[name] = _types.size();
	_types.push_back({ name, type });
}

bool Schema::supportsIntrospection() const noexcept
{
	return !_noIntrospection;
}

const std::shared_ptr<const BaseType>& Schema::LookupType(std::string_view name) const
{
	auto itr = _typeMap.find(name);

	if (itr == _typeMap.end())
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

std::shared_ptr<const BaseType> Schema::WrapType(
	introspection::TypeKind kind, std::shared_ptr<const BaseType> ofType)
{
	auto& wrappers = (kind == introspection::TypeKind::LIST) ? _listWrappers : _nonNullWrappers;
	auto& mutex =
		(kind == introspection::TypeKind::LIST) ? _listWrappersMutex : _nonNullWrappersMutex;
	std::shared_lock shared_lock { mutex };
	std::unique_lock unique_lock { mutex, std::defer_lock };
	auto itr = wrappers.find(ofType);

	if (itr == wrappers.end())
	{
		// Trade the shared_lock for a unique_lock.
		shared_lock.unlock();
		unique_lock.lock();

		std::tie(itr, std::ignore) = wrappers.emplace(ofType, WrapperType::Make(kind, ofType));
	}

	return itr->second;
}

void Schema::AddDirective(std::shared_ptr<Directive> directive)
{
	_directives.emplace_back(std::move(directive));
}

const std::vector<std::pair<std::string_view, std::shared_ptr<const BaseType>>>& Schema::types()
	const noexcept
{
	return _types;
}

const std::shared_ptr<const ObjectType>& Schema::queryType() const noexcept
{
	return _query;
}

const std::shared_ptr<const ObjectType>& Schema::mutationType() const noexcept
{
	return _mutation;
}

const std::shared_ptr<const ObjectType>& Schema::subscriptionType() const noexcept
{
	return _subscription;
}

const std::vector<std::shared_ptr<const Directive>>& Schema::directives() const noexcept
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

const std::vector<std::shared_ptr<const Field>>& BaseType::fields() const noexcept
{
	static const std::vector<std::shared_ptr<const Field>> defaultValue {};
	return defaultValue;
}

const std::vector<std::shared_ptr<const InterfaceType>>& BaseType::interfaces() const noexcept
{
	static const std::vector<std::shared_ptr<const InterfaceType>> defaultValue {};
	return defaultValue;
}

const std::vector<std::weak_ptr<const BaseType>>& BaseType::possibleTypes() const noexcept
{
	static const std::vector<std::weak_ptr<const BaseType>> defaultValue {};
	return defaultValue;
}

const std::vector<std::shared_ptr<const EnumValue>>& BaseType::enumValues() const noexcept
{
	static const std::vector<std::shared_ptr<const EnumValue>> defaultValue {};
	return defaultValue;
}

const std::vector<std::shared_ptr<const InputValue>>& BaseType::inputFields() const noexcept
{
	static const std::vector<std::shared_ptr<const InputValue>> defaultValue {};
	return defaultValue;
}

const std::weak_ptr<const BaseType>& BaseType::ofType() const noexcept
{
	static const std::weak_ptr<const BaseType> defaultValue;
	return defaultValue;
}

struct ScalarType::init
{
	std::string_view name;
	std::string_view description;
};

std::shared_ptr<ScalarType> ScalarType::Make(std::string_view name, std::string_view description)
{
	return std::make_shared<ScalarType>(init { name, description });
}

ScalarType::ScalarType(init&& params)
	: BaseType(introspection::TypeKind::SCALAR, params.description)
	, _name(params.name)
{
}

std::string_view ScalarType::name() const noexcept
{
	return _name;
}

struct ObjectType::init
{
	std::string_view name;
	std::string_view description;
};

std::shared_ptr<ObjectType> ObjectType::Make(std::string_view name, std::string_view description)
{
	return std::make_shared<ObjectType>(init { name, description });
}

ObjectType::ObjectType(init&& params)
	: BaseType(introspection::TypeKind::OBJECT, params.description)
	, _name(params.name)
{
}

void ObjectType::AddInterfaces(
	std::initializer_list<std::shared_ptr<const InterfaceType>> interfaces)
{
	_interfaces.resize(interfaces.size());
	std::copy(interfaces.begin(), interfaces.end(), _interfaces.begin());

	for (const auto& interface : interfaces)
	{
		std::const_pointer_cast<InterfaceType>(interface)->AddPossibleType(
			std::static_pointer_cast<ObjectType>(shared_from_this()));
	}
}

void ObjectType::AddFields(std::initializer_list<std::shared_ptr<Field>> fields)
{
	_fields.resize(fields.size());
	std::copy(fields.begin(), fields.end(), _fields.begin());
}

std::string_view ObjectType::name() const noexcept
{
	return _name;
}

const std::vector<std::shared_ptr<const Field>>& ObjectType::fields() const noexcept
{
	return _fields;
}

const std::vector<std::shared_ptr<const InterfaceType>>& ObjectType::interfaces() const noexcept
{
	return _interfaces;
}

struct InterfaceType::init
{
	std::string_view name;
	std::string_view description;
};

std::shared_ptr<InterfaceType> InterfaceType::Make(
	std::string_view name, std::string_view description)
{
	return std::make_shared<InterfaceType>(init { name, description });
}

InterfaceType::InterfaceType(init&& params)
	: BaseType(introspection::TypeKind::INTERFACE, params.description)
	, _name(params.name)
{
}

void InterfaceType::AddPossibleType(std::weak_ptr<ObjectType> possibleType)
{
	_possibleTypes.push_back(possibleType);
}

void InterfaceType::AddFields(std::initializer_list<std::shared_ptr<Field>> fields)
{
	_fields.resize(fields.size());
	std::copy(fields.begin(), fields.end(), _fields.begin());
}

std::string_view InterfaceType::name() const noexcept
{
	return _name;
}

const std::vector<std::shared_ptr<const Field>>& InterfaceType::fields() const noexcept
{
	return _fields;
}

const std::vector<std::weak_ptr<const BaseType>>& InterfaceType::possibleTypes() const noexcept
{
	return _possibleTypes;
}

struct UnionType::init
{
	std::string_view name;
	std::string_view description;
};

std::shared_ptr<UnionType> UnionType::Make(std::string_view name, std::string_view description)
{
	return std::make_shared<UnionType>(init { name, description });
}

UnionType::UnionType(init&& params)
	: BaseType(introspection::TypeKind::UNION, params.description)
	, _name(params.name)
{
}

void UnionType::AddPossibleTypes(std::initializer_list<std::weak_ptr<const BaseType>> possibleTypes)
{
	_possibleTypes.resize(possibleTypes.size());
	std::copy(possibleTypes.begin(), possibleTypes.end(), _possibleTypes.begin());
}

std::string_view UnionType::name() const noexcept
{
	return _name;
}

const std::vector<std::weak_ptr<const BaseType>>& UnionType::possibleTypes() const noexcept
{
	return _possibleTypes;
}

struct EnumType::init
{
	std::string_view name;
	std::string_view description;
};

std::shared_ptr<EnumType> EnumType::Make(std::string_view name, std::string_view description)
{
	return std::make_shared<EnumType>(init { name, description });
}

EnumType::EnumType(init&& params)
	: BaseType(introspection::TypeKind::ENUM, params.description)
	, _name(params.name)
{
}

void EnumType::AddEnumValues(std::initializer_list<EnumValueType> enumValues)
{
	_enumValues.resize(enumValues.size());
	std::transform(enumValues.begin(),
		enumValues.end(),
		_enumValues.begin(),
		[](const auto& value) {
			return EnumValue::Make(value.value, value.description, value.deprecationReason);
		});
}

std::string_view EnumType::name() const noexcept
{
	return _name;
}

const std::vector<std::shared_ptr<const EnumValue>>& EnumType::enumValues() const noexcept
{
	return _enumValues;
}

struct InputObjectType::init
{
	std::string_view name;
	std::string_view description;
};

std::shared_ptr<InputObjectType> InputObjectType::Make(
	std::string_view name, std::string_view description)
{
	return std::make_shared<InputObjectType>(init { name, description });
}

InputObjectType::InputObjectType(init&& params)
	: BaseType(introspection::TypeKind::INPUT_OBJECT, params.description)
	, _name(params.name)
{
}

void InputObjectType::AddInputValues(std::initializer_list<std::shared_ptr<InputValue>> inputValues)
{
	_inputValues.resize(inputValues.size());
	std::copy(inputValues.begin(), inputValues.end(), _inputValues.begin());
}

std::string_view InputObjectType::name() const noexcept
{
	return _name;
}

const std::vector<std::shared_ptr<const InputValue>>& InputObjectType::inputFields() const noexcept
{
	return _inputValues;
}

struct WrapperType::init
{
	introspection::TypeKind kind;
	std::weak_ptr<const BaseType> ofType;
};

std::shared_ptr<WrapperType> WrapperType::Make(
	introspection::TypeKind kind, std::weak_ptr<const BaseType> ofType)
{
	return std::make_shared<WrapperType>(init { kind, std::move(ofType) });
}

WrapperType::WrapperType(init&& params)
	: BaseType(params.kind, std::string_view())
	, _ofType(std::move(params.ofType))
{
}

const std::weak_ptr<const BaseType>& WrapperType::ofType() const noexcept
{
	return _ofType;
}

struct Field::init
{
	std::string_view name;
	std::string_view description;
	std::optional<std::string_view> deprecationReason;
	std::weak_ptr<const BaseType> type;
	std::vector<std::shared_ptr<const InputValue>> args;
};

std::shared_ptr<Field> Field::Make(std::string_view name, std::string_view description,
	std::optional<std::string_view> deprecationReason, std::weak_ptr<const BaseType> type,
	std::initializer_list<std::shared_ptr<InputValue>> args)
{
	init params { name, description, deprecationReason, std::move(type) };

	params.args.resize(args.size());
	std::copy(args.begin(), args.end(), params.args.begin());

	return std::make_shared<Field>(std::move(params));
}

Field::Field(init&& params)
	: _name(params.name)
	, _description(params.description)
	, _deprecationReason(params.deprecationReason)
	, _type(std::move(params.type))
	, _args(std::move(params.args))
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

const std::vector<std::shared_ptr<const InputValue>>& Field::args() const noexcept
{
	return _args;
}

const std::weak_ptr<const BaseType>& Field::type() const noexcept
{
	return _type;
}

const std::optional<std::string_view>& Field::deprecationReason() const noexcept
{
	return _deprecationReason;
}

struct InputValue::init
{
	std::string_view name;
	std::string_view description;
	std::weak_ptr<const BaseType> type;
	std::string_view defaultValue;
};

std::shared_ptr<InputValue> InputValue::Make(std::string_view name, std::string_view description,
	std::weak_ptr<const BaseType> type, std::string_view defaultValue)
{
	return std::make_shared<InputValue>(init { name, description, std::move(type), defaultValue });
}

InputValue::InputValue(init&& params)
	: _name(params.name)
	, _description(params.description)
	, _type(std::move(params.type))
	, _defaultValue(params.defaultValue)
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

const std::weak_ptr<const BaseType>& InputValue::type() const noexcept
{
	return _type;
}

std::string_view InputValue::defaultValue() const noexcept
{
	return _defaultValue;
}

struct EnumValue::init
{
	std::string_view name;
	std::string_view description;
	std::optional<std::string_view> deprecationReason;
};

std::shared_ptr<EnumValue> EnumValue::Make(std::string_view name, std::string_view description,
	std::optional<std::string_view> deprecationReason)
{
	return std::make_shared<EnumValue>(init { name, description, deprecationReason });
}

EnumValue::EnumValue(init&& params)
	: _name(params.name)
	, _description(params.description)
	, _deprecationReason(params.deprecationReason)
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

struct Directive::init
{
	std::string_view name;
	std::string_view description;
	std::vector<introspection::DirectiveLocation> locations;
	std::vector<std::shared_ptr<const InputValue>> args;
};

std::shared_ptr<Directive> Directive::Make(std::string_view name, std::string_view description,
	std::initializer_list<introspection::DirectiveLocation> locations,
	std::initializer_list<std::shared_ptr<InputValue>> args)
{
	init params { name, description };

	params.locations.resize(locations.size());
	std::copy(locations.begin(), locations.end(), params.locations.begin());

	params.args.resize(args.size());
	std::copy(args.begin(), args.end(), params.args.begin());

	return std::make_shared<Directive>(std::move(params));
}

Directive::Directive(init&& params)
	: _name(params.name)
	, _description(params.description)
	, _locations(std::move(params.locations))
	, _args(std::move(params.args))
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

const std::vector<std::shared_ptr<const InputValue>>& Directive::args() const noexcept
{
	return _args;
}

} // namespace graphql::schema
