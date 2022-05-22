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
					result.line = static_cast<size_t>(member.second.get<int>());
				}

				continue;
			}

			if (member.first == "column"sv)
			{
				if (member.second.type() == response::Type::Int)
				{
					result.column = static_cast<size_t>(member.second.get<int>());
				}

				continue;
			}
		}
	}

	return result;
}

ErrorPathSegment parseServiceErrorPathSegment(response::Value&& segment)
{
	ErrorPathSegment result { int {} };

	switch (segment.type())
	{
		case response::Type::Int:
			result = segment.get<int>();
			break;

		case response::Type::String:
			result = segment.release<std::string>();
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
					result.message = member.second.release<std::string>();
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
						[](response::Value& location) {
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
						[](response::Value& segment) {
							return parseServiceErrorPathSegment(std::move(segment));
						});
				}

				continue;
			}
		}
	}

	return result;
}

ServiceResponse parseServiceResponse(response::Value response)
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
response::Value Variable<int>::serialize(int&& value)
{
	return response::Value { value };
}

template <>
response::Value Variable<double>::serialize(double&& value)
{
	return response::Value { value };
}

template <>
response::Value Variable<std::string>::serialize(std::string&& value)
{
	return response::Value { std::move(value) };
}

template <>
response::Value Variable<bool>::serialize(bool&& value)
{
	return response::Value { value };
}

template <>
response::Value Variable<response::Value>::serialize(response::Value&& value)
{
	return response::Value { std::move(value) };
}

template <>
response::Value Variable<response::IdType>::serialize(response::IdType&& value)
{
	return response::Value { std::move(value) };
}

template <>
int Response<int>::parse(response::Value&& value)
{
	if (value.type() != response::Type::Int)
	{
		throw std::logic_error { "not an integer" };
	}

	return value.get<int>();
}

template <>
double Response<double>::parse(response::Value&& value)
{
	if (value.type() != response::Type::Float && value.type() != response::Type::Int)
	{
		throw std::logic_error { "not a float" };
	}

	return value.get<double>();
}

template <>
std::string Response<std::string>::parse(response::Value&& value)
{
	if (value.type() != response::Type::String)
	{
		throw std::logic_error { "not a string" };
	}

	return value.release<std::string>();
}

template <>
bool Response<bool>::parse(response::Value&& value)
{
	if (value.type() != response::Type::Boolean)
	{
		throw std::logic_error { "not a boolean" };
	}

	return value.get<bool>();
}

template <>
response::Value Response<response::Value>::parse(response::Value&& value)
{
	return { std::move(value) };
}

template <>
response::IdType Response<response::IdType>::parse(response::Value&& value)
{
	if (!value.maybe_id())
	{
		throw std::logic_error { "not an ID" };
	}

	return value.release<response::IdType>();
}

} // namespace graphql::client
