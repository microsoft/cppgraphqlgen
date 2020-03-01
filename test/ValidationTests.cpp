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

	ASSERT_GE(errors.size(), 1);
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
		}

		query {
			dog {
				...fieldNotDefined
				...aliasedLyingFieldTargetNotDefined
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 2);
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
		}

		query {
			dog {
				...definedOnImplementorsButNotInterface
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 1);
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
		}

		query {
			dog {
				...directFieldSelectionOnUnion
			}
		})"_graphql;

	auto errors = _service->validate(*ast.root);

	ASSERT_EQ(errors.size(), 2);
	response::Value error1(response::Type::Map);
	service::addErrorMessage(std::move(errors[0].message), error1);
	service::addErrorLocation(errors[0].location, error1);
	EXPECT_EQ(R"js({"message":"Undefined field type: CatOrDog name: name","locations":[{"line":2,"column":4}]})js", response::toJSON(std::move(error1))) << "error should match";
	response::Value error2(response::Type::Map);
	service::addErrorMessage(std::move(errors[1].message), error2);
	service::addErrorLocation(errors[1].location, error2);
	EXPECT_EQ(R"js({"message":"Undefined field type: CatOrDog name: barkVolume","locations":[{"line":3,"column":4}]})js", response::toJSON(std::move(error2))) << "error should match";
}
