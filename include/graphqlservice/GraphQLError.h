// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLERROR_H
#define GRAPHQLERROR_H

// clang-format off
#ifdef GRAPHQL_DLLEXPORTS
	#ifdef IMPL_GRAPHQLERROR_DLL
		#define GRAPHQLERROR_EXPORT __declspec(dllexport)
	#else // !IMPL_GRAPHQLERROR_DLL
		#define GRAPHQLERROR_EXPORT __declspec(dllimport)
	#endif // !IMPL_GRAPHQLERROR_DLL
#else // !GRAPHQL_DLLEXPORTS
	#define GRAPHQLERROR_EXPORT
#endif // !GRAPHQL_DLLEXPORTS
// clang-format on

#include <queue>
#include <string>
#include <variant>

namespace graphql::error {

struct schema_location
{
	size_t line = 0;
	size_t column = 1;

	GRAPHQLERROR_EXPORT bool operator==(const schema_location& rhs) const noexcept
	{
		return line == rhs.line && column == rhs.column;
	}
};

constexpr schema_location emptyLocation {};

using path_segment = std::variant<std::string, size_t>;
using field_path = std::queue<path_segment>;

struct schema_error
{
	std::string message;
	schema_location location;
	field_path path;

	GRAPHQLERROR_EXPORT bool operator==(const schema_error& rhs) const noexcept
	{
		return location == rhs.location && message == rhs.message && path == rhs.path;
	}
};

// This exception bubbles up 1 or more error messages to the JSON results.
class schema_exception : public std::exception
{
public:
	GRAPHQLERROR_EXPORT explicit schema_exception(std::vector<schema_error>&& structuredErrors);
	GRAPHQLERROR_EXPORT explicit schema_exception(std::vector<std::string>&& messages);

	schema_exception() = delete;

	GRAPHQLERROR_EXPORT const char* what() const noexcept override;

	GRAPHQLERROR_EXPORT const std::vector<schema_error>& getStructuredErrors() const noexcept;
	GRAPHQLERROR_EXPORT std::vector<schema_error> getStructuredErrors() noexcept;

protected:
	static std::vector<schema_error> convertMessages(std::vector<std::string>&& messages) noexcept;

private:
	std::vector<schema_error> _structuredErrors;
};
} // namespace graphql::error

#endif // GRAPHQLERROR_H