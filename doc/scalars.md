# Custom Scalars

GraphQL lets a schema declare [custom scalar](https://spec.graphql.org/October2021/#sec-Scalars.Custom-Scalars)
types, e.g.:

```graphql
scalar DateTime
scalar BigInt
```

The wire representation of a scalar is always a `graphql::response::Value` (the JSON-like
discriminated union described in [Query Responses](./responses.md)). By default `schemagen`
generates the resolvers and argument accessors for every custom scalar in terms of
`response::Value`, so you receive and return the value as generic JSON-like data:

```cpp
service::AwaitableScalar<response::Value> getWhen(service::FieldParams&& params) const override;
```

That is convenient, but it forces your implementation to store and manipulate the value as a
`response::Value` even when a dedicated C++ type would be much more natural or efficient (for
example a `std::chrono::system_clock::time_point` for `DateTime`, or a 64-bit integer for a
`BigInt` that does not fit in the 32-bit `response::IntType`).

## The `@cppType` directive

You can map a custom scalar onto any C++ type using the `@cppType` directive on the `scalar`
declaration:

```graphql
scalar BigInt @cppType(
	name: "bigint::BigInt"
	header: "BigIntScalar.h"
)
```

* `name` (required) is the fully-qualified C++ type that `schemagen` uses everywhere the scalar
  appears in the generated code, instead of `response::Value`. It may be qualified with any
  namespace (`bigint::BigInt`, `my::project::BigInt`, ...).
* `header` (optional) is added as an `#include` at the top of the generated schema header so the
  custom type and its conversion specializations are visible to the generated code.

With the directive above, the generated resolver interface changes to use your type directly:

```cpp
service::AwaitableScalar<bigint::BigInt> getEcho(
	service::FieldParams&& params, bigint::BigInt&& valueArg) const override;
```

The scalar is still declared to clients exactly as `scalar BigInt` in introspection; `@cppType`
is a code-generation hint only and never appears in the served schema.

## Providing the conversions

Because `schemagen` no longer knows how to translate the scalar to and from the `response::Value`
wire representation, you provide that translation by specializing three members of the
`graphql::service` templates. These are the type-erased boundary between your C++ type and the
JSON-like data on the wire:

```cpp
namespace graphql::service {

// Deserialize an argument or input value into your custom type.
template <>
bigint::BigInt Argument<bigint::BigInt>::convert(const response::Value& value);

// Serialize a resolver result of your custom type back into a response::Value.
template <>
AwaitableResolver Result<bigint::BigInt>::convert(
	AwaitableScalar<bigint::BigInt> result, ResolverParams&& params);

// Validate a cached response::Value (returned directly by a resolver) for this scalar.
template <>
void Result<bigint::BigInt>::validateScalar(const response::Value& value);

} // namespace graphql::service
```

`Result<T>::convert` is usually implemented with the `ModifiedResult<T>::resolve` helper, passing a
callback that turns a `T&&` into a `response::Value`:

```cpp
template <>
AwaitableResolver Result<bigint::BigInt>::convert(
	AwaitableScalar<bigint::BigInt> result, ResolverParams&& params)
{
	return ModifiedResult<bigint::BigInt>::resolve(std::move(result),
		std::move(params),
		[](bigint::BigInt&& value, const ResolverParams&) {
			return response::Value { std::to_string(value.value) };
		});
}
```

Declare these specializations in the header named by `@cppType`'s `header` argument so they are
visible to the generated code, and define them in a matching `.cpp` which is compiled and linked
into the schema library.

## Nullable scalar arguments

Nullable arguments of built-in scalar types are passed as `std::optional<T>`. So that your custom
scalar behaves the same way (instead of being treated like a generated `INPUT_OBJECT` type, which
would be wrapped in `std::unique_ptr<T>`), specialize the `CustomScalarArgument` trait and inherit
from `std::true_type`:

```cpp
namespace graphql::service {

template <>
struct CustomScalarArgument<bigint::BigInt> : std::true_type
{
};

} // namespace graphql::service
```

## Complete example

See [`samples/scalar`](../samples/scalar) for a complete, buildable example of a `BigInt` custom
scalar whose C++ storage is a `std::int64_t` (serialized on the wire as a JSON `String` so it keeps
full precision), along with the [`test/ScalarTests.cpp`](../test/ScalarTests.cpp) unit tests which
exercise serializing, deserializing, nullable arguments, and error handling through live queries.
