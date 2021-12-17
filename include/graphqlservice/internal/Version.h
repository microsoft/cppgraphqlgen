// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef VERSION_H
#define VERSION_H

#include <string_view>

namespace graphql::internal {

constexpr std::string_view FullVersion { "4.0.0" };

constexpr size_t MajorVersion = 4;
constexpr size_t MinorVersion = 0;
constexpr size_t PatchVersion = 0;

} // namespace graphql::internal

#endif // VERSION_H
