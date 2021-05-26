// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GENERATORUTIL_H
#define GENERATORUTIL_H

#include <iostream>
#include <string>
#include <string_view>

namespace graphql::generator {

// RAII object to help with emitting matching include guard begin and end statements
class IncludeGuardScope
{
public:
	explicit IncludeGuardScope(std::ostream& outputFile, std::string_view headerFileName) noexcept;
	~IncludeGuardScope() noexcept;

private:
	std::ostream& _outputFile;
	std::string _includeGuardName;
};

// RAII object to help with emitting matching namespace begin and end statements
class NamespaceScope
{
public:
	explicit NamespaceScope(
		std::ostream& outputFile, std::string_view cppNamespace, bool deferred = false) noexcept;
	NamespaceScope(NamespaceScope&& other) noexcept;
	~NamespaceScope() noexcept;

	bool enter() noexcept;
	bool exit() noexcept;

private:
	bool _inside = false;
	std::ostream& _outputFile;
	std::string_view _cppNamespace;
};

// Keep track of whether we want to add a blank separator line once some additional content is about
// to be output.
class PendingBlankLine
{
public:
	explicit PendingBlankLine(std::ostream& outputFile) noexcept;

	void add() noexcept;
	bool reset() noexcept;

private:
	bool _pending = true;
	std::ostream& _outputFile;
};

} // namespace graphql::generator

#endif // GENERATORUTIL_H
