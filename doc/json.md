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

std::string toJSON(Value&& response);

Value parseJSON(const std::string& json);

} /* namespace graphql::response */
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