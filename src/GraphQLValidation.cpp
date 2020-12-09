// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLValidation.h"

namespace graphql::service {

std::optional<std::reference_wrapper<const ValidateDirective>> ValidationContext::getDirective(
	const std::string_view& name) const
{
	// TODO: string is a work around, the directives map will be moved to string_view soon
	const auto& itr = _directives.find(std::string { name });
	if (itr == _directives.cend())
	{
		return std::nullopt;
	}
	return std::optional<std::reference_wrapper<const ValidateDirective>>(itr->second);
}

std::optional<std::reference_wrapper<const std::string>> ValidationContext::getOperationType(
	const std::string_view& name) const
{
	if (name == strQuery)
	{
		return std::optional<std::reference_wrapper<const std::string>>(_operationTypes.queryType);
	}
	if (name == strMutation)
	{
		return std::optional<std::reference_wrapper<const std::string>>(
			_operationTypes.mutationType);
	}
	if (name == strSubscription)
	{
		return std::optional<std::reference_wrapper<const std::string>>(
			_operationTypes.subscriptionType);
	}
	return std::nullopt;
}

constexpr std::string_view introspectionQuery = R"gql(
query IntrospectionQuery {
  __schema {
    queryType { name }
    mutationType { name }
    subscriptionType { name }
    types { ...FullType }
    directives {
      name
      locations
      args { ...InputValue }
    }
  }
}

fragment FullType on __Type {
  kind
  name
  fields(includeDeprecated: true) {
    name
    args { ...InputValue }
    type { ...TypeRef }
  }
  inputFields { ...InputValue }
  interfaces { ...TypeRef }
  enumValues(includeDeprecated: true) { name }
  possibleTypes { ...TypeRef }
}

fragment InputValue on __InputValue {
  name
  type { ...TypeRef }
  defaultValue
}

fragment TypeRef on __Type {
  kind
  name
  ofType {
    kind
    name
    ofType {
      kind
      name
      ofType {
        kind
        name
        ofType {
          kind
          name
          ofType {
            kind
            name
            ofType {
              kind
              name
              ofType {
                kind
                name
              }
            }
          }
        }
      }
    }
  }
}
)gql";

IntrospectionValidationContext::IntrospectionValidationContext(const Request& service)
{
	auto ast = peg::parseString(introspectionQuery);
	// This is taking advantage of the fact that during validation we can choose to execute
	// unvalidated queries against the Introspection schema. This way we can use fragment
	// cycles to expand an arbitrary number of wrapper types.
	ast.validated = true;

	std::shared_ptr<RequestState> state;
	const std::string operationName;
	response::Value variables(response::Type::Map);
	_introspectionQuery = service.resolve(state, ast, operationName, std::move(variables)).get();

	populate();
}

IntrospectionValidationContext::IntrospectionValidationContext(response::Value&& introspectionQuery)
	: _introspectionQuery(std::move(introspectionQuery))
{
	populate();
}

