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
GRAPHQLSERVICE_EXPORT response::AwaitableValue resolve(RequestResolveParams params) const;
```

The `RequestResolveParams` struct is defined in the same header:
```cpp
struct RequestResolveParams
{
	// Required query information.
	peg::ast& query;
	std::string_view operationName {};
	response::Value variables { response::Type::Map };

	// Optional async execution awaitable.
	await_async launch;

	// Optional sub-class of RequestState which will be passed to each resolver and field accessor.
	std::shared_ptr<RequestState> state;
};
```

The only parameter which cannot be default initialized is `query`. 

The `service::await_async` launch policy is described in [awaitable.md](./awaitable.md).
By default, the resolvers will run on the same thread synchronously.

The `response::AwaitableValue` return type is a type alias in [GraphQLResponse.h](../include/graphqlservice/GraphQLResponse.h):
```cpp
using AwaitableValue = internal::Awaitable<Value>;
```
The `internal::Awaitable<T>` template is described in [awaitable.md](./awaitable.md).

### `graphql::service::Request` and `graphql::<schema>::Operations`

Anywhere in the documentation where it mentions `graphql::service::Request`
methods, the concrete type will actually be `graphql::<schema>::Operations`.
This `class` is defined by `schemagen` and inherits from
`graphql::service::Request`. It links the top-level objects for the custom
schema to the `resolve` methods on its base class. See
`graphql::today::Operations` in [TodaySchema.h](../samples/today/schema/TodaySchema.h)
for an example.

## Generated Service Schema

The `schemagen` tool generates type-erased C++ types in the `graphql::<schema>::object`
namespace with `resolveField` methods for each `field` which parse the arguments from
the `query` and automatically dispatch the call to a `getField` method on the
implementation type to retrieve the `field` result. On `object` types, it will also
recursively call the `resolvers` for each of the `fields` in the nested `SelectionSet`.
See for example the generated `graphql::today::object::Appointment` object from the `today`
sample in [AppointmentObject.cpp](../samples/today/schema/AppointmentObject.cpp):
```cpp
service::AwaitableResolver Appointment::resolveId(service::ResolverParams&& params) const
{
	std::unique_lock resolverLock(_resolverMutex);
	auto directives = std::move(params.fieldDirectives);
	auto result = _pimpl->getId(service::FieldParams(service::SelectionSetParams{ params }, std::move(directives)));
	resolverLock.unlock();

	return service::ModifiedResult<response::IdType>::convert(std::move(result), std::move(params));
}
```
In this example, the `resolveId` method invokes `Concept::getId(service::FieldParams&&)`,
which is implemented by `Model<T>::getId(service::FieldParams&&)`:
```cpp
service::AwaitableScalar<response::IdType> getId(service::FieldParams&& params) const final
{
	if constexpr (methods::AppointmentHas::getIdWithParams<T>)
	{
		return { _pimpl->getId(std::move(params)) };
	}
	else if constexpr (methods::AppointmentHas::getId<T>)
	{
		return { _pimpl->getId() };
	}
	else
	{
		throw std::runtime_error(R"ex(Appointment::getId is not implemented)ex");
	}
}
```

There are a couple of interesting points in this example:
1. The `methods::AppointmentHas::getIdWithParams<T>` and
`methods::AppointmentHas::getIdWith<T>` concepts are automatically generated at the top of
[AppointmentObject.h](../samples/today/schema/AppointmentObject.h). The implementation
of the virtual method from the `object::Appointment::Concept` interface uses
`if constexpr (...)` to conditionally compile just one of the 3 method bodies, depending
on whether or not `T` matches those concepts:
```cpp
namespace methods::AppointmentHas {

template <class TImpl>
concept getIdWithParams = requires (TImpl impl, service::FieldParams params) 
{
	{ service::AwaitableScalar<response::IdType> { impl.getId(std::move(params)) } };
};

template <class TImpl>
concept getId = requires (TImpl impl) 
{
	{ service::AwaitableScalar<response::IdType> { impl.getId() } };
};

...

} // namespace methods::AppointmentHas
```
2. This schema was generated with default stub implementations (using the
`schemagen --stubs` parameter) which speeds up initial development with NYI
(Not Yet Implemented) stubs. If the implementation type `T` does not match either
concept, it will still implement this method on `object::Appointment::Model<T>`, but
it will always throw a `std::runtime_error` indicating that the method was not implemented.
Compared to the type-erased objects generated for the [learn](../samples/learn/), such as
[HumanObject.h](../samples/learn/schema/HumanObject.h), without `schemagen --stubs` it
adds a `static_assert` instead, so it will trigger a compile-time error if you do not
implement all of the field getters:
```cpp
service::AwaitableScalar<std::string> getId(service::FieldParams&& params) const final
{
	if constexpr (methods::HumanHas::getIdWithParams<T>)
	{
		return { _pimpl->getId(std::move(params)) };
	}
	else
	{
		static_assert(methods::HumanHas::getId<T>, R"msg(Human::getId is not implemented)msg");
		return { _pimpl->getId() };
	}
}
```

Although the `id field` does not take any arguments according to the sample
[schema](../samples/today/schema.today.graphql), this example also shows how every `getField`
method on the `object::Appointment::Concept` takes a `graphql::service::FieldParams` struct
as its first parameter from the resolver. If the implementation type can take that parameter
and matches the concept, the `object::Appointment::Model<T>` `getField` method will pass
it through to the implementation type. If it does not, it will silently ignore that
parameter and invoke the implementation type `getField` method without it. There are more
details on this in the [fieldparams.md](./fieldparams.md) document.