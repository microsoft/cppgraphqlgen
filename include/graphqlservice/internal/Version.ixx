// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Version.h"

export module GraphQL.Internal.Version;

export namespace graphql::internal {

namespace constants {

constexpr std::string_view FullVersion = internal::FullVersion;

constexpr std::size_t MajorVersion = internal::MajorVersion;
constexpr std::size_t MinorVersion = internal::MinorVersion;
constexpr std::size_t PatchVersion = internal::PatchVersion;

} // namespace constants

using namespace constants;

} // namespace graphql::internal
