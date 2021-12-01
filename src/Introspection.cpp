// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/introspection/Introspection.h"

namespace graphql::introspection {

Schema::Schema(const std::shared_ptr<schema::Schema>& schema)
	: _schema(schema)
{
}

std::vector<std::shared_ptr<object::Type>> Schema::getTypes() const
{
	const auto& types = _schema->types();
	std::vector<std::shared_ptr<object::Type>> result(types.size());

	std::transform(types.begin(), types.end(), result.begin(), [](const auto& entry) {
		return std::make_shared<object::Type>(std::make_shared<Type>(entry.second));
	});

	return result;
}

std::shared_ptr<object::Type> Schema::getQueryType() const
{
	const auto& queryType = _schema->queryType();

	return queryType ? std::make_shared<object::Type>(std::make_shared<Type>(queryType)) : nullptr;
}

std::shared_ptr<object::Type> Schema::getMutationType() const
{
	const auto& mutationType = _schema->mutationType();

	return mutationType ? std::make_shared<object::Type>(std::make_shared<Type>(mutationType))
						: nullptr;
}

std::shared_ptr<object::Type> Schema::getSubscriptionType() const
{
	const auto& subscriptionType = _schema->subscriptionType();

	return subscriptionType
		? std::make_shared<object::Type>(std::make_shared<Type>(subscriptionType))
		: nullptr;
}

std::vector<std::shared_ptr<object::Directive>> Schema::getDirectives() const
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

TypeKind Type::getKind() const
{
	return _type->kind();
}

std::optional<response::StringType> Type::getName() const
{
	const auto name = _type->name();

	return { name.empty() ? std::nullopt : std::make_optional<response::StringType>(name) };
}

std::optional<response::StringType> Type::getDescription() const
{
	const auto description = _type->description();

	return { description.empty() ? std::nullopt
								 : std::make_optional<response::StringType>(description) };
}

std::optional<std::vector<std::shared_ptr<object::Field>>> Type::getFields(
	std::optional<response::BooleanType>&& includeDeprecatedArg) const
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

std::optional<std::vector<std::shared_ptr<object::Type>>> Type::getInterfaces() const
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

std::optional<std::vector<std::shared_ptr<object::Type>>> Type::getPossibleTypes() const
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

std::optional<std::vector<std::shared_ptr<object::EnumValue>>> Type::getEnumValues(
	std::optional<response::BooleanType>&& includeDeprecatedArg) const
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

std::optional<std::vector<std::shared_ptr<object::InputValue>>> Type::getInputFields() const
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

std::shared_ptr<object::Type> Type::getOfType() const
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

response::StringType Field::getName() const
{
	return response::StringType { _field->name() };
}

std::optional<response::StringType> Field::getDescription() const
{
	const auto description = _field->description();

	return { description.empty() ? std::nullopt
								 : std::make_optional<response::StringType>(description) };
}

std::vector<std::shared_ptr<object::InputValue>> Field::getArgs() const
{
	const auto& args = _field->args();
	std::vector<std::shared_ptr<object::InputValue>> result(args.size());

	std::transform(args.begin(), args.end(), result.begin(), [](const auto& entry) {
		return std::make_shared<object::InputValue>(std::make_shared<InputValue>(entry));
	});

	return result;
}

std::shared_ptr<object::Type> Field::getType() const
{
	const auto type = _field->type().lock();

	return type ? std::make_shared<object::Type>(std::make_shared<Type>(type)) : nullptr;
}

response::BooleanType Field::getIsDeprecated() const
{
	return _field->deprecationReason().has_value();
}

std::optional<response::StringType> Field::getDeprecationReason() const
{
	const auto& deprecationReason = _field->deprecationReason();

	return { deprecationReason ? std::make_optional<response::StringType>(*deprecationReason)
							   : std::nullopt };
}

InputValue::InputValue(const std::shared_ptr<const schema::InputValue>& inputValue)
	: _inputValue(inputValue)
{
}

response::StringType InputValue::getName() const
{
	return response::StringType { _inputValue->name() };
}

std::optional<response::StringType> InputValue::getDescription() const
{
	const auto description = _inputValue->description();

	return { description.empty() ? std::nullopt
								 : std::make_optional<response::StringType>(description) };
}

std::shared_ptr<object::Type> InputValue::getType() const
{
	const auto type = _inputValue->type().lock();

	return type ? std::make_shared<object::Type>(std::make_shared<Type>(type)) : nullptr;
}

std::optional<response::StringType> InputValue::getDefaultValue() const
{
	const auto defaultValue = _inputValue->defaultValue();

	return { defaultValue.empty() ? std::nullopt
								  : std::make_optional<response::StringType>(defaultValue) };
}

EnumValue::EnumValue(const std::shared_ptr<const schema::EnumValue>& enumValue)
	: _enumValue(enumValue)
{
}

response::StringType EnumValue::getName() const
{
	return response::StringType { _enumValue->name() };
}

std::optional<response::StringType> EnumValue::getDescription() const
{
	const auto description = _enumValue->description();

	return { description.empty() ? std::nullopt
								 : std::make_optional<response::StringType>(description) };
}

response::BooleanType EnumValue::getIsDeprecated() const
{
	return _enumValue->deprecationReason().has_value();
}

std::optional<response::StringType> EnumValue::getDeprecationReason() const
{
	const auto& deprecationReason = _enumValue->deprecationReason();

	return { deprecationReason ? std::make_optional<response::StringType>(*deprecationReason)
							   : std::nullopt };
}

Directive::Directive(const std::shared_ptr<const schema::Directive>& directive)
	: _directive(directive)
{
}

response::StringType Directive::getName() const
{
	return response::StringType { _directive->name() };
}

std::optional<response::StringType> Directive::getDescription() const
{
	const auto description = _directive->description();

	return { description.empty() ? std::nullopt
								 : std::make_optional<response::StringType>(description) };
}

std::vector<DirectiveLocation> Directive::getLocations() const
{
	return { _directive->locations() };
}

std::vector<std::shared_ptr<object::InputValue>> Directive::getArgs() const
{
	const auto& args = _directive->args();
	std::vector<std::shared_ptr<object::InputValue>> result(args.size());

	std::transform(args.begin(), args.end(), result.begin(), [](const auto& entry) {
		return std::make_shared<object::InputValue>(std::make_shared<InputValue>(entry));
	});

	return result;
}

} // namespace graphql::introspection
