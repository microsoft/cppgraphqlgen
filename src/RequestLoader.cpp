// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "RequestLoader.h"
#include "Validation.h"

#include "graphqlservice/introspection/Introspection.h"

#include "graphqlservice/GraphQLGrammar.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iterator>

using namespace std::literals;

namespace graphql::generator {

RequestLoader::RequestLoader(RequestOptions&& requestOptions, const SchemaLoader& schemaLoader)
	: _requestOptions(std::move(requestOptions))
	, _schemaLoader(schemaLoader)
{
	std::ifstream document { _requestOptions.requestFilename };
	std::istreambuf_iterator<char> itr { document }, itrEnd;
	_requestText = std::string { itr, itrEnd };

	buildSchema();

	_ast = peg::parseFile(_requestOptions.requestFilename);

	if (!_ast.root)
	{
		throw std::logic_error("Unable to parse the request document, but there was no error "
							   "message from the parser!");
	}

	validateRequest();
}

std::string_view RequestLoader::getRequestFilename() const noexcept
{
	return _requestOptions.requestFilename;
}

std::string_view RequestLoader::getOperationName() const noexcept
{
	return _requestOptions.operationName.empty() ? "(default)"sv : _requestOptions.operationName;
}

std::string_view RequestLoader::getRequestText() const noexcept
{
	return trimWhitespace(_requestText);
}

const ResponseType& RequestLoader::getVariablesType() const noexcept
{
	return _variablesType;
}

const ResponseType& RequestLoader::getResponseType() const noexcept
{
	return _responseType;
}

void RequestLoader::buildSchema()
{
	_schema = std::make_shared<schema::Schema>(_requestOptions.noIntrospection);
	introspection::AddTypesToSchema(_schema);
	addTypesToSchema();
}

void RequestLoader::addTypesToSchema()
{
	if (!_schemaLoader.getScalarTypes().empty())
	{
		for (const auto& scalarType : _schemaLoader.getScalarTypes())
		{
			_schema->AddType(scalarType.type,
				schema::ScalarType::Make(scalarType.type, scalarType.description));
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::EnumType>> enumTypes;

	if (!_schemaLoader.getEnumTypes().empty())
	{
		for (const auto& enumType : _schemaLoader.getEnumTypes())
		{
			const auto itr = enumTypes
								 .emplace(std::make_pair(enumType.type,
									 schema::EnumType::Make(enumType.type, enumType.description)))
								 .first;

			_schema->AddType(enumType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::InputObjectType>> inputTypes;

	if (!_schemaLoader.getInputTypes().empty())
	{
		for (const auto& inputType : _schemaLoader.getInputTypes())
		{
			const auto itr =
				inputTypes
					.emplace(std::make_pair(inputType.type,
						schema::InputObjectType::Make(inputType.type, inputType.description)))
					.first;

			_schema->AddType(inputType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::UnionType>> unionTypes;

	if (!_schemaLoader.getUnionTypes().empty())
	{
		for (const auto& unionType : _schemaLoader.getUnionTypes())
		{
			const auto itr =
				unionTypes
					.emplace(std::make_pair(unionType.type,
						schema::UnionType::Make(unionType.type, unionType.description)))
					.first;

			_schema->AddType(unionType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::InterfaceType>> interfaceTypes;

	if (!_schemaLoader.getInterfaceTypes().empty())
	{
		for (const auto& interfaceType : _schemaLoader.getInterfaceTypes())
		{
			const auto itr =
				interfaceTypes
					.emplace(std::make_pair(interfaceType.type,
						schema::InterfaceType::Make(interfaceType.type, interfaceType.description)))
					.first;

			_schema->AddType(interfaceType.type, itr->second);
		}
	}

	std::map<std::string_view, std::shared_ptr<schema::ObjectType>> objectTypes;

	if (!_schemaLoader.getObjectTypes().empty())
	{
		for (const auto& objectType : _schemaLoader.getObjectTypes())
		{
			const auto itr =
				objectTypes
					.emplace(std::make_pair(objectType.type,
						schema::ObjectType::Make(objectType.type, objectType.description)))
					.first;

			_schema->AddType(objectType.type, itr->second);
		}
	}

	for (const auto& enumType : _schemaLoader.getEnumTypes())
	{
		const auto itr = enumTypes.find(enumType.type);

		if (itr != enumTypes.cend() && !enumType.values.empty())
		{
			std::vector<schema::EnumValueType> values(enumType.values.size());

			std::transform(enumType.values.cbegin(),
				enumType.values.cend(),
				values.begin(),
				[](const EnumValueType& value) noexcept {
					return schema::EnumValueType {
						value.value,
						value.description,
						value.deprecationReason,
					};
				});

			itr->second->AddEnumValues(std::move(values));
		}
	}

	for (const auto& inputType : _schemaLoader.getInputTypes())
	{
		const auto itr = inputTypes.find(inputType.type);

		if (itr != inputTypes.cend() && !inputType.fields.empty())
		{
			std::vector<std::shared_ptr<const schema::InputValue>> fields(inputType.fields.size());

			std::transform(inputType.fields.cbegin(),
				inputType.fields.cend(),
				fields.begin(),
				[this](const InputField& field) noexcept {
					return schema::InputValue::Make(field.name,
						field.description,
						getSchemaType(field.type, field.modifiers),
						field.defaultValueString);
				});

			itr->second->AddInputValues(std::move(fields));
		}
	}

	for (const auto& unionType : _schemaLoader.getUnionTypes())
	{
		const auto itr = unionTypes.find(unionType.type);

		if (!unionType.options.empty())
		{
			std::vector<std::weak_ptr<const schema::BaseType>> options(unionType.options.size());

			std::transform(unionType.options.cbegin(),
				unionType.options.cend(),
				options.begin(),
				[this](std::string_view option) noexcept {
					return _schema->LookupType(option);
				});

			itr->second->AddPossibleTypes(std::move(options));
		}
	}

	for (const auto& interfaceType : _schemaLoader.getInterfaceTypes())
	{
		const auto itr = interfaceTypes.find(interfaceType.type);

		if (!interfaceType.fields.empty())
		{
			std::vector<std::shared_ptr<const schema::Field>> fields(interfaceType.fields.size());

			std::transform(interfaceType.fields.cbegin(),
				interfaceType.fields.cend(),
				fields.begin(),
				[this](const OutputField& field) noexcept {
					std::vector<std::shared_ptr<const schema::InputValue>> arguments(
						field.arguments.size());

					std::transform(field.arguments.cbegin(),
						field.arguments.cend(),
						arguments.begin(),
						[this](const InputField& argument) noexcept {
							return schema::InputValue::Make(argument.name,
								argument.description,
								getSchemaType(argument.type, argument.modifiers),
								argument.defaultValueString);
						});

					return schema::Field::Make(field.name,
						field.description,
						field.deprecationReason,
						getSchemaType(field.type, field.modifiers),
						std::move(arguments));
				});

			itr->second->AddFields(std::move(fields));
		}
	}

	for (const auto& objectType : _schemaLoader.getObjectTypes())
	{
		const auto itr = objectTypes.find(objectType.type);

		if (!objectType.interfaces.empty())
		{
			std::vector<std::shared_ptr<const schema::InterfaceType>> interfaces(
				objectType.interfaces.size());

			std::transform(objectType.interfaces.cbegin(),
				objectType.interfaces.cend(),
				interfaces.begin(),
				[this, &interfaceTypes](std::string_view interfaceName) noexcept {
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
				[this](const OutputField& field) noexcept {
					std::vector<std::shared_ptr<const schema::InputValue>> arguments(
						field.arguments.size());

					std::transform(field.arguments.cbegin(),
						field.arguments.cend(),
						arguments.begin(),
						[this](const InputField& argument) noexcept {
							return schema::InputValue::Make(argument.name,
								argument.description,
								getSchemaType(argument.type, argument.modifiers),
								argument.defaultValueString);
						});

					return schema::Field::Make(field.name,
						field.description,
						field.deprecationReason,
						getSchemaType(field.type, field.modifiers),
						std::move(arguments));
				});

			itr->second->AddFields(std::move(fields));
		}
	}

	for (const auto& directive : _schemaLoader.getDirectives())
	{
		std::vector<introspection::DirectiveLocation> locations(directive.locations.size());

		std::transform(directive.locations.cbegin(),
			directive.locations.cend(),
			locations.begin(),
			[](std::string_view locationName) noexcept {
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
			[this](const InputField& argument) noexcept {
				return schema::InputValue::Make(argument.name,
					argument.description,
					getSchemaType(argument.type, argument.modifiers),
					argument.defaultValueString);
			});

		_schema->AddDirective(schema::Directive::Make(directive.name,
			directive.description,
			std::move(locations),
			std::move(arguments)));
	}

	for (const auto& operationType : _schemaLoader.getOperationTypes())
	{
		const auto itr = objectTypes.find(operationType.type);

		if (operationType.operation == service::strQuery)
		{
			_schema->AddQueryType(itr->second);
		}
		else if (operationType.operation == service::strMutation)
		{
			_schema->AddMutationType(itr->second);
		}
		else if (operationType.operation == service::strSubscription)
		{
			_schema->AddSubscriptionType(itr->second);
		}
	}
}

std::shared_ptr<const schema::BaseType> RequestLoader::getSchemaType(
	std::string_view type, const TypeModifierStack& modifiers) const noexcept
{
	std::shared_ptr<const schema::BaseType> introspectionType = _schema->LookupType(type);

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
						introspectionType = _schema->WrapType(introspection::TypeKind::NON_NULL,
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
					introspectionType = _schema->WrapType(introspection::TypeKind::LIST,
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
				_schema->WrapType(introspection::TypeKind::NON_NULL, std::move(introspectionType));
		}
	}

	return introspectionType;
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

std::string_view RequestLoader::trimWhitespace(std::string_view content) noexcept
{
	const auto isSpacePredicate = [](char ch) noexcept {
		return std::isspace(static_cast<int>(ch)) != 0;
	};
	const auto skip = std::distance(content.begin(),
		std::find_if_not(content.begin(), content.end(), isSpacePredicate));
	const auto length =
		std::distance(std::find_if_not(content.rbegin(), content.rend(), isSpacePredicate),
			content.rend());

	if (skip >= 0 && length >= skip)
	{
		content = content.substr(static_cast<size_t>(skip), static_cast<size_t>(length - skip));
	}

	return content;
}

} /* namespace graphql::generator */
