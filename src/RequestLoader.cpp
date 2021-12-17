// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "RequestLoader.h"
#include "SchemaLoader.h"
#include "Validation.h"

#include "graphqlservice/internal/Grammar.h"

#include "graphqlservice/introspection/IntrospectionSchema.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>

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

	findOperation();
	collectFragments();

	const peg::ast_node* selection = nullptr;

	peg::on_first_child<peg::selection_set>(*_operation, [&selection](const peg::ast_node& child) {
		selection = &child;
	});

	if (!selection)
	{
		throw std::logic_error("Request successfully validated, but there was no selection set on "
							   "the operation object!");
	}

	SelectionVisitor visitor { _schemaLoader, _fragments, _schema, _responseType.type };

	visitor.visit(*selection);
	_responseType.fields = visitor.getFields();

	collectVariables();

	// Variables can reference both input types and enums.
	for (const auto& variable : _variables)
	{
		collectInputTypes(variable.type);
		collectEnums(variable.type);
	}

	// Handle nested input types by fully declaring the dependencies first.
	reorderInputTypeDependencies();

	// The response can also reference enums.
	for (const auto& responseField : _responseType.fields)
	{
		collectEnums(responseField);
	}
}

std::string_view RequestLoader::getRequestFilename() const noexcept
{
	return _requestOptions.requestFilename;
}

std::string_view RequestLoader::getOperationDisplayName() const noexcept
{
	return _operationName.empty() ? "(unnamed)"sv : _operationName;
}

std::string RequestLoader::getOperationNamespace() const noexcept
{
	std::string result;

	if (_operationName.empty())
	{
		result = _operationType;
		result.front() = static_cast<char>(std::toupper(result.front()));
	}
	else
	{
		result = _operationName;
	}

	return result;
}

std::string_view RequestLoader::getOperationType() const noexcept
{
	return _operationType;
}

std::string_view RequestLoader::getRequestText() const noexcept
{
	return trimWhitespace(_requestText);
}

const ResponseType& RequestLoader::getResponseType() const noexcept
{
	return _responseType;
}

const RequestVariableList& RequestLoader::getVariables() const noexcept
{
	return _variables;
}

const RequestSchemaTypeList& RequestLoader::getReferencedInputTypes() const noexcept
{
	return _referencedInputTypes;
}

const RequestSchemaTypeList& RequestLoader::getReferencedEnums() const noexcept
{
	return _referencedEnums;
}

std::string RequestLoader::getInputCppType(const RequestSchemaType& wrappedInputType) const noexcept
{
	size_t templateCount = 0;
	std::ostringstream cppType;
	auto [inputType, modifiers] = unwrapSchemaType(RequestSchemaType { wrappedInputType });

	for (auto modifier : modifiers)
	{
		switch (modifier)
		{
			case service::TypeModifier::Nullable:
				cppType << R"cpp(std::optional<)cpp";
				++templateCount;
				break;

			case service::TypeModifier::List:
				cppType << R"cpp(std::vector<)cpp";
				++templateCount;
				break;

			default:
				break;
		}
	}

	cppType << _schemaLoader.getCppType(inputType->name());

	for (size_t i = 0; i < templateCount; ++i)
	{
		cppType << R"cpp(>)cpp";
	}

	return cppType.str();
}

std::string RequestLoader::getOutputCppType(
	std::string_view outputCppType, const TypeModifierStack& modifiers) noexcept
{
	bool nonNull = true;
	size_t templateCount = 0;
	std::ostringstream cppType;

	for (auto modifier : modifiers)
	{
		if (!nonNull)
		{
			cppType << R"cpp(std::optional<)cpp";
			++templateCount;
		}

		switch (modifier)
		{
			case service::TypeModifier::None:
				nonNull = true;
				break;

			case service::TypeModifier::Nullable:
				nonNull = false;
				break;

			case service::TypeModifier::List:
				nonNull = true;
				cppType << R"cpp(std::vector<)cpp";
				++templateCount;
				break;
		}
	}

	if (!nonNull)
	{
		cppType << R"cpp(std::optional<)cpp";
		++templateCount;
	}

	cppType << outputCppType;

	for (size_t i = 0; i < templateCount; ++i)
	{
		cppType << R"cpp(>)cpp";
	}

	return cppType.str();
}

