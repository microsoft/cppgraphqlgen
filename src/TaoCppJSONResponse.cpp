// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/JSONResponse.h"

#include <tao/json.hpp>

#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

namespace graphql::response {

class StreamWriter
{
public:
	StreamWriter(std::ostream& stream)
		: _writer { stream }
	{
	}

	void start_object()
	{
		_scopeStack.push_back(Scope::Object);
		_writer.begin_object();
	}

	void add_member(const std::string& key)
	{
		_writer.key(key);
	}

	void end_object()
	{
		_writer.end_object();
		_scopeStack.pop_back();
		end_value();
	}

	void start_array()
	{
		_scopeStack.push_back(Scope::Object);
		_writer.begin_array();
	}

	void end_arrary()
	{
		_writer.end_array();
		_scopeStack.pop_back();
		end_value();
	}

	void write_null()
	{
		_writer.null();
		end_value();
	}

	void write_string(const std::string& value)
	{
		_writer.string(value);
		end_value();
	}

	void write_bool(bool value)
	{
		_writer.boolean(value);
		end_value();
	}

	void write_int(int value)
	{
		_writer.number(static_cast<std::int64_t>(value));
		end_value();
	}

	void write_float(double value)
	{
		_writer.number(value);
		end_value();
	}

private:
	enum class Scope
	{
		Array,
		Object,
	};

	void end_value()
	{
		if (_scopeStack.empty())
		{
			return;
		}

		switch (_scopeStack.back())
		{
			case Scope::Array:
				_writer.element();
				break;

			case Scope::Object:
				_writer.member();
				break;
		}
	}

	tao::json::events::to_stream _writer;
	std::vector<Scope> _scopeStack;
};

std::string toJSON(Value&& response)
{
	std::ostringstream stream;
	Writer writer { std::make_unique<StreamWriter>(stream) };
	writer.write(std::move(response));
	return stream.str();
}

struct ResponseHandler
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

	void null()
	{
		setValue(Value());
	}

	void boolean(bool b)
	{
		setValue(Value(b));
	}

	void number(double d)
	{
		auto value = Value(Type::Float);

		value.set<FloatType>(std::move(d));
		setValue(std::move(value));
	}

	void number(std::int64_t i)
	{
		if (i < std::numeric_limits<std::int32_t>::min()
			|| i > std::numeric_limits<std::int32_t>::max())
		{
			// https://spec.graphql.org/October2021/#sec-Int
			number(static_cast<double>(i));
		}
		else
		{
			static_assert(sizeof(std::int32_t) == sizeof(IntType),
				"GraphQL only supports 32-bit signed integers");
			auto value = Value(Type::Int);

			value.set<IntType>(static_cast<std::int32_t>(i));
			setValue(std::move(value));
		}
	}

	void number(std::uint64_t i)
	{
		if (i > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
		{
			// https://spec.graphql.org/October2021/#sec-Int
			number(static_cast<double>(i));
		}
		else
		{
			number(static_cast<std::int64_t>(i));
		}
	}

	void string(std::string&& str)
	{
		setValue(Value(std::move(str)).from_json());
	}

	void begin_array()
	{
		_responseStack.push_back(Value(Type::List));
	}

	void element()
	{
	}

	void end_array()
	{
		setValue(getResponse());
	}

	void begin_object()
	{
		_responseStack.push_back(Value(Type::Map));
	}

	void key(std::string&& str)
	{
		_keyStack.push_back(std::move(str));
	}

	void member()
	{
	}

	void end_object()
	{
		setValue(getResponse());
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
	tao::json::events::from_string(handler, json);

	return handler.getResponse();
}

} // namespace graphql::response
