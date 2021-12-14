# Field Resolvers

GraphQL schemas define types with named fields, and each of those fields may
take arguments which alter the behavior of that field. You can think of
`fields` much like methods on an object instance in OOP (Object Oriented
Programming). Each field is implemented using a `resolver`, which may
recursively invoke additional `resolvers` for fields of the resulting objects,
e.g.:
```graphql
query {
    foo(id: "bar") {
        baz
    }
}
```

This query would invoke the `resolver` for the `foo field` on the top-level
`query` object, passing it the string `"bar"` as the `id` argument. Then it
would invoke the `resolver` for the `baz` field on the result of the `foo
field resolver`.

## Top-level Resolvers

The `schema` type in GraphQL defines the types for top-level operation types.
By convention, these are often named after the operation type, although you
could give them different names:
```graphql
schema {
    query: Query
    mutation: Mutation
    subscription: Subscription
}
```

Executing a query or mutation starts by calling `Request::resolve` from [GraphQLService.h](../include/graphqlservice/GraphQLService.h):
```cpp
GRAPHQLSERVICE_EXPORT response::AwaitableValue resolve(
	const std::shared_ptr<RequestState>& state, peg::ast& query,
	const std::string& operationName, response::Value&& variables) const;
```
By default, the `std::future` results are resolved on-demand but synchronously.
You can also use an override of `Request::resolve` which lets you substitute
the `std::launch::async` option to begin executing the query on multiple
threads in parallel:
```cpp
GRAPHQLSERVICE_EXPORT response::AwaitableValue resolve(std::launch launch,
	const std::shared_ptr<RequestState>& state, peg::ast& query,
	const std::string& operationName, response::Value&& variables) const;
```

### `graphql::service::Request` and `graphql::<schema>::Operations`

Anywhere in the documentation where it mentions `graphql::service::Request`
methods, the concrete type will actually be `graphql::<schema>::Operations`.
This `class` is defined by `schemagen` and inherits from
`graphql::service::Request`. It links the top-level objects for the custom
schema to the `resolve` methods on its base class. See
`graphql::today::Operations` in [TodaySchema.h](../samples/separate/TodaySchema.h)
for an example.

## Generated Service Schema

The `schemagen` tool generates C++ types in the `graphql::<schema>::object`
namespace with `resolveField` methods for each `field` which parse the
arguments from the `query` and automatically dispatch the call to a `getField`
virtual method to retrieve the `field` result. On `object` types, it will also
recursively call the `resolvers` for each of the `fields` in the nested
`SelectionSet`. See for example the generated
`graphql::today::object::Appointment` object from the `today` sample in
[AppointmentObject.h](../samples/separate/AppointmentObject.h).
```cpp
service::AwaitableResolver resolveId(service::ResolverParams&& params);
```
In this example, the `resolveId` method invokes `getId`:
```cpp
virtual service::AwaitableScalar<response::IdType> getId(service::FieldParams&& params) const override;
```

There are a couple of interesting quirks in this example:
1. The `Appointment object` implements and inherits from the `Node interface`,
which already declared `getId` as a pure-virtual method. That's what the
`override` keyword refers to.
2. This schema was generated with default stub implementations (without the
`schemagen --no-stubs` parameter) which speed up initial development with NYI
(Not Yet Implemented) stubs. With that parameter, there would be no
declaration of `Appointment::getId` since it would inherit a pure-virtual
declaration and the implementer would need to define an override on the
concrete implementation of `graphql::today::object::Appointment`. The NYI stub
will throw a `std::runtime_error`, which the `resolver` converts into an entry
in the `response errors` collection:
```cpp
throw std::runtime_error(R"ex(Appointment::getId is not implemented)ex");
```

Although the `id field` does not take any arguments according to the sample
[schema](../samples/schema.today.graphql), this example also shows how
every `getField` method takes a `graphql::service::FieldParams` struct as
its first parameter. There are more details on this in the [fieldparams.md](./fieldparams.md)
document.