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
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(size_t { 3 }, actual.size()) << "should get 3 entries";
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
		auto actual =
			service::StringArgument::require<service::TypeModifier::List>("value", parsed);
	}
	catch (service::schema_exception& ex)
	{
		exceptionWhat = response::toJSON(ex.getErrors());
		caughtException = true;
	}

	ASSERT_TRUE(caughtException);
	EXPECT_EQ(R"js([{"message":"Invalid argument: value error: not a string"}])js", exceptionWhat)
		<< "exception should match";
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
		actual = service::StringArgument::require<service::TypeModifier::List,
			service::TypeModifier::Nullable>("value", parsed);
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(size_t { 4 }, actual.size()) << "should get 4 entries";
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
		actual = service::StringArgument::require<service::TypeModifier::List,
			service::TypeModifier::List>("value", parsed);
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(size_t { 2 }, actual.size()) << "should get 2 entries";
	ASSERT_EQ(size_t { 2 }, actual[0].size()) << "should get 2 entries";
	EXPECT_EQ("list1string1", actual[0][0]) << "entry should match";
	EXPECT_EQ("list1string2", actual[0][1]) << "entry should match";
	ASSERT_EQ(size_t { 2 }, actual[1].size()) << "should get 2 entries";
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
		actual = service::StringArgument::require<service::TypeModifier::List,
			service::TypeModifier::Nullable,
			service::TypeModifier::List>("value", parsed);
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(size_t { 2 }, actual.size()) << "should get 2 entries";
	EXPECT_FALSE(actual[0].has_value()) << "should be null";
	ASSERT_EQ(size_t { 2 }, actual[1]->size()) << "should get 2 entries";
	EXPECT_EQ("list2string1", (*actual[1])[0]) << "entry should match";
	EXPECT_EQ("list2string2", (*actual[1])[1]) << "entry should match";
}

TEST(ArgumentsCase, TaskStateEnum)
{
	response::Value response(response::Type::Map);
	response::Value status(response::Type::EnumValue);
	status.set<std::string>("Started");
	response.emplace_back("status", std::move(status));
	today::TaskState actual = static_cast<today::TaskState>(-1);

	try
	{
		actual = service::ModifiedArgument<today::TaskState>::require("status", response);
	}
	catch (service::schema_exception& ex)
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
	catch (service::schema_exception& ex)
	{
		caughtException = true;
		exceptionWhat = response::toJSON(ex.getErrors());
	}

	EXPECT_NE(today::TaskState::Started, actual)
		<< "should not parse the enum from a known string value";
	ASSERT_TRUE(caughtException);
	EXPECT_EQ(R"js([{"message":"Invalid argument: status error: not a valid TaskState value"}])js",
		exceptionWhat)
		<< "exception should match";
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
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	EXPECT_EQ(today::TaskState::Started, actual) << "should parse the enum";
}

TEST(ArgumentsCase, TaskStateEnumConstexpr)
{
	using namespace std::literals;

	constexpr auto actual =
		internal::sorted_map_lookup<internal::shorter_or_less>(today::getTaskStateValues(),
			"Started"sv);

	static_assert(today::TaskState::Started == *actual, "can also perform lookups at compile time");

	ASSERT_TRUE(actual) << "should find a value";
	EXPECT_TRUE(today::TaskState::Started == *actual) << "should parse the enum";
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
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(response::Type::Map, actual.type()) << "should parse the object";
	values = actual.release<response::MapType>();
	ASSERT_EQ(size_t { 1 }, values.size()) << "should have a single key/value";
	ASSERT_EQ("foo", values.front().first) << "should match the key";
	ASSERT_EQ("bar", values.front().second.get<std::string>()) << "should match the value";
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
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(response::Type::List, actual.type()) << "should parse the array";
	values = actual.release<response::ListType>();
	ASSERT_EQ(size_t { 2 }, values.size()) << "should have 2 values";
	ASSERT_EQ("foo", values.front().get<std::string>()) << "should match the value";
	ASSERT_EQ("bar", values.back().get<std::string>()) << "should match the value";
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
	catch (service::schema_exception& ex)
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
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_EQ(response::Type::String, actual.type()) << "should parse the object";
	ASSERT_EQ("foobar", actual.get<std::string>()) << "should match the value";
}

