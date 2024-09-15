// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Version.h"

export module GraphQL.Internal.Version;

export namespace graphql::internal {

// clang-format off
constexpr std::string_view FullVersion = version::FullVersion;

constexpr std::size_t MajorVersion = version::MajorVersion;
constexpr std::size_t MinorVersion = version::MinorVersion;
constexpr std::size_t PatchVersion = version::PatchVersion;
// clang-format on

} // namespace graphql::internal
