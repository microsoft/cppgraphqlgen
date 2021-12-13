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

class StringWriter
{
public:
	StringWriter(rapidjson::StringBuffer& buffer)
		: _writer { buffer }
	{
	}

	void start_object()
	{
		_writer.StartObject();
	}

	void add_member(const std::string& key)
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

	void end_arrary()
	{
		_writer.EndArray();
	}

	void write_null()
	{
		_writer.Null();
	}

	void write_string(const std::string& value)
	{
		_writer.String(value.c_str());
	}

	void write_bool(bool value)
	{
		_writer.Bool(value);
	}

	void write_int(int value)
	{
		_writer.Int(value);
	}

	void write_float(double value)
	{
		_writer.Double(value);
	}

private:
	rapidjson::Writer<rapidjson::StringBuffer> _writer;
};

std::string toJSON(Value&& response)
{
	rapidjson::StringBuffer buffer;
	Writer writer { std::make_unique<StringWriter>(buffer) };

	writer.write(std::move(response));
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
		// http://spec.graphql.org/June2018/#sec-Int
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
			// http://spec.graphql.org/June2018/#sec-Int
			throw std::overflow_error("GraphQL only supports 32-bit signed integers");
		}
		return Int(static_cast<int>(i));
	}

	bool Int64(int64_t /*i*/)
	{
		// http://spec.graphql.org/June2018/#sec-Int
		throw std::overflow_error("GraphQL only supports 32-bit signed integers");
	}

	bool Uint64(uint64_t /*i*/)
	{
		// http://spec.graphql.org/June2018/#sec-Int
		throw std::overflow_error("GraphQL only supports 32-bit signed integers");
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