void IntrospectionValidationContext::populate()
{
	commonTypes.string = makeScalarType("String");
	commonTypes.nonNullString = makeNonNullOfType(commonTypes.string);

	const auto& itrData = _introspectionQuery.find(std::string { strData });
	if (itrData == _introspectionQuery.end())
	{
		return;
	}

	const auto& data = itrData->second;
	const auto& itrSchema = data.find(R"gql(__schema)gql");
	if (itrSchema != data.end() && itrSchema->second.type() == response::Type::Map)
	{
		for (auto itr = itrSchema->second.begin(); itr < itrSchema->second.end(); itr++)
		{
			const auto& member = *itr;
			if (member.second.type() == response::Type::Map)
			{
				const auto& itrType = member.second.find(R"gql(name)gql");
				if (itrType != member.second.end()
					&& itrType->second.type() == response::Type::String)
				{
					if (member.first == R"gql(queryType)gql")
					{
						_operationTypes.queryType = itrType->second.get<response::StringType>();
					}
					else if (member.first == R"gql(mutationType)gql")
					{
						_operationTypes.mutationType = itrType->second.get<response::StringType>();
					}
					else if (member.first == R"gql(subscriptionType)gql")
					{
						_operationTypes.subscriptionType =
							itrType->second.get<response::StringType>();
					}
				}
			}
			else if (member.second.type() == response::Type::List
				&& member.first == R"gql(types)gql")
			{
				const auto& entries = member.second.get<response::ListType>();

				// first iteration add the named types
				for (const auto& entry : entries)
				{
					if (entry.type() != response::Type::Map)
					{
						continue;
					}

					const auto& itrName = entry.find(R"gql(name)gql");
					const auto& itrKind = entry.find(R"gql(kind)gql");

					if (itrName != entry.end() && itrName->second.type() == response::Type::String
						&& itrKind != entry.end()
						&& itrKind->second.type() == response::Type::EnumValue)
					{
						const auto& name = itrName->second.get<response::StringType>();
						const auto& kind =
							ModifiedArgument<introspection::TypeKind>::convert(itrKind->second);

						if (kind == introspection::TypeKind::OBJECT)
						{
							addObject(name);
						}
						else if (kind == introspection::TypeKind::INPUT_OBJECT)
						{
							addInputObject(name);
						}
						else if (kind == introspection::TypeKind::INTERFACE)
						{
							addInterface(name, entry);
						}
						else if (kind == introspection::TypeKind::UNION)
						{
							addUnion(name, entry);
						}
						else if (kind == introspection::TypeKind::ENUM)
						{
							addEnum(name, entry);
						}
						else if (kind == introspection::TypeKind::SCALAR)
						{
							addScalar(name);
						}
					}
				}

				// second iteration add the fields that refer to given types
				for (const auto& entry : entries)
				{
					if (entry.type() != response::Type::Map)
					{
						continue;
					}

					const auto& itrName = entry.find(R"gql(name)gql");
					const auto& itrKind = entry.find(R"gql(kind)gql");

					if (itrName != entry.end() && itrName->second.type() == response::Type::String
						&& itrKind != entry.end()
						&& itrKind->second.type() == response::Type::EnumValue)
					{
						const auto& name = itrName->second.get<response::StringType>();
						const auto& kind =
							ModifiedArgument<introspection::TypeKind>::convert(itrKind->second);

						if (kind == introspection::TypeKind::OBJECT)
						{
							auto type = getNamedValidateType<ObjectType>(name);
							addTypeFields(type, entry);
						}
						else if (kind == introspection::TypeKind::INTERFACE
							|| kind == introspection::TypeKind::UNION)
						{
							auto type =
								getNamedValidateType<PossibleTypesContainerValidateType>(name);
							addTypeFields(type, entry);
							addPossibleTypes(type, entry);
						}
						else if (kind == introspection::TypeKind::INPUT_OBJECT)
						{
							auto type = getNamedValidateType<InputObjectType>(name);
							addInputTypeFields(type, entry);
						}
					}
				}
			}
			else if (member.second.type() == response::Type::List
				&& member.first == R"gql(directives)gql")
			{
				const auto& entries = member.second.get<response::ListType>();

				for (const auto& entry : entries)
				{
					if (entry.type() != response::Type::Map)
					{
						continue;
					}

					const auto& itrName = entry.find(R"gql(name)gql");
					const auto& itrLocations = entry.find(R"gql(locations)gql");

					if (itrName != entry.end() && itrName->second.type() == response::Type::String
						&& itrLocations != entry.end()
						&& itrLocations->second.type() == response::Type::List)
					{
						const auto& name = itrName->second.get<response::StringType>();
						const auto& locations = itrLocations->second.get<response::ListType>();

						addDirective(name, locations, entry);
					}
				}
			}
		}
	}
}

ValidateTypeFieldArguments IntrospectionValidationContext::getArguments(
	const response::ListType& args)
{
	ValidateTypeFieldArguments result;

	for (const auto& arg : args)
	{
		if (arg.type() != response::Type::Map)
		{
			continue;
		}

		const auto& itrName = arg.find(R"gql(name)gql");
		const auto& itrType = arg.find(R"gql(type)gql");
		const auto& itrDefaultValue = arg.find(R"gql(defaultValue)gql");

		if (itrName != arg.end() && itrName->second.type() == response::Type::String
			&& itrType != arg.end() && itrType->second.type() == response::Type::Map)
		{
			ValidateArgument argument;

			argument.defaultValue = (itrDefaultValue != arg.end()
				&& itrDefaultValue->second.type() == response::Type::String);
			argument.nonNullDefaultValue = argument.defaultValue
				&& itrDefaultValue->second.get<response::StringType>() != R"gql(null)gql";
			argument.type = getTypeFromMap(itrType->second);

			result[itrName->second.get<response::StringType>()] = std::move(argument);
		}
	}

	return result;
}

