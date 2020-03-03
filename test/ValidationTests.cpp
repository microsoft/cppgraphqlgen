// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "ValidationMock.h"

#include <graphqlservice/JSONResponse.h>

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
	auto ast = R"(query getDogName {
			dog {
				name
				color
			}
		}

		extend type Dog {
			color: String
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 2);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Undefined field type: Dog name: color","locations":[{"line":4,"column":5}]})js", response::toJSON(std::move(error1))) << "error should match";
	response::Value error2(response::Type::Map);
	service::addErrorMessage(std::move(errors[1].message), error2);
	service::addErrorLocation(errors[1].location, error2);
	EXPECT_EQ(R"js({"message":"Unexpected type definition","locations":[{"line":8,"column":3}]})js", response::toJSON(std::move(error2))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example92)
{
	// http://spec.graphql.org/June2018/#example-069e1
	auto ast = R"(query getDogName {
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

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample93)
{
	// http://spec.graphql.org/June2018/#example-5e409
	auto ast = R"(query getName {
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

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Duplicate operation name: getName","locations":[{"line":7,"column":3}]})js", response::toJSON(std::move(error1))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample94)
{
	// http://spec.graphql.org/June2018/#example-77c2e
	auto ast = R"(query dogOperation {
			dog {
				name
			}
		}

		mutation dogOperation {
			mutateDog {
				id
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Duplicate operation name: dogOperation","locations":[{"line":7,"column":3}]})js", response::toJSON(std::move(error1))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example95)
{
	// http://spec.graphql.org/June2018/#example-be853
	auto ast = R"({
			dog {
				name
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample96)
{
	// http://spec.graphql.org/June2018/#example-44b85
	auto ast = R"({
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

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Anonymous operation not alone","locations":[{"line":1,"column":1}]})js", response::toJSON(std::move(error1))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example97)
{
	// http://spec.graphql.org/June2018/#example-5bbc3
	auto ast = R"(subscription sub {
			newMessage {
				body
				sender
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, Example98)
{
	// http://spec.graphql.org/June2018/#example-13061
	auto ast = R"(subscription sub {
			...newMessageFields
		}

		fragment newMessageFields on Subscription {
			newMessage {
				body
				sender
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample99)
{
	// http://spec.graphql.org/June2018/#example-3997d
	auto ast = R"(subscription sub {
			newMessage {
				body
				sender
			}
			disallowedSecondRootField
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Subscription with more than one root field name: sub","locations":[{"line":1,"column":1}]})js", response::toJSON(std::move(error1))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample100)
{
	// http://spec.graphql.org/June2018/#example-18466
	auto ast = R"(subscription sub {
			...multipleSubscriptions
		}

		fragment multipleSubscriptions on Subscription {
			newMessage {
				body
				sender
			}
			disallowedSecondRootField
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Subscription with more than one root field name: sub","locations":[{"line":1,"column":1}]})js", response::toJSON(std::move(error1))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample101)
{
	// http://spec.graphql.org/June2018/#example-2353b
	auto ast = R"(subscription sub {
			newMessage {
				body
				sender
			}
			__typename
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Subscription with more than one root field name: sub","locations":[{"line":1,"column":1}]})js", response::toJSON(std::move(error1))) << "error should match";
}

TEST_F(ValidationExamplesCase, CounterExample102)
{
	// http://spec.graphql.org/June2018/#example-48706
	auto ast = R"(fragment fieldNotDefined on Dog {
			meowVolume
		}

		fragment aliasedLyingFieldTargetNotDefined on Dog {
			barkVolume: kawVolume
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	EXPECT_EQ(errors.size(), 4) << "2 undefined fields + 2 unused fragments";
	ASSERT_GE(errors.size(), 2);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Undefined field type: Dog name: meowVolume","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(error1))) << "error should match";
	response::Value error2(response::Type::Map);
	service::addErrorMessage(std::move(errors[1].message), error2);
	service::addErrorLocation(errors[1].location, error2);
	EXPECT_EQ(R"js({"message":"Undefined field type: Dog name: kawVolume","locations":[{"line":6,"column":4}]})js", response::toJSON(std::move(error2))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example103)
{
	// http://spec.graphql.org/June2018/#example-d34e0
	auto ast = R"(fragment interfaceFieldSelection on Pet {
			name
		}

		query {
			dog {
				...interfaceFieldSelection
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample104)
{
	// http://spec.graphql.org/June2018/#example-db33b
	auto ast = R"(fragment definedOnImplementorsButNotInterface on Pet {
			nickname
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	EXPECT_EQ(errors.size(), 2) << "1 undefined field + 1 unused fragment";
	ASSERT_GE(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Undefined field type: Pet name: nickname","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(error1))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example105)
{
	// http://spec.graphql.org/June2018/#example-245fa
	auto ast = R"(fragment inDirectFieldSelectionOnUnion on CatOrDog {
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

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample106)
{
	// http://spec.graphql.org/June2018/#example-252ad
	auto ast = R"(fragment directFieldSelectionOnUnion on CatOrDog {
			name
			barkVolume
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	EXPECT_EQ(errors.size(), 3) << "2 undefined fields + 1 unused fragment";
	ASSERT_GE(errors.size(), 2);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Field on union type: CatOrDog name: name","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(error1))) << "error should match";
	response::Value error2(response::Type::Map);
	service::addErrorMessage(std::move(errors[1].message), error2);
	service::addErrorLocation(errors[1].location, error2);
	EXPECT_EQ(R"js({"message":"Field on union type: CatOrDog name: barkVolume","locations":[{"line":3,"column":4}]})js", response::toJSON(std::move(error2))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example107)
{
	// http://spec.graphql.org/June2018/#example-4e10c
	auto ast = R"(fragment mergeIdenticalFields on Dog {
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

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample108)
{
	// http://spec.graphql.org/June2018/#example-a2230
	auto ast = R"(fragment conflictingBecauseAlias on Dog {
			name: nickname
			name
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	EXPECT_EQ(errors.size(), 2) << "1 conflicting field + 1 unused fragment";
	ASSERT_GE(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Conflicting field type: Dog name: name","locations":[{"line":3,"column":4}]})js", response::toJSON(std::move(error1))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example109)
{
	// http://spec.graphql.org/June2018/#example-b6369
	auto ast = R"(fragment mergeIdenticalFieldsWithIdenticalArgs on Dog {
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

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample110)
{
	// http://spec.graphql.org/June2018/#example-00fbf
	auto ast = R"(fragment conflictingArgsOnValues on Dog {
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

	auto errors = _service->validate(*ast.root);

	EXPECT_EQ(errors.size(), 8) << "4 conflicting fields + 4 unused fragments";
	ASSERT_GE(errors.size(), 4);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":3,"column":4}]})js", response::toJSON(std::move(error1))) << "error should match";
	response::Value error2(response::Type::Map);
	service::addErrorMessage(std::move(errors[1].message), error2);
	service::addErrorLocation(errors[1].location, error2);
	EXPECT_EQ(R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":8,"column":4}]})js", response::toJSON(std::move(error2))) << "error should match";
	response::Value error3(response::Type::Map);
	service::addErrorMessage(std::move(errors[2].message), error3);
	service::addErrorLocation(errors[2].location, error3);
	EXPECT_EQ(R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":13,"column":4}]})js", response::toJSON(std::move(error3))) << "error should match";
	response::Value error4(response::Type::Map);
	service::addErrorMessage(std::move(errors[3].message), error4);
	service::addErrorLocation(errors[3].location, error4);
	EXPECT_EQ(R"js({"message":"Conflicting field type: Dog name: doesKnowCommand","locations":[{"line":18,"column":4}]})js", response::toJSON(std::move(error4))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example111)
{
	// http://spec.graphql.org/June2018/#example-a8406
	auto ast = R"(fragment safeDifferingFields on Pet {
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

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample112)
{
	// http://spec.graphql.org/June2018/#example-54e3d
	auto ast = R"(fragment conflictingDifferingResponses on Pet {
			... on Dog {
				someValue: nickname
			}
			... on Cat {
				someValue: meowVolume
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	EXPECT_EQ(errors.size(), 2) << "1 conflicting field + 1 unused fragment";
	ASSERT_GE(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Conflicting field type: Cat name: meowVolume","locations":[{"line":6,"column":5}]})js", response::toJSON(std::move(error1))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example113)
{
	// http://spec.graphql.org/June2018/#example-e23c5
	auto ast = R"(fragment scalarSelection on Dog {
			barkVolume
		}

		query {
			dog {
				...scalarSelection
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample114)
{
	// http://spec.graphql.org/June2018/#example-13b69
	auto ast = R"(fragment scalarSelectionsNotAllowedOnInt on Dog {
			barkVolume {
				sinceWhen
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	EXPECT_EQ(errors.size(), 2) << "1 invalid field + 1 unused fragment";
	ASSERT_GE(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Field on scalar type: Int name: sinceWhen","locations":[{"line":3,"column":5}]})js", response::toJSON(std::move(error1))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example115)
{
	// http://spec.graphql.org/June2018/#example-9bada
	auto ast = R"(query {
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

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample116)
{
	// http://spec.graphql.org/June2018/#example-d68ee
	auto ast = R"(query directQueryOnObjectWithoutSubFields {
			human
		}

		query directQueryOnInterfaceWithoutSubFields {
			pet
		}

		query directQueryOnUnionWithoutSubFields {
			catOrDog
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 3) << "3 invalid fields";
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Missing fields on non-scalar type: Human","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(error1))) << "error should match";
	response::Value error2(response::Type::Map);
	service::addErrorMessage(std::move(errors[1].message), error2);
	service::addErrorLocation(errors[1].location, error2);
	EXPECT_EQ(R"js({"message":"Missing fields on non-scalar type: Pet","locations":[{"line":6,"column":4}]})js", response::toJSON(std::move(error2))) << "error should match";
	response::Value error3(response::Type::Map);
	service::addErrorMessage(std::move(errors[2].message), error3);
	service::addErrorLocation(errors[2].location, error3);
	EXPECT_EQ(R"js({"message":"Missing fields on non-scalar type: CatOrDog","locations":[{"line":10,"column":4}]})js", response::toJSON(std::move(error3))) << "error should match";
}

TEST_F(ValidationExamplesCase, Example117)
{
	// http://spec.graphql.org/June2018/#example-760cb
	auto ast = R"(fragment argOnRequiredArg on Dog {
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

	auto errors = _service->validate(*ast.root);

	ASSERT_TRUE(errors.empty());
}

TEST_F(ValidationExamplesCase, CounterExample118)
{
	// http://spec.graphql.org/June2018/#example-d5639
	auto ast = R"(fragment invalidArgName on Dog {
			doesKnowCommand(command: CLEAN_UP_HOUSE)
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	EXPECT_EQ(errors.size(), 2) << "1 undefined argument + 1 unused fragment";
	ASSERT_GE(errors.size(), 1);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Undefined argument type: Dog field: doesKnowCommand argument: command","locations":[{"line":2,"column":20}]})js", response::toJSON(std::move(error1))) << "error should match";
}
