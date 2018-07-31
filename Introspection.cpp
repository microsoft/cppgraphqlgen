// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Introspection.h"

namespace facebook {
namespace graphql {
namespace introspection {

Schema::Schema()
{
	AddType("Int", std::make_shared<ScalarType>("Int"));
	AddType("Float", std::make_shared<ScalarType>("Float"));
	AddType("String", std::make_shared<ScalarType>("String"));
	AddType("Boolean", std::make_shared<ScalarType>("Boolean"));
	AddType("ID", std::make_shared<ScalarType>("ID"));
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

void Schema::AddType(std::string name, std::shared_ptr<object::__Type> type)
{
	_typeMap[name] = _types.size();
	_types.push_back({ std::move(name), std::move(type) });
}

std::shared_ptr<object::__Type> Schema::LookupType(const std::string& name) const
{
	auto itr = _typeMap.find(name);

	if (itr == _typeMap.cend())
	{
		return nullptr;
	}

	return _types[itr->second].second;
}

std::vector<std::shared_ptr<object::__Type>> Schema::getTypes() const
{
	std::vector<std::shared_ptr<object::__Type>> result(_types.size());

	std::transform(_types.cbegin(), _types.cend(), result.begin(),
		[](const std::pair<std::string, std::shared_ptr<object::__Type>>& namedType)
	{
		return namedType.second;
	});

	return result;
}

std::shared_ptr<object::__Type> Schema::getQueryType() const
{
	return _query;
}

std::shared_ptr<object::__Type> Schema::getMutationType() const
{
	return _mutation;
}

std::shared_ptr<object::__Type> Schema::getSubscriptionType() const
{
	return _subscription;
}

std::vector<std::shared_ptr<object::__Directive>> Schema::getDirectives() const
{
	return {};
}

std::unique_ptr<std::string> BaseType::getName() const
{
	return nullptr;
}

std::unique_ptr<std::string> BaseType::getDescription() const
{
	return nullptr;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> BaseType::getFields(std::unique_ptr<bool> /*includeDeprecated*/) const
{
	return nullptr;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> BaseType::getInterfaces() const
{
	return nullptr;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> BaseType::getPossibleTypes() const
{
	return nullptr;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>> BaseType::getEnumValues(std::unique_ptr<bool> /*includeDeprecated*/) const
{
	return nullptr;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>> BaseType::getInputFields() const
{
	return nullptr;
}

std::shared_ptr<object::__Type> BaseType::getOfType() const
{
	return nullptr;
}

ScalarType::ScalarType(std::string name)
	: _name(std::move(name))
{
}

__TypeKind ScalarType::getKind() const
{
	return __TypeKind::SCALAR;
}

std::unique_ptr<std::string> ScalarType::getName() const
{
	std::unique_ptr<std::string> result(new std::string(_name));

	return result;
}

ObjectType::ObjectType(std::string name)
	: _name(std::move(name))
{
}

void ObjectType::AddInterfaces(std::vector<std::shared_ptr<InterfaceType>> interfaces)
{
	_interfaces = std::move(interfaces);
}

void ObjectType::AddFields(std::vector<std::shared_ptr<Field>> fields)
{
	_fields = std::move(fields);
}

__TypeKind ObjectType::getKind() const
{
	return __TypeKind::OBJECT;
}

std::unique_ptr<std::string> ObjectType::getName() const
{
	std::unique_ptr<std::string> result(new std::string(_name));

	return result;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> ObjectType::getFields(std::unique_ptr<bool> /*includeDeprecated*/) const
{
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> result(new std::vector<std::shared_ptr<object::__Field>>(_fields.size()));

	std::copy(_fields.cbegin(), _fields.cend(), result->begin());

	return result;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> ObjectType::getInterfaces() const
{
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> result(new std::vector<std::shared_ptr<object::__Type>>(_interfaces.size()));

	std::copy(_interfaces.cbegin(), _interfaces.cend(), result->begin());

	return result;
}

InterfaceType::InterfaceType(std::string name)
	: _name(std::move(name))
{
}

void InterfaceType::AddFields(std::vector<std::shared_ptr<Field>> fields)
{
	_fields = std::move(fields);
}

__TypeKind InterfaceType::getKind() const
{
	return __TypeKind::INTERFACE;
}

std::unique_ptr<std::string> InterfaceType::getName() const
{
	std::unique_ptr<std::string> result(new std::string(_name));

	return result;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> InterfaceType::getFields(std::unique_ptr<bool> /*includeDeprecated*/) const
{
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> result(new std::vector<std::shared_ptr<object::__Field>>(_fields.size()));

	std::copy(_fields.cbegin(), _fields.cend(), result->begin());

	return result;
}

UnionType::UnionType(std::string name)
	: _name(std::move(name))
{
}

void UnionType::AddPossibleTypes(std::vector<std::shared_ptr<object::__Type>> possibleTypes)
{
	_possibleTypes = std::move(possibleTypes);
}

__TypeKind UnionType::getKind() const
{
	return __TypeKind::UNION;
}

std::unique_ptr<std::string> UnionType::getName() const
{
	std::unique_ptr<std::string> result(new std::string(_name));

	return result;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> UnionType::getPossibleTypes() const
{
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> result(new std::vector<std::shared_ptr<object::__Type>>(_possibleTypes.size()));

	std::copy(_possibleTypes.cbegin(), _possibleTypes.cend(), result->begin());

	return result;
}

EnumType::EnumType(std::string name)
	: _name(std::move(name))
{
}

void EnumType::AddEnumValues(std::vector<std::string> enumValues)
{
	_enumValues.reserve(_enumValues.size() + enumValues.size());

	for (auto& value : enumValues)
	{
		_enumValues.push_back(std::make_shared<EnumValue>(std::move(value)));
	}
}

__TypeKind EnumType::getKind() const
{
	return __TypeKind::ENUM;
}

std::unique_ptr<std::string> EnumType::getName() const
{
	std::unique_ptr<std::string> result(new std::string(_name));

	return result;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>> EnumType::getEnumValues(std::unique_ptr<bool> includeDeprecated) const
{
	std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>> result(new std::vector<std::shared_ptr<object::__EnumValue>>(_enumValues.size()));

	std::copy(_enumValues.cbegin(), _enumValues.cend(), result->begin());

	return result;
}

InputObjectType::InputObjectType(std::string name)
	: _name(std::move(name))
{
}

void InputObjectType::AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues)
{
	_inputValues = std::move(inputValues);
}

__TypeKind InputObjectType::getKind() const
{
	return __TypeKind::INPUT_OBJECT;
}

std::unique_ptr<std::string> InputObjectType::getName() const
{
	std::unique_ptr<std::string> result(new std::string(_name));

	return result;
}

std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>> InputObjectType::getInputFields() const
{
	std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>> result(new std::vector<std::shared_ptr<object::__InputValue>>(_inputValues.size()));

	std::copy(_inputValues.cbegin(), _inputValues.cend(), result->begin());

	return result;
}

WrapperType::WrapperType(__TypeKind kind, std::shared_ptr<object::__Type> ofType)
	: _kind(kind)
	, _ofType(std::move(ofType))
{
}

__TypeKind WrapperType::getKind() const
{
	return _kind;
}

std::shared_ptr<object::__Type> WrapperType::getOfType() const
{
	return _ofType;
}

Field::Field(std::string name, std::vector<std::shared_ptr<InputValue>> args, std::shared_ptr<object::__Type> type)
	: _name(std::move(name))
	, _args(std::move(args))
	, _type(std::move(type))
{
}

std::string Field::getName() const
{
	return _name;
}

std::unique_ptr<std::string> Field::getDescription() const
{
	return nullptr;
}

std::vector<std::shared_ptr<object::__InputValue>> Field::getArgs() const
{
	std::vector<std::shared_ptr<object::__InputValue>> result(_args.size());

	std::copy(_args.cbegin(), _args.cend(), result.begin());

	return result;
}

std::shared_ptr<object::__Type> Field::getType() const
{
	return _type;
}

bool Field::getIsDeprecated() const
{
	return false;
}

std::unique_ptr<std::string> Field::getDeprecationReason() const
{
	return nullptr;
}

InputValue::InputValue(std::string name, std::shared_ptr<object::__Type> type, const web::json::value& defaultValue)
	: _name(std::move(name))
	, _type(std::move(type))
	, _defaultValue(formatDefaultValue(defaultValue))
{
}

std::string InputValue::getName() const
{
	return _name;
}

std::unique_ptr<std::string> InputValue::getDescription() const
{
	return nullptr;
}

std::shared_ptr<object::__Type> InputValue::getType() const
{
	return _type;
}

std::unique_ptr<std::string> InputValue::getDefaultValue() const
{
	std::unique_ptr<std::string> result(new std::string(_defaultValue));

	return result;
}

std::string InputValue::formatDefaultValue(const web::json::value& defaultValue) noexcept
{
	utility::ostringstream_t output;

	if (defaultValue.is_object())
	{
		bool firstValue = true;

		output << _XPLATSTR("{ ");

		for (const auto& entry : defaultValue.as_object())
		{
			if (!firstValue)
			{
				output << _XPLATSTR(", ");
			}
			firstValue = false;

			output << entry.first
				<< _XPLATSTR(": ")
				<< utility::conversions::to_string_t(formatDefaultValue(entry.second));
		}

		output << _XPLATSTR(" }");
	}
	else if (defaultValue.is_array())
	{
		bool firstValue = true;

		output << _XPLATSTR("[ ");

		for (const auto& entry : defaultValue.as_array())
		{
			if (!firstValue)
			{
				output << _XPLATSTR(", ");
			}
			firstValue = false;

			output << utility::conversions::to_string_t(formatDefaultValue(entry));
		}

		output << _XPLATSTR(" ]");
	}
	else
	{
		output << defaultValue;
	}

	return utility::conversions::to_utf8string(output.str());
}

EnumValue::EnumValue(std::string name)
	: _name(std::move(name))
{
}

std::string EnumValue::getName() const
{
	return _name;
}

std::unique_ptr<std::string> EnumValue::getDescription() const
{
	return nullptr;
}

bool EnumValue::getIsDeprecated() const
{
	return false;
}

std::unique_ptr<std::string> EnumValue::getDeprecationReason() const
{
	return nullptr;
}

} /* namespace facebook */
} /* namespace graphql */
} /* namespace introspection */