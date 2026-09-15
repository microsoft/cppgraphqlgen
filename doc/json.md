# Converting to/from JSON

## `graphqljson` Library Target

Converting between `graphql::response::Value` in [GraphQLResponse.h](../include/GraphQLResponse.h)
and JSON strings is done in an optional library target called `graphqljson`.

## Default RapidJSON Implementation

The included implementation uses [RapidJSON](https://github.com/Tencent/rapidjson)
release 1.1.0, but if you don't need JSON support, or you want to integrate
a different JSON library, you can set `GRAPHQL_USE_RAPIDJSON=OFF` in your
CMake configuration.

## Using Custom JSON Libraries

If you want to use a different JSON library, you can add implementations of
the functions in [JSONResponse.h](../include/JSONResponse.h):
```cpp
namespace graphql::response {

JSONRESPONSE_EXPORT std::string toJSON(Value&& response);

JSONRESPONSE_EXPORT Value parseJSON(const std::string& json);

} // namespace graphql::response
```

You will also need to update the [CMakeLists.txt](../src/CMakeLists.txt) file
in the [../src](../src) directory to add your own implementation. See the
comment in that file for more information:
```cmake
# RapidJSON is the only option for JSON serialization used in this project, but if you want
# to use another JSON library you can implement an alternate version of the functions in
# JSONResponse.cpp to serialize to and from GraphQLResponse and build graphqljson from that.
# You will also need to define how to build the graphqljson library target with your
# implementation, and you should set BUILD_GRAPHQLJSON so that the test dependencies know
# about your version of graphqljson.
option(GRAPHQL_USE_RAPIDJSON "Use RapidJSON for JSON serialization." ON)
```

## response::Writer

You can plug-in a type-erased streaming `response::Writer` to serialize a `response::Value`
to some other output mechanism, without building a single string buffer for the entire
document in memory. For example, you might use this to write directly to a buffered IPC pipe
or network connection:
```cpp
class [[nodiscard("unnecessary construction")]] Writer final
{
private:
	struct Concept
	{
		virtual ~Concept() = default;

		virtual void start_object() const = 0;
		virtual void add_member(const std::string& key) const = 0;
		virtual void end_object() const = 0;

		virtual void start_array() const = 0;
		virtual void end_array() const = 0;

		virtual void write_null() const = 0;
		virtual void write_string(const std::string& value) const = 0;
		virtual void write_bool(bool value) const = 0;
		virtual void write_int(int value) const = 0;
		virtual void write_float(double value) const = 0;
	};
...

public:
	template <class T>
	Writer(std::unique_ptr<T> writer)
		: _concept { std::static_pointer_cast<const Concept>(
			std::make_shared<Model<T>>(std::move(writer))) }
	{
	}

	GRAPHQLRESPONSE_EXPORT void write(Value value) const;
};
```

Internally, this is what `graphqljson` uses to implement `response::toJSON` with RapidJSON.
It wraps a `rapidjson::Writer` in `response::Writer` and then writes into a
`rapidjson::StringBuffer` through that.