void IntrospectionValidationContext::addTypeFields(
	std::shared_ptr<ContainerValidateType<ValidateTypeField>> type,
	const response::Value& typeDescriptionMap)
{
	std::unordered_map<std::string, ValidateTypeField> fields;

	const auto& itrFields = typeDescriptionMap.find(R"gql(fields)gql");
	if (itrFields != typeDescriptionMap.end() && itrFields->second.type() == response::Type::List)
	{
		const auto& entries = itrFields->second.get<response::ListType>();

		for (const auto& entry : entries)
		{
			if (entry.type() != response::Type::Map)
			{
				continue;
			}

			const auto& itrFieldName = entry.find(R"gql(name)gql");
			const auto& itrFieldType = entry.find(R"gql(type)gql");

			if (itrFieldName != entry.end() && itrFieldName->second.type() == response::Type::String
				&& itrFieldType != entry.end()
				&& itrFieldType->second.type() == response::Type::Map)
			{
				const auto& fieldName = itrFieldName->second.get<response::StringType>();
				ValidateTypeField subField;

				subField.returnType = getTypeFromMap(itrFieldType->second);

				const auto& itrArgs = entry.find(R"gql(args)gql");
				if (itrArgs != entry.end() && itrArgs->second.type() == response::Type::List)
				{
					subField.arguments = getArguments(itrArgs->second.get<response::ListType>());
				}

				fields[std::move(fieldName)] = std::move(subField);
			}
		}

		if (type->name() == _operationTypes.queryType)
		{
			fields[R"gql(__schema)gql"] =
				ValidateTypeField { makeNonNullOfType(makeObjectType(R"gql(__Schema)gql")) };

			fields[R"gql(__type)gql"] = ValidateTypeField { makeObjectType(R"gql(__Type)gql"),
				ValidateTypeFieldArguments {
					{ R"gql(name)gql", ValidateArgument { commonTypes.nonNullString } } } };
		}
	}

	fields[R"gql(__typename)gql"] = ValidateTypeField { commonTypes.nonNullString };

	type->setFields(std::move(fields));
}

void IntrospectionValidationContext::addPossibleTypes(
	std::shared_ptr<PossibleTypesContainerValidateType> type,
	const response::Value& typeDescriptionMap)
{
	const auto& itrPossibleTypes = typeDescriptionMap.find(R"gql(possibleTypes)gql");
	std::set<const ValidateType*> possibleTypes;

	if (itrPossibleTypes != typeDescriptionMap.end()
		&& itrPossibleTypes->second.type() == response::Type::List)
	{
		const auto& matchingTypeEntries = itrPossibleTypes->second.get<response::ListType>();

		for (const auto& matchingTypeEntry : matchingTypeEntries)
		{
			if (matchingTypeEntry.type() != response::Type::Map)
			{
				continue;
			}

			const auto& itrMatchingTypeName = matchingTypeEntry.find(R"gql(name)gql");
			if (itrMatchingTypeName != matchingTypeEntry.end()
				&& itrMatchingTypeName->second.type() == response::Type::String)
			{
				const auto& possibleType =
					getNamedValidateType(itrMatchingTypeName->second.get<response::StringType>());
				possibleTypes.insert(possibleType.get());
			}
		}
	}

	type->setPossibleTypes(std::move(possibleTypes));
}

void IntrospectionValidationContext::addInputTypeFields(
	std::shared_ptr<InputObjectType> type, const response::Value& typeDescriptionMap)
{
	const auto& itrFields = typeDescriptionMap.find(R"gql(inputFields)gql");
	if (itrFields != typeDescriptionMap.end() && itrFields->second.type() == response::Type::List)
	{
		type->setFields(getArguments(itrFields->second.get<response::ListType>()));
	}
}

