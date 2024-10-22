// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/JSONResponse.h"

#define RAPIDJSON_NAMESPACE graphql::rapidjson
#include <rapidjson/rapidjson.h>

#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <limits>
#include <stdexcept>
#include <vector>

namespace graphql::response {

class StreamWriter : public std::enable_shared_from_this<StreamWriter>
{
public:
	StreamWriter(rapidjson::StringBuffer& buffer)
		: _writer { buffer }
	{
	}

	void add_value(std::shared_ptr<const Value>&& value)
	{
		auto writer = std::make_shared<ValueVisitor>(shared_from_this());

		ValueTokenStream(Value { *value }).visit(writer);
	}

	void reserve(std::size_t /* count */)
	{
	}

	void start_object()
	{
		_writer.StartObject();
	}

	void add_member(std::string&& key)
	{
		_writer.Key(key.c_str());
	}

	void end_object()
	{
		_writer.EndObject();
	}

	void start_array()
	{
		_writer.StartArray();
	}

	void end_array()
	{
		_writer.EndArray();
	}

	void add_null()
	{
		_writer.Null();
	}

	void add_string(std::string&& value)
	{
		_writer.String(value.c_str());
	}

	void add_enum(std::string&& value)
	{
		add_string(std::move(value));
	}

	void add_id(IdType&& value)
	{
		add_string(value.release<std::string>());
	}

	void add_bool(bool value)
	{
		_writer.Bool(value);
	}

	void add_int(int value)
	{
		_writer.Int(value);
	}

	void add_float(double value)
	{
		_writer.Double(value);
	}

	void complete()
	{
	}

private:
	rapidjson::Writer<rapidjson::StringBuffer> _writer;
};

std::string toJSON(Value&& response)
{
	rapidjson::StringBuffer buffer;
	auto writer = std::make_shared<ValueVisitor>(std::make_shared<StreamWriter>(buffer));

	ValueTokenStream(std::move(response)).visit(writer);

	return buffer.GetString();
}

struct ResponseHandler : rapidjson::BaseReaderHandler<rapidjson::UTF8<>, ResponseHandler>
{
	ResponseHandler()
	{
		// Start with a single null value.
		_responseStack.push_back({});
	}

	Value getResponse()
	{
		auto response = std::move(_responseStack.back());

		_responseStack.pop_back();

		return response;
	}

	bool Null()
	{
		setValue(Value());
		return true;
	}

	bool Bool(bool b)
	{
		setValue(Value(b));
		return true;
	}

	bool Int(int i)
	{
		// https://spec.graphql.org/October2021/#sec-Int
		static_assert(sizeof(i) == 4, "GraphQL only supports 32-bit signed integers");
		auto value = Value(Type::Int);

		value.set<IntType>(std::move(i));
		setValue(std::move(value));
		return true;
	}

	bool Uint(unsigned int i)
	{
		if (i > static_cast<unsigned int>(std::numeric_limits<int>::max()))
		{
			// https://spec.graphql.org/October2021/#sec-Int
			return Double(static_cast<double>(i));
		}
		return Int(static_cast<int>(i));
	}

	bool Int64(int64_t i)
	{
		// https://spec.graphql.org/October2021/#sec-Int
		return Double(static_cast<double>(i));
	}

	bool Uint64(uint64_t i)
	{
		// https://spec.graphql.org/October2021/#sec-Int
		return Double(static_cast<double>(i));
	}

	bool Double(double d)
	{
		auto value = Value(Type::Float);

		value.set<FloatType>(std::move(d));
		setValue(std::move(value));
		return true;
	}

	bool String(const Ch* str, rapidjson::SizeType /*length*/, bool /*copy*/)
	{
		setValue(Value(std::string(str)).from_json());
		return true;
	}

	bool StartObject()
	{
		_responseStack.push_back(Value(Type::Map));
		return true;
	}

	bool Key(const Ch* str, rapidjson::SizeType /*length*/, bool /*copy*/)
	{
		_keyStack.push_back(str);
		return true;
	}

	bool EndObject(rapidjson::SizeType /*count*/)
	{
		setValue(getResponse());
		return true;
	}

	bool StartArray()
	{
		_responseStack.push_back(Value(Type::List));
		return true;
	}

	bool EndArray(rapidjson::SizeType /*count*/)
	{
		setValue(getResponse());
		return true;
	}

private:
	void setValue(Value&& value)
	{
		switch (_responseStack.back().type())
		{
			case Type::Map:
				_responseStack.back().emplace_back(std::move(_keyStack.back()), std::move(value));
				_keyStack.pop_back();
				break;

			case Type::List:
				_responseStack.back().emplace_back(std::move(value));
				break;

			default:
				_responseStack.back() = std::move(value);
				break;
		}
	}

	std::vector<std::string> _keyStack;
	std::vector<Value> _responseStack;
};

Value parseJSON(const std::string& json)
{
	ResponseHandler handler;
	rapidjson::Reader reader;
	rapidjson::StringStream ss(json.c_str());

	reader.Parse(ss, handler);

	return handler.getResponse();
}

} // namespace graphql::response
