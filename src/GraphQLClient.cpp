// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLClient.h"

using namespace std::literals;

namespace graphql::client {

ErrorLocation parseServiceErrorLocation(response::Value&& location)
{
	ErrorLocation result;

	if (location.type() == response::Type::Map)
	{
		auto members = location.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == "line"sv)
			{
				if (member.second.type() == response::Type::Int)
				{
					result.line = static_cast<size_t>(member.second.get<response::IntType>());
				}

				continue;
			}

			if (member.first == "column"sv)
			{
				if (member.second.type() == response::Type::Int)
				{
					result.column = static_cast<size_t>(member.second.get<response::IntType>());
				}

				continue;
			}
		}
	}

	return result;
}

ErrorPathSegment parseServiceErrorPathSegment(response::Value&& segment)
{
	ErrorPathSegment result { response::IntType {} };

	switch (segment.type())
	{
		case response::Type::Int:
			result = segment.get<response::IntType>();
			break;

		case response::Type::String:
			result = segment.release<response::StringType>();
			break;

		default:
			break;
	}

	return result;
}

Error parseServiceError(response::Value&& error)
{
	Error result;

	if (error.type() == response::Type::Map)
	{
		auto members = error.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == "message"sv)
			{
				if (member.second.type() == response::Type::String)
				{
					result.message = member.second.release<response::StringType>();
				}

				continue;
			}

			if (member.first == "locations"sv)
			{
				if (member.second.type() == response::Type::List)
				{
					auto locations = member.second.release<response::ListType>();

					result.locations.reserve(locations.size());
					std::transform(locations.begin(),
						locations.end(),
						std::back_inserter(result.locations),
						[](response::Value& location)
					{
						return parseServiceErrorLocation(std::move(location));
					});
				}

				continue;
			}

			if (member.first == "path"sv)
			{
				if (member.second.type() == response::Type::List)
				{
					auto segments = member.second.release<response::ListType>();

					result.path.reserve(segments.size());
					std::transform(segments.begin(),
						segments.end(),
						std::back_inserter(result.path),
						[](response::Value& segment)
					{
						return parseServiceErrorPathSegment(std::move(segment));
					});
				}

				continue;
			}
		}
	}

	return result;
}

ServiceResponse parseServiceResponse(response::Value&& response)
{
	ServiceResponse result;

	if (response.type() == response::Type::Map)
	{
		auto members = response.release<response::MapType>();

		for (auto& member : members)
		{
			if (member.first == "data"sv)
			{
				// The generated client code can parse this.
				result.data = std::move(member.second);
				continue;
			}

			if (member.first == "errors"sv)
			{
				if (member.second.type() == response::Type::List)
				{
					auto errors = member.second.release<response::ListType>();

					result.errors.reserve(errors.size());
					std::transform(errors.begin(),
						errors.end(),
						std::back_inserter(result.errors),
						[](response::Value& error) {
							return parseServiceError(std::move(error));
						});
				}

				continue;
			}
		}
	}

	return result;
}

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
