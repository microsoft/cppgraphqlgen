// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLVALIDATION_H
#define GRAPHQLVALIDATION_H

// clang-format off
#ifdef GRAPHQL_DLLEXPORTS
	#ifdef IMPL_GRAPHQLSERVICE_DLL
		#define GRAPHQLVALIDATION_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLSERVICE_DLL
		#define GRAPHQLVALIDATION_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLSERVICE_DLL
#else // !GRAPHQL_DLLEXPORTS
	#define GRAPHQLVALIDATION_EXPORT
#endif // !GRAPHQL_DLLEXPORTS
// clang-format on

#include "graphqlservice/GraphQLService.h"
#include "graphqlservice/IntrospectionSchema.h"

#include <optional>
#include <string_view>
#include <unordered_map>

namespace graphql::service {

class ValidateType
{
public:
	virtual introspection::TypeKind kind() const = 0;
	virtual const std::string_view name() const = 0;
	virtual bool isInputType() const = 0;
	virtual bool isValid() const = 0;
	virtual std::shared_ptr<const ValidateType> getInnerType() const = 0;
	virtual bool operator==(const ValidateType& other) const = 0;

	static constexpr bool isKindInput(introspection::TypeKind typeKind)
	{
		switch (typeKind)
		{
			case introspection::TypeKind::SCALAR:
			case introspection::TypeKind::ENUM:
			case introspection::TypeKind::INPUT_OBJECT:
				return true;
			default:
				return false;
		}
	}
};

class NamedValidateType
	: public ValidateType
	, public std::enable_shared_from_this<NamedValidateType>
{
public:
	std::string _name;

	NamedValidateType(const std::string_view& name)
		: _name(std::string(name))
	{
	}

	const std::string_view name() const final
	{
		return _name;
	}

	bool isValid() const final
	{
		return !name().empty();
	}

	std::shared_ptr<const ValidateType> getInnerType() const final
	{
		return shared_from_this();
	}

	bool operator==(const ValidateType& other) const final
	{
		if (this == &other)
		{
			return true;
		}

		if (kind() != other.kind())
		{
			return false;
		}

		return _name == other.name();
	}
};

template <introspection::TypeKind typeKind>
class NamedType : public NamedValidateType
{
public:
	NamedType<typeKind>(const std::string_view& name)
		: NamedValidateType(name)
	{
	}

	introspection::TypeKind kind() const final
	{
		return typeKind;
	}

	bool isInputType() const final
	{
		return ValidateType::isKindInput(typeKind);
	}
};

using ScalarType = NamedType<introspection::TypeKind::SCALAR>;

class EnumType final : public NamedType<introspection::TypeKind::ENUM>
{
public:
	EnumType(const std::string_view& name, std::unordered_set<std::string>&& values)
		: NamedType<introspection::TypeKind::ENUM>(name)
		, _values(std::move(values))
	{
	}

	bool find(const std::string_view& key) const
	{
		// TODO: in the future the set will be of string_view
		return _values.find(std::string { key }) != _values.end();
	}

private:
	std::unordered_set<std::string> _values;
};

template <introspection::TypeKind typeKind>
class WrapperOfType final : public ValidateType
{
public:
	WrapperOfType<typeKind>(std::shared_ptr<ValidateType> ofType)
		: _ofType(std::move(ofType))
	{
	}

	std::shared_ptr<ValidateType> ofType() const
	{
		return _ofType;
	}

	introspection::TypeKind kind() const final
	{
		return typeKind;
	}

	const std::string_view name() const final
	{
		return _name;
	}

	bool isInputType() const final
	{
		return _ofType ? _ofType->isInputType() : false;
	}

	bool isValid() const final
	{
		return _ofType ? _ofType->isValid() : false;
	}

	std::shared_ptr<const ValidateType> getInnerType() const final
	{
		return _ofType ? _ofType->getInnerType() : nullptr;
	}

	bool operator==(const ValidateType& otherType) const final
	{
		if (this == &otherType)
		{
			return true;
		}

		if (typeKind != otherType.kind())
		{
			return false;
		}

		const auto& other = static_cast<const WrapperOfType<typeKind>&>(otherType);
		if (_ofType == other.ofType())
		{
			return true;
		}
		if (_ofType && other.ofType())
		{
			return *_ofType == *other.ofType();
		}
		return false;
	}

private:
	std::shared_ptr<ValidateType> _ofType;
	static inline const std::string_view _name = ""sv;
};

using ListOfType = WrapperOfType<introspection::TypeKind::LIST>;
using NonNullOfType = WrapperOfType<introspection::TypeKind::NON_NULL>;

struct ValidateArgument
{
	std::shared_ptr<ValidateType> type;
	bool defaultValue = false;
	bool nonNullDefaultValue = false;
};

using ValidateTypeFieldArguments = std::unordered_map<std::string, ValidateArgument>;

struct ValidateTypeField
{
	std::shared_ptr<ValidateType> returnType;
	ValidateTypeFieldArguments arguments;
};

using ValidateDirectiveArguments = std::unordered_map<std::string, ValidateArgument>;

template <typename FieldType>
class ContainerValidateType : public NamedValidateType
{
public:
	ContainerValidateType<FieldType>(const std::string_view& name)
		: NamedValidateType(name)
	{
	}

