// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLClient.h"

namespace graphql::client {

template <>
response::Value ModifiedVariable<response::IntType>::serialize(response::IntType&& value)
{
	return response::Value { value };
}

template <>
response::Value ModifiedVariable<response::FloatType>::serialize(response::FloatType&& value)
{
	return response::Value { value };
}

template <>
response::Value ModifiedVariable<response::StringType>::serialize(response::StringType&& value)
{
	return response::Value { std::move(value) };
}

template <>
response::Value ModifiedVariable<response::BooleanType>::serialize(response::BooleanType&& value)
{
	return response::Value { value };
}

template <>
response::Value ModifiedVariable<response::Value>::serialize(response::Value&& value)
{
	return response::Value { std::move(value) };
}

template <>
response::Value ModifiedVariable<response::IdType>::serialize(response::IdType&& value)
{
	return response::Value { value };
}

template <>
response::IntType ModifiedResponse<response::IntType>::parse(response::Value&& value)
{
	if (value.type() != response::Type::Int)
	{
		throw std::logic_error { "not an integer" };
	}

	return value.get<response::IntType>();
}

template <>
response::FloatType ModifiedResponse<response::FloatType>::parse(response::Value&& value)
{
	if (value.type() != response::Type::Float && value.type() != response::Type::Int)
	{
		throw std::logic_error { "not a float" };
	}

	return value.get<response::FloatType>();
}

template <>
response::StringType ModifiedResponse<response::StringType>::parse(response::Value&& value)
{
	if (value.type() != response::Type::String)
	{
		throw std::logic_error { "not a string" };
	}

	return value.release<response::StringType>();
}

template <>
response::BooleanType ModifiedResponse<response::BooleanType>::parse(response::Value&& value)
{
	if (value.type() != response::Type::Boolean)
	{
		throw std::logic_error { "not a boolean" };
	}

	return value.get<response::BooleanType>();
}

template <>
response::Value ModifiedResponse<response::Value>::parse(response::Value&& value)
{
	return response::Value { std::move(value) };
}

template <>
response::IdType ModifiedResponse<response::IdType>::parse(response::Value&& value)
{
	if (value.type() != response::Type::String)
	{
		throw std::logic_error { "not a string" };
	}

	return value.release<response::IdType>();
}

} // namespace graphql::client
