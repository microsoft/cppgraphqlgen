// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <graphqlservice/Introspection.h>

namespace facebook {
namespace graphql {
namespace introspection {

Schema::Schema()
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

void Schema::AddType(response::StringType name, std::shared_ptr<object::__Type> type)
{
	_typeMap[name] = _types.size();
	_types.push_back({ std::move(name), std::move(type) });
}

const std::shared_ptr<object::__Type>& Schema::LookupType(const response::StringType& name) const
{
	auto itr = _typeMap.find(name);

	if (itr == _typeMap.cend())
	{
		return nullptr;
	}

	return _types[itr->second].second;
}

void Schema::AddDirective(std::shared_ptr<object::__Directive> directive)
{
	_directives.emplace_back(std::move(directive));
}

std::future<std::vector<std::shared_ptr<object::__Type>>> Schema::getTypes(const std::shared_ptr<service::RequestState>&) const
{
	auto keepAlive = shared_from_this();

	return std::async(std::launch::deferred,
		[this, keepAlive]()
	{
		std::vector<std::shared_ptr<object::__Type>> result(_types.size());

		std::transform(_types.cbegin(), _types.cend(), result.begin(),
			[](const std::pair<response::StringType, std::shared_ptr<object::__Type>>& namedType)
		{
			return namedType.second;
		});

		return result;
	});
}

std::future<std::shared_ptr<object::__Type>> Schema::getQueryType(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_query);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> Schema::getMutationType(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_mutation);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> Schema::getSubscriptionType(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_subscription);

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::__Directive>>> Schema::getDirectives(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::vector<std::shared_ptr<object::__Directive>>> promise;

	promise.set_value(_directives);

	return promise.get_future();
}

BaseType::BaseType(response::StringType description)
	: _description(std::move(description))
{
}

std::future<std::unique_ptr<response::StringType>> BaseType::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> BaseType::getDescription(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_description.empty()
		? nullptr
		: new response::StringType(_description)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> BaseType::getFields(const std::shared_ptr<service::RequestState>&, std::unique_ptr<response::BooleanType>&& /*includeDeprecated*/) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> BaseType::getInterfaces(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> BaseType::getPossibleTypes(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> BaseType::getEnumValues(const std::shared_ptr<service::RequestState>&, std::unique_ptr<response::BooleanType>&& /*includeDeprecated*/) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> BaseType::getInputFields(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> BaseType::getOfType(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(nullptr);

	return promise.get_future();
}

ScalarType::ScalarType(response::StringType name, response::StringType description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

std::future<__TypeKind> ScalarType::getKind(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::SCALAR);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> ScalarType::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

ObjectType::ObjectType(response::StringType name, response::StringType description)
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

std::future<__TypeKind> ObjectType::getKind(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::OBJECT);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> ObjectType::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> ObjectType::getFields(const std::shared_ptr<service::RequestState>& state, std::unique_ptr<response::BooleanType>&& includeDeprecated) const
{
	const bool deprecated = includeDeprecated && *includeDeprecated;
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> result(new std::vector<std::shared_ptr<object::__Field>>());

	result->reserve(_fields.size());
	std::copy_if(_fields.cbegin(), _fields.cend(), std::back_inserter(*result),
		[state, deprecated](const std::shared_ptr<Field>& field)
	{
		return deprecated
			|| !field->getIsDeprecated(state).get();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> ObjectType::getInterfaces(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>> result(new std::vector<std::shared_ptr<object::__Type>>(_interfaces.size()));

	std::copy(_interfaces.cbegin(), _interfaces.cend(), result->begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

InterfaceType::InterfaceType(response::StringType name, response::StringType description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void InterfaceType::AddFields(std::vector<std::shared_ptr<Field>> fields)
{
	_fields = std::move(fields);
}

std::future<__TypeKind> InterfaceType::getKind(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::INTERFACE);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> InterfaceType::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> InterfaceType::getFields(const std::shared_ptr<service::RequestState>& state, std::unique_ptr<response::BooleanType>&& includeDeprecated) const
{
	const bool deprecated = includeDeprecated && *includeDeprecated;
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__Field>>> result(new std::vector<std::shared_ptr<object::__Field>>());

	result->reserve(_fields.size());
	std::copy_if(_fields.cbegin(), _fields.cend(), std::back_inserter(*result),
		[state, deprecated](const std::shared_ptr<Field>& field)
	{
		return deprecated
			|| !field->getIsDeprecated(state).get();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

UnionType::UnionType(response::StringType name, response::StringType description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void UnionType::AddPossibleTypes(std::vector<std::weak_ptr<object::__Type>> possibleTypes)
{
	_possibleTypes = std::move(possibleTypes);
}

std::future<__TypeKind> UnionType::getKind(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::UNION);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> UnionType::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__Type>>>> UnionType::getPossibleTypes(const std::shared_ptr<service::RequestState>&) const
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

EnumType::EnumType(response::StringType name, response::StringType description)
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
			std::unique_ptr<response::StringType>(value.deprecationReason
				? new response::StringType(value.deprecationReason)
				: nullptr)));
	}
}

std::future<__TypeKind> EnumType::getKind(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::ENUM);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> EnumType::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> EnumType::getEnumValues(const std::shared_ptr<service::RequestState>& state, std::unique_ptr<response::BooleanType>&& includeDeprecated) const
{
	const bool deprecated = includeDeprecated && *includeDeprecated;
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__EnumValue>>> result(new std::vector<std::shared_ptr<object::__EnumValue>>());

	result->reserve(_enumValues.size());
	std::copy_if(_enumValues.cbegin(), _enumValues.cend(), std::back_inserter(*result),
		[state, deprecated](const std::shared_ptr<object::__EnumValue>& value)
	{
		return deprecated
			|| !value->getIsDeprecated(state).get();
	});
	promise.set_value(std::move(result));

	return promise.get_future();
}

InputObjectType::InputObjectType(response::StringType name, response::StringType description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void InputObjectType::AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues)
{
	_inputValues = std::move(inputValues);
}

std::future<__TypeKind> InputObjectType::getKind(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(__TypeKind::INPUT_OBJECT);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> InputObjectType::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(new response::StringType(_name)));

	return promise.get_future();
}

std::future<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> InputObjectType::getInputFields(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>>> promise;
	std::unique_ptr<std::vector<std::shared_ptr<object::__InputValue>>> result(new std::vector<std::shared_ptr<object::__InputValue>>(_inputValues.size()));

	std::copy(_inputValues.cbegin(), _inputValues.cend(), result->begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

WrapperType::WrapperType(__TypeKind kind, const std::shared_ptr<object::__Type>& ofType)
	: BaseType(response::StringType())
	, _kind(kind)
	, _ofType(ofType)
{
}

std::future<__TypeKind> WrapperType::getKind(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<__TypeKind> promise;

	promise.set_value(_kind);

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> WrapperType::getOfType(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_ofType.lock());

	return promise.get_future();
}

Field::Field(response::StringType name, response::StringType description, std::unique_ptr<response::StringType>&& deprecationReason, std::vector<std::shared_ptr<InputValue>> args, const std::shared_ptr<object::__Type>& type)
	: _name(std::move(name))
	, _description(std::move(description))
	, _deprecationReason(std::move(deprecationReason))
	, _args(std::move(args))
	, _type(type)
{
}

std::future<response::StringType> Field::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<response::StringType> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> Field::getDescription(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_description.empty()
		? nullptr
		: new response::StringType(_description)));

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::__InputValue>>> Field::getArgs(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::vector<std::shared_ptr<object::__InputValue>>> promise;
	std::vector<std::shared_ptr<object::__InputValue>> result(_args.size());

	std::copy(_args.cbegin(), _args.cend(), result.begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> Field::getType(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_type.lock());

	return promise.get_future();
}

std::future<response::BooleanType> Field::getIsDeprecated(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<response::BooleanType> promise;

	promise.set_value(_deprecationReason != nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> Field::getDeprecationReason(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(_deprecationReason
		? std::unique_ptr<response::StringType>(new response::StringType(*_deprecationReason))
		: nullptr);

	return promise.get_future();
}

InputValue::InputValue(response::StringType name, response::StringType description, const std::shared_ptr<object::__Type>& type, response::StringType defaultValue)
	: _name(std::move(name))
	, _description(std::move(description))
	, _type(type)
	, _defaultValue(std::move(defaultValue))
{
}

std::future<response::StringType> InputValue::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<response::StringType> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> InputValue::getDescription(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_description.empty()
		? nullptr
		: new response::StringType(_description)));

	return promise.get_future();
}

std::future<std::shared_ptr<object::__Type>> InputValue::getType(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::shared_ptr<object::__Type>> promise;

	promise.set_value(_type.lock());

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> InputValue::getDefaultValue(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_defaultValue.empty()
		? nullptr
		: new response::StringType(_defaultValue)));

	return promise.get_future();
}

EnumValue::EnumValue(response::StringType name, response::StringType description, std::unique_ptr<response::StringType>&& deprecationReason)
	: _name(std::move(name))
	, _description(std::move(description))
	, _deprecationReason(std::move(deprecationReason))
{
}

std::future<response::StringType> EnumValue::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<response::StringType> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> EnumValue::getDescription(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_description.empty()
		? nullptr
		: new response::StringType(_description)));

	return promise.get_future();
}

std::future<response::BooleanType> EnumValue::getIsDeprecated(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<response::BooleanType> promise;

	promise.set_value(_deprecationReason != nullptr);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> EnumValue::getDeprecationReason(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(_deprecationReason
		? std::unique_ptr<response::StringType>(new response::StringType(*_deprecationReason))
		: nullptr);

	return promise.get_future();
}

Directive::Directive(response::StringType name, response::StringType description, std::vector<response::StringType> locations, std::vector<std::shared_ptr<InputValue>> args)
	: _name(std::move(name))
	, _description(std::move(description))
	, _locations([](std::vector<response::StringType>&& locationsArg) -> std::vector<__DirectiveLocation>
		{
			std::vector<__DirectiveLocation> result(locationsArg.size());

			std::transform(locationsArg.begin(), locationsArg.end(), result.begin(),
				[](std::string& name) -> __DirectiveLocation
				{
					response::Value location(response::Type::EnumValue);

					location.set<response::StringType>(std::move(name));
					return service::ModifiedArgument<__DirectiveLocation>::convert(location);
				});

			return result;
		}(std::move(locations)))
	, _args(std::move(args))
{
}

std::future<response::StringType> Directive::getName(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<response::StringType> promise;

	promise.set_value(_name);

	return promise.get_future();
}

std::future<std::unique_ptr<response::StringType>> Directive::getDescription(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::unique_ptr<response::StringType>> promise;

	promise.set_value(std::unique_ptr<response::StringType>(_description.empty()
		? nullptr
		: new response::StringType(_description)));

	return promise.get_future();
}

std::future<std::vector<__DirectiveLocation>> Directive::getLocations(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::vector<__DirectiveLocation>> promise;
	std::vector< __DirectiveLocation> result(_locations.size());

	std::copy(_locations.cbegin(), _locations.cend(), result.begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

std::future<std::vector<std::shared_ptr<object::__InputValue>>> Directive::getArgs(const std::shared_ptr<service::RequestState>&) const
{
	std::promise<std::vector<std::shared_ptr<object::__InputValue>>> promise;
	std::vector<std::shared_ptr<object::__InputValue>> result(_args.size());

	std::copy(_args.cbegin(), _args.cend(), result.begin());
	promise.set_value(std::move(result));

	return promise.get_future();
}

} /* namespace facebook */
} /* namespace graphql */
} /* namespace introspection */
