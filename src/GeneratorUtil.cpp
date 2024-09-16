// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GeneratorUtil.h"

#include <algorithm>
#include <ranges>

namespace graphql::generator {

IncludeGuardScope::IncludeGuardScope(
	std::ostream& outputFile, std::string_view headerFileName) noexcept
	: _outputFile(outputFile)
	, _includeGuardName(headerFileName.size(), char {})
{
	std::ranges::transform(headerFileName, _includeGuardName.begin(), [](char ch) noexcept -> char {
		if (ch == '.')
		{
			return '_';
		}

		return static_cast<char>(std::toupper(ch));
	});

	_outputFile << R"cpp(// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// WARNING! Do not edit this file manually, your changes will be overwritten.

#pragma once

#ifndef )cpp" << _includeGuardName
				<< R"cpp(
#define )cpp" << _includeGuardName
				<< R"cpp(

)cpp";
}

IncludeGuardScope::~IncludeGuardScope() noexcept
{
	_outputFile << R"cpp(
#endif // )cpp" << _includeGuardName
				<< R"cpp(
)cpp";
}

NamespaceScope::NamespaceScope(
	std::ostream& outputFile, std::string_view cppNamespace, bool deferred /*= false*/) noexcept
	: _outputFile(outputFile)
	, _cppNamespace(cppNamespace)
{
	if (!deferred)
	{
		enter();
	}
}

NamespaceScope::NamespaceScope(NamespaceScope&& other) noexcept
	: _inside(other._inside)
	, _outputFile(other._outputFile)
	, _cppNamespace(other._cppNamespace)
{
	other._inside = false;
}

NamespaceScope::~NamespaceScope() noexcept
{
	exit();
}

bool NamespaceScope::enter() noexcept
{
	if (!_inside)
	{
		_inside = true;

		if (_cppNamespace.empty())
		{
			_outputFile << R"cpp(namespace {
)cpp";
		}
		else
		{
			_outputFile << R"cpp(namespace )cpp" << _cppNamespace << R"cpp( {
)cpp";
		}

		return true;
	}

	return false;
}

bool NamespaceScope::exit() noexcept
{
	if (_inside)
	{
		if (_cppNamespace.empty())
		{
			_outputFile << R"cpp(} // namespace
)cpp";
		}
		else
		{
			_outputFile << R"cpp(} // namespace )cpp" << _cppNamespace << R"cpp(
)cpp";
		}

		_inside = false;
		return true;
	}

	return false;
}

PendingBlankLine::PendingBlankLine(std::ostream& outputFile) noexcept
	: _outputFile(outputFile)
{
}

void PendingBlankLine::add() noexcept
{
	_pending = true;
}

bool PendingBlankLine::reset() noexcept
{
	if (_pending)
	{
		_outputFile << std::endl;
		_pending = false;
		return true;
	}

	return false;
}

} // namespace graphql::generator