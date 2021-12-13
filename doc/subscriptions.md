# Subscriptions

Subscriptions in GraphQL are asynchronous and event driven. Typically you need
to have a callback installed, e.g. over a WebSocket connection, which receives
the response to a subscribed query anytime it is updated. Since this library
is protocol agnostic, it doesn't include the delivery mechanism. But it does
provide a way to register callbacks when adding a subscription, and you can
define trigger conditions when delivering an update to selectively dispatch
the subscriptions to those listeners.

## Adding/Removing a Listener

Subscriptions are created or removed by calling the `Request::subscribe`
and `Request::unsubscribe` methods in [GraphQLService.h](../include/graphqlservice/GraphQLService.h):
```cpp
GRAPHQLSERVICE_EXPORT SubscriptionKey subscribe(
	RequestSubscribeParams&& params, SubscriptionCallback&& callback);
GRAPHQLSERVICE_EXPORT AwaitableSubscribe subscribe(
	std::launch launch, RequestSubscribeParams&& params, SubscriptionCallback&& callback);

GRAPHQLSERVICE_EXPORT void unsubscribe(SubscriptionKey key);
GRAPHQLSERVICE_EXPORT AwaitableUnsubscribe unsubscribe(std::launch launch, SubscriptionKey key);
```
You need to fill in a `RequestSubscribeParams` struct with the [parsed](./parsing.md)
query and any other relevant operation parameters:
```cpp
// You can still sub-class RequestState and use that in the state parameter to Request::subscribe
// to add your own state to the service callbacks that you receive while executing the subscription
// query.
struct RequestSubscribeParams
{
	std::shared_ptr<RequestState> state;
	peg::ast query;
	std::string operationName;
	response::Value variables;
};
```
The `SubscriptionCallback` signature is:
```cpp
// Subscription callbacks receive the response::Value representing the result of evaluating the
// SelectionSet against the payload.
using SubscriptionCallback = std::function<void(response::Value&&)>;
```

## `ResolverContext::NotifySubscribe` and `ResolverContext::NotifyUnsubscribe`

If you use the async version of `subscribe` and `unsubscribe` which take a
`std::launch` parameter, and you provide a default instance of the
`Subscription` object to the `Request`/`Operations` constructor, you will get
additional callbacks with the `ResolverContext::NotifySubscribe` and
`ResolverContext::NotifyUnsubscribe` values for the
`FieldParams::resolverContext` member. These are passed by the async
`subscribe` and `unsubscribe` calls to the default subscription object, and
they provide an opportunity to acquire or release resources that are required
to implement the subscription.

You can provide separate implementations of the `Subscription` object as the
default in the `Operations` constructor and as the payload of a specific
`deliver` call. Whether you override the `Subscription` object or not, the
event payload for a `deliver` call will be resolved with
`ResolverContext::Subscription`.

## Delivering Subscription Updates

If you pass an empty `std::shared_ptr<Object>` for the `subscriptionObject`
parameter, `deliver` will fall back to resolving the query against the default
`Subscription` object passed to the `Request`/`Operations` constructor. If both
`Subscription` object parameters are empty, `deliver` will throw an exception.

There are currently five `Request::deliver` overrides you can choose from when
sending updates to any subscribed listeners. The first one is the simplest,
it will evaluate each subscribed query against the `subscriptionObject`
parameter (which should match the `Subscription` type in the `schema`). It will
unconditionally invoke every subscribed `SubscriptionCallback` callback with
the response to its query:
```cpp
GRAPHQLSERVICE_EXPORT void deliver(
	const SubscriptionName& name, const std::shared_ptr<Object>& subscriptionObject) const;
```

The second override adds argument filtering. It will look at the field 
arguments in the subscription `query`, and if all of the required parameters
in the `arguments` parameter are present and are an exact match it will
dispatch the callback to that subscription:
```cpp
GRAPHQLSERVICE_EXPORT void deliver(const SubscriptionName& name,
	const SubscriptionArguments& arguments,
	const std::shared_ptr<Object>& subscriptionObject) const;
```

The third override adds directive filtering. It will look at both the field
arguments and the directives with their own arguments in the subscription
`query`, and if all of them match it will dispatch the callback to that
subscription:
```cpp
GRAPHQLSERVICE_EXPORT void deliver(const SubscriptionName& name,
	const SubscriptionArguments& arguments, const SubscriptionArguments& directives,
	const std::shared_ptr<Object>& subscriptionObject) const;
```

The last two overrides let you customize the the way that the required
arguments and directives are matched. Instead of an exact match or making all
of the arguments required, it will dispatch the callback if the `apply`
function parameters return true for every required field and directive in the
subscription `query`.
```cpp
GRAPHQLSERVICE_EXPORT void deliver(const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const std::shared_ptr<Object>& subscriptionObject) const;
GRAPHQLSERVICE_EXPORT void deliver(const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const SubscriptionFilterCallback& applyDirectives,
	const std::shared_ptr<Object>& subscriptionObject) const;
```

By default, `deliver` invokes all of the `SubscriptionCallback` listeners with
`std::future` payloads which are resolved on-demand but synchronously. You can
also use an override of `Request::resolve` which lets you substitute the
`std::launch::async` option to begin executing the queries and invoke the
callbacks on multiple threads in parallel:
```cpp
GRAPHQLSERVICE_EXPORT AwaitableDeliver deliver(std::launch launch, const SubscriptionName& name,
	std::shared_ptr<Object> subscriptionObject) const;
GRAPHQLSERVICE_EXPORT AwaitableDeliver deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionArguments& arguments, std::shared_ptr<Object> subscriptionObject) const;
GRAPHQLSERVICE_EXPORT AwaitableDeliver deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionArguments& arguments, const SubscriptionArguments& directives,
	std::shared_ptr<Object> subscriptionObject) const;
GRAPHQLSERVICE_EXPORT AwaitableDeliver deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	std::shared_ptr<Object> subscriptionObject) const;
GRAPHQLSERVICE_EXPORT AwaitableDeliver deliver(std::launch launch, const SubscriptionName& name,
	const SubscriptionFilterCallback& applyArguments,
	const SubscriptionFilterCallback& applyDirectives,
	std::shared_ptr<Object> subscriptionObject) const;
```

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