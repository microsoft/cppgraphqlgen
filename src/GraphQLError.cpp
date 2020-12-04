// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLError.h"

#include <algorithm>

namespace graphql::error {

schema_exception::schema_exception(std::vector<schema_error>&& structuredErrors)
	: _structuredErrors(std::move(structuredErrors))
{
}

schema_exception::schema_exception(std::vector<std::string>&& messages)
	: schema_exception(convertMessages(std::move(messages)))
{
}

std::vector<schema_error> schema_exception::convertMessages(
	std::vector<std::string>&& messages) noexcept
{
	std::vector<schema_error> errors(messages.size());

	std::transform(messages.begin(),
		messages.end(),
		errors.begin(),
		[](std::string& message) noexcept {
			return schema_error { std::move(message) };
		});

	return errors;
}

const char* schema_exception::what() const noexcept
{
	const char* message = nullptr;

	if (_structuredErrors.size() > 0)
	{
		if (!_structuredErrors[0].message.empty())
		{
			message = _structuredErrors[0].message.c_str();
		}
	}

	return (message == nullptr) ? "Unknown schema error" : message;
}

const std::vector<schema_error>& schema_exception::getStructuredErrors() const noexcept
{
	return _structuredErrors;
}

std::vector<schema_error> schema_exception::getStructuredErrors() noexcept
{
	auto structuredErrors = std::move(_structuredErrors);

	return structuredErrors;
}

} /* namespace graphql::error */
