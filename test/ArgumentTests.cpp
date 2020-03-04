// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include "TodaySchema.h"

#include "graphqlservice/JSONResponse.h"

using namespace graphql;


TEST(ArgumentsCase, ListArgumentStrings)
{
	auto parsed = response::parseJSON(R"js({"value":[
		"string1",
		"string2",
		"string3"
	]})js");
	std::vector<std::string> actual;

	try
	{
		actual = service::StringArgument::require<service::TypeModifier::List>("value", parsed);
	}
	catch (service::schema_exception & ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(3, actual.size()) << "should get 3 entries";
	EXPECT_EQ("string1", actual[0]) << "entry should match";
	EXPECT_EQ("string2", actual[1]) << "entry should match";
	EXPECT_EQ("string3", actual[2]) << "entry should match";
}

TEST(ArgumentsCase, ListArgumentStringsNonNullable)
{
	auto parsed = response::parseJSON(R"js({"value":[
		"string1",
		null,
		"string2",
		"string3"
	]})js");
	bool caughtException = false;
	std::string exceptionWhat;

	try
	{
		auto actual = service::StringArgument::require<service::TypeModifier::List>("value", parsed);
	}
	catch (service::schema_exception & ex)
	{
		exceptionWhat = response::toJSON(ex.getErrors());
		caughtException = true;
	}

	ASSERT_TRUE(caughtException);
	EXPECT_EQ(R"js([{"message":"Invalid argument: value error: not a string"}])js", exceptionWhat) << "exception should match";
}

TEST(ArgumentsCase, ListArgumentStringsNullable)
{
	auto parsed = response::parseJSON(R"js({"value":[
		"string1",
		"string2",
		null,
		"string3"
	]})js");
	std::vector<std::optional<std::string>> actual;

	try
	{
		actual = service::StringArgument::require<
			service::TypeModifier::List,
			service::TypeModifier::Nullable
		>("value", parsed);
	}
	catch (service::schema_exception & ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(4, actual.size()) << "should get 4 entries";
	ASSERT_TRUE(actual[0].has_value()) << "should not be null";
	EXPECT_EQ("string1", *actual[0]) << "entry should match";
	ASSERT_TRUE(actual[1].has_value()) << "should not be null";
	EXPECT_EQ("string2", *actual[1]) << "entry should match";
	EXPECT_FALSE(actual[2].has_value()) << "should be null";
	ASSERT_TRUE(actual[3].has_value()) << "should not be null";
	EXPECT_EQ("string3", *actual[3]) << "entry should match";
}

TEST(ArgumentsCase, ListArgumentListArgumentStrings)
{
	auto parsed = response::parseJSON(R"js({"value":[
		["list1string1", "list1string2"],
		["list2string1", "list2string2"]
	]})js");
	std::vector<std::vector<std::string>> actual;

	try
	{
		actual = service::StringArgument::require<
			service::TypeModifier::List,
			service::TypeModifier::List
		>("value", parsed);
	}
	catch (service::schema_exception & ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(2, actual.size()) << "should get 2 entries";
	ASSERT_EQ(2, actual[0].size()) << "should get 2 entries";
	EXPECT_EQ("list1string1", actual[0][0]) << "entry should match";
	EXPECT_EQ("list1string2", actual[0][1]) << "entry should match";
	ASSERT_EQ(2, actual[1].size()) << "should get 2 entries";
	EXPECT_EQ("list2string1", actual[1][0]) << "entry should match";
	EXPECT_EQ("list2string2", actual[1][1]) << "entry should match";
}

TEST(ArgumentsCase, ListArgumentNullableListArgumentStrings)
{
	auto parsed = response::parseJSON(R"js({"value":[
		null,
		["list2string1", "list2string2"]
	]})js");
	std::vector<std::optional<std::vector<std::string>>> actual;

	try
	{
		actual = service::StringArgument::require<
			service::TypeModifier::List,
			service::TypeModifier::Nullable,
			service::TypeModifier::List
		>("value", parsed);
	}
	catch (service::schema_exception & ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(2, actual.size()) << "should get 2 entries";
	EXPECT_FALSE(actual[0].has_value()) << "should be null";
	ASSERT_EQ(2, actual[1]->size()) << "should get 2 entries";
	EXPECT_EQ("list2string1", (*actual[1])[0]) << "entry should match";
	EXPECT_EQ("list2string2", (*actual[1])[1]) << "entry should match";
}

