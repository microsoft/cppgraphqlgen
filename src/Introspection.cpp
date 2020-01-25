// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <graphqlservice/Introspection.h>

namespace graphql::introspection {

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

void Schema::AddType(response::StringType&& name, std::shared_ptr<object::Type> type)
{
	_typeMap[name] = _types.size();
	_types.push_back({ std::move(name), std::move(type) });
}

const std::shared_ptr<object::Type>& Schema::LookupType(const response::StringType& name) const
{
	if (_typeMap.find(name) == _typeMap.cend())
	{
		throw service::schema_exception { { "type not found" } };
	}

	return _types[_typeMap.find(name)->second].second;
}

const std::shared_ptr<object::Type>& Schema::WrapType(TypeKind kind, const std::shared_ptr<object::Type>& ofType)
{
	auto& wrappers = (kind == TypeKind::LIST)
		? _listWrappers
		: _nonNullWrappers;
	auto itr = wrappers.find(ofType);

	if (itr == wrappers.cend())
	{
		std::tie(itr, std::ignore) = wrappers.insert({ ofType, std::make_shared<WrapperType>(kind, ofType) });
	}

	return itr->second;
}

void Schema::AddDirective(std::shared_ptr<object::Directive> directive)
{
	_directives.emplace_back(std::move(directive));
}

service::FieldResult<std::vector<std::shared_ptr<object::Type>>> Schema::getTypes(service::FieldParams&&) const
{
	auto spThis = shared_from_this();

	return std::async(std::launch::deferred,
		[this, spThis]()
	{
		std::vector<std::shared_ptr<object::Type>> result(_types.size());

		std::transform(_types.cbegin(), _types.cend(), result.begin(),
			[](const std::pair<response::StringType, std::shared_ptr<object::Type>>& namedType)
		{
			return namedType.second;
		});

		return result;
	});
}

service::FieldResult<std::shared_ptr<object::Type>> Schema::getQueryType(service::FieldParams&&) const
{
	return _query;
}

service::FieldResult<std::shared_ptr<object::Type>> Schema::getMutationType(service::FieldParams&&) const
{
	return _mutation;
}

service::FieldResult<std::shared_ptr<object::Type>> Schema::getSubscriptionType(service::FieldParams&&) const
{
	return _subscription;
}

service::FieldResult<std::vector<std::shared_ptr<object::Directive>>> Schema::getDirectives(service::FieldParams&&) const
{
	return _directives;
}

BaseType::BaseType(response::StringType&& description)
	: _description(std::move(description))
{
}

service::FieldResult<std::optional<response::StringType>> BaseType::getName(service::FieldParams&&) const
{
	return std::nullopt;
}

service::FieldResult<std::optional<response::StringType>> BaseType::getDescription(service::FieldParams&&) const
{
	return { _description.empty()
		? std::nullopt
		: std::make_optional<response::StringType>(_description) };
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Field>>>> BaseType::getFields(service::FieldParams&&, std::optional<response::BooleanType>&& /*includeDeprecatedArg*/) const
{
	return std::nullopt;
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> BaseType::getInterfaces(service::FieldParams&&) const
{
	return std::nullopt;
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> BaseType::getPossibleTypes(service::FieldParams&&) const
{
	return std::nullopt;
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::EnumValue>>>> BaseType::getEnumValues(service::FieldParams&&, std::optional<response::BooleanType>&& /*includeDeprecatedArg*/) const
{
	return std::nullopt;
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::InputValue>>>> BaseType::getInputFields(service::FieldParams&&) const
{
	return std::nullopt;
}

service::FieldResult<std::shared_ptr<object::Type>> BaseType::getOfType(service::FieldParams&&) const
{
	return std::shared_ptr<object::Type>{};
}

ScalarType::ScalarType(response::StringType&& name, response::StringType&& description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

service::FieldResult<TypeKind> ScalarType::getKind(service::FieldParams&&) const
{
	return TypeKind::SCALAR;
}

service::FieldResult<std::optional<response::StringType>> ScalarType::getName(service::FieldParams&&) const
{
	std::promise<std::optional<response::StringType>> promise;

	promise.set_value(std::make_optional<response::StringType>(_name));

	return promise.get_future();
}

ObjectType::ObjectType(response::StringType&& name, response::StringType&& description)
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

service::FieldResult<TypeKind> ObjectType::getKind(service::FieldParams&&) const
{
	return TypeKind::OBJECT;
}

service::FieldResult<std::optional<response::StringType>> ObjectType::getName(service::FieldParams&&) const
{
	return std::make_optional<response::StringType>(_name);
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Field>>>> ObjectType::getFields(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const
{
	const bool deprecated = includeDeprecatedArg && *includeDeprecatedArg;
	auto result = std::make_optional<std::vector<std::shared_ptr<object::Field>>>();

	result->reserve(_fields.size());
	std::copy_if(_fields.cbegin(), _fields.cend(), std::back_inserter(*result),
		[&params, deprecated](const std::shared_ptr<Field>& field)
	{
		return deprecated
			|| !field->getIsDeprecated(service::FieldParams(params, response::Value(response::Type::Map))).get();
	});

	return { std::move(result) };
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> ObjectType::getInterfaces(service::FieldParams&&) const
{
	auto result = std::make_optional<std::vector<std::shared_ptr<object::Type>>>(_interfaces.size());

	std::copy(_interfaces.cbegin(), _interfaces.cend(), result->begin());
	
	return { std::move(result) };
}

InterfaceType::InterfaceType(response::StringType&& name, response::StringType&& description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void InterfaceType::AddFields(std::vector<std::shared_ptr<Field>> fields)
{
	_fields = std::move(fields);
}

service::FieldResult<TypeKind> InterfaceType::getKind(service::FieldParams&&) const
{
	return TypeKind::INTERFACE;
}

service::FieldResult<std::optional<response::StringType>> InterfaceType::getName(service::FieldParams&&) const
{
	return std::make_optional<response::StringType>(_name);
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Field>>>> InterfaceType::getFields(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const
{
	const bool deprecated = includeDeprecatedArg && *includeDeprecatedArg;
	auto result = std::make_optional<std::vector<std::shared_ptr<object::Field>>>();

	result->reserve(_fields.size());
	std::copy_if(_fields.cbegin(), _fields.cend(), std::back_inserter(*result),
		[&params, deprecated](const std::shared_ptr<Field>& field)
	{
		return deprecated
			|| !field->getIsDeprecated(service::FieldParams(params, response::Value(response::Type::Map))).get();
	});
	
	return { std::move(result) };
}

UnionType::UnionType(response::StringType&& name, response::StringType&& description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void UnionType::AddPossibleTypes(std::vector<std::weak_ptr<object::Type>> possibleTypes)
{
	_possibleTypes = std::move(possibleTypes);
}

service::FieldResult<TypeKind> UnionType::getKind(service::FieldParams&&) const
{
	return TypeKind::UNION;
}

service::FieldResult<std::optional<response::StringType>> UnionType::getName(service::FieldParams&&) const
{
	return std::make_optional<response::StringType>(_name);
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> UnionType::getPossibleTypes(service::FieldParams&&) const
{
	auto result = std::make_optional<std::vector<std::shared_ptr<object::Type>>>(_possibleTypes.size());

	std::transform(_possibleTypes.cbegin(), _possibleTypes.cend(), result->begin(),
		[](const std::weak_ptr<object::Type>& weak)
	{
		return weak.lock();
	});

	return { std::move(result) };
}

EnumType::EnumType(response::StringType&& name, response::StringType&& description)
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
			std::move(value.deprecationReason)));
	}
}

service::FieldResult<TypeKind> EnumType::getKind(service::FieldParams&&) const
{
	return TypeKind::ENUM;
}

service::FieldResult<std::optional<response::StringType>> EnumType::getName(service::FieldParams&&) const
{
	return std::make_optional<response::StringType>(_name);
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::EnumValue>>>> EnumType::getEnumValues(service::FieldParams&& params, std::optional<response::BooleanType>&& includeDeprecatedArg) const
{
	const bool deprecated = includeDeprecatedArg && *includeDeprecatedArg;
	auto result = std::make_optional<std::vector<std::shared_ptr<object::EnumValue>>>();

	result->reserve(_enumValues.size());
	std::copy_if(_enumValues.cbegin(), _enumValues.cend(), std::back_inserter(*result),
		[&params, deprecated](const std::shared_ptr<object::EnumValue>& value)
	{
		return deprecated
			|| !value->getIsDeprecated(service::FieldParams(params, response::Value(response::Type::Map))).get();
	});

	return { std::move(result) };
}

InputObjectType::InputObjectType(response::StringType&& name, response::StringType&& description)
	: BaseType(std::move(description))
	, _name(std::move(name))
{
}

void InputObjectType::AddInputValues(std::vector<std::shared_ptr<InputValue>> inputValues)
{
	_inputValues = std::move(inputValues);
}

service::FieldResult<TypeKind> InputObjectType::getKind(service::FieldParams&&) const
{
	return TypeKind::INPUT_OBJECT;
}

service::FieldResult<std::optional<response::StringType>> InputObjectType::getName(service::FieldParams&&) const
{
	return std::make_optional<response::StringType>(_name);
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::InputValue>>>> InputObjectType::getInputFields(service::FieldParams&&) const
{
	auto result = std::make_optional<std::vector<std::shared_ptr<object::InputValue>>>(_inputValues.size());

	std::copy(_inputValues.cbegin(), _inputValues.cend(), result->begin());

	return { std::move(result) };
}

WrapperType::WrapperType(TypeKind kind, const std::shared_ptr<object::Type>& ofType)
	: BaseType(response::StringType())
	, _kind(kind)
	, _ofType(ofType)
{
}

service::FieldResult<TypeKind> WrapperType::getKind(service::FieldParams&&) const
{
	return _kind;
}

service::FieldResult<std::shared_ptr<object::Type>> WrapperType::getOfType(service::FieldParams&&) const
{
	return _ofType.lock();
}

Field::Field(response::StringType&& name, response::StringType&& description, std::optional<response::StringType>&& deprecationReason, std::vector<std::shared_ptr<InputValue>>&& args, const std::shared_ptr<object::Type>& type)
	: _name(std::move(name))
	, _description(std::move(description))
	, _deprecationReason(std::move(deprecationReason))
	, _args(std::move(args))
	, _type(type)
{
}

service::FieldResult<response::StringType> Field::getName(service::FieldParams&&) const
{
	return _name;
}

service::FieldResult<std::optional<response::StringType>> Field::getDescription(service::FieldParams&&) const
{
	return {
		_description.empty()
			? std::nullopt
			: std::make_optional<response::StringType>(_description)
	};
}

service::FieldResult<std::vector<std::shared_ptr<object::InputValue>>> Field::getArgs(service::FieldParams&&) const
{
	std::vector<std::shared_ptr<object::InputValue>> result(_args.size());

	std::copy(_args.cbegin(), _args.cend(), result.begin());

	return { std::move(result) };
}

service::FieldResult<std::shared_ptr<object::Type>> Field::getType(service::FieldParams&&) const
{
	return _type.lock();
}

service::FieldResult<response::BooleanType> Field::getIsDeprecated(service::FieldParams&&) const
{
	return _deprecationReason.has_value();
}

service::FieldResult<std::optional<response::StringType>> Field::getDeprecationReason(service::FieldParams&&) const
{
	return {
		_deprecationReason
			? std::make_optional<response::StringType>(*_deprecationReason)
			: std::nullopt
	};
}

InputValue::InputValue(response::StringType&& name, response::StringType&& description, const std::shared_ptr<object::Type>& type, response::StringType&& defaultValue)
	: _name(std::move(name))
	, _description(std::move(description))
	, _type(type)
	, _defaultValue(std::move(defaultValue))
{
}

service::FieldResult<response::StringType> InputValue::getName(service::FieldParams&&) const
{
	return _name;
}

service::FieldResult<std::optional<response::StringType>> InputValue::getDescription(service::FieldParams&&) const
{
	return {
		_description.empty()
			? std::nullopt
			: std::make_optional<response::StringType>(_description)
	};
}

service::FieldResult<std::shared_ptr<object::Type>> InputValue::getType(service::FieldParams&&) const
{
	return _type.lock();
}

service::FieldResult<std::optional<response::StringType>> InputValue::getDefaultValue(service::FieldParams&&) const
{
	return {
		_defaultValue.empty()
			? std::nullopt
			: std::make_optional<response::StringType>(_defaultValue)
	};
}

EnumValue::EnumValue(response::StringType&& name, response::StringType&& description, std::optional<response::StringType>&& deprecationReason)
	: _name(std::move(name))
	, _description(std::move(description))
	, _deprecationReason(std::move(deprecationReason))
{
}

service::FieldResult<response::StringType> EnumValue::getName(service::FieldParams&&) const
{
	return _name;
}

service::FieldResult<std::optional<response::StringType>> EnumValue::getDescription(service::FieldParams&&) const
{
	return {
		_description.empty()
			? std::nullopt
			: std::make_optional<response::StringType>(_description)
	};
}

service::FieldResult<response::BooleanType> EnumValue::getIsDeprecated(service::FieldParams&&) const
{
	return _deprecationReason.has_value();
}

service::FieldResult<std::optional<response::StringType>> EnumValue::getDeprecationReason(service::FieldParams&&) const
{
	return {
		_deprecationReason
			? std::make_optional<response::StringType>(*_deprecationReason)
			: std::nullopt
	};
}

Directive::Directive(response::StringType&& name, response::StringType&& description, std::vector<response::StringType>&& locations, std::vector<std::shared_ptr<InputValue>>&& args)
	: _name(std::move(name))
	, _description(std::move(description))
	, _locations([](std::vector<response::StringType>&& locationsArg) -> std::vector<DirectiveLocation>
		{
			std::vector<DirectiveLocation> result(locationsArg.size());

			std::transform(locationsArg.begin(), locationsArg.end(), result.begin(),
				[](std::string& name) -> DirectiveLocation
				{
					response::Value location(response::Type::EnumValue);

					location.set<response::StringType>(std::move(name));
					return service::ModifiedArgument<DirectiveLocation>::convert(location);
				});

			return result;
		}(std::move(locations)))
	, _args(std::move(args))
{
}

service::FieldResult<response::StringType> Directive::getName(service::FieldParams&&) const
{
	return _name;
}

service::FieldResult<std::optional<response::StringType>> Directive::getDescription(service::FieldParams&&) const
{
	return {
		_description.empty()
			? std::nullopt
			: std::make_optional<response::StringType>(_description)
	};
}

service::FieldResult<std::vector<DirectiveLocation>> Directive::getLocations(service::FieldParams&&) const
{
	std::vector<DirectiveLocation> result(_locations.size());

	std::copy(_locations.cbegin(), _locations.cend(), result.begin());

	return { std::move(result) };
}

service::FieldResult<std::vector<std::shared_ptr<object::InputValue>>> Directive::getArgs(service::FieldParams&&) const
{
	std::vector<std::shared_ptr<object::InputValue>> result(_args.size());

	std::copy(_args.cbegin(), _args.cend(), result.begin());

	return { std::move(result) };
}

} /* namespace graphql::introspection */
