// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Introspection.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace facebook {
namespace graphql {
namespace introspection {

Schema::Schema()
{
	AddType("Int", std::make_shared<ScalarType>("Int", ""));
	AddType("Float", std::make_shared<ScalarType>("Float", ""));
	AddType("String", std::make_shared<ScalarType>("String", ""));
	AddType("Boolean", std::make_shared<ScalarType>("Boolean", ""));
	AddType("ID", std::make_shared<ScalarType>("ID", ""));
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

std::future<std::vector<std::shared_ptr<object::__Type>>> Schema::getTypes(service::RequestId) const
{
	return std::async(std::launch::deferred,
		[this]()
	{
		std::vector<std::shared_ptr<object::__Type>> result(_types.size());

		std::transform(_types.cbegin(), _types.cend(), result.begin(),
			[](const std::pair<std::string, std::shared_ptr<object::__Type>>& namedType)
		{
			return namedType.second;
		});

		return result;
	});
}

std::future<std::shared_ptr<object::__Type>> Schema::getQueryType(service::RequestId) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_query);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> Schema::getMutationType(service::RequestId) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_mutation);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> Schema::getSubscriptionType(service::RequestId) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_subscription);

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::__Directive>>> Schema::getDirectives(service::RequestId) const
{
	std::promise<std::vector<std::shared_ptr<object::__Directive>>> promise;

	// TODO: preserve directives
	promise.set_value({});

	return promise.get_future();
}

BaseType::BaseType(std::string description)
	: _description(std::move(description))
{
}