TEST(ArgumentsCase, FindArgumentNoTemplateArguments)
{
	response::Value response(response::Type::Map);
	response.emplace_back("scalar", response::Value("foobar"));
	std::pair<response::Value, bool> actual { {}, false };

	try
	{
		actual = service::ModifiedArgument<response::Value>::find("scalar", response);
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_TRUE(actual.second) << "should find the argument";
	ASSERT_EQ(response::Type::String, actual.first.type()) << "should parse the object";
	ASSERT_EQ("foobar", actual.first.get<std::string>()) << "should match the value";
}

TEST(ArgumentsCase, FindArgumentEmptyTemplateArgs)
{
	response::Value response(response::Type::Map);
	response.emplace_back("scalar", response::Value("foobar"));
	std::pair<response::Value, bool> actual { {}, false };

	try
	{
		actual = service::ModifiedArgument<response::Value>::find<>("scalar", response);
	}
	catch (service::schema_exception& ex)
	{
		FAIL() << response::toJSON(ex.getErrors());
	}

	ASSERT_TRUE(actual.second) << "should find the argument";
	ASSERT_EQ(response::Type::String, actual.first.type()) << "should parse the object";
	ASSERT_EQ("foobar", actual.first.get<std::string>()) << "should match the value";
}

struct FakeInput
{
};

enum class FakeEnum
{
	Foo,
	Bar,
};

TEST(ArgumentsCase, ScalarArgumentClass)
{
	constexpr bool boolType = service::ScalarArgumentClass<bool>;
	constexpr bool stringClass = service::ScalarArgumentClass<std::string>;
	constexpr bool idTypeClass = service::ScalarArgumentClass<response::IdType>;
	constexpr bool valueClass = service::ScalarArgumentClass<response::Value>;
	constexpr bool fakeStruct = service::ScalarArgumentClass<FakeInput>;
	constexpr bool fakeEnum = service::ScalarArgumentClass<FakeEnum>;

	static_assert(!boolType, "ScalarArgumentClass<bool> is false");
	static_assert(stringClass, "ScalarArgumentClass<std::string> is true");
	static_assert(idTypeClass, "ScalarArgumentClass<response::IdType> is true");
	static_assert(valueClass, "ScalarArgumentClass<response::Value> is true");
	static_assert(!fakeStruct, "ScalarArgumentClass<FakeInput> is false");
	static_assert(!fakeEnum, "ScalarArgumentClass<FakeEnum> is false");
	ASSERT_FALSE(boolType) << "ScalarArgumentClass<bool> is false";
	ASSERT_TRUE(stringClass) << "ScalarArgumentClass<std::string> is true";
	ASSERT_TRUE(idTypeClass) << "ScalarArgumentClass<response::IdType> is true";
	ASSERT_TRUE(valueClass) << "ScalarArgumentClass<response::Value> is true";
	ASSERT_FALSE(fakeStruct) << "ScalarArgumentClass<FakeInput> is false";
	ASSERT_FALSE(fakeEnum) << "ScalarArgumentClass<FakeEnum> is false";
}

TEST(ArgumentsCase, InputArgumentClass)
{
	constexpr bool boolType = service::InputArgumentClass<bool>;
	constexpr bool stringClass = service::InputArgumentClass<std::string>;
	constexpr bool idTypeClass = service::InputArgumentClass<response::IdType>;
	constexpr bool valueClass = service::InputArgumentClass<response::Value>;
	constexpr bool fakeStruct = service::InputArgumentClass<FakeInput>;
	constexpr bool fakeEnum = service::InputArgumentClass<FakeEnum>;

	static_assert(!boolType, "InputArgumentClass<bool> is false");
	static_assert(!stringClass, "InputArgumentClass<std::string> is false");
	static_assert(!idTypeClass, "InputArgumentClass<response::IdType> is false");
	static_assert(!valueClass, "InputArgumentClass<response::Value> is false");
	static_assert(fakeStruct, "InputArgumentClass<FakeInput> is true");
	static_assert(!fakeEnum, "InputArgumentClass<FakeEnum> is false");
	ASSERT_FALSE(boolType) << "InputArgumentClass<bool> is false";
	ASSERT_FALSE(stringClass) << "InputArgumentClass<std::string> is false";
	ASSERT_FALSE(idTypeClass) << "InputArgumentClass<response::IdType> is false";
	ASSERT_FALSE(valueClass) << "InputArgumentClass<response::Value> is false";
	ASSERT_TRUE(fakeStruct) << "InputArgumentClass<FakeInput> is true";
	ASSERT_FALSE(fakeEnum) << "InputArgumentClass<FakeEnum> is false";
}

TEST(ArgumentsCase, OnlyNoneModifiers)
{
	constexpr bool emptyModifiers = service::OnlyNoneModifiers<>;
	constexpr bool threeNone = service::OnlyNoneModifiers<service::TypeModifier::None,
		service::TypeModifier::None,
		service::TypeModifier::None>;
	constexpr bool firtNullable = service::OnlyNoneModifiers<service::TypeModifier::Nullable,
		service::TypeModifier::None,
		service::TypeModifier::None>;
	constexpr bool middleList = service::OnlyNoneModifiers<service::TypeModifier::None,
		service::TypeModifier::List,
		service::TypeModifier::None>;

	static_assert(emptyModifiers, "OnlyNoneModifiers<> is true");
	static_assert(threeNone, "OnlyNoneModifiers<None, None, None> is true");
	static_assert(!firtNullable, "OnlyNoneModifiers<Nullable, None, None> is false");
	static_assert(!middleList, "OnlyNoneModifiers<None, List, None> is false");
	ASSERT_TRUE(emptyModifiers) << "OnlyNoneModifiers<> is true";
	ASSERT_TRUE(threeNone) << "OnlyNoneModifiers<None, None, None> is true";
	ASSERT_FALSE(firtNullable) << "OnlyNoneModifiers<Nullable, None, None> is false";
	ASSERT_FALSE(middleList) << "OnlyNoneModifiers<None, List, None> is false";
}

TEST(ArgumentsCase, InputArgumentUniquePtr)
{
	constexpr bool boolType = service::InputArgumentUniquePtr<bool>;
	constexpr bool stringClass = service::InputArgumentUniquePtr<std::string>;
	constexpr bool idTypeClass = service::InputArgumentUniquePtr<response::IdType>;
	constexpr bool valueClass = service::InputArgumentUniquePtr<response::Value>;
	constexpr bool fakeStruct = service::InputArgumentUniquePtr<FakeInput>;
	constexpr bool fakeEnum = service::InputArgumentUniquePtr<FakeEnum>;

	static_assert(!boolType, "InputArgumentUniquePtr<bool> is false");
	static_assert(!stringClass, "InputArgumentUniquePtr<std::string> is false");
	static_assert(!idTypeClass, "InputArgumentUniquePtr<response::IdType> is false");
	static_assert(!valueClass, "InputArgumentUniquePtr<response::Value> is false");
	static_assert(fakeStruct, "InputArgumentUniquePtr<FakeInput> is true");
	static_assert(!fakeEnum, "InputArgumentUniquePtr<FakeEnum> is false");
	ASSERT_FALSE(boolType) << "InputArgumentUniquePtr<bool> is false";
	ASSERT_FALSE(stringClass) << "InputArgumentUniquePtr<std::string> is false";
	ASSERT_FALSE(idTypeClass) << "InputArgumentUniquePtr<response::IdType> is false";
	ASSERT_FALSE(valueClass) << "InputArgumentUniquePtr<response::Value> is false";
	ASSERT_TRUE(fakeStruct) << "InputArgumentUniquePtr<FakeInput> is true";
	ASSERT_FALSE(fakeEnum) << "InputArgumentUniquePtr<FakeEnum> is false";
}

template <typename Type>
using NullableType = typename service::ModifiedArgument<Type>::template ArgumentTraits<Type,
	service::TypeModifier::Nullable>::type;

TEST(ArgumentsCase, ArgumentTraitsUniquePtr)
{
	constexpr bool boolType = std::is_same_v<NullableType<bool>, std::optional<bool>>;
	constexpr bool stringClass =
		std::is_same_v<NullableType<std::string>, std::optional<std::string>>;
	constexpr bool idTypeClass =
		std::is_same_v<NullableType<response::IdType>, std::optional<response::IdType>>;
	constexpr bool valueClass =
		std::is_same_v<NullableType<response::Value>, std::optional<response::Value>>;
	constexpr bool fakeStruct = std::is_same_v<NullableType<FakeInput>, std::unique_ptr<FakeInput>>;
	constexpr bool fakeEnum = std::is_same_v<NullableType<FakeEnum>, std::optional<FakeEnum>>;

	static_assert(boolType, "NullableType<bool> is std::optional<bool>");
	static_assert(stringClass, "NullableType<std::string> is std::optional<std::string>");
	static_assert(idTypeClass, "NullableType<response::IdType> is std::optional<response::IdType>");
	static_assert(valueClass, "NullableType<response::Value> is std::optional<response::Value>");
	static_assert(fakeStruct, "NullableType<FakeInput> is std::unique_ptr<FakeInput>");
	static_assert(fakeEnum, "NullableType<FakeEnum> is std::optional<FakeEnum>");
	ASSERT_TRUE(boolType) << "NullableType<bool> is std::optional<bool>";
	ASSERT_TRUE(stringClass) << "NullableType<std::string> is std::optional<std::string>";
	ASSERT_TRUE(idTypeClass) << "NullableType<response::IdType> is std::optional<response::IdType>";
	ASSERT_TRUE(valueClass) << "NullableType<response::Value> is std::optional<response::Value>";
	ASSERT_TRUE(fakeStruct) << "NullableType<FakeInput> is std::unique_ptr<FakeInput>";
	ASSERT_TRUE(fakeEnum) << "NullableType<FakeEnum> is std::optional<FakeEnum>";
}
