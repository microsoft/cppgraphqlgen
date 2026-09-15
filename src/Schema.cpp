// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/internal/Schema.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

#include <tuple>

using namespace std::literals;

namespace graphql::schema {

Schema::Schema(bool noIntrospection, std::string_view description)
	: _noIntrospection(noIntrospection)
	, _description(description)
{
}

std::shared_ptr<Schema> Schema::StitchSchema(const std::shared_ptr<const Schema>& added) const
{
	const auto noIntrospection = _noIntrospection || added->_noIntrospection;
	const auto description = _description.empty() ? added->_description : _description;
	auto schema = std::make_shared<Schema>(noIntrospection, description);

	if (_types.empty())
	{
		schema->_query = added->_query;
		schema->_mutation = added->_mutation;
		schema->_subscription = added->_subscription;
		schema->_typeMap = added->_typeMap;
		schema->_types = added->_types;
		schema->_directives = added->_directives;
	}
	else if (added->_types.empty())
	{
		schema->_query = _query;
		schema->_mutation = _mutation;
		schema->_subscription = _subscription;
		schema->_typeMap = _typeMap;
		schema->_types = _types;
		schema->_directives = _directives;
	}
	else
	{
		internal::string_view_map<std::shared_ptr<ObjectType>> objectTypes;
		internal::string_view_map<std::shared_ptr<InterfaceType>> interfaceTypes;
		internal::string_view_map<std::shared_ptr<UnionType>> unionTypes;
		internal::string_view_map<std::shared_ptr<EnumType>> enumTypes;
		internal::string_view_map<std::shared_ptr<InputObjectType>> inputObjectTypes;

		for (const auto& entry : _types)
		{
			const auto& [name, originalType] = entry;

			switch (originalType->kind())
			{
				case introspection::TypeKind::SCALAR:
				{
					const auto originalDescription = originalType->description();
					const auto originalSpecifiedByURL = originalType->specifiedByURL();
					const auto itrAdded = added->_typeMap.find(name);
					auto scalarType = ScalarType::Make(name,
						originalDescription.empty() && itrAdded != added->_typeMap.end()
							? added->_types[itrAdded->second].second->description()
							: originalDescription,
						originalSpecifiedByURL.empty() && itrAdded != added->_typeMap.end()
							? added->_types[itrAdded->second].second->description()
							: originalSpecifiedByURL);

					schema->AddType(name, std::move(scalarType));
					break;
				}

				case introspection::TypeKind::OBJECT:
				{
					const auto originalDescription = originalType->description();
					const auto itrAdded = added->_typeMap.find(name);
					auto objectType = ObjectType::Make(name,
						originalDescription.empty() && itrAdded != added->_typeMap.end()
							? added->_types[itrAdded->second].second->description()
							: originalDescription);

					schema->AddType(name, objectType);
					objectTypes[name] = std::move(objectType);
					break;
				}

				case introspection::TypeKind::INTERFACE:
				{
					const auto originalDescription = originalType->description();
					const auto itrAdded = added->_typeMap.find(name);
					auto interfaceType = InterfaceType::Make(name,
						originalDescription.empty() && itrAdded != added->_typeMap.end()
							? added->_types[itrAdded->second].second->description()
							: originalDescription);

					schema->AddType(name, interfaceType);
					interfaceTypes[name] = std::move(interfaceType);
					break;
				}

				case introspection::TypeKind::UNION:
				{
					const auto originalDescription = originalType->description();
					const auto itrAdded = added->_typeMap.find(name);
					auto unionType = UnionType::Make(name,
						originalDescription.empty() && itrAdded != added->_typeMap.end()
							? added->_types[itrAdded->second].second->description()
							: originalDescription);

					schema->AddType(name, unionType);
					unionTypes[name] = std::move(unionType);
					break;
				}

				case introspection::TypeKind::ENUM:
				{
					const auto originalDescription = originalType->description();
					const auto itrAdded = added->_typeMap.find(name);
					auto enumType = EnumType::Make(name,
						originalDescription.empty() && itrAdded != added->_typeMap.end()
							? added->_types[itrAdded->second].second->description()
							: originalDescription);

					schema->AddType(name, enumType);
					enumTypes[name] = std::move(enumType);
					break;
				}

				case introspection::TypeKind::INPUT_OBJECT:
				{
					const auto originalDescription = originalType->description();
					const auto itrAdded = added->_typeMap.find(name);
					auto inputObjectType = InputObjectType::Make(name,
						originalDescription.empty() && itrAdded != added->_typeMap.end()
							? added->_types[itrAdded->second].second->description()
							: originalDescription);

					schema->AddType(name, inputObjectType);
					inputObjectTypes[name] = std::move(inputObjectType);
					break;
				}

				case introspection::TypeKind::LIST:
				case introspection::TypeKind::NON_NULL:
					break;
			}
		}

		for (const auto& entry : added->_types)
		{
			const auto& [name, addedType] = entry;
			const auto itrOriginal = _typeMap.find(name);

			if (itrOriginal != _typeMap.end())
			{
				continue;
			}

			switch (addedType->kind())
			{
				case introspection::TypeKind::SCALAR:
				{
					auto scalarType = ScalarType::Make(name,
						addedType->description(),
						addedType->specifiedByURL());

					schema->AddType(name, std::move(scalarType));
					break;
				}

				case introspection::TypeKind::OBJECT:
				{
					auto objectType = ObjectType::Make(name, addedType->description());

					schema->AddType(name, objectType);
					objectTypes[name] = std::move(objectType);
					break;
				}

				case introspection::TypeKind::INTERFACE:
				{
					auto interfaceType = InterfaceType::Make(name, addedType->description());

					schema->AddType(name, interfaceType);
					interfaceTypes[name] = std::move(interfaceType);
					break;
				}

				case introspection::TypeKind::UNION:
				{
					auto unionType = UnionType::Make(name, addedType->description());

					schema->AddType(name, unionType);
					unionTypes[name] = std::move(unionType);
					break;
				}

				case introspection::TypeKind::ENUM:
				{
					auto enumType = EnumType::Make(name, addedType->description());

					schema->AddType(name, enumType);
					enumTypes[name] = std::move(enumType);
					break;
				}

				case introspection::TypeKind::INPUT_OBJECT:
				{
					auto inputObjectType = InputObjectType::Make(name, addedType->description());

					schema->AddType(name, inputObjectType);
					inputObjectTypes[name] = std::move(inputObjectType);
					break;
				}

				case introspection::TypeKind::LIST:
				case introspection::TypeKind::NON_NULL:
					break;
			}
		}

		for (const auto& entry : enumTypes)
		{
			const auto& [name, stitchedType] = entry;
			const auto itrOriginal = _typeMap.find(name);
			const auto itrAdded = added->_typeMap.find(name);
			internal::string_view_set names;
			std::vector<EnumValueType> stitchedValues;

			if (itrOriginal != _typeMap.end())
			{
				const auto& originalType = _types[itrOriginal->second].second;
				const auto& enumValues = originalType->enumValues();

				for (const auto& value : enumValues)
				{
					names.emplace(value->name());
					stitchedValues.push_back({
						value->name(),
						value->description(),
						value->deprecationReason(),
					});
				}
			}

			if (itrAdded != added->_typeMap.end())
			{
				const auto& addedType = added->_types[itrAdded->second].second;
				const auto& enumValues = addedType->enumValues();

				for (const auto& value : enumValues)
				{
					if (!names.emplace(value->name()).second)
					{
						continue;
					}

					stitchedValues.push_back({
						value->name(),
						value->description(),
						value->deprecationReason(),
					});
				}
			}

			stitchedType->AddEnumValues(std::move(stitchedValues));
		}

		for (const auto& entry : inputObjectTypes)
		{
			const auto& [name, stitchedType] = entry;
			const auto itrOriginal = _typeMap.find(name);
			const auto itrAdded = added->_typeMap.find(name);
			internal::string_view_set names;
			std::vector<std::shared_ptr<const InputValue>> stitchedValues;

			if (itrOriginal != _typeMap.end())
			{
				const auto& originalType = _types[itrOriginal->second].second;
				const auto& inputObjectValues = originalType->inputFields();

				for (const auto& value : inputObjectValues)
				{
					names.emplace(value->name());
					stitchedValues.push_back(InputValue::Make(value->name(),
						value->description(),
						schema->StitchFieldType(value->type().lock()),
						value->defaultValue()));
				}
			}

			if (itrAdded != added->_typeMap.end())
			{
				const auto& addedType = added->_types[itrAdded->second].second;
				const auto& inputObjectValues = addedType->inputFields();

				for (const auto& value : inputObjectValues)
				{
					if (!names.emplace(value->name()).second)
					{
						continue;
					}

					stitchedValues.push_back(InputValue::Make(value->name(),
						value->description(),
						schema->StitchFieldType(value->type().lock()),
						value->defaultValue()));
				}
			}

			stitchedType->AddInputValues(std::move(stitchedValues));
		}

		for (const auto& entry : interfaceTypes)
		{
			const auto& [name, stitchedType] = entry;
			const auto itrOriginal = _typeMap.find(name);
			const auto itrAdded = added->_typeMap.find(name);
			internal::string_view_set names;
			std::vector<std::shared_ptr<const Field>> stitchedFields;

			if (itrOriginal != _typeMap.end())
			{
				const auto& originalType = _types[itrOriginal->second].second;
				const auto& interfaceFields = originalType->fields();

				for (const auto& interfaceField : interfaceFields)
				{
					std::vector<std::shared_ptr<const InputValue>> stitchedArgs;

					for (const auto& arg : interfaceField->args())
					{
						stitchedArgs.push_back(InputValue::Make(arg->name(),
							arg->description(),
							schema->StitchFieldType(arg->type().lock()),
							arg->defaultValue()));
					}

					names.emplace(interfaceField->name());
					stitchedFields.push_back(Field::Make(interfaceField->name(),
						interfaceField->description(),
						interfaceField->deprecationReason(),
						schema->StitchFieldType(interfaceField->type().lock()),
						std::move(stitchedArgs)));
				}
			}

			if (itrAdded != added->_typeMap.end())
			{
				const auto& addedType = added->_types[itrAdded->second].second;
				const auto& interfaceFields = addedType->fields();

				for (const auto& interfaceField : interfaceFields)
				{
					if (!names.emplace(interfaceField->name()).second)
					{
						continue;
					}

					std::vector<std::shared_ptr<const InputValue>> stitchedArgs;

					for (const auto& arg : interfaceField->args())
					{
						stitchedArgs.push_back(InputValue::Make(arg->name(),
							arg->description(),
							schema->StitchFieldType(arg->type().lock()),
							arg->defaultValue()));
					}

					stitchedFields.push_back(Field::Make(interfaceField->name(),
						interfaceField->description(),
						interfaceField->deprecationReason(),
						schema->StitchFieldType(interfaceField->type().lock()),
						std::move(stitchedArgs)));
				}
			}

			stitchedType->AddFields(std::move(stitchedFields));
		}

		for (const auto& entry : unionTypes)
		{
			const auto& [name, stitchedType] = entry;
			const auto itrOriginal = _typeMap.find(name);
			const auto itrAdded = added->_typeMap.find(name);
			internal::string_view_set names;
			std::vector<std::weak_ptr<const BaseType>> stitchedValues;

			if (itrOriginal != _typeMap.end())
			{
				const auto& originalType = _types[itrOriginal->second].second;
				const auto& possibleTypes = originalType->possibleTypes();

				for (const auto& possibleType : possibleTypes)
				{
					const auto possible = possibleType.lock();

					names.emplace(possible->name());
					stitchedValues.push_back(schema->LookupType(possible->name()));
				}
			}

			if (itrAdded != added->_typeMap.end())
			{
				const auto& addedType = added->_types[itrAdded->second].second;
				const auto& possibleTypes = addedType->possibleTypes();

				for (const auto& possibleType : possibleTypes)
				{
					const auto possible = possibleType.lock();

					if (!names.emplace(possible->name()).second)
					{
						continue;
					}

					stitchedValues.push_back(schema->LookupType(possible->name()));
				}
			}

			stitchedType->AddPossibleTypes(std::move(stitchedValues));
		}

		for (const auto& entry : objectTypes)
		{
			const auto& [name, stitchedType] = entry;
			const auto itrOriginal = _typeMap.find(name);
			const auto itrAdded = added->_typeMap.find(name);
			internal::string_view_set interfaceNames;
			internal::string_view_set fieldNames;
			std::vector<std::shared_ptr<const InterfaceType>> stitchedInterfaces;
			std::vector<std::shared_ptr<const Field>> stitchedValues;

			if (itrOriginal != _typeMap.end())
			{
				const auto& originalType = _types[itrOriginal->second].second;
				const auto& objectInterfaces = originalType->interfaces();

				for (const auto& interfaceType : objectInterfaces)
				{
					interfaceNames.emplace(interfaceType->name());
					stitchedInterfaces.push_back(interfaceTypes[interfaceType->name()]);
				}

				const auto& objectFields = originalType->fields();

				for (const auto& objectField : objectFields)
				{
					std::vector<std::shared_ptr<const InputValue>> stitchedArgs;

					for (const auto& arg : objectField->args())
					{
						stitchedArgs.push_back(InputValue::Make(arg->name(),
							arg->description(),
							schema->StitchFieldType(arg->type().lock()),
							arg->defaultValue()));
					}

					fieldNames.emplace(objectField->name());
					stitchedValues.push_back(Field::Make(objectField->name(),
						objectField->description(),
						objectField->deprecationReason(),
						schema->StitchFieldType(objectField->type().lock()),
						std::move(stitchedArgs)));
				}
			}

			if (itrAdded != added->_typeMap.end())
			{
				const auto& addedType = added->_types[itrAdded->second].second;
				const auto& objectInterfaces = addedType->interfaces();

				for (const auto& interfaceType : objectInterfaces)
				{
					if (!interfaceNames.emplace(interfaceType->name()).second)
					{
						continue;
					}

					stitchedInterfaces.push_back(interfaceTypes[interfaceType->name()]);
				}

				const auto& objectFields = addedType->fields();

				for (const auto& objectField : objectFields)
				{
					if (!fieldNames.emplace(objectField->name()).second)
					{
						continue;
					}

					std::vector<std::shared_ptr<const InputValue>> stitchedArgs;

					for (const auto& arg : objectField->args())
					{
						stitchedArgs.push_back(InputValue::Make(arg->name(),
							arg->description(),
							schema->StitchFieldType(arg->type().lock()),
							arg->defaultValue()));
					}

					stitchedValues.push_back(Field::Make(objectField->name(),
						objectField->description(),
						objectField->deprecationReason(),
						schema->StitchFieldType(objectField->type().lock()),
						std::move(stitchedArgs)));
				}
			}

			stitchedType->AddInterfaces(std::move(stitchedInterfaces));
			stitchedType->AddFields(std::move(stitchedValues));
		}

		internal::string_view_set directiveNames;
		std::vector<std::shared_ptr<Directive>> stitchedDirectives;

		for (const auto& originalDirective : _directives)
		{
			if (!directiveNames.emplace(originalDirective->name()).second)
			{
				continue;
			}

			std::vector<std::shared_ptr<const InputValue>> stitchedArgs;

			for (const auto& arg : originalDirective->args())
			{
				stitchedArgs.push_back(InputValue::Make(arg->name(),
					arg->description(),
					schema->StitchFieldType(arg->type().lock()),
					arg->defaultValue()));
			}

			stitchedDirectives.push_back(Directive::Make(originalDirective->name(),
				originalDirective->description(),
				std::vector<introspection::DirectiveLocation> { originalDirective->locations() },
				std::move(stitchedArgs),
				originalDirective->isRepeatable()));
		}

		for (const auto& addedDirective : added->_directives)
		{
			if (!directiveNames.emplace(addedDirective->name()).second)
			{
				continue;
			}

			std::vector<std::shared_ptr<const InputValue>> stitchedArgs;

			for (const auto& arg : addedDirective->args())
			{
				stitchedArgs.push_back(InputValue::Make(arg->name(),
					arg->description(),
					schema->StitchFieldType(arg->type().lock()),
					arg->defaultValue()));
			}

			stitchedDirectives.push_back(Directive::Make(addedDirective->name(),
				addedDirective->description(),
				std::vector<introspection::DirectiveLocation> { addedDirective->locations() },
				std::move(stitchedArgs),
				addedDirective->isRepeatable()));
		}

		for (auto& directive : stitchedDirectives)
		{
			schema->AddDirective(std::move(directive));
		}

		if (_query)
		{
			schema->AddQueryType(objectTypes[_query->name()]);
		}
		else if (added->_query)
		{
			schema->AddQueryType(objectTypes[added->_query->name()]);
		}

		if (_mutation)
		{
			schema->AddMutationType(objectTypes[_mutation->name()]);
		}
		else if (added->_mutation)
		{
			schema->AddMutationType(objectTypes[added->_mutation->name()]);
		}

		if (_subscription)
		{
			schema->AddSubscriptionType(objectTypes[_subscription->name()]);
		}
		else if (added->_subscription)
		{
			schema->AddSubscriptionType(objectTypes[added->_subscription->name()]);
		}
	}

	return schema;
}

std::shared_ptr<const BaseType> Schema::StitchFieldType(std::shared_ptr<const BaseType> fieldType)
{
	switch (fieldType->kind())
	{
		case introspection::TypeKind::LIST:
			return WrapType(introspection::TypeKind::LIST,
				StitchFieldType(fieldType->ofType().lock()));

		case introspection::TypeKind::NON_NULL:
			return WrapType(introspection::TypeKind::NON_NULL,
				StitchFieldType(fieldType->ofType().lock()));

		default:
			return LookupType(fieldType->name());
	}
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

std::string_view Schema::description() const noexcept
{
	return _description;
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

std::string_view BaseType::specifiedByURL() const noexcept
{
	return ""sv;
}

struct ScalarType::init
{
	std::string_view name;
	std::string_view description;
	std::string_view specifiedByURL;
};

std::shared_ptr<ScalarType> ScalarType::Make(
	std::string_view name, std::string_view description, std::string_view specifiedByURL)
{
	return std::make_shared<ScalarType>(init { name, description, specifiedByURL });
}

ScalarType::ScalarType(init&& params)
	: BaseType(introspection::TypeKind::SCALAR, params.description)
	, _name(params.name)
	, _specifiedByURL(params.specifiedByURL)
{
}

std::string_view ScalarType::name() const noexcept
{
	return _name;
}

std::string_view ScalarType::specifiedByURL() const noexcept
{
	return _specifiedByURL;
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

void ObjectType::AddInterfaces(std::vector<std::shared_ptr<const InterfaceType>>&& interfaces)
{
	_interfaces = std::move(interfaces);

	for (const auto& interface : _interfaces)
	{
		std::const_pointer_cast<InterfaceType>(interface)->AddPossibleType(shared_from_this());
	}
}

void ObjectType::AddFields(std::vector<std::shared_ptr<const Field>>&& fields)
{
	_fields = std::move(fields);
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

void InterfaceType::AddPossibleType(std::weak_ptr<BaseType> possibleType)
{
	_possibleTypes.push_back(possibleType);
}

void InterfaceType::AddInterfaces(std::vector<std::shared_ptr<const InterfaceType>>&& interfaces)
{
	_interfaces = std::move(interfaces);

	for (const auto& interface : _interfaces)
	{
		std::const_pointer_cast<InterfaceType>(interface)->AddPossibleType(shared_from_this());
	}
}

void InterfaceType::AddFields(std::vector<std::shared_ptr<const Field>>&& fields)
{
	_fields = std::move(fields);
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

const std::vector<std::shared_ptr<const InterfaceType>>& InterfaceType::interfaces() const noexcept
{
	return _interfaces;
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

void UnionType::AddPossibleTypes(std::vector<std::weak_ptr<const BaseType>>&& possibleTypes)
{
	_possibleTypes = std::move(possibleTypes);
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

void EnumType::AddEnumValues(std::vector<EnumValueType>&& enumValues)
{
	_enumValues.resize(enumValues.size());
	std::ranges::transform(enumValues, _enumValues.begin(), [](const auto& value) {
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

void InputObjectType::AddInputValues(std::vector<std::shared_ptr<const InputValue>>&& inputValues)
{
	_inputValues = std::move(inputValues);
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
	std::vector<std::shared_ptr<const InputValue>>&& args)
{
	init params { name, description, deprecationReason, std::move(type), std::move(args) };

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
	bool isRepeatable;
};

std::shared_ptr<Directive> Directive::Make(std::string_view name, std::string_view description,
	std::vector<introspection::DirectiveLocation>&& locations,
	std::vector<std::shared_ptr<const InputValue>>&& args, bool isRepeatable)
{
	init params { name, description, std::move(locations), std::move(args), isRepeatable };

	return std::make_shared<Directive>(std::move(params));
}

Directive::Directive(init&& params)
	: _name(params.name)
	, _description(params.description)
	, _locations(std::move(params.locations))
	, _args(std::move(params.args))
	, _isRepeatable(params.isRepeatable)
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

bool Directive::isRepeatable() const noexcept
{
	return _isRepeatable;
}

} // namespace graphql::schema