std::pair<RequestSchemaType, TypeModifierStack> RequestLoader::unwrapSchemaType(
	RequestSchemaType&& type) noexcept
{
	std::pair<RequestSchemaType, TypeModifierStack> result { std::move(type), {} };
	bool wrapped = true;
	bool nonNull = false;

	while (wrapped)
	{
		switch (result.first->kind())
		{
			case introspection::TypeKind::NON_NULL:
				nonNull = true;
				result.first = result.first->ofType().lock();
				break;

			case introspection::TypeKind::LIST:
				if (!nonNull)
				{
					result.second.push_back(service::TypeModifier::Nullable);
				}

				nonNull = false;
				result.second.push_back(service::TypeModifier::List);
				result.first = result.first->ofType().lock();
				break;

			default:
				if (!nonNull)
				{
					result.second.push_back(service::TypeModifier::Nullable);
				}

				nonNull = false;
				wrapped = false;
				break;
		}
	}

	return result;
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
				schema::ScalarType::Make(scalarType.type,
					scalarType.description,
					scalarType.specifiedByURL));
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

				locationValue.set<std::string>(std::string { locationName });

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
			std::move(arguments),
			directive.isRepeatable));
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

RequestSchemaType RequestLoader::getSchemaType(
	std::string_view type, const TypeModifierStack& modifiers) const noexcept
{
	RequestSchemaType introspectionType = _schema->LookupType(type);

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

void RequestLoader::findOperation()
{
	peg::on_first_child_if<peg::operation_definition>(*_ast.root,
		[this](const peg::ast_node& operationDefinition) noexcept -> bool {
			std::string_view operationType = service::strQuery;

			peg::on_first_child<peg::operation_type>(operationDefinition,
				[&operationType](const peg::ast_node& child) {
					operationType = child.string_view();
				});

			std::string_view name;

			peg::on_first_child<peg::operation_name>(operationDefinition,
				[&name](const peg::ast_node& child) {
					name = child.string_view();
				});

			if (_requestOptions.operationName.empty() || name == _requestOptions.operationName)
			{
				_operationName = name;
				_operationType = operationType;
				_operation = &operationDefinition;
				return true;
			}

			return false;
		});

	if (!_operation)
	{
		std::ostringstream message;

		message << "Missing operation";

		if (!_operationName.empty())
		{
			message << " name: " << _operationName;
		}

		throw service::schema_exception { { message.str() } };
	}

	if (_operationType == service::strQuery)
	{
		_responseType.type = _schema->queryType();
	}
	else if (_operationType == service::strMutation)
	{
		_responseType.type = _schema->mutationType();
	}
	else if (_operationType == service::strSubscription)
	{
		_responseType.type = _schema->subscriptionType();
	}

	if (!_responseType.type)
	{
		std::ostringstream message;

		message << "Unsupported operation type: " << _operationType;

		if (!_operationName.empty())
		{
			message << " name: " << _operationName;
		}

		throw service::schema_exception { { message.str() } };
	}

	_responseType.cppType = SchemaLoader::getSafeCppName(
		_operationName.empty() ? _responseType.type->name() : _operationName);
}

void RequestLoader::collectFragments() noexcept
{
	peg::for_each_child<peg::fragment_definition>(*_ast.root, [this](const peg::ast_node& child) {
		_fragments.emplace(child.children.front()->string_view(), &child);
	});
}

void RequestLoader::collectVariables() noexcept
{
	peg::for_each_child<peg::variable>(*_operation,
		[this](const peg::ast_node& variableDefinition) {
			RequestVariable variable;
			TypeVisitor variableType;
			service::schema_location defaultValueLocation;

			for (const auto& child : variableDefinition.children)
			{
				if (child->is_type<peg::variable_name>())
				{
					// Skip the $ prefix
					variable.name = child->string_view().substr(1);
					variable.cppName = _schemaLoader.getSafeCppName(variable.name);
				}
				else if (child->is_type<peg::named_type>() || child->is_type<peg::list_type>()
					|| child->is_type<peg::nonnull_type>())
				{
					variableType.visit(*child);
				}
				else if (child->is_type<peg::default_value>())
				{
					const auto position = child->begin();
					DefaultValueVisitor defaultValue;

					defaultValue.visit(*child->children.back());
					variable.defaultValue = defaultValue.getValue();
					variable.defaultValueString = child->children.back()->string_view();

					defaultValueLocation = { position.line, position.column };
				}
			}

			const auto [type, modifiers] = variableType.getType();

			variable.type = _schema->LookupType(type);
			variable.modifiers = modifiers;

			if (!variable.defaultValueString.empty()
				&& variable.defaultValue.type() == response::Type::Null
				&& (modifiers.empty() || modifiers.front() != service::TypeModifier::Nullable))
			{
				std::ostringstream error;

				error << "Expected Non-Null default value for variable name: " << variable.name;

				throw service::schema_exception {
					{ service::schema_error { error.str(), std::move(defaultValueLocation) } }
				};
			}

			variable.position = variableDefinition.begin();

			_variables.push_back(std::move(variable));
		});
}

void RequestLoader::collectInputTypes(const RequestSchemaType& variableType) noexcept
{
	switch (variableType->kind())
	{
		case introspection::TypeKind::INPUT_OBJECT:
		{
			if (_inputTypeNames.emplace(variableType->name()).second)
			{
				_referencedInputTypes.push_back(variableType);

				// Input types can reference other input types and enums.
				for (const auto& inputField : variableType->inputFields())
				{
					const auto fieldType = inputField->type().lock();

					collectInputTypes(fieldType);
					collectEnums(fieldType);
				}
			}

			break;
		}

		case introspection::TypeKind::LIST:
		case introspection::TypeKind::NON_NULL:
		{
			collectInputTypes(variableType->ofType().lock());
			break;
		}

		default:
			break;
	}
}

void RequestLoader::reorderInputTypeDependencies() noexcept
{
	if (_referencedInputTypes.empty())
	{
		return;
	}

	// Build the dependency list for each input type.
	struct InputTypeDependencies
	{
		RequestSchemaType type;
		std::string_view name;
		std::unordered_set<std::string_view> dependencies;
	};

	std::vector<InputTypeDependencies> inputTypes(_referencedInputTypes.size());

	std::transform(_referencedInputTypes.cbegin(),
		_referencedInputTypes.cend(),
		inputTypes.begin(),
		[](const RequestSchemaType& type) noexcept {
			InputTypeDependencies result { type, type->name() };
			const auto& fields = type->inputFields();

			std::for_each(fields.begin(),
				fields.end(),
				[&result](const std::shared_ptr<const schema::InputValue>& field) noexcept {
					const auto fieldType = field->type().lock();

					if (fieldType->kind() == introspection::TypeKind::INPUT_OBJECT)
					{
						result.dependencies.insert(fieldType->name());
					}
				});

			return result;
		});

	std::unordered_set<std::string_view> handled;
	auto itr = inputTypes.begin();

	while (itr != inputTypes.end())
	{
		// Put all of the input types without unhandled dependencies at the front.
		const auto itrDependent = std::stable_partition(itr,
			inputTypes.end(),
			[&handled](const InputTypeDependencies& entry) noexcept {
				return std::find_if(entry.dependencies.cbegin(),
						   entry.dependencies.cend(),
						   [&handled](std::string_view dependency) noexcept {
							   return handled.find(dependency) == handled.cend();
						   })
					== entry.dependencies.cend();
			});

		if (itrDependent != inputTypes.end())
		{
			std::for_each(itr,
				itrDependent,
				[&handled](const InputTypeDependencies& entry) noexcept {
					handled.insert(entry.name);
				});
		}

		itr = itrDependent;
	}

	if (!handled.empty())
	{
		std::transform(inputTypes.begin(),
			inputTypes.end(),
			_referencedInputTypes.begin(),
			[](InputTypeDependencies& entry) noexcept {
				auto result = std::move(entry.type);

				return result;
			});
	}
}

void RequestLoader::collectEnums(const RequestSchemaType& variableType) noexcept
{
	switch (variableType->kind())
	{
		case introspection::TypeKind::ENUM:
		{
			if (_enumNames.emplace(variableType->name()).second)
			{
				_referencedEnums.push_back(variableType);
			}

			break;
		}

		case introspection::TypeKind::LIST:
		case introspection::TypeKind::NON_NULL:
		{
			collectEnums(variableType->ofType().lock());
			break;
		}

		default:
			break;
	}
}

void RequestLoader::collectEnums(const ResponseField& responseField) noexcept
{
	switch (responseField.type->kind())
	{
		case introspection::TypeKind::ENUM:
		{
			if (_enumNames.emplace(responseField.type->name()).second)
			{
				_referencedEnums.push_back(responseField.type);
			}

			break;
		}

		case introspection::TypeKind::OBJECT:
		case introspection::TypeKind::INTERFACE:
		case introspection::TypeKind::UNION:
		{
			for (const auto& field : responseField.children)
			{
				collectEnums(field);
			}

			break;
		}

		default:
			break;
	}
}

RequestLoader::SelectionVisitor::SelectionVisitor(const SchemaLoader& schemaLoader,
	const FragmentDefinitionMap& fragments, const std::shared_ptr<schema::Schema>& schema,
	const RequestSchemaType& type)
	: _schemaLoader(schemaLoader)
	, _fragments(fragments)
	, _schema(schema)
	, _type(type)
{
}

ResponseFieldList RequestLoader::SelectionVisitor::getFields()
{
	auto fields = std::move(_fields);

	return fields;
}

void RequestLoader::SelectionVisitor::visit(const peg::ast_node& selection)
{
	for (const auto& child : selection.children)
	{
		if (child->is_type<peg::field>())
		{
			visitField(*child);
		}
		else if (child->is_type<peg::fragment_spread>())
		{
			visitFragmentSpread(*child);
		}
		else if (child->is_type<peg::inline_fragment>())
		{
			visitInlineFragment(*child);
		}
	}
}

void RequestLoader::SelectionVisitor::visitField(const peg::ast_node& field)
{
	std::string_view name;

	peg::on_first_child<peg::field_name>(field, [&name](const peg::ast_node& child) {
		name = child.string_view();
	});

	std::string_view alias;

	peg::on_first_child<peg::alias_name>(field, [&alias](const peg::ast_node& child) {
		alias = child.string_view();
	});

	if (alias.empty())
	{
		alias = name;
	}

	if (!_names.emplace(alias).second)
	{
		// Skip fields which map to the same response name as a field we've already visited.
		// Validation should handle merging multiple references to the same field or to
		// compatible fields.
		return;
	}

	ResponseField responseField;

	responseField.name = alias;
	responseField.cppName = SchemaLoader::getSafeCppName(alias);
	responseField.position = field.begin();

	// Special case to handle __typename on any ResponseType
	if (name == R"gql(__typename)gql"sv)
	{
		responseField.type = _schema->LookupType("String"sv);
		_fields.push_back(std::move(responseField));
		return;
	}

	if (_schema->supportsIntrospection() && _type == _schema->queryType())
	{
		if (name == R"gql(__schema)gql"sv)
		{
			responseField.type = _schema->LookupType("__Schema"sv);
		}
		else if (name == R"gql(__type)gql"sv)
		{
			responseField.type = _schema->LookupType("__Type"sv);
			responseField.modifiers = { service::TypeModifier::Nullable };
		}
	}

	if (!responseField.type)
	{
		const auto& typeFields = _type->fields();
		const auto itr = std::find_if(typeFields.begin(),
			typeFields.end(),
			[name](const std::shared_ptr<const schema::Field>& typeField) noexcept {
				return typeField->name() == name;
			});

		if (itr == typeFields.end())
		{
			// Skip fields that are not found on the current type.
			return;
		}

		responseField.type = (*itr)->type().lock();
	}

	std::tie(responseField.type, responseField.modifiers) =
		unwrapSchemaType(std::move(responseField.type));

	const peg::ast_node* selection = nullptr;

	peg::on_first_child<peg::selection_set>(field, [&selection](const peg::ast_node& child) {
		selection = &child;
	});

	if (selection)
	{
		switch (responseField.type->kind())
		{
			case introspection::TypeKind::OBJECT:
			case introspection::TypeKind::INTERFACE:
			case introspection::TypeKind::UNION:
			{
				SelectionVisitor selectionVisitor { _schemaLoader,
					_fragments,
					_schema,
					responseField.type };

				selectionVisitor.visit(*selection);

				auto selectionFields = selectionVisitor.getFields();

				if (!selectionFields.empty())
				{
					responseField.children = std::move(selectionFields);
				}

				break;
			}

			default:
				break;
		}
	}

	_fields.push_back(std::move(responseField));
}

void RequestLoader::SelectionVisitor::visitFragmentSpread(const peg::ast_node& fragmentSpread)
{
	const auto name = fragmentSpread.children.front()->string_view();
	auto itr = _fragments.find(name);

	if (itr == _fragments.end())
	{
		auto position = fragmentSpread.begin();
		std::ostringstream error;

		error << "Unknown fragment name: " << name;

		throw service::schema_exception {
			{ service::schema_error { error.str(), { position.line, position.column } } }
		};
	}

	const auto typeCondition = itr->second->children[1].get();
	const auto fragmentType = _schema->LookupType(typeCondition->children.front()->string_view());
	const auto& selection = *(itr->second->children.back());
	SelectionVisitor selectionVisitor { _schemaLoader, _fragments, _schema, fragmentType };

	selectionVisitor.visit(selection);
	mergeFragmentFields(selectionVisitor.getFields());
}

void RequestLoader::SelectionVisitor::visitInlineFragment(const peg::ast_node& inlineFragment)
{
	const peg::ast_node* typeCondition = nullptr;

	peg::on_first_child<peg::type_condition>(inlineFragment,
		[&typeCondition](const peg::ast_node& child) {
			typeCondition = &child;
		});

	peg::on_first_child<peg::selection_set>(inlineFragment,
		[this, typeCondition](const peg::ast_node& child) {
			const auto fragmentType = typeCondition
				? _schema->LookupType(typeCondition->children.front()->string_view())
				: _type;
			const auto& selection = child;
			SelectionVisitor selectionVisitor { _schemaLoader, _fragments, _schema, fragmentType };

			selectionVisitor.visit(selection);
			mergeFragmentFields(selectionVisitor.getFields());
		});
}

void RequestLoader::SelectionVisitor::mergeFragmentFields(
	ResponseFieldList&& fragmentFields) noexcept
{
	fragmentFields.erase(std::remove_if(fragmentFields.begin(),
							 fragmentFields.end(),
							 [this](const ResponseField& fragmentField) noexcept {
								 return !_names.emplace(fragmentField.name).second;
							 }),
		fragmentFields.cend());

	if (!fragmentFields.empty())
	{
		_fields.reserve(_fields.size() + fragmentFields.size());
		for (auto& fragmentField : fragmentFields)
		{
			_fields.push_back(std::move(fragmentField));
		}
	}
}

} // namespace graphql::generator
