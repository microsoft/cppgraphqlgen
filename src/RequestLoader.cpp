// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "RequestLoader.h"
#include "Validation.h"

#include "graphqlservice/introspection/Introspection.h"

#include "graphqlservice/GraphQLGrammar.h"

#include <array>
#include <sstream>

namespace graphql::generator {

RequestLoader::RequestLoader(RequestOptions&& requestOptions, const SchemaLoader& schemaLoader)
	: _requestOptions(std::move(requestOptions))
{
	_ast = peg::parseFile(_requestOptions.requestFilename);

	if (!_ast.root)
	{
		throw std::logic_error("Unable to parse the request document, but there was no error "
			"message from the parser!");
	}

	_schema = buildSchema(schemaLoader);
	validateRequest();
}

void RequestLoader::validateRequest() const
{
	service::ValidateExecutableVisitor validation { _schema };

	validation.visit(*_ast.root);

	auto errors = validation.getStructuredErrors();

	if (!errors.empty())
	{
		throw service::schema_exception { std::move(errors) };
	}
}

std::shared_ptr<schema::Schema> RequestLoader::buildSchema(const SchemaLoader& schemaLoader) const
{
	auto schema = std::make_shared<schema::Schema>(_requestOptions.noIntrospection);

	introspection::AddTypesToSchema(schema);
	addTypesToSchema(schemaLoader, schema);

	return schema;
}

void RequestLoader::addTypesToSchema(const SchemaLoader& schemaLoader, const std::shared_ptr<schema::Schema>& schema) const
{
	if (!schemaLoader.getScalarTypes().empty())
	{
		for (const auto& scalarType : schemaLoader.getScalarTypes())
		{
			schema->AddType(scalarType.type,
				schema::ScalarType::Make(scalarType.type, scalarType.description));
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::EnumType>> enumTypes;

	if (!schemaLoader.getEnumTypes().empty())
	{
		for (const auto& enumType : schemaLoader.getEnumTypes())
		{
			const auto itr = enumTypes
				.emplace(std::make_pair(enumType.type,
					schema::EnumType::Make(enumType.type, enumType.description)))
				.first;

			schema->AddType(enumType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::InputObjectType>> inputTypes;

	if (!schemaLoader.getInputTypes().empty())
	{
		for (const auto& inputType : schemaLoader.getInputTypes())
		{
			const auto itr =
				inputTypes
				.emplace(std::make_pair(inputType.type,
					schema::InputObjectType::Make(inputType.type, inputType.description)))
				.first;

			schema->AddType(inputType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::UnionType>> unionTypes;

	if (!schemaLoader.getUnionTypes().empty())
	{
		for (const auto& unionType : schemaLoader.getUnionTypes())
		{
			const auto itr =
				unionTypes
				.emplace(std::make_pair(unionType.type,
					schema::UnionType::Make(unionType.type, unionType.description)))
				.first;

			schema->AddType(unionType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::InterfaceType>> interfaceTypes;

	if (!schemaLoader.getInterfaceTypes().empty())
	{
		for (const auto& interfaceType : schemaLoader.getInterfaceTypes())
		{
			const auto itr =
				interfaceTypes
				.emplace(std::make_pair(interfaceType.type,
					schema::InterfaceType::Make(interfaceType.type, interfaceType.description)))
				.first;

			schema->AddType(interfaceType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::ObjectType>> objectTypes;

	if (!schemaLoader.getObjectTypes().empty())
	{
		for (const auto& objectType : schemaLoader.getObjectTypes())
		{
			const auto itr =
				objectTypes
				.emplace(std::make_pair(objectType.type,
					schema::ObjectType::Make(objectType.type, objectType.description)))
				.first;

			schema->AddType(objectType.type, itr->second);
		}
	}

	for (const auto& enumType : schemaLoader.getEnumTypes())
	{
		const auto itr = enumTypes.find(enumType.type);

		if (itr != enumTypes.cend() && !enumType.values.empty())
		{
			std::vector<schema::EnumValueType> values(enumType.values.size());

			std::transform(enumType.values.cbegin(),
				enumType.values.cend(),
				values.begin(),
				[](const EnumValueType& value) noexcept
			{
				return schema::EnumValueType {
					value.value,
					value.description,
					value.deprecationReason,
				};
			});

			itr->second->AddEnumValues(std::move(values));
		}
	}

	for (const auto& inputType : schemaLoader.getInputTypes())
	{
		const auto itr = inputTypes.find(inputType.type);

		if (itr != inputTypes.cend() && !inputType.fields.empty())
		{
			std::vector<std::shared_ptr<const schema::InputValue>> fields(inputType.fields.size());

			std::transform(inputType.fields.cbegin(),
				inputType.fields.cend(),
				fields.begin(),
				[schema](const InputField& field) noexcept
			{
				return schema::InputValue::Make(field.name,
					field.description,
					getIntrospectionType(schema, field.type, field.modifiers),
					field.defaultValueString);
			});

			itr->second->AddInputValues(std::move(fields));
		}
	}

	for (const auto& unionType : schemaLoader.getUnionTypes())
	{
		const auto itr = unionTypes.find(unionType.type);

		if (!unionType.options.empty())
		{
			std::vector<std::weak_ptr<const schema::BaseType>> options(unionType.options.size());

			std::transform(unionType.options.cbegin(),
				unionType.options.cend(),
				options.begin(),
				[schema](std::string_view option) noexcept
			{
				return schema->LookupType(option);
			});

			itr->second->AddPossibleTypes(std::move(options));
		}
	}

	for (const auto& interfaceType : schemaLoader.getInterfaceTypes())
	{
		const auto itr = interfaceTypes.find(interfaceType.type);

		if (!interfaceType.fields.empty())
		{
			std::vector<std::shared_ptr<const schema::Field>> fields(interfaceType.fields.size());

			std::transform(interfaceType.fields.cbegin(),
				interfaceType.fields.cend(),
				fields.begin(),
				[schema](const OutputField& field) noexcept
			{
				std::vector<std::shared_ptr<const schema::InputValue>> arguments(
					field.arguments.size());

				std::transform(field.arguments.cbegin(),
					field.arguments.cend(),
					arguments.begin(),
					[schema](const InputField& argument) noexcept
				{
					return schema::InputValue::Make(argument.name,
						argument.description,
						getIntrospectionType(schema, argument.type, argument.modifiers),
						argument.defaultValueString);
				});

				return schema::Field::Make(field.name,
					field.description,
					field.deprecationReason,
					getIntrospectionType(schema, field.type, field.modifiers),
					std::move(arguments));
			});

			itr->second->AddFields(std::move(fields));
		}
	}

	for (const auto& objectType : schemaLoader.getObjectTypes())
	{
		const auto itr = objectTypes.find(objectType.type);

		if (!objectType.interfaces.empty())
		{
			std::vector<std::shared_ptr<const schema::InterfaceType>> interfaces(
				objectType.interfaces.size());

			std::transform(objectType.interfaces.cbegin(),
				objectType.interfaces.cend(),
				interfaces.begin(),
				[schema, &interfaceTypes](std::string_view interfaceName) noexcept
			{
				return interfaceTypes[interfaceName];
			});

			itr->second->AddInterfaces(std::move(interfaces));
		}

		if (!objectType.fields.empty())
		{
			std::vector<std::shared_ptr<const schema::Field>> fields(objectType.fields.size());

			std::transform(objectType.fields.cbegin(),
				objectType.fields.cend(),
				fields.begin(),
				[schema](const OutputField& field) noexcept
			{
				std::vector<std::shared_ptr<const schema::InputValue>> arguments(
					field.arguments.size());

				std::transform(field.arguments.cbegin(),
					field.arguments.cend(),
					arguments.begin(),
					[schema](const InputField& argument) noexcept
				{
					return schema::InputValue::Make(argument.name,
						argument.description,
						getIntrospectionType(schema, argument.type, argument.modifiers),
						argument.defaultValueString);
				});

				return schema::Field::Make(field.name,
					field.description,
					field.deprecationReason,
					getIntrospectionType(schema, field.type, field.modifiers),
					std::move(arguments));
			});

			itr->second->AddFields(std::move(fields));
		}
	}

	for (const auto& directive : schemaLoader.getDirectives())
	{
		std::vector<introspection::DirectiveLocation> locations(directive.locations.size());

		std::transform(directive.locations.cbegin(),
			directive.locations.cend(),
			locations.begin(),
			[](std::string_view locationName) noexcept
		{
			response::Value locationValue(response::Type::EnumValue);

			locationValue.set<response::StringType>(response::StringType { locationName });

			return service::ModifiedArgument<introspection::DirectiveLocation>::convert(
				locationValue);
		});

		std::vector<std::shared_ptr<const schema::InputValue>> arguments(
			directive.arguments.size());

		std::transform(directive.arguments.cbegin(),
			directive.arguments.cend(),
			arguments.begin(),
			[schema](const InputField& argument) noexcept
		{
			return schema::InputValue::Make(argument.name,
				argument.description,
				getIntrospectionType(schema, argument.type, argument.modifiers),
				argument.defaultValueString);
		});

		schema->AddDirective(schema::Directive::Make(directive.name,
			directive.description,
			std::move(locations),
			std::move(arguments)));
	}

	for (const auto& operationType : schemaLoader.getOperationTypes())
	{
		const auto itr = objectTypes.find(operationType.type);

		if (operationType.operation == service::strQuery)
		{
			schema->AddQueryType(itr->second);
		}
		else if (operationType.operation == service::strMutation)
		{
			schema->AddMutationType(itr->second);
		}
		else if (operationType.operation == service::strSubscription)
		{
			schema->AddSubscriptionType(itr->second);
		}
	}
}

std::shared_ptr<const schema::BaseType> RequestLoader::getIntrospectionType(
	const std::shared_ptr<schema::Schema>& schema, std::string_view type,
	const TypeModifierStack& modifiers) noexcept
{
	std::shared_ptr<const schema::BaseType> introspectionType = schema->LookupType(type);

	if (introspectionType)
	{
		bool nonNull = true;

		for (auto itr = modifiers.crbegin(); itr != modifiers.crend(); ++itr)
		{
			if (nonNull)
			{
				switch (*itr)
				{
					case service::TypeModifier::None:
					case service::TypeModifier::List:
						introspectionType = schema->WrapType(introspection::TypeKind::NON_NULL,
							std::move(introspectionType));
						break;

					case service::TypeModifier::Nullable:
						// If the next modifier is Nullable that cancels the non-nullable state.
						nonNull = false;
						break;
				}
			}

			switch (*itr)
			{
				case service::TypeModifier::None:
				{
					nonNull = true;
					break;
				}

				case service::TypeModifier::List:
				{
					nonNull = true;
					introspectionType = schema->WrapType(introspection::TypeKind::LIST,
						std::move(introspectionType));
					break;
				}

				case service::TypeModifier::Nullable:
					break;
			}
		}

		if (nonNull)
		{
			introspectionType =
				schema->WrapType(introspection::TypeKind::NON_NULL, std::move(introspectionType));
		}
	}

	return introspectionType;
}

} /* namespace graphql::generator */
