// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include "Version.h"

export module GraphQL.Internal.Version;

export namespace graphql::internal {

namespace version {

constexpr std::string_view FullVersion = graphql::internal::FullVersion;
constexpr std::size_t MajorVersion = graphql::internal::MajorVersion;
constexpr std::size_t MinorVersion = graphql::internal::MinorVersion;
constexpr std::size_t PatchVersion = graphql::internal::PatchVersion;

} // namespace version

using namespace version;

} // namespace graphql::internal