	using FieldsContainer = std::unordered_map<std::string, FieldType>;

	std::optional<std::reference_wrapper<const FieldType>> getField(
		const std::string_view& name) const
	{
		// TODO: string is a work around, the _fields set will be moved to string_view soon
		const auto& itr = _fields.find(std::string { name });
		if (itr == _fields.cend())
		{
			return std::nullopt;
		}

		return std::optional<std::reference_wrapper<const FieldType>>(itr->second);
	}

	void setFields(FieldsContainer&& fields)
	{
		_fields = std::move(fields);
	}

	typename FieldsContainer::const_iterator begin() const
	{
		return _fields.cbegin();
	}

	typename FieldsContainer::const_iterator end() const
	{
		return _fields.cend();
	}

	virtual bool matchesType(const ValidateType& other) const
	{
		if (*this == other)
		{
			return true;
		}

		switch (other.kind())
		{
			case introspection::TypeKind::INTERFACE:
			case introspection::TypeKind::UNION:
				return static_cast<const ContainerValidateType&>(other).matchesType(*this);

			default:
				return false;
		}
	}

private:
	FieldsContainer _fields;
};

template <introspection::TypeKind typeKind, typename FieldType>
class ContainerType : public ContainerValidateType<FieldType>
{
public:
	ContainerType<typeKind, FieldType>(const std::string_view& name)
		: ContainerValidateType<FieldType>(name)
	{
	}

	introspection::TypeKind kind() const final
	{
		return typeKind;
	}

	bool isInputType() const final
	{
		return ValidateType::isKindInput(typeKind);
	}
};

using ObjectType = ContainerType<introspection::TypeKind::OBJECT, ValidateTypeField>;
using InputObjectType = ContainerType<introspection::TypeKind::INPUT_OBJECT, ValidateArgument>;

class PossibleTypesContainerValidateType : public ContainerValidateType<ValidateTypeField>
{
public:
	PossibleTypesContainerValidateType(const std::string_view& name)
		: ContainerValidateType<ValidateTypeField>(name)
	{
	}

	const std::set<const ValidateType*>& possibleTypes() const
	{
		return _possibleTypes;
	}

	bool matchesType(const ValidateType& other) const final
	{
		if (*this == other)
		{
			return true;
		}

		switch (other.kind())
		{
			case introspection::TypeKind::OBJECT:
				return _possibleTypes.find(&other) != _possibleTypes.cend();

			case introspection::TypeKind::INTERFACE:
			case introspection::TypeKind::UNION:
			{
				const auto& types =
					static_cast<const PossibleTypesContainerValidateType&>(other).possibleTypes();
				for (const auto& itr : _possibleTypes)
				{
					if (types.find(itr) != types.cend())
					{
						return true;
					}
				}
				return false;
			}

			default:
				return false;
		}
	}

	void setPossibleTypes(std::set<const ValidateType*>&& possibleTypes)
	{
		_possibleTypes = std::move(possibleTypes);
	}

private:
	std::set<const ValidateType*> _possibleTypes;
};

template <introspection::TypeKind typeKind>
class PossibleTypesContainer final : public PossibleTypesContainerValidateType
{
public:
	PossibleTypesContainer<typeKind>(const std::string_view& name)
		: PossibleTypesContainerValidateType(name)
	{
	}

	introspection::TypeKind kind() const final
	{
		return typeKind;
	}

	bool isInputType() const final
	{
		return ValidateType::isKindInput(typeKind);
	}
};

using InterfaceType = PossibleTypesContainer<introspection::TypeKind::INTERFACE>;
using UnionType = PossibleTypesContainer<introspection::TypeKind::UNION>;

struct ValidateDirective
{
	std::set<introspection::DirectiveLocation> locations;
	ValidateDirectiveArguments arguments;
};

class ValidationContext
{
public:
	struct OperationTypes
	{
		std::string queryType;
		std::string mutationType;
		std::string subscriptionType;
	};

	GRAPHQLVALIDATION_EXPORT std::optional<std::reference_wrapper<const ValidateDirective>>
	getDirective(const std::string_view& name) const;
	GRAPHQLVALIDATION_EXPORT std::optional<std::reference_wrapper<const std::string>>
	getOperationType(const std::string_view& name) const;

