// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "ValidationMock.h"

#include "graphqlservice/JSONResponse.h"

#include <chrono>
#include <cstddef>

using namespace graphql;

using namespace std::literals;

class ValidationExamplesCase : public ::testing::Test
{
public:
	static void SetUpTestCase()
	{
		_service = std::make_shared<validation::Operations>(std::make_shared<validation::Query>(),
			std::make_shared<validation::Mutation>());
	}

	static void TearDownTestCase()
	{
		_service.reset();
	}

protected:
	static std::shared_ptr<validation::Operations> _service;
};

std::shared_ptr<validation::Operations> ValidationExamplesCase::_service;

TEST_F(ValidationExamplesCase, CounterExample102)
{
	// https://spec.graphql.org/October2021/#example-12752
	auto query = R"(query getDogName {
			dog {
				name
				color
			}
		}

		extend type Dog {
			color: String
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), std::size_t { 2 });
	EXPECT_EQ(
		R"js({"message":"Undefined field type: Dog name: color","locations":[{"line":4,"column":5}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(R"js({"message":"Unexpected type definition","locations":[{"line":8,"column":3}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example103)
{
	// https://spec.graphql.org/October2021/#example-069e1
	auto query = R"(query getDogName {
			dog {
				name
			}
		}

		query getOwnerName {
			dog {
				owner {
					name
				}
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample104)
{
	// https://spec.graphql.org/October2021/#example-5e409
	auto query = R"(query getName {
			dog {
				name
			}
		}

		query getName {
			dog {
				owner {
					name
				}
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Duplicate operation name: getName","locations":[{"line":7,"column":3}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample105)
{
	// https://spec.graphql.org/October2021/#example-77c2e
	auto query = R"(query dogOperation {
			dog {
				name
			}
		}

		mutation dogOperation {
			mutateDog {
				id
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Duplicate operation name: dogOperation","locations":[{"line":7,"column":3}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example106)
{
	// https://spec.graphql.org/October2021/#example-be853
	auto query = R"({
			dog {
				name
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample107)
{
	// https://spec.graphql.org/October2021/#example-44b85
	auto query = R"({
			dog {
				name
			}
		}

		query getName {
			dog {
				owner {
					name
				}
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Anonymous operation not alone","locations":[{"line":1,"column":1}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example108)
{
	// https://spec.graphql.org/October2021/#example-5bbc3
	auto query = R"(subscription sub {
			newMessage {
				body
				sender
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example109)
{
	// https://spec.graphql.org/October2021/#example-13061
	auto query = R"(subscription sub {
			...newMessageFields
		}

		fragment newMessageFields on Subscription {
			newMessage {
				body
				sender
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample110)
{
	// https://spec.graphql.org/October2021/#example-3997d
	auto query = R"(subscription sub {
			newMessage {
				body
				sender
			}
			disallowedSecondRootField
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Subscription with more than one root field name: sub","locations":[{"line":1,"column":1}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample111)
{
	// https://spec.graphql.org/October2021/#example-18466
	auto query = R"(subscription sub {
			...multipleSubscriptions
		}

		fragment multipleSubscriptions on Subscription {
			newMessage {
				body
				sender
			}
			disallowedSecondRootField
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Subscription with more than one root field name: sub","locations":[{"line":1,"column":1}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample112)
{
	// https://spec.graphql.org/October2021/#example-a8fa1
	auto query = R"(subscription sub {
			__typename
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Subscription with Introspection root field name: sub","locations":[{"line":1,"column":1}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample113)
{
	// https://spec.graphql.org/October2021/#example-48706
	auto query = R"(fragment fieldNotDefined on Dog {
			meowVolume
		}

		fragment aliasedLyingFieldTargetNotDefined on Dog {
			barkVolume: kawVolume
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 4 }) << "2 undefined fields + 2 unused fragments";
	ASSERT_GE(errors.size(), std::size_t { 2 });
	EXPECT_EQ(
		R"js({"message":"Undefined field type: Dog name: meowVolume","locations":[{"line":2,"column":4}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Undefined field type: Dog name: kawVolume","locations":[{"line":6,"column":4}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example114)
{
	// https://spec.graphql.org/October2021/#example-d34e0
	auto query = R"(fragment interfaceFieldSelection on Pet {
			name
		}

		query {
			dog {
				...interfaceFieldSelection
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample115)
{
	// https://spec.graphql.org/October2021/#example-db33b
	auto query = R"(fragment definedOnImplementorsButNotInterface on Pet {
			nickname
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 undefined field + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Undefined field type: Pet name: nickname","locations":[{"line":2,"column":4}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example116)
{
	// https://spec.graphql.org/October2021/#example-245fa
	auto query = R"(fragment inDirectFieldSelectionOnUnion on CatOrDog {
			__typename
			... on Pet {
				name
			}
			... on Dog {
				barkVolume
			}
		}

		query {
			dog {
				...inDirectFieldSelectionOnUnion
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample117)
{
	// https://spec.graphql.org/October2021/#example-252ad
	auto query = R"(fragment directFieldSelectionOnUnion on CatOrDog {
			name
			barkVolume
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 3 }) << "2 undefined fields + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 2 });
	EXPECT_EQ(
		R"js({"message":"Field on union type: CatOrDog name: name","locations":[{"line":2,"column":4}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Field on union type: CatOrDog name: barkVolume","locations":[{"line":3,"column":4}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example118)
{
	// https://spec.graphql.org/October2021/#example-4e10c
	auto query = R"(fragment mergeIdenticalFields on Dog {
			name
			name
		}

		fragment mergeIdenticalAliasesAndFields on Dog {
			otherName: name
			otherName: name
		}

		query {
			dog {
				...mergeIdenticalFields
				...mergeIdenticalAliasesAndFields
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample119)
{
	// https://spec.graphql.org/October2021/#example-a2230
	auto query = R"(fragment conflictingBecauseAlias on Dog {
			name: nickname
			name
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 conflicting field + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Conflicting field type: Dog name: name","locations":[{"line":3,"column":4}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example120)
{
	// https://spec.graphql.org/October2021/#example-b6369
	auto query = R"(fragment mergeIdenticalFieldsWithIdenticalArgs on Dog {
			doesKnowCommand(dogCommand: SIT)
			doesKnowCommand(dogCommand: SIT)
		}

		fragment mergeIdenticalFieldsWithIdenticalValues on Dog {
			doesKnowCommand(dogCommand: $dogCommand)
			doesKnowCommand(dogCommand: $dogCommand)
		}

		query q1 {
			dog {
				...mergeIdenticalFieldsWithIdenticalArgs
			}
		}

		query q2 ($dogCommand: DogCommand!) {
			dog {
				...mergeIdenticalFieldsWithIdenticalValues
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample121)
{
	// https://spec.graphql.org/October2021/#example-00fbf
	auto query = R"(fragment conflictingArgsOnValues on Dog {
			doesKnowCommand(dogCommand: SIT)
			doesKnowCommand(dogCommand: HEEL)
		}

		fragment conflictingArgsValueAndVar on Dog {
			doesKnowCommand(dogCommand: SIT)
			doesKnowCommand(dogCommand: $dogCommand)
		}

		fragment conflictingArgsWithVars on Dog {
			doesKnowCommand(dogCommand: $varOne)
			doesKnowCommand(dogCommand: $varTwo)
		}

		fragment differingArgs on Dog {
			doesKnowCommand(dogCommand: SIT)
			doesKnowCommand
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 9 })
		<< "4 conflicting fields + 1 missing argument + 4 unused fragments";
	ASSERT_GE(errors.size(), std::size_t { 4 });
	EXPECT_EQ(
		R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":3,"column":4}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":8,"column":4}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":13,"column":4}]})js",
		response::toJSON(std::move(errors[2])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":18,"column":4}]})js",
		response::toJSON(std::move(errors[3])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example122)
{
	// https://spec.graphql.org/October2021/#example-a8406
	auto query = R"(fragment safeDifferingFields on Pet {
			... on Dog {
				volume: barkVolume
			}
			... on Cat {
				volume: meowVolume
			}
		}

		fragment safeDifferingArgs on Pet {
			... on Dog {
				doesKnowCommand(dogCommand: SIT)
			}
			... on Cat {
				doesKnowCommand(catCommand: JUMP)
			}
		}

		query {
			dog {
				...safeDifferingFields
				...safeDifferingArgs
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample123)
{
	// https://spec.graphql.org/October2021/#example-54e3d
	auto query = R"(fragment conflictingDifferingResponses on Pet {
			... on Dog {
				someValue: nickname
			}
			... on Cat {
				someValue: meowVolume
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 conflicting field + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Conflicting field type: Cat name: meowVolume","locations":[{"line":6,"column":5}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example124)
{
	// https://spec.graphql.org/October2021/#example-e23c5
	auto query = R"(fragment scalarSelection on Dog {
			barkVolume
		}

		query {
			dog {
				...scalarSelection
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample125)
{
	// https://spec.graphql.org/October2021/#example-13b69
	auto query = R"(fragment scalarSelectionsNotAllowedOnInt on Dog {
			barkVolume {
				sinceWhen
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 invalid field + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Field on scalar type: Int name: sinceWhen","locations":[{"line":3,"column":5}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example126)
{
	// https://spec.graphql.org/October2021/#example-9bada
	auto query = R"(query {
			human {
				name
			}
			pet {
				name
			}
			catOrDog {
				... on Cat {
					volume: meowVolume
				}
				... on Dog {
					volume: barkVolume
				}
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample127)
{
	// https://spec.graphql.org/October2021/#example-d68ee
	auto query = R"(query directQueryOnObjectWithoutSubFields {
			human
		}

		query directQueryOnInterfaceWithoutSubFields {
			pet
		}

		query directQueryOnUnionWithoutSubFields {
			catOrDog
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), std::size_t { 3 }) << "3 invalid fields";
	EXPECT_EQ(
		R"js({"message":"Missing fields on non-scalar type: Human","locations":[{"line":2,"column":4}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Missing fields on non-scalar type: Pet","locations":[{"line":6,"column":4}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Missing fields on non-scalar type: CatOrDog","locations":[{"line":10,"column":4}]})js",
		response::toJSON(std::move(errors[2])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example128)
{
	// https://spec.graphql.org/October2021/#example-dfd15
	auto query = R"(fragment argOnRequiredArg on Dog {
			doesKnowCommand(dogCommand: SIT)
		}

		fragment argOnOptional on Dog {
			isHousetrained(atOtherHomes: true) @include(if: true)
		}

		query {
			dog {
				...argOnRequiredArg
				...argOnOptional
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample129)
{
	// https://spec.graphql.org/October2021/#example-d5639
	auto query = R"(fragment invalidArgName on Dog {
			doesKnowCommand(command: CLEAN_UP_HOUSE)
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 3 })
		<< "1 undefined argument + 1 missing argument + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Undefined argument type: Dog field: doesKnowCommand name: command","locations":[{"line":2,"column":20}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample130)
{
	// https://spec.graphql.org/October2021/#example-df41e
	auto query = R"(fragment invalidArgName on Dog {
			isHousetrained(atOtherHomes: true) @include(unless: false)
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 3 })
		<< "1 undefined argument + 1 missing argument + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Undefined argument directive: include name: unless","locations":[{"line":2,"column":48}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example131)
{
	// https://spec.graphql.org/October2021/#example-73706
	auto query = R"(query {
			arguments {
				multipleReqs(x: 1, y: 2)
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example132)
{
	// https://spec.graphql.org/October2021/#example-bda7e
	auto query = R"(fragment multipleArgs on Arguments {
			multipleReqs(x: 1, y: 2)
		}

		fragment multipleArgsReverseOrder on Arguments {
			multipleReqs(y: 1, x: 2)
		}

		query q1 {
			arguments {
				...multipleArgs
			}
		}

		query q2 {
			arguments {
				...multipleArgsReverseOrder
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example133)
{
	// https://spec.graphql.org/October2021/#example-503bd
	auto query = R"(fragment goodBooleanArg on Arguments {
			booleanArgField(booleanArg: true)
		}

		fragment goodNonNullArg on Arguments {
			nonNullBooleanArgField(nonNullBooleanArg: true)
		}

		query {
			arguments {
				...goodBooleanArg
				...goodNonNullArg
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example134)
{
	// https://spec.graphql.org/October2021/#example-1f1d2
	auto query = R"(fragment goodBooleanArgDefault on Arguments {
			booleanArgField
		}

		query {
			arguments {
				...goodBooleanArgDefault
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample135)
{
	// https://spec.graphql.org/October2021/#example-f12a1
	auto query = R"(fragment missingRequiredArg on Arguments {
			nonNullBooleanArgField
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 missing argument + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Missing argument type: Arguments field: nonNullBooleanArgField name: nonNullBooleanArg","locations":[{"line":2,"column":4}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample136)
{
	// https://spec.graphql.org/October2021/#example-0bc81
	auto query = R"(fragment missingRequiredArg on Arguments {
			nonNullBooleanArgField(nonNullBooleanArg: null)
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 missing argument + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Required non-null argument type: Arguments field: nonNullBooleanArgField name: nonNullBooleanArg","locations":[{"line":2,"column":4}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example137)
{
	// https://spec.graphql.org/October2021/#example-3703b
	auto query = R"({
			dog {
				...fragmentOne
				...fragmentTwo
			}
		}

		fragment fragmentOne on Dog {
			name
		}

		fragment fragmentTwo on Dog {
			owner {
				name
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample138)
{
	// https://spec.graphql.org/October2021/#example-2c3e3
	auto query = R"({
			dog {
				...fragmentOne
			}
		}

		fragment fragmentOne on Dog {
			name
		}

		fragment fragmentOne on Dog {
			owner {
				name
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 1 }) << "1 duplicate fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Duplicate fragment name: fragmentOne","locations":[{"line":11,"column":3}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example139)
{
	// https://spec.graphql.org/October2021/#example-1b2da
	auto query = R"(fragment correctType on Dog {
			name
		}

		fragment inlineFragment on Dog {
			... on Dog {
				name
			}
		}

		fragment inlineFragment2 on Dog {
			... @include(if: true) {
				name
			}
		}

		query {
			dog {
				...correctType
				...inlineFragment
				...inlineFragment2
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample140)
{
	// https://spec.graphql.org/October2021/#example-463f6
	auto query = R"(fragment notOnExistingType on NotInSchema {
			name
		}

		fragment inlineNotExistingType on Dog {
			... on NotInSchema {
				name
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 4 }) << "2 not existing types + 2 unused fragments";
	ASSERT_GE(errors.size(), std::size_t { 2 });
	EXPECT_EQ(
		R"js({"message":"Undefined target type on fragment definition: notOnExistingType name: NotInSchema","locations":[{"line":1,"column":28}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Undefined target type on inline fragment name: NotInSchema","locations":[{"line":6,"column":8}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example141)
{
	// https://spec.graphql.org/October2021/#example-3c8d4
	auto query = R"(fragment fragOnObject on Dog {
			name
		}

		fragment fragOnInterface on Pet {
			name
		}

		fragment fragOnUnion on CatOrDog {
			... on Dog {
				name
			}
		}

		query {
			dog {
				...fragOnObject
				...fragOnInterface
				...fragOnUnion
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample142)
{
	// https://spec.graphql.org/October2021/#example-4d5e5
	auto query = R"(fragment fragOnScalar on Int {
			something
		}

		fragment inlineFragOnScalar on Dog {
			... on Boolean {
				somethingElse
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 4 }) << "2 not existing types + 2 unused fragments";
	ASSERT_GE(errors.size(), std::size_t { 2 });
	EXPECT_EQ(
		R"js({"message":"Scalar target type on fragment definition: fragOnScalar name: Int","locations":[{"line":1,"column":23}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Scalar target type on inline fragment name: Boolean","locations":[{"line":6,"column":8}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample143)
{
	// https://spec.graphql.org/October2021/#example-9e1e3
	auto query = R"(fragment nameFragment on Dog { # unused
			name
		}

		{
			dog {
				name
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 1 }) << "1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Unused fragment definition name: nameFragment","locations":[{"line":1,"column":1}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample144)
{
	// https://spec.graphql.org/October2021/#example-28421
	auto query = R"({
			dog {
				...undefinedFragment
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 undefined fragment + 1 missing field";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Undefined fragment spread name: undefinedFragment","locations":[{"line":3,"column":8}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample145)
{
	// https://spec.graphql.org/October2021/#example-9ceb4
	auto query = R"({
			dog {
				...nameFragment
			}
		}

		fragment nameFragment on Dog {
			name
			...barkVolumeFragment
		}

		fragment barkVolumeFragment on Dog {
			barkVolume
			...nameFragment
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "2 cyclic fragments";
	ASSERT_GE(errors.size(), std::size_t { 2 });
	EXPECT_EQ(
		R"js({"message":"Cyclic fragment spread name: nameFragment","locations":[{"line":14,"column":7}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Cyclic fragment spread name: barkVolumeFragment","locations":[{"line":9,"column":7}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example146)
{
	// https://spec.graphql.org/October2021/#example-08734
	auto query = R"({
			dog {
				name
				barkVolume
				name
				barkVolume
				name
				barkVolume
				name
				# forever...
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample147)
{
	// https://spec.graphql.org/October2021/#example-cd11c
	auto query = R"({
			dog {
				...dogFragment
			}
		}

		fragment dogFragment on Dog {
			name
			owner {
				name
				...ownerFragment
			}
		}

		fragment ownerFragment on Human {
			name
			pets {
				name
				...dogFragment
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "2 cyclic fragments";
	ASSERT_GE(errors.size(), std::size_t { 2 });
	EXPECT_EQ(
		R"js({"message":"Cyclic fragment spread name: dogFragment","locations":[{"line":19,"column":8}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Cyclic fragment spread name: ownerFragment","locations":[{"line":11,"column":8}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example148)
{
	// https://spec.graphql.org/October2021/#example-0fc38
	auto query = R"(fragment dogFragment on Dog {
			... on Dog {
				barkVolume
			}
		}

		query {
			dog {
				...dogFragment
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample149)
{
	// https://spec.graphql.org/October2021/#example-4d411
	auto query = R"(fragment catInDogFragmentInvalid on Dog {
			... on Cat {
				meowVolume
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 incompatible type + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Incompatible target type on inline fragment name: Cat","locations":[{"line":2,"column":8}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example150)
{
	// https://spec.graphql.org/October2021/#example-2c8d0
	auto query = R"(fragment petNameFragment on Pet {
			name
		}

		fragment interfaceWithinObjectFragment on Dog {
			...petNameFragment
		}

		query {
			dog {
				...interfaceWithinObjectFragment
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example151)
{
	// https://spec.graphql.org/October2021/#example-41843
	auto query = R"(fragment catOrDogNameFragment on CatOrDog {
			... on Cat {
				meowVolume
			}
		}

		fragment unionWithObjectFragment on Dog {
			...catOrDogNameFragment
		}

		query {
			dog {
				...unionWithObjectFragment
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example152)
{
	// https://spec.graphql.org/October2021/#example-85110
	auto query = R"(fragment petFragment on Pet {
			name
			... on Dog {
				barkVolume
			}
		}

		fragment catOrDogFragment on CatOrDog {
			... on Cat {
				meowVolume
			}
		}

		query {
			dog {
				...petFragment
				...catOrDogFragment
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample153)
{
	// https://spec.graphql.org/October2021/#example-a8dcc
	auto query = R"(fragment sentientFragment on Sentient {
			... on Dog {
				barkVolume
			}
		}

		fragment humanOrAlienFragment on HumanOrAlien {
			... on Cat {
				meowVolume
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 4 }) << "2 incompatible type + 2 unused fragments";
	ASSERT_GE(errors.size(), std::size_t { 2 });
	EXPECT_EQ(
		R"js({"message":"Incompatible target type on inline fragment name: Dog","locations":[{"line":2,"column":8}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Incompatible target type on inline fragment name: Cat","locations":[{"line":8,"column":8}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example154)
{
	// https://spec.graphql.org/October2021/#example-dc875
	auto query = R"(fragment unionWithInterface on Pet {
			...dogOrHumanFragment
		}

		fragment dogOrHumanFragment on DogOrHuman {
			... on Dog {
				barkVolume
			}
		}

		query {
			dog {
				...unionWithInterface
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample155)
{
	// https://spec.graphql.org/October2021/#example-c9c63
	auto query = R"(fragment nonIntersectingInterfaces on Pet {
			...sentientFragment
		}

		fragment sentientFragment on Sentient {
			name
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 3 }) << "1 incompatible type + 2 unused fragments";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Incompatible fragment spread target type: Sentient name: sentientFragment","locations":[{"line":2,"column":7}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example156)
{
	// https://spec.graphql.org/October2021/#example-bc12a
	auto query = R"(fragment interfaceWithInterface on Node {
			...resourceFragment
		}

		fragment resourceFragment on Resource {
			url
		}

		query {
			resource {
				...interfaceWithInterface
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example157)
{
	// https://spec.graphql.org/October2021/#example-7ee0e
	auto query = R"(fragment goodBooleanArg on Arguments {
			booleanArgField(booleanArg: true)
		}

		fragment coercedIntIntoFloatArg on Arguments {
			# Note: The input coercion rules for Float allow Int literals.
			floatArgField(floatArg: 123)
		}

		query goodComplexDefaultValue($search: ComplexInput = { name: "Fido" }) {
			findDog(complex: $search) {
				name
			}

			arguments {
				...goodBooleanArg
				...coercedIntIntoFloatArg
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample158)
{
	// https://spec.graphql.org/October2021/#example-3a7c1
	auto query = R"(fragment stringIntoInt on Arguments {
			intArgField(intArg: "123")
		}

		query badComplexValue {
			findDog(complex: { name: 123 }) {
				name
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 5 })
		<< "2 expected values + 2 incompatible arguments + 1 unused fragment";
	ASSERT_GE(errors.size(), std::size_t { 4 });
	EXPECT_EQ(R"js({"message":"Expected Int value","locations":[{"line":2,"column":24}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Incompatible argument type: Arguments field: intArgField name: intArg","locations":[{"line":2,"column":16}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
	EXPECT_EQ(R"js({"message":"Expected String value","locations":[{"line":6,"column":29}]})js",
		response::toJSON(std::move(errors[2])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Incompatible argument type: Query field: findDog name: complex","locations":[{"line":6,"column":12}]})js",
		response::toJSON(std::move(errors[3])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example159)
{
	// https://spec.graphql.org/October2021/#example-a940b
	auto query = R"({
			findDog(complex: { name: "Fido" }) {
				name
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample160)
{
	// https://spec.graphql.org/October2021/#example-1a5f6
	auto query = R"({
			findDog(complex: { favoriteCookieFlavor: "Bacon" }) {
				name
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 undefined field + 1 incompatible argument";
	ASSERT_GE(errors.size(), std::size_t { 2 });
	EXPECT_EQ(
		R"js({"message":"Undefined Input Object field type: ComplexInput name: favoriteCookieFlavor","locations":[{"line":2,"column":45}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Incompatible argument type: Query field: findDog name: complex","locations":[{"line":2,"column":12}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample161)
{
	// https://spec.graphql.org/October2021/#example-5d541
	auto query = R"({
			findDog(complex: { name: "Fido", name: "Fido" }) {
				name
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 1 }) << "1 conflicting field";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Conflicting input field name: name","locations":[{"line":2,"column":37}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample162)
{
	// https://spec.graphql.org/October2021/#example-55f3f
	auto query = R"(query @skip(if: $foo) {
			dog {
				name
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 1 }) << "1 unexpected location";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Unexpected location for directive: skip name: QUERY","locations":[{"line":1,"column":7}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample163)
{
	// https://spec.graphql.org/October2021/#example-b2e6c
	auto query = R"(query ($foo: Boolean = true, $bar: Boolean = false) {
			dog @skip(if: $foo) @skip(if: $bar) {
				name
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 conflicting directive + 1 unused variable";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Conflicting directive name: skip","locations":[{"line":2,"column":24}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example164)
{
	// https://spec.graphql.org/October2021/#example-c5ee9
	auto query = R"(query ($foo: Boolean = true, $bar: Boolean = false) {
			dog @skip(if: $foo) {
				name
			}
			dog @skip(if: $bar) {
				nickname
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample165)
{
	// https://spec.graphql.org/October2021/#example-abc9c
	auto query = R"(query houseTrainedQuery($atOtherHomes: Boolean, $atOtherHomes: Boolean) {
			dog {
				isHousetrained(atOtherHomes: $atOtherHomes)
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 1 }) << "1 conflicting variable";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Conflicting variable operation: houseTrainedQuery name: atOtherHomes","locations":[{"line":1,"column":49}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example166)
{
	// https://spec.graphql.org/October2021/#example-54c93
	auto query = R"(query A($atOtherHomes: Boolean) {
			...HouseTrainedFragment
		}

		query B($atOtherHomes: Boolean) {
			...HouseTrainedFragment
		}

		fragment HouseTrainedFragment on Query {
			dog {
				isHousetrained(atOtherHomes: $atOtherHomes)
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example167)
{
	// https://spec.graphql.org/October2021/#example-ce150
	auto query = R"(query takesComplexInput($complexInput: ComplexInput) {
			findDog(complex: $complexInput) {
				name
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example168)
{
	// https://spec.graphql.org/October2021/#example-a4255
	auto query = R"(query takesBoolean($atOtherHomes: Boolean) {
			dog {
				isHousetrained(atOtherHomes: $atOtherHomes)
			}
		}

		query takesComplexInput($complexInput: ComplexInput) {
			findDog(complex: $complexInput) {
				name
			}
		}

		query TakesListOfBooleanBang($booleans: [Boolean!]) {
			booleanList(booleanListArg: $booleans)
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample169)
{
	// https://spec.graphql.org/October2021/#example-aeba9
	auto query = R"(query takesCat($cat: Cat) {
			dog {
				name
			}
		}

		query takesDogBang($dog: Dog!) {
			dog {
				name
			}
		}

		query takesListOfPet($pets: [Pet]) {
			dog {
				name
			}
		}

		query takesCatOrDog($catOrDog: CatOrDog) {
			dog {
				name
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 4 }) << "4 invalid variable types";
	ASSERT_GE(errors.size(), std::size_t { 4 });
	EXPECT_EQ(
		R"js({"message":"Invalid variable type operation: takesCat name: cat","locations":[{"line":1,"column":22}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Invalid variable type operation: takesDogBang name: dog","locations":[{"line":7,"column":28}]})js",
		response::toJSON(std::move(errors[1])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Invalid variable type operation: takesListOfPet name: pets","locations":[{"line":13,"column":31}]})js",
		response::toJSON(std::move(errors[2])))
		<< "error should match";
	EXPECT_EQ(
		R"js({"message":"Invalid variable type operation: takesCatOrDog name: catOrDog","locations":[{"line":19,"column":34}]})js",
		response::toJSON(std::move(errors[3])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example170)
{
	// https://spec.graphql.org/October2021/#example-38119
	auto query = R"(query variableIsDefined($atOtherHomes: Boolean) {
			dog {
				isHousetrained(atOtherHomes: $atOtherHomes)
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample171)
{
	// https://spec.graphql.org/October2021/#example-5ba94
	auto query = R"(query variableIsNotDefined {
			dog {
				isHousetrained(atOtherHomes: $atOtherHomes)
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 undefined variable + 1 incompatible argument";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Undefined variable name: atOtherHomes","locations":[{"line":3,"column":34}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example172)
{
	// https://spec.graphql.org/October2021/#example-559c2
	auto query = R"(query variableIsDefinedUsedInSingleFragment($atOtherHomes: Boolean) {
			dog {
				...isHousetrainedFragment
			}
		}

		fragment isHousetrainedFragment on Dog {
			isHousetrained(atOtherHomes: $atOtherHomes)
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample173)
{
	// https://spec.graphql.org/October2021/#example-93d3e
	auto query = R"(query variableIsNotDefinedUsedInSingleFragment {
			dog {
				...isHousetrainedFragment
			}
		}

		fragment isHousetrainedFragment on Dog {
			isHousetrained(atOtherHomes: $atOtherHomes)
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 undefined variable + 1 incompatible argument";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Undefined variable name: atOtherHomes","locations":[{"line":8,"column":33}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample174)
{
	// https://spec.graphql.org/October2021/#example-ee7be
	auto query = R"(query variableIsNotDefinedUsedInNestedFragment {
			dog {
				...outerHousetrainedFragment
			}
		}

		fragment outerHousetrainedFragment on Dog {
			...isHousetrainedFragment
		}

		fragment isHousetrainedFragment on Dog {
			isHousetrained(atOtherHomes: $atOtherHomes)
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 undefined variable + 1 incompatible argument";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Undefined variable name: atOtherHomes","locations":[{"line":12,"column":33}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example175)
{
	// https://spec.graphql.org/October2021/#example-d601e
	auto query = R"(query housetrainedQueryOne($atOtherHomes: Boolean) {
			dog {
				...isHousetrainedFragment
			}
		}

		query housetrainedQueryTwo($atOtherHomes: Boolean) {
			dog {
				...isHousetrainedFragment
			}
		}

		fragment isHousetrainedFragment on Dog {
			isHousetrained(atOtherHomes: $atOtherHomes)
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample176)
{
	// https://spec.graphql.org/October2021/#example-2b284
	auto query = R"(query housetrainedQueryOne($atOtherHomes: Boolean) {
			dog {
				...isHousetrainedFragment
			}
		}

		query housetrainedQueryTwoNotDefined {
			dog {
				...isHousetrainedFragment
			}
		}

		fragment isHousetrainedFragment on Dog {
			isHousetrained(atOtherHomes: $atOtherHomes)
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 }) << "1 undefined variable + 1 incompatible argument";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Undefined variable name: atOtherHomes","locations":[{"line":14,"column":33}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample177)
{
	// https://spec.graphql.org/October2021/#example-464b6
	auto query = R"(query variableUnused($atOtherHomes: Boolean) {
			dog {
				isHousetrained
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 1 }) << "1 unused variable";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Unused variable name: atOtherHomes","locations":[{"line":1,"column":22}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example178)
{
	// https://spec.graphql.org/October2021/#example-6d4bb
	auto query = R"(query variableUsedInFragment($atOtherHomes: Boolean) {
			dog {
				...isHousetrainedFragment
			}
		}

		fragment isHousetrainedFragment on Dog {
			isHousetrained(atOtherHomes: $atOtherHomes)
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample179)
{
	// https://spec.graphql.org/October2021/#example-a30e2
	auto query = R"(query variableNotUsedWithinFragment($atOtherHomes: Boolean) {
			dog {
				...isHousetrainedWithoutVariableFragment
			}
		}

		fragment isHousetrainedWithoutVariableFragment on Dog {
			isHousetrained
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 1 }) << "1 unused variable";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Unused variable name: atOtherHomes","locations":[{"line":1,"column":37}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample180)
{
	// https://spec.graphql.org/October2021/#example-e647f
	auto query = R"(query queryWithUsedVar($atOtherHomes: Boolean) {
			dog {
				...isHousetrainedFragment
			}
		}

		query queryWithExtraVar($atOtherHomes: Boolean, $extra: Int) {
			dog {
				...isHousetrainedFragment
			}
		}

		fragment isHousetrainedFragment on Dog {
			isHousetrained(atOtherHomes: $atOtherHomes)
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 1 }) << "1 unused variable";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Unused variable name: extra","locations":[{"line":7,"column":51}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample181)
{
	// https://spec.graphql.org/October2021/#example-2028e
	auto query = R"(query intCannotGoIntoBoolean($intArg: Int) {
			arguments {
				booleanArgField(booleanArg: $intArg)
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 })
		<< "1 incompatible variable + 1 incompatible argument";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Incompatible variable type: Int name: Boolean","locations":[{"line":3,"column":33}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample182)
{
	// https://spec.graphql.org/October2021/#example-8d369
	auto query = R"(query booleanListCannotGoIntoBoolean($booleanListArg: [Boolean]) {
			arguments {
				booleanArgField(booleanArg: $booleanListArg)
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 })
		<< "1 incompatible variable + 1 incompatible argument";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Expected Scalar variable type","locations":[{"line":3,"column":33}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample183)
{
	// https://spec.graphql.org/October2021/#example-ed727
	auto query = R"(query booleanArgQuery($booleanArg: Boolean) {
			arguments {
				nonNullBooleanArgField(nonNullBooleanArg: $booleanArg)
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 })
		<< "1 incompatible variable + 1 incompatible argument";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Expected Non-Null variable type","locations":[{"line":3,"column":47}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example184)
{
	// https://spec.graphql.org/October2021/#example-c5959
	auto query = R"(query nonNullListToList($nonNullBooleanList: [Boolean]!) {
			arguments {
				booleanListArgField(booleanListArg: $nonNullBooleanList)
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample185)
{
	// https://spec.graphql.org/October2021/#example-64255
	auto query = R"(query listToNonNullList($booleanList: [Boolean]) {
			arguments {
				nonNullBooleanListField(nonNullBooleanListArg: $booleanList)
			}
		})"_graphql;

	auto errors =
		service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), std::size_t { 2 })
		<< "1 incompatible variable + 1 incompatible argument";
	ASSERT_GE(errors.size(), std::size_t { 1 });
	EXPECT_EQ(
		R"js({"message":"Expected Non-Null variable type","locations":[{"line":3,"column":52}]})js",
		response::toJSON(std::move(errors[0])))
		<< "error should match";
}

TEST_F(ValidationExamplesCase, Example186)
{
	// https://spec.graphql.org/October2021/#example-0877c
	auto query = R"(query booleanArgQueryWithDefault($booleanArg: Boolean) {
			arguments {
				optionalNonNullBooleanArgField(optionalBooleanArg: $booleanArg)
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example187)
{
	// https://spec.graphql.org/October2021/#example-d24d9
	auto query = R"(query booleanArgQueryWithDefault($booleanArg: Boolean = true) {
			arguments {
				nonNullBooleanArgField(nonNullBooleanArg: $booleanArg)
			}
		})"_graphql;

	auto errors = _service->validate(query);

	ASSERT_TRUE(errors.empty());
}
