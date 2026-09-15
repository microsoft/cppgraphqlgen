// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef BIGINTSCALAR_H
#define BIGINTSCALAR_H

#include "graphqlservice/GraphQLService.h"

#include <cstdint>

// This is the custom C++ type backing the GraphQL `BigInt` scalar. It is associated with the
// scalar in schema.scalar.graphql using the `@cppType` directive:
//
//   scalar BigInt @cppType(name: "bigint::BigInt" header: "BigIntScalar.h")
//
// A BigInt can hold values which do not fit in the 32-bit response::IntType, so instead of forcing
// the field accessors to work with response::Value, schemagen generates the resolvers and argument
// accessors in terms of bigint::BigInt. The service::Argument and service::Result specializations
// below are the type-erased bridge that (de)serializes the value to and from the JSON-like
// response::Value wire representation (here, a String so the full 64-bit precision survives).
namespace bigint {

struct [[nodiscard("unnecessary construction")]] BigInt
{
	BigInt() noexcept = default;

	explicit BigInt(std::int64_t value) noexcept
		: value { value }
	{
	}

	[[nodiscard]] bool operator==(const BigInt& rhs) const noexcept
	{
		return value == rhs.value;
	}

	std::int64_t value = 0;
};

} // namespace bigint

namespace graphql::service {

// Parse a BigInt argument from the request (a JSON String, or an Int for small values).
template <>
[[nodiscard("unnecessary conversion")]] bigint::BigInt Argument<bigint::BigInt>::convert(
	const response::Value& value);

// Treat bigint::BigInt as a scalar argument (wrapped in std::optional when nullable) rather than a
// generated INPUT_OBJECT type (which would be wrapped in std::unique_ptr).
template <>
struct CustomScalarArgument<bigint::BigInt> : std::true_type
{
};

// Serialize a BigInt result back into a response::Value (a JSON String).
template <>
[[nodiscard("unnecessary conversion")]] AwaitableResolver Result<bigint::BigInt>::convert(
	AwaitableScalar<bigint::BigInt> result, ResolverParams&& params);

// Validate that a cached response::Value for this scalar is the expected wire representation.
template <>
void Result<bigint::BigInt>::validateScalar(const response::Value& value);

} // namespace graphql::service

#endif // BIGINTSCALAR_H
