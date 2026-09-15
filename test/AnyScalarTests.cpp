// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "graphqlservice/GraphQLResponse.h"
#include "graphqlservice/JSONResponse.h"

#include <any>
#include <cstdint>
#include <memory>
#include <string>

using namespace graphql;

namespace {

// A toy custom scalar whose C++ representation (a 64-bit integer) can't be expressed with any of
// response::Value's built-in alternatives, since response::IntType is only 32-bit. A hand-written
// resolver can store this directly and hand it off to the token stream via response::AnyScalar,
// without any schema/codegen changes.
struct Int64Scalar
{
	std::int64_t value = 0;
};

// Serialize the payload onto the wire as an ordinary JSON string of the decimal value.
response::AnyScalar makeBigInt(std::int64_t value)
{
	return response::AnyScalar {
		std::any { Int64Scalar { value } },
		[](const std::any& payload, const std::shared_ptr<response::ValueVisitor>& visitor) {
			const auto& scalar = std::any_cast<const Int64Scalar&>(payload);
			visitor->add_string(std::to_string(scalar.value));
		},
	};
}

} // namespace

TEST(AnyScalarCase, ConstructAndInspect)
{
	auto value = response::Value { makeBigInt(9223372036854775807LL) };

	ASSERT_TRUE(response::Type::Scalar == value.type());
	ASSERT_TRUE(value.isAny());
}

TEST(AnyScalarCase, SerializeToJSON)
{
	auto value = response::Value { makeBigInt(9223372036854775807LL) };
	const auto json = response::toJSON(std::move(value));

	ASSERT_EQ(R"js("9223372036854775807")js", json);
}

TEST(AnyScalarCase, SerializeInsideMap)
{
	response::Value map { response::Type::Map };
	map.emplace_back("bigInt", response::Value { makeBigInt(-9223372036854775807LL) });

	const auto json = response::toJSON(std::move(map));

	ASSERT_EQ(R"js({"bigInt":"-9223372036854775807"})js", json);
}

TEST(AnyScalarCase, CopySharesOwnership)
{
	auto original = response::Value { makeBigInt(42) };
	auto copy = response::Value { original };

	ASSERT_TRUE(copy.isAny());
	// Copies share ownership of the same AnyScalar payload (no deep copy), so they compare equal.
	ASSERT_TRUE(original == copy);
}

TEST(AnyScalarCase, DistinctPayloadsCompareUnequal)
{
	auto lhs = response::Value { makeBigInt(1) };
	auto rhs = response::Value { makeBigInt(1) };

	// Even with equal logical values, distinct AnyScalar payloads are compared by identity.
	ASSERT_FALSE(lhs == rhs);
}

TEST(AnyScalarCase, ReleaseAny)
{
	auto value = response::Value { makeBigInt(7) };
	auto payload = value.releaseAny();

	ASSERT_TRUE(static_cast<bool>(payload));
	ASSERT_FALSE(value.isAny());

	const auto& scalar = std::any_cast<const Int64Scalar&>(payload->value);
	ASSERT_EQ(7, scalar.value);
}

TEST(AnyScalarCase, PlainScalarIsNotAny)
{
	response::Value scalar { response::Type::Scalar };
	scalar.set<response::ScalarType>(response::Value { 5 });

	ASSERT_TRUE(response::Type::Scalar == scalar.type());
	ASSERT_FALSE(scalar.isAny());
}
