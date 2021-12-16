# Subscriptions

Subscriptions in GraphQL are asynchronous and event driven. Typically you need
to have a callback installed, e.g. over a WebSocket connection, which receives
the response to a subscribed query anytime it is updated. Since this library
is protocol agnostic, it doesn't include the delivery mechanism. But it does
provide a way to register callbacks when adding a subscription, and you can
define trigger conditions when delivering an update to selectively dispatch
the subscriptions to those listeners.

## Adding a Listener

Subscriptions are created by calling the `Request::subscribe` method in
[GraphQLService.h](../include/graphqlservice/GraphQLService.h):
```cpp
GRAPHQLSERVICE_EXPORT AwaitableSubscribe subscribe(RequestSubscribeParams params);
```

You need to fill in a `RequestSubscribeParams` struct with the subscription event
callback, the [parsed](./parsing.md) `query` and any other relevant operation parameters:
```cpp
struct RequestSubscribeParams
{
	// Callback which receives the event data.
	SubscriptionCallback callback;

	// Required query information.
	peg::ast query;
	std::string operationName {};
	response::Value variables { response::Type::Map };

	// Optional async execution awaitable.
	await_async launch;

	// Optional sub-class of RequestState which will be passed to each resolver and field accessor.
	std::shared_ptr<RequestState> state;
};
```

The `SubscriptionCallback` signature is:
```cpp
// Subscription callbacks receive the response::Value representing the result of evaluating the
// SelectionSet against the payload.
using SubscriptionCallback = std::function<void(response::Value)>;
```

The `service::await_async` launch policy is described in [awaitable.md](./awaitable.md).
By default, the resolvers will run on the same thread synchronously.

The `std::shared_ptr<RequestState>` state is described in [fieldparams.md](./fieldparams.md).

The `AwaitableSubscribe` return type is a type alias in
[GraphQLResponse.h](../include/graphqlservice/GraphQLResponse.h):
```cpp
using AwaitableSubscribe = internal::Awaitable<SubscriptionKey>;
```
The `internal::Awaitable<T>` template is described in [awaitable.md](./awaitable.md).

## Removing a Listener

Subscriptions are removed by calling the `Request::unsubscribe` method in
[GraphQLService.h](../include/graphqlservice/GraphQLService.h):
```cpp
GRAPHQLSERVICE_EXPORT AwaitableUnsubscribe unsubscribe(RequestUnsubscribeParams params);
```

You need to fill in a `RequestUnsubscribeParams` struct with the `SubscriptionKey`
returned by `Request::subscribe` in `AwaitableSubscribe`:
```cpp
struct RequestUnsubscribeParams
{
	// Key returned by a previous call to subscribe.
	SubscriptionKey key;

	// Optional async execution awaitable.
	await_async launch;
};
```

The `service::await_async` launch policy is described in [awaitable.md](./awaitable.md).
By default, the resolvers will run on the same thread synchronously.

## `ResolverContext::NotifySubscribe` and `ResolverContext::NotifyUnsubscribe`

If you provide a default instance of the `Subscription` object to the `Request`/
`Operations` constructor, you will get additional callbacks with the
`ResolverContext::NotifySubscribe` and `ResolverContext::NotifyUnsubscribe` values
for the `FieldParams::resolverContext` member. These are passed by the
`subscribe` and `unsubscribe` calls to the default subscription object, and
they provide an opportunity to acquire or release resources that are required
to implement the subscription.

You can provide separate implementations of the `Subscription` object as the
default in the `Operations` constructor and as the payload of a specific
`deliver` call. Whether you override the `Subscription` object or not, the
event payload for a `deliver` call will be resolved with
`ResolverContext::Subscription`.

## Delivering Subscription Updates

If you pass an empty `std::shared_ptr<object::Subscription>` for the
`subscriptionObject` parameter, `deliver` will fall back to resolving the query
against the default `Subscription` object passed to the `Request`/`Operations`
constructor. If both `Subscription` object parameters are empty, `deliver`
will throw an exception:
```cpp
GRAPHQLSERVICE_EXPORT AwaitableDeliver deliver(RequestDeliverParams params) const;
```

The `Request::deliver` method determines which subscriptions should receive
an event based on several factors, which makes the `RequestDeliverParams` struct
more complicated:
```cpp
struct RequestDeliverParams
{
	// Deliver to subscriptions on this field.
	std::string_view field;

	// Optional filter to control which subscriptions will receive the event. If not specified,
	// every subscription on this field will receive the event and evaluate their queries.
	RequestDeliverFilter filter;

	// Optional async execution awaitable.
	await_async launch;

	// Optional override for the default Subscription operation object.
	std::shared_ptr<Object> subscriptionObject;
};
```

First, the `Request::deliver` method selects only the subscriptions which are listening
to events for this field. Then, if you specify the `RequestDeliverFilter`, it filters the set
of subscriptions down to the ones which it matches:
```cpp
// Deliver to a specific subscription key, or apply custom criteria for the field name, arguments,
// and directives in the Subscription query.
using RequestDeliverFilter = std::optional<std::variant<SubscriptionKey, SubscriptionFilter>>;
```

The simplest filter is the `SubscriptionKey`. If you specify that, and the subscription is
listening to `field` events, `Request::deliver` will only deliver to that one subscription.

The `SubscriptionFilter struct` adds alternative filters for the field arguments and any field
directives which might have been specified in the subscription query. For each one, you can either
specify a callback which will test each argument or directive, or you can specify a set of required
arguments or directives which must all be present:
```cpp
using SubscriptionArguments = std::map<std::string_view, response::Value>;
using SubscriptionArgumentFilterCallback = std::function<bool(response::MapType::const_reference)>;
using SubscriptionDirectiveFilterCallback = std::function<bool(Directives::const_reference)>;

struct SubscriptionFilter
{
	// Optional field argument filter, which can either be a set of required arguments, or a
	// callback which returns true if the arguments match custom criteria.
	std::optional<std::variant<SubscriptionArguments, SubscriptionArgumentFilterCallback>>
		arguments;

	// Optional field directives filter, which can either be a set of required directives and
	// arguments, or a callback which returns true if the directives match custom criteria.
	std::optional<std::variant<Directives, SubscriptionDirectiveFilterCallback>> directives;
};
```

The `service::await_async` launch policy is described in [awaitable.md](./awaitable.md).
By default, the resolvers will run on the same thread synchronously.

The optional `std::shared_ptr<Object> subscriptionObject` parameter can override the
default Subscription operation object passed to the `Operations` constructor, or supply
one if no default instance was included.

## Handling Multiple Operation Types

Some service implementations (e.g. Apollo over HTTP) use a single pipe to
manage subscriptions and resolve queries or mutations. You can't necessarily
tell which operation type it is without parsing the query and searching for
a specific operation name, so it's hard to tell whether you should call
`resolve` or `subscribe` when the request is received that way. To help with
that, there's a public `Request::findOperationDefinition` method which returns
the operation type as a `std::string_view` along with a pointer to the AST node
for the selected operation in the parsed query:
```cpp
GRAPHQLSERVICE_EXPORT std::pair<std::string_view, const peg::ast_node*> findOperationDefinition(
	peg::ast& query, std::string_view operationName) const;
```