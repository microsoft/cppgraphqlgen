# Query Responses

## Value Types

As the comment in
[GraphQLResponse.h](../include/graphqlservice/GraphQLResponse.h) says, GraphQL
responses are not technically JSON-specific, although that is probably the most
common way of representing them. These are the primitive types that may be
represented in GraphQL, as of the
[October 2021 spec](https://spec.graphql.org/October2021/#sec-Serialization-Format):

```c++
enum class [[nodiscard("unnecessary conversion")]] Type : std::uint8_t {
	Map,	   // JSON Object
	List,	   // JSON Array
	String,	   // JSON String
	Null,	   // JSON null
	Boolean,   // JSON true or false
	Int,	   // JSON Number
	Float,	   // JSON Number
	EnumValue, // JSON String
	ID,		   // JSON String
	Scalar,	   // JSON any type
};
```

## Common Accessors

Anywhere that a GraphQL result, a scalar type, or a GraphQL value literal is
used, it's represented in `cppgraphqlgen` using an instance of
`graphql::response::Value`. These can be constructed with any of the types in
the `graphql::response::Type` enum, and depending on the type with which they
are initialized, different accessors will be enabled.

Every type implements specializations for some subset of `get()` which does
not allocate any memory, `set(...)` which takes an r-value, and `release()`
which transfers ownership along with any extra allocations to the caller.
Which of these methods are supported and what C++ types they use are
determined by the `ValueTypeTraits<ValueType>` template and its
specializations.

## Map and List

`Map` and `List` types enable collection methods like `reserve(size_t)`,
`size()`, and `emplace_back(...)`. `Map` additionally implements `begin()`
and `end()` for range-based for loops and `find(const std::string&)` and
`operator[](const std::string&)` for key-based lookups. `List` has an
`operator[](size_t)` for index-based instead of key-based lookups.