std::future<std::unique_ptr<std::string>> BaseType::getName(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> BaseType::getDescription(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(_description.empty()
		? nullptr
		: new std::string(_description)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> BaseType::getFields(service::RequestId, std::unique_ptr<bool>&& /*includeDeprecated*/) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> BaseType::getInterfaces(service::RequestId) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> BaseType::getPossibleTypes(service::RequestId) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> BaseType::getEnumValues(service::RequestId, std::unique_ptr<bool>&& /*includeDeprecated*/) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> BaseType::getInputFields(service::RequestId) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> BaseType::getOfType(service::RequestId) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

ScalarType::ScalarType(std::string name, std::string description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

std::future<__TypeKind> ScalarType::getKind(service::RequestId) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::SCALAR);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> ScalarType::getName(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(new std::string(_name)));

	return promise.get_future();
}

ObjectType::ObjectType(std::string name, std::string description)
	: BaseType(std::move(description))
	, _name(std::move(name))
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

std::future<__TypeKind> ObjectType::getKind(service::RequestId) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::OBJECT);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> ObjectType::getName(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(new std::string(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> ObjectType::getFields(service::RequestId requestId, std::unique_ptr<bool>&& includeDeprecated) const
{
	const bool deprecated = includeDeprecated && *includeDeprecated;
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> result(new std::vector<std::shared_ptr<object::__Field>>());

	result->reserve(_fields.size());
	std::copy_if(_fields.cbegin(), _fields.cend(), std::back_inserter(*result),
		[requestId, deprecated](const std::shared_ptr<Field>& field)
	{
		return deprecated
			|| !field->getIsDeprecated(requestId).get();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> ObjectType::getInterfaces(service::RequestId) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> result(new std::vector<std::shared_ptr<object::__Type>>(_interfaces.size()));

	std::copy(_interfaces.cbegin(), _interfaces.cend(), result->begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

InterfaceType::InterfaceType(std::string name, std::string description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void InterfaceType::AddFields(std::vector<std::shared_ptr<Field>> fields)
{
	_fields = std::move(fields);
}

std::future<__TypeKind> InterfaceType::getKind(service::RequestId) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::INTERFACE);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> InterfaceType::getName(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(new std::string(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> InterfaceType::getFields(service::RequestId requestId, std::unique_ptr<bool>&& includeDeprecated) const
{
	const bool deprecated = includeDeprecated && *includeDeprecated;
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> result(new std::vector<std::shared_ptr<object::__Field>>());

	result->reserve(_fields.size());
	std::copy_if(_fields.cbegin(), _fields.cend(), std::back_inserter(*result),
		[requestId, deprecated](const std::shared_ptr<Field>& field)
	{
		return deprecated
			|| !field->getIsDeprecated(requestId).get();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

UnionType::UnionType(std::string name, std::string description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void UnionType::AddPossibleTypes(std::vector<std::shared_ptr<object::__Type>> possibleTypes)
{
	_possibleTypes.resize(possibleTypes.size());
	std::transform(possibleTypes.cbegin(), possibleTypes.cend(), _possibleTypes.begin(),
		[](const std::shared_ptr<object::__Type>& shared)
	{
		return shared;
	});
}

std::future<__TypeKind> UnionType::getKind(service::RequestId) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::UNION);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> UnionType::getName(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(new std::string(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> UnionType::getPossibleTypes(service::RequestId) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> result(new std::vector<std::shared_ptr<object::__Type>>(_possibleTypes.size()));

	std::transform(_possibleTypes.cbegin(), _possibleTypes.cend(), result->begin(),
		[](const std::weak_ptr<object::__Type>& weak)
	{
		return weak.lock();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

EnumType::EnumType(std::string name, std::string description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void EnumType::AddEnumValues(std::vector<EnumValueType> enumValues)
{
	_enumValues.reserve(_enumValues.size() + enumValues.size());

	for (auto& value : enumValues)
	{
		_enumValues.push_back(std::make_shared<EnumValue>(std::move(value.value),
			std::move(value.description),
			std::unique_ptr<std::string>(value.deprecationReason
				? new std::string(value.deprecationReason)
				: nullptr)));
	}
}

std::future<__TypeKind> EnumType::getKind(service::RequestId) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::ENUM);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> EnumType::getName(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(new std::string(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> EnumType::getEnumValues(service::RequestId requestId, std::unique_ptr<bool>&& includeDeprecated) const
{
	const bool deprecated = includeDeprecated && *includeDeprecated;
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>> result(new std::vector<std::shared_ptr<object::__EnumValue>>());

	result->reserve(_enumValues.size());
	std::copy_if(_enumValues.cbegin(), _enumValues.cend(), std::back_inserter(*result),
		[requestId, deprecated](const std::shared_ptr<object::__EnumValue>& value)
	{
		return deprecated
			|| !value->getIsDeprecated(requestId).get();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

InputObjectType::InputObjectType(std::string name, std::string description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void InputObjectType::AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues)
{
	_inputValues = std::move(inputValues);
}

std::future<__TypeKind> InputObjectType::getKind(service::RequestId) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::INPUT_OBJECT);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> InputObjectType::getName(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(new std::string(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> InputObjectType::getInputFields(service::RequestId) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>> result(new std::vector<std::shared_ptr<object::__InputValue>>(_inputValues.size()));

	std::copy(_inputValues.cbegin(), _inputValues.cend(), result->begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

WrapperType::WrapperType(__TypeKind kind, std::shared_ptr<object::__Type> ofType)
	: BaseType(std::string())
	, _kind(kind)
	, _ofType(std::move(ofType))
{
}

std::future<__TypeKind> WrapperType::getKind(service::RequestId) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(_kind);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> WrapperType::getOfType(service::RequestId) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_ofType.lock());

	return promise.get_future();
}

Field::Field(std::string name, std::string description, std::unique_ptr<std::string>&& deprecationReason, std::vector<std::shared_ptr<InputValue>> args, std::shared_ptr<object::__Type> type)
	: _name(std::move(name))
	, _description(std::move(description))
	, _deprecationReason(std::move(deprecationReason))
	, _args(std::move(args))
	, _type(std::move(type))
{
}

std::future<std::string> Field::getName(service::RequestId) const
{
	std::promise<std::string> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> Field::getDescription(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(_description.empty()
		? nullptr
		: new std::string(_description)));

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::__InputValue>>> Field::getArgs(service::RequestId) const
{
	std::promise<std::vector<std::shared_ptr<object::__InputValue>>> promise;
	std::vector<std::shared_ptr<object::__InputValue>> result(_args.size());

	std::copy(_args.cbegin(), _args.cend(), result.begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> Field::getType(service::RequestId) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_type.lock());

	return promise.get_future();
}

std::future<bool> Field::getIsDeprecated(service::RequestId) const
{
	std::promise<bool> promise;

	promise.set_value(_deprecationReason != nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> Field::getDeprecationReason(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(_deprecationReason
		? std::unique_ptr<std::string>(new std::string(*_deprecationReason))
		: nullptr);

	return promise.get_future();
}

InputValue::InputValue(std::string name, std::string description, std::shared_ptr<object::__Type> type, const rapidjson::Value& defaultValue)
	: _name(std::move(name))
	, _description(std::move(description))
	, _type(std::move(type))
	, _defaultValue(formatDefaultValue(defaultValue))
{
}

std::future<std::string> InputValue::getName(service::RequestId) const
{
	std::promise<std::string> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> InputValue::getDescription(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(_description.empty()
		? nullptr
		: new std::string(_description)));

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> InputValue::getType(service::RequestId) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_type.lock());

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> InputValue::getDefaultValue(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(new std::string(_defaultValue)));

	return promise.get_future();
}

std::string InputValue::formatDefaultValue(const rapidjson::Value& defaultValue) noexcept
{
	std::ostringstream output;

	if (defaultValue.IsObject())
	{
		bool firstValue = true;

		output << "{ ";

		for (const auto& entry : defaultValue.GetObject())
		{
			if (!firstValue)
			{
				output << ", ";
			}
			firstValue = false;

			output << "\"" << entry.name.GetString()
				<< "\": " << formatDefaultValue(entry.value);
		}

		output << " }";
	}
	else if (defaultValue.IsArray())
	{
		bool firstValue = true;

		output << "[ ";

		for (const auto& entry : defaultValue.GetArray())
		{
			if (!firstValue)
			{
				output << ", ";
			}
			firstValue = false;

			output << formatDefaultValue(entry);
		}

		output << " ]";
	}
	else
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		defaultValue.Accept(writer);
		output << buffer.GetString();
	}

	return output.str();
}

EnumValue::EnumValue(std::string name, std::string description, std::unique_ptr<std::string>&& deprecationReason)
	: _name(std::move(name))
	, _description(std::move(description))
	, _deprecationReason(std::move(deprecationReason))
{
}

std::future<std::string> EnumValue::getName(service::RequestId) const
{
	std::promise<std::string> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> EnumValue::getDescription(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(std::unique_ptr<std::string>(_description.empty()
		? nullptr
		: new std::string(_description)));

	return promise.get_future();
}

std::future<bool> EnumValue::getIsDeprecated(service::RequestId) const
{
	std::promise<bool> promise;

	promise.set_value(_deprecationReason != nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::string>> EnumValue::getDeprecationReason(service::RequestId) const
{
	std::promise<std::unique_ptr<std::string>> promise;

	promise.set_value(_deprecationReason
		? std::unique_ptr<std::string>(new std::string(*_deprecationReason))
		: nullptr);

	return promise.get_future();
}

} /* namespace facebook */
} /* namespace graphql */
} /* namespace introspection */