// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "ValidationMock.h"

#include "graphqlservice/JSONResponse.h"

#include <chrono>

using namespace graphql;

using namespace std::literals;

class ValidationExamplesCase : public ::testing::Test
{
public:
	static void SetUpTestCase()
	{
		_service = std::make_shared<validation::Operations>(
			std::make_shared<validation::Query>(),
			std::make_shared<validation::Mutation>(),
			std::make_shared<validation::Subscription>());
	}

	static void TearDownTestCase()
	{
		_service.reset();
	}

protected:
	static std::shared_ptr<validation::Operations> _service;
};

std::shared_ptr<validation::Operations> ValidationExamplesCase::_service;

TEST_F(ValidationExamplesCase, CounterExample91)
{
	// http://spec.graphql.org/June2018/#example-12752
	auto query = R"(query getDogName {
			dog {
				name
				color
			}
		}

		extend type Dog {
			color: String
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), 2);
	EXPECT_EQ(R"js({"message":"Undefined field type: Dog name: color","locations":[{"line":4,"column":5}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Unexpected type definition","locations":[{"line":8,"column":3}]})js", response::toJSON(std::move(errors[1]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example92)
{
	// http://spec.graphql.org/June2018/#example-069e1
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample93)
{
	// http://spec.graphql.org/June2018/#example-5e409
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), 1);
	EXPECT_EQ(R"js({"message":"Duplicate operation name: getName","locations":[{"line":7,"column":3}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample94)
{
	// http://spec.graphql.org/June2018/#example-77c2e
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), 1);
	EXPECT_EQ(R"js({"message":"Duplicate operation name: dogOperation","locations":[{"line":7,"column":3}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example95)
{
	// http://spec.graphql.org/June2018/#example-be853
	auto query = R"({
			dog {
				name
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample96)
{
	// http://spec.graphql.org/June2018/#example-44b85
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), 1);
	EXPECT_EQ(R"js({"message":"Anonymous operation not alone","locations":[{"line":1,"column":1}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example97)
{
	// http://spec.graphql.org/June2018/#example-5bbc3
	auto query = R"(subscription sub {
			newMessage {
				body
				sender
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example98)
{
	// http://spec.graphql.org/June2018/#example-13061
	auto query = R"(subscription sub {
			...newMessageFields
		}

		fragment newMessageFields on Subscription {
			newMessage {
				body
				sender
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample99)
{
	// http://spec.graphql.org/June2018/#example-3997d
	auto query = R"(subscription sub {
			newMessage {
				body
				sender
			}
			disallowedSecondRootField
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), 1);
	EXPECT_EQ(R"js({"message":"Subscription with more than one root field name: sub","locations":[{"line":1,"column":1}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample100)
{
	// http://spec.graphql.org/June2018/#example-18466
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), 1);
	EXPECT_EQ(R"js({"message":"Subscription with more than one root field name: sub","locations":[{"line":1,"column":1}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample101)
{
	// http://spec.graphql.org/June2018/#example-2353b
	auto query = R"(subscription sub {
			newMessage {
				body
				sender
			}
			__typename
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), 1);
	EXPECT_EQ(R"js({"message":"Subscription with more than one root field name: sub","locations":[{"line":1,"column":1}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample102)
{
	// http://spec.graphql.org/June2018/#example-48706
	auto query = R"(fragment fieldNotDefined on Dog {
			meowVolume
		}

		fragment aliasedLyingFieldTargetNotDefined on Dog {
			barkVolume: kawVolume
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 4) << "2 undefined fields + 2 unused fragments";
	ASSERT_GE(errors.size(), size_t { 2 });
	EXPECT_EQ(R"js({"message":"Undefined field type: Dog name: meowVolume","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Undefined field type: Dog name: kawVolume","locations":[{"line":6,"column":4}]})js", response::toJSON(std::move(errors[1]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example103)
{
	// http://spec.graphql.org/June2018/#example-d34e0
	auto query = R"(fragment interfaceFieldSelection on Pet {
			name
		}

		query {
			dog {
				...interfaceFieldSelection
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample104)
{
	// http://spec.graphql.org/June2018/#example-db33b
	auto query = R"(fragment definedOnImplementorsButNotInterface on Pet {
			nickname
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 2) << "1 undefined field + 1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Undefined field type: Pet name: nickname","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example105)
{
	// http://spec.graphql.org/June2018/#example-245fa
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample106)
{
	// http://spec.graphql.org/June2018/#example-252ad
	auto query = R"(fragment directFieldSelectionOnUnion on CatOrDog {
			name
			barkVolume
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 3) << "2 undefined fields + 1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 2 });
	EXPECT_EQ(R"js({"message":"Field on union type: CatOrDog name: name","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Field on union type: CatOrDog name: barkVolume","locations":[{"line":3,"column":4}]})js", response::toJSON(std::move(errors[1]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example107)
{
	// http://spec.graphql.org/June2018/#example-4e10c
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample108)
{
	// http://spec.graphql.org/June2018/#example-a2230
	auto query = R"(fragment conflictingBecauseAlias on Dog {
			name: nickname
			name
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 2) << "1 conflicting field + 1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Conflicting field type: Dog name: name","locations":[{"line":3,"column":4}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example109)
{
	// http://spec.graphql.org/June2018/#example-b6369
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

		query q2 {
			dog {
				...mergeIdenticalFieldsWithIdenticalValues
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample110)
{
	// http://spec.graphql.org/June2018/#example-00fbf
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 9) << "4 conflicting fields + 1 missing argument + 4 unused fragments";
	ASSERT_GE(errors.size(), size_t { 4 });
	EXPECT_EQ(R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":3,"column":4}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":8,"column":4}]})js", response::toJSON(std::move(errors[1]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":13,"column":4}]})js", response::toJSON(std::move(errors[2]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":18,"column":4}]})js", response::toJSON(std::move(errors[3]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example111)
{
	// http://spec.graphql.org/June2018/#example-a8406
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample112)
{
	// http://spec.graphql.org/June2018/#example-54e3d
	auto query = R"(fragment conflictingDifferingResponses on Pet {
			... on Dog {
				someValue: nickname
			}
			... on Cat {
				someValue: meowVolume
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 2) << "1 conflicting field + 1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Conflicting field type: Cat name: meowVolume","locations":[{"line":6,"column":5}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example113)
{
	// http://spec.graphql.org/June2018/#example-e23c5
	auto query = R"(fragment scalarSelection on Dog {
			barkVolume
		}

		query {
			dog {
				...scalarSelection
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample114)
{
	// http://spec.graphql.org/June2018/#example-13b69
	auto query = R"(fragment scalarSelectionsNotAllowedOnInt on Dog {
			barkVolume {
				sinceWhen
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 2) << "1 invalid field + 1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Field on scalar type: Int name: sinceWhen","locations":[{"line":3,"column":5}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example115)
{
	// http://spec.graphql.org/June2018/#example-9bada
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample116)
{
	// http://spec.graphql.org/June2018/#example-d68ee
	auto query = R"(query directQueryOnObjectWithoutSubFields {
			human
		}

		query directQueryOnInterfaceWithoutSubFields {
			pet
		}

		query directQueryOnUnionWithoutSubFields {
			catOrDog
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_EQ(errors.size(), 3) << "3 invalid fields";
	EXPECT_EQ(R"js({"message":"Missing fields on non-scalar type: Human","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Missing fields on non-scalar type: Pet","locations":[{"line":6,"column":4}]})js", response::toJSON(std::move(errors[1]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Missing fields on non-scalar type: CatOrDog","locations":[{"line":10,"column":4}]})js", response::toJSON(std::move(errors[2]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example117)
{
	// http://spec.graphql.org/June2018/#example-760cb
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample118)
{
	// http://spec.graphql.org/June2018/#example-d5639
	auto query = R"(fragment invalidArgName on Dog {
			doesKnowCommand(command: CLEAN_UP_HOUSE)
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 3) << "1 undefined argument + 1 missing argument + 1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Undefined argument type: Dog field: doesKnowCommand name: command","locations":[{"line":2,"column":20}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample119)
{
	// http://spec.graphql.org/June2018/#example-4feee
	auto query = R"(fragment invalidArgName on Dog {
			isHousetrained(atOtherHomes: true) @include(unless: false)
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 3) << "1 undefined argument + 1 missing argument + 1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Undefined argument directive: include name: unless","locations":[{"line":2,"column":48}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example120)
{
	// http://spec.graphql.org/June2018/#example-1891c
	auto query = R"(query {
			arguments {
				multipleReqs(x: 1, y: 2)
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example121)
{
	// http://spec.graphql.org/June2018/#example-18fab
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example122)
{
	// http://spec.graphql.org/June2018/#example-503bd
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example123)
{
	// http://spec.graphql.org/June2018/#example-1f1d2
	auto query = R"(fragment goodBooleanArgDefault on Arguments {
			booleanArgField
		}

		query {
			arguments {
				...goodBooleanArgDefault
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample124)
{
	// http://spec.graphql.org/June2018/#example-f12a1
	auto query = R"(fragment missingRequiredArg on Arguments {
			nonNullBooleanArgField
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 2) << "1 missing argument + 1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Missing argument type: Arguments field: nonNullBooleanArgField name: nonNullBooleanArg","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample125)
{
	// http://spec.graphql.org/June2018/#example-0bc81
	auto query = R"(fragment missingRequiredArg on Arguments {
			nonNullBooleanArgField(nonNullBooleanArg: null)
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 2) << "1 missing argument + 1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Required non-null argument type: Arguments field: nonNullBooleanArgField name: nonNullBooleanArg","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example126)
{
	// http://spec.graphql.org/June2018/#example-3703b
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample127)
{
	// http://spec.graphql.org/June2018/#example-2c3e3
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 1) << "1 duplicate fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Duplicate fragment name: fragmentOne","locations":[{"line":11,"column":3}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example128)
{
	// http://spec.graphql.org/June2018/#example-1b2da
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample129)
{
	// http://spec.graphql.org/June2018/#example-463f6
	auto query = R"(fragment notOnExistingType on NotInSchema {
			name
		}

		fragment inlineNotExistingType on Dog {
			... on NotInSchema {
				name
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 4) << "2 not existing types + 2 unused fragments";
	ASSERT_GE(errors.size(), size_t { 2 });
	EXPECT_EQ(R"js({"message":"Undefined target type on fragment definition: notOnExistingType name: NotInSchema","locations":[{"line":1,"column":28}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Undefined target type on inline fragment name: NotInSchema","locations":[{"line":6,"column":8}]})js", response::toJSON(std::move(errors[1]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example130)
{
	// http://spec.graphql.org/June2018/#example-3c8d4
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample131)
{
	// http://spec.graphql.org/June2018/#example-4d5e5
	auto query = R"(fragment fragOnScalar on Int {
			something
		}

		fragment inlineFragOnScalar on Dog {
			... on Boolean {
				somethingElse
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 4) << "2 not existing types + 2 unused fragments";
	ASSERT_GE(errors.size(), size_t { 2 });
	EXPECT_EQ(R"js({"message":"Scalar target type on fragment definition: fragOnScalar name: Int","locations":[{"line":1,"column":23}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Scalar target type on inline fragment name: Boolean","locations":[{"line":6,"column":8}]})js", response::toJSON(std::move(errors[1]))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample132)
{
	// http://spec.graphql.org/June2018/#example-9e1e3
	auto query = R"(fragment nameFragment on Dog { # unused
			name
		}

		{
			dog {
				name
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 1) << "1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Unused fragment definition name: nameFragment","locations":[{"line":1,"column":1}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample133)
{
	// http://spec.graphql.org/June2018/#example-28421
	auto query = R"({
			dog {
				...undefinedFragment
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 2) << "1 undefined fragment + 1 missing field";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Undefined fragment spread name: undefinedFragment","locations":[{"line":3,"column":8}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample134)
{
	// http://spec.graphql.org/June2018/#example-9ceb4
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 2) << "2 cyclic fragments";
	ASSERT_GE(errors.size(), size_t { 2 });
	EXPECT_EQ(R"js({"message":"Cyclic fragment spread name: nameFragment","locations":[{"line":14,"column":7}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Cyclic fragment spread name: barkVolumeFragment","locations":[{"line":9,"column":7}]})js", response::toJSON(std::move(errors[1]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example135)
{
	// http://spec.graphql.org/June2018/#example-08734
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample136)
{
	// http://spec.graphql.org/June2018/#example-6bbad
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 2) << "2 cyclic fragments";
	ASSERT_GE(errors.size(), size_t { 2 });
	EXPECT_EQ(R"js({"message":"Cyclic fragment spread name: dogFragment","locations":[{"line":19,"column":8}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
	EXPECT_EQ(R"js({"message":"Cyclic fragment spread name: ownerFragment","locations":[{"line":11,"column":8}]})js", response::toJSON(std::move(errors[1]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example137)
{
	// http://spec.graphql.org/June2018/#example-0fc38
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample138)
{
	// http://spec.graphql.org/June2018/#example-4d411
	auto query = R"(fragment catInDogFragmentInvalid on Dog {
			... on Cat {
				meowVolume
			}
		})"_graphql;

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	EXPECT_EQ(errors.size(), 2) << "1 incompatible type + 1 unused fragment";
	ASSERT_GE(errors.size(), size_t { 1 });
	EXPECT_EQ(R"js({"message":"Incompatible target type on inline fragment name: Cat","locations":[{"line":2,"column":8}]})js", response::toJSON(std::move(errors[0]))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example139)
{
	// http://spec.graphql.org/June2018/#example-2c8d0
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example140)
{
	// http://spec.graphql.org/June2018/#example-41843
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

	auto errors = service::buildErrorValues(_service->validate(query)).release<response::ListType>();

	ASSERT_TRUE(errors.empty());
}
