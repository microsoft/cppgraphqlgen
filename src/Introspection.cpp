// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/introspection/Introspection.h"

namespace graphql::introspection {

Schema::Schema(const std::shared_ptr<schema::Schema>& schema)
	: _schema(schema)
{
}

service::FieldResult<std::vector<std::shared_ptr<object::Type>>> Schema::getTypes(
	service::FieldParams&&) const
{
	const auto& types = _schema->types();
	std::vector<std::shared_ptr<object::Type>> result(types.size());

	std::transform(types.begin(), types.end(), result.begin(), [](const auto& entry) {
		return std::make_shared<object::Type>(std::make_shared<Type>(entry.second));
	});

	return result;
}

service::FieldResult<std::shared_ptr<object::Type>> Schema::getQueryType(
	service::FieldParams&&) const
{
	const auto& queryType = _schema->queryType();

	return queryType ? std::make_shared<object::Type>(std::make_shared<Type>(queryType)) : nullptr;
}

service::FieldResult<std::shared_ptr<object::Type>> Schema::getMutationType(
	service::FieldParams&&) const
{
	const auto& mutationType = _schema->mutationType();

	return mutationType ? std::make_shared<object::Type>(std::make_shared<Type>(mutationType))
						: nullptr;
}

service::FieldResult<std::shared_ptr<object::Type>> Schema::getSubscriptionType(
	service::FieldParams&&) const
{
	const auto& subscriptionType = _schema->subscriptionType();

	return subscriptionType
		? std::make_shared<object::Type>(std::make_shared<Type>(subscriptionType))
		: nullptr;
}

service::FieldResult<std::vector<std::shared_ptr<object::Directive>>> Schema::getDirectives(
	service::FieldParams&&) const
{
	const auto& directives = _schema->directives();
	std::vector<std::shared_ptr<object::Directive>> result(directives.size());

	std::transform(directives.begin(), directives.end(), result.begin(), [](const auto& entry) {
		return std::make_shared<object::Directive>(std::make_shared<Directive>(entry));
	});

	return result;
}

Type::Type(const std::shared_ptr<const schema::BaseType>& type)
	: _type(type)
{
}

service::FieldResult<TypeKind> Type::getKind(service::FieldParams&&) const
{
	return _type->kind();
}

service::FieldResult<std::optional<response::StringType>> Type::getName(
	service::FieldParams&&) const
{
	const auto name = _type->name();

	return { name.empty() ? std::nullopt : std::make_optional<response::StringType>(name) };
}

service::FieldResult<std::optional<response::StringType>> Type::getDescription(
	service::FieldParams&&) const
{
	const auto description = _type->description();

	return { description.empty() ? std::nullopt
								 : std::make_optional<response::StringType>(description) };
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Field>>>> Type::getFields(
	service::FieldParams&&, std::optional<response::BooleanType>&& includeDeprecatedArg) const
{
	switch (_type->kind())
	{
		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
			break;

		default:
			return std::nullopt;
	}

	const auto& fields = _type->fields();
	const bool deprecated = includeDeprecatedArg && *includeDeprecatedArg;
	auto result = std::make_optional<std::vector<std::shared_ptr<object::Field>>>();

	result->reserve(fields.size());
	for (const auto& field : fields)
	{
		if (deprecated || !field->deprecationReason())
		{
			result->push_back(std::make_shared<object::Field>(std::make_shared<Field>(field)));
		}
	}

	return result;
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> Type::getInterfaces(
	service::FieldParams&&) const
{
	switch (_type->kind())
	{
		case introspection::TypeKind::OBJECT:
			break;

		default:
			return std::nullopt;
	}

	const auto& interfaces = _type->interfaces();
	auto result = std::make_optional<std::vector<std::shared_ptr<object::Type>>>(interfaces.size());

	std::transform(interfaces.begin(), interfaces.end(), result->begin(), [](const auto& entry) {
		return std::make_shared<object::Type>(std::make_shared<Type>(entry));
	});

	return result;
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::Type>>>> Type::
	getPossibleTypes(service::FieldParams&&) const
{
	switch (_type->kind())
	{
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
			break;

		default:
			return std::nullopt;
	}

	const auto& possibleTypes = _type->possibleTypes();
	auto result =
		std::make_optional<std::vector<std::shared_ptr<object::Type>>>(possibleTypes.size());

	std::transform(possibleTypes.begin(),
		possibleTypes.end(),
		result->begin(),
		[](const auto& entry) {
			return std::make_shared<object::Type>(std::make_shared<Type>(entry.lock()));
		});

	return result;
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::EnumValue>>>> Type::
	getEnumValues(
		service::FieldParams&&, std::optional<response::BooleanType>&& includeDeprecatedArg) const
{
	switch (_type->kind())
	{
		case introspection::TypeKind::ENUM:
			break;

		default:
			return std::nullopt;
	}

	const auto& enumValues = _type->enumValues();
	const bool deprecated = includeDeprecatedArg && *includeDeprecatedArg;
	auto result = std::make_optional<std::vector<std::shared_ptr<object::EnumValue>>>();

	result->reserve(enumValues.size());
	for (const auto& value : enumValues)
	{
		if (deprecated || !value->deprecationReason())
		{
			result->push_back(
				std::make_shared<object::EnumValue>(std::make_shared<EnumValue>(value)));
		}
	}

	return result;
}

service::FieldResult<std::optional<std::vector<std::shared_ptr<object::InputValue>>>> Type::
	getInputFields(service::FieldParams&&) const
{
	switch (_type->kind())
	{
		case introspection::TypeKind::INPUT_OBJECT:
			break;

		default:
			return std::nullopt;
	}

	const auto& inputFields = _type->inputFields();
	auto result =
		std::make_optional<std::vector<std::shared_ptr<object::InputValue>>>(inputFields.size());

	std::transform(inputFields.begin(), inputFields.end(), result->begin(), [](const auto& entry) {
		return std::make_shared<object::InputValue>(std::make_shared<InputValue>(entry));
	});

	return result;
}

service::FieldResult<std::shared_ptr<object::Type>> Type::getOfType(service::FieldParams&&) const
{
	switch (_type->kind())
	{
		case introspection::TypeKind::LIST:
		case introspection::TypeKind::NON_NULL:
			break;

		default:
			return nullptr;
	}

	const auto ofType = _type->ofType().lock();

	return ofType ? std::make_shared<object::Type>(std::make_shared<Type>(ofType)) : nullptr;
}

Field::Field(const std::shared_ptr<const schema::Field>& field)
	: _field(field)
{
}

service::FieldResult<response::StringType> Field::getName(service::FieldParams&&) const
{
	return response::StringType { _field->name() };
}

service::FieldResult<std::optional<response::StringType>> Field::getDescription(
	service::FieldParams&&) const
{
	const auto description = _field->description();

	return { description.empty() ? std::nullopt
								 : std::make_optional<response::StringType>(description) };
}

service::FieldResult<std::vector<std::shared_ptr<object::InputValue>>> Field::getArgs(
	service::FieldParams&&) const
{
	const auto& args = _field->args();
	std::vector<std::shared_ptr<object::InputValue>> result(args.size());

	std::transform(args.begin(), args.end(), result.begin(), [](const auto& entry) {
		return std::make_shared<object::InputValue>(std::make_shared<InputValue>(entry));
	});

	return result;
}

service::FieldResult<std::shared_ptr<object::Type>> Field::getType(service::FieldParams&&) const
{
	const auto type = _field->type().lock();

	return type ? std::make_shared<object::Type>(std::make_shared<Type>(type)) : nullptr;
}

service::FieldResult<response::BooleanType> Field::getIsDeprecated(service::FieldParams&&) const
{
	return _field->deprecationReason().has_value();
}

service::FieldResult<std::optional<response::StringType>> Field::getDeprecationReason(
	service::FieldParams&&) const
{
	const auto& deprecationReason = _field->deprecationReason();

	return { deprecationReason ? std::make_optional<response::StringType>(*deprecationReason)
							   : std::nullopt };
}

InputValue::InputValue(const std::shared_ptr<const schema::InputValue>& inputValue)
	: _inputValue(inputValue)
{
}

service::FieldResult<response::StringType> InputValue::getName(service::FieldParams&&) const
{
	return response::StringType { _inputValue->name() };
}

service::FieldResult<std::optional<response::StringType>> InputValue::getDescription(
	service::FieldParams&&) const
{
	const auto description = _inputValue->description();

	return { description.empty() ? std::nullopt
								 : std::make_optional<response::StringType>(description) };
}

service::FieldResult<std::shared_ptr<object::Type>> InputValue::getType(
	service::FieldParams&&) const
{
	const auto type = _inputValue->type().lock();

	return type ? std::make_shared<object::Type>(std::make_shared<Type>(type)) : nullptr;
}

service::FieldResult<std::optional<response::StringType>> InputValue::getDefaultValue(
	service::FieldParams&&) const
{
	const auto defaultValue = _inputValue->defaultValue();

	return { defaultValue.empty() ? std::nullopt
								  : std::make_optional<response::StringType>(defaultValue) };
}

EnumValue::EnumValue(const std::shared_ptr<const schema::EnumValue>& enumValue)
	: _enumValue(enumValue)
{
}

service::FieldResult<response::StringType> EnumValue::getName(service::FieldParams&&) const
{
	return response::StringType { _enumValue->name() };
}

service::FieldResult<std::optional<response::StringType>> EnumValue::getDescription(
	service::FieldParams&&) const
{
	const auto description = _enumValue->description();

	return { description.empty() ? std::nullopt
								 : std::make_optional<response::StringType>(description) };
}

service::FieldResult<response::BooleanType> EnumValue::getIsDeprecated(service::FieldParams&&) const
{
	return _enumValue->deprecationReason().has_value();
}

service::FieldResult<std::optional<response::StringType>> EnumValue::getDeprecationReason(
	service::FieldParams&&) const
{
	const auto& deprecationReason = _enumValue->deprecationReason();

	return { deprecationReason ? std::make_optional<response::StringType>(*deprecationReason)
							   : std::nullopt };
}

Directive::Directive(const std::shared_ptr<const schema::Directive>& directive)
	: _directive(directive)
{
}

service::FieldResult<response::StringType> Directive::getName(service::FieldParams&&) const
{
	return response::StringType { _directive->name() };
}

service::FieldResult<std::optional<response::StringType>> Directive::getDescription(
	service::FieldParams&&) const
{
	const auto description = _directive->description();

	return { description.empty() ? std::nullopt
								 : std::make_optional<response::StringType>(description) };
}

service::FieldResult<std::vector<DirectiveLocation>> Directive::getLocations(
	service::FieldParams&&) const
{
	return { _directive->locations() };
}

service::FieldResult<std::vector<std::shared_ptr<object::InputValue>>> Directive::getArgs(
	service::FieldParams&&) const
{
	const auto& args = _directive->args();
	std::vector<std::shared_ptr<object::InputValue>> result(args.size());

	std::transform(args.begin(), args.end(), result.begin(), [](const auto& entry) {
		return std::make_shared<object::InputValue>(std::make_shared<InputValue>(entry));
	});

	return result;
}

} // namespace graphql::introspection