void IntrospectionValidationContext::addEnum(
	const std::string_view& enumName, const response::Value& enumDescriptionMap)
{
	const auto& itrEnumValues = enumDescriptionMap.find(R"gql(enumValues)gql");
	if (itrEnumValues != enumDescriptionMap.end()
		&& itrEnumValues->second.type() == response::Type::List)
	{
		std::unordered_set<std::string> enumValues;
		const auto& enumValuesEntries = itrEnumValues->second.get<response::ListType>();

		for (const auto& enumValuesEntry : enumValuesEntries)
		{
			if (enumValuesEntry.type() != response::Type::Map)
			{
				continue;
			}

			const auto& itrEnumValuesName = enumValuesEntry.find(R"gql(name)gql");
			if (itrEnumValuesName != enumValuesEntry.end()
				&& itrEnumValuesName->second.type() == response::Type::String)
			{
				enumValues.insert(itrEnumValuesName->second.get<response::StringType>());
			}
		}

		if (!enumValues.empty())
		{
			makeNamedValidateType(EnumType { enumName, std::move(enumValues) });
		}
	}
}

void IntrospectionValidationContext::addObject(const std::string_view& name)
{
	makeNamedValidateType(ObjectType { name });
}

void IntrospectionValidationContext::addInputObject(const std::string_view& name)
{
	makeNamedValidateType(InputObjectType { name });
}

void IntrospectionValidationContext::addInterface(
	const std::string_view& name, const response::Value& typeDescriptionMap)
{
	makeNamedValidateType(InterfaceType { name });
}

void IntrospectionValidationContext::addUnion(
	const std::string_view& name, const response::Value& typeDescriptionMap)
{
	makeNamedValidateType(UnionType { name });
}

void IntrospectionValidationContext::addDirective(const std::string_view& name,
	const response::ListType& locations, const response::Value& descriptionMap)
{
	ValidateDirective directive;

	for (const auto& location : locations)
	{
		if (location.type() != response::Type::EnumValue)
		{
			continue;
		}

		directive.locations.insert(
			ModifiedArgument<introspection::DirectiveLocation>::convert(location));
	}

	const auto& itrArgs = descriptionMap.find(R"gql(args)gql");
	if (itrArgs != descriptionMap.end() && itrArgs->second.type() == response::Type::List)
	{
		directive.arguments = getArguments(itrArgs->second.get<response::ListType>());
	}

	// TODO: string is a work around, the directives will be moved to string_view soon
	_directives[std::string { name }] = std::move(directive);
}

std::shared_ptr<ValidateType> IntrospectionValidationContext::getTypeFromMap(
	const response::Value& typeMap)
{
	const auto& itrKind = typeMap.find(R"gql(kind)gql");
	if (itrKind == typeMap.end() || itrKind->second.type() != response::Type::EnumValue)
	{
		return std::shared_ptr<ValidateType>();
	}

	introspection::TypeKind kind =
		ModifiedArgument<introspection::TypeKind>::convert(itrKind->second);
	const auto& itrName = typeMap.find(R"gql(name)gql");
	if (itrName != typeMap.end() && itrName->second.type() == response::Type::String)
	{
		const auto& name = itrName->second.get<response::StringType>();
		if (!name.empty())
		{
			return getNamedValidateType(name);
		}
	}

	const auto& itrOfType = typeMap.find(R"gql(ofType)gql");
	if (itrOfType != typeMap.end() && itrOfType->second.type() == response::Type::Map)
	{
		std::shared_ptr<ValidateType> ofType = getTypeFromMap(itrOfType->second);
		if (ofType)
		{
			if (kind == introspection::TypeKind::LIST)
			{
				return makeListOfType(std::move(ofType));
			}
			else if (kind == introspection::TypeKind::NON_NULL)
			{
				return makeNonNullOfType(std::move(ofType));
			}
		}

		// should never reach
		return nullptr;
	}

	// should never reach
	return nullptr;
}

void IntrospectionValidationContext::addScalar(const std::string_view& scalarName)
{
	makeNamedValidateType(ScalarType { scalarName });
}

} /* namespace graphql::service */
