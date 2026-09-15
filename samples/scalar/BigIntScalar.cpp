// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "BigIntScalar.h"

#include <charconv>
#include <string>
#include <string_view>

using namespace std::literals;

namespace {

using namespace graphql;

// Parse a std::int64_t out of the JSON-like response::Value wire representation. BigInt values are
// serialized as Strings so they survive without the precision loss of a 32-bit response::IntType,
// but we also accept a plain Int for convenience when the value is small enough.
[[nodiscard]] std::int64_t parseBigInt(const response::Value& value)
{
	switch (value.type())
	{
		case response::Type::Int:
			return static_cast<std::int64_t>(value.get<response::IntType>());

		case response::Type::String:
		{
			const auto& str = value.get<response::StringType>();
			std::int64_t result = 0;
			const auto* first = str.data();
			const auto* last = first + str.size();
			const auto [ptr, ec] = std::from_chars(first, last, result);

			if (ec != std::errc {} || ptr != last)
			{
				throw service::schema_exception { { "not a valid BigInt value"s } };
			}

			return result;
		}

		default:
			throw service::schema_exception { { "not a valid BigInt value"s } };
	}
}

} // namespace

namespace graphql::service {

template <>
bigint::BigInt Argument<bigint::BigInt>::convert(const response::Value& value)
{
	return bigint::BigInt { parseBigInt(value) };
}

template <>
AwaitableResolver Result<bigint::BigInt>::convert(
	AwaitableScalar<bigint::BigInt> result, ResolverParams&& params)
{
	return ModifiedResult<bigint::BigInt>::resolve(std::move(result),
		std::move(params),
		[](bigint::BigInt&& value, const ResolverParams&) {
			return response::Value { std::to_string(value.value) };
		});
}

template <>
void Result<bigint::BigInt>::validateScalar(const response::Value& value)
{
	// Reuse the argument parser to confirm the cached wire representation is well formed.
	static_cast<void>(parseBigInt(value));
}

} // namespace graphql::service
