// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef GRAPHQLVERSION_H
#define GRAPHQLVERSION_H

#include <cstddef>
#include <string_view>

namespace graphql::internal {

inline namespace version {

constexpr std::string_view FullVersion { "5.0.0" };

constexpr std::size_t MajorVersion = 5;
constexpr std::size_t MinorVersion = 0;
constexpr std::size_t PatchVersion = 0;

} // namespace version

} // namespace graphql::internal

#endif // GRAPHQLVERSION_H