	template <typename T = NamedValidateType,
		typename std::enable_if<std::is_base_of<NamedValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<T> getNamedValidateType(const std::string_view& name) const
	{
		const auto& itr = _namedCache.find(name);
		if (itr != _namedCache.cend())
		{
			return std::dynamic_pointer_cast<T>(itr->second);
		}

		return nullptr;
	}

	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<ListOfType> getListOfType(std::shared_ptr<T>&& ofType) const
	{
		const auto& itr = _listOfCache.find(ofType.get());
		if (itr != _listOfCache.cend())
		{
			return itr->second;
		}

		return nullptr;
	}

	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<NonNullOfType> getNonNullOfType(std::shared_ptr<T>&& ofType) const
	{
		const auto& itr = _nonNullCache.find(ofType.get());
		if (itr != _nonNullCache.cend())
		{
			return itr->second;
		}

		return nullptr;
	}

protected:
	template <typename T,
		typename std::enable_if<std::is_base_of<NamedValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<T> makeNamedValidateType(T&& typeDef)
	{
		const std::string_view key(typeDef.name());

		const auto& itr = _namedCache.find(key);
		if (itr != _namedCache.cend())
		{
			return std::dynamic_pointer_cast<T>(itr->second);
		}

		auto type = std::make_shared<T>(std::move(typeDef));
		_namedCache.insert({ type->name(), type });

		return type;
	}

	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<ListOfType> makeListOfType(std::shared_ptr<T>&& ofType)
	{
		const ValidateType* key = ofType.get();

		const auto& itr = _listOfCache.find(key);
		if (itr != _listOfCache.cend())
		{
			return itr->second;
		}

		return _listOfCache.insert({ key, std::make_shared<ListOfType>(std::move(ofType)) })
			.first->second;
	}

	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<ListOfType> makeListOfType(std::shared_ptr<T>& ofType)
	{
		return makeListOfType(std::shared_ptr<T>(ofType));
	}

	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<NonNullOfType> makeNonNullOfType(std::shared_ptr<T>&& ofType)
	{
		const ValidateType* key = ofType.get();

		const auto& itr = _nonNullCache.find(key);
		if (itr != _nonNullCache.cend())
		{
			return itr->second;
		}

		return _nonNullCache.insert({ key, std::make_shared<NonNullOfType>(std::move(ofType)) })
			.first->second;
	}

	template <typename T,
		typename std::enable_if<std::is_base_of<ValidateType, T>::value>::type* = nullptr>
	std::shared_ptr<NonNullOfType> makeNonNullOfType(std::shared_ptr<T>& ofType)
	{
		return makeNonNullOfType(std::shared_ptr<T>(ofType));
	}

	std::shared_ptr<ScalarType> makeScalarType(const std::string_view& name)
	{
		return makeNamedValidateType(ScalarType { name });
	}

	std::shared_ptr<ObjectType> makeObjectType(const std::string_view& name)
	{
		return makeNamedValidateType(ObjectType { name });
	}

	using Directives = std::unordered_map<std::string, ValidateDirective>;

	OperationTypes _operationTypes;
	Directives _directives;

private:
	// These members store Introspection schema information which does not change between queries.

	std::unordered_map<const ValidateType*, std::shared_ptr<ListOfType>> _listOfCache;
	std::unordered_map<const ValidateType*, std::shared_ptr<NonNullOfType>> _nonNullCache;
	std::unordered_map<std::string_view, std::shared_ptr<NamedValidateType>> _namedCache;
};

class IntrospectionValidationContext : public ValidationContext
{
public:
	IntrospectionValidationContext(const Request& service);
	IntrospectionValidationContext(const response::Value& introspectionQuery);

private:
	void populate(const response::Value& introspectionQuery);

	struct
	{
		std::shared_ptr<ScalarType> string;
		std::shared_ptr<NonNullOfType> nonNullString;
	} commonTypes;

	ValidateTypeFieldArguments getArguments(const response::ListType& argumentsMember);

	std::shared_ptr<ValidateType> getTypeFromMap(const response::Value& typeMap);

	void addScalar(const std::string_view& scalarName);
	void addEnum(const std::string_view& enumName, const response::Value& enumDescriptionMap);
	void addObject(const std::string_view& name);
	void addInputObject(const std::string_view& name);
	void addInterface(const std::string_view& name, const response::Value& typeDescriptionMap);
	void addUnion(const std::string_view& name, const response::Value& typeDescriptionMap);
	void addDirective(const std::string_view& name, const response::ListType& locations,
		const response::Value& descriptionMap);

	void addTypeFields(std::shared_ptr<ContainerValidateType<ValidateTypeField>> type,
		const response::Value& typeDescriptionMap);
	void addPossibleTypes(std::shared_ptr<PossibleTypesContainerValidateType> type,
		const response::Value& typeDescriptionMap);
	void addInputTypeFields(
		std::shared_ptr<InputObjectType> type, const response::Value& typeDescriptionMap);
};

} /* namespace graphql::service */

#endif // GRAPHQLVALIDATION_H