TEST(ArgumentsCase, TaskStateEnum)
{
	response::Value response(response::Type::Map);
	response::Value status(response::Type::EnumValue);
	status.set<response::StringType>("Started");
	response.emplace_back("status", std::move(status));
	today::TaskState actual = static_cast<today::TaskState>(-1);

	try
	{
		actual = service::ModifiedArgument<today::TaskState>::require("status", response);
	}
	catch (service::schema_exception & ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	EXPECT_EQ(today::TaskState::Started, actual) << "should parse the enum";
}

TEST(ArgumentsCase, TaskStateEnumFromString)
{
	response::Value response(response::Type::Map);
	response::Value status("Started");
	response.emplace_back("status", std::move(status));
	today::TaskState actual = static_cast<today::TaskState>(-1);
	bool caughtException = false;
	std::string exceptionWhat;

	try
	{
		actual = service::ModifiedArgument<today::TaskState>::require("status", response);
	}
	catch (service::schema_exception & ex)
	{
		caughtException = true;
		exceptionWhat = response::toJSON(ex.getErrors());
	}

	EXPECT_NE(today::TaskState::Started, actual) << "should not parse the enum from a known string value";
	ASSERT_TRUE(caughtException);
	EXPECT_EQ(R"js([{"message":"Invalid argument: status error: not a valid TaskState value"}])js", exceptionWhat) << "exception should match";
}

TEST(ArgumentsCase, TaskStateEnumFromJSONString)
{
	response::Value response(response::Type::Map);
	response::Value status("Started");
	response.emplace_back("status", status.from_json());
	today::TaskState actual = static_cast<today::TaskState>(-1);


	try
	{
		actual = service::ModifiedArgument<today::TaskState>::require("status", response);
	}
	catch (service::schema_exception & ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	EXPECT_EQ(today::TaskState::Started, actual) << "should parse the enum";
}

TEST(ArgumentsCase, ScalarArgumentMap)
{
	response::Value response(response::Type::Map);
	response.emplace_back("scalar", response::parseJSON(R"js({ "foo": "bar" })js"));
	response::Value actual;
	response::MapType values;

	try
	{
		actual = service::ModifiedArgument<response::Value>::require("scalar", response);
	}
	catch (service::schema_exception & ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(response::Type::Map, actual.type()) << "should parse the object";
	values = actual.release<response::MapType>();
	ASSERT_EQ(1, values.size()) << "should have a single key/value";
	ASSERT_EQ("foo", values.front().first) << "should match the key";
	ASSERT_EQ("bar", values.front().second.get<response::StringType>()) << "should match the value";
}

TEST(ArgumentsCase, ScalarArgumentList)
{
	response::Value response(response::Type::Map);
	response.emplace_back("scalar", response::parseJSON(R"js([ "foo", "bar" ])js"));
	response::Value actual;
	response::ListType values;

	try
	{
		actual = service::ModifiedArgument<response::Value>::require("scalar", response);
	}
	catch (service::schema_exception & ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(response::Type::List, actual.type()) << "should parse the array";
	values = actual.release<response::ListType>();
	ASSERT_EQ(2, values.size()) << "should have 2 values";
	ASSERT_EQ("foo", values.front().get<response::StringType>()) << "should match the value";
	ASSERT_EQ("bar", values.back().get<response::StringType>()) << "should match the value";
}

TEST(ArgumentsCase, ScalarArgumentNull)
{
	response::Value response(response::Type::Map);
	response.emplace_back("scalar", {});
	response::Value actual;

	try
	{
		actual = service::ModifiedArgument<response::Value>::require("scalar", response);
	}
	catch (service::schema_exception & ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(response::Type::Null, actual.type()) << "should match null";
}

TEST(ArgumentsCase, ScalarArgumentString)
{
	response::Value response(response::Type::Map);
	response.emplace_back("scalar", response::Value("foobar"));
	response::Value actual;

	try
	{
		actual = service::ModifiedArgument<response::Value>::require("scalar", response);
	}
	catch (service::schema_exception & ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(response::Type::String, actual.type()) << "should parse the object";
	ASSERT_EQ("foobar", actual.get<response::StringType>()) << "should match the value";
}
