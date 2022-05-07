// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLVERSION_H
#define GRAPHQLVERSION_H

#include <string_view>

namespace graphql::internal {

constexpr std::string_view FullVersion { "4.3.1" };

constexpr size_t MajorVersion = 4;
constexpr size_t MinorVersion = 3;
constexpr size_t PatchVersion = 1;

} // namespace graphql::internal

#endif // GRAPHQLVERSION_H
