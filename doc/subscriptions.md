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
and `Request::unsubscribe` methods in [GraphQLService.h](../include/GraphQLService.h):
```cpp
SubscriptionKey subscribe(SubscriptionParams&& params, SubscriptionCallback&& callback);
void unsubscribe(SubscriptionKey key);
```
You need to fill in a `SubscriptionParams` struct with the [parsed](./parsing.md)
query and any other relevant operation parameters:
```cpp
// You can still sub-class RequestState and use that in the state parameter to Request::subscribe
// to add your own state to the service callbacks that you receive while executing the subscription
// query.
struct SubscriptionParams
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
using SubscriptionCallback = std::function<void(std::future<response::Value>)>;
```

## Delivering Subscription Updates

There are currently three `Request::deliver` overrides you can choose from when
sending updates to any subscribed listeners. The first one is the simplest,
it will evaluate each subscribed query against the `subscriptionObject`
parameter (which should match the `Subscription` type in the `schema`). It will
unconditionally invoke every subscribed `SubscriptionCallback` callback with
the response to its query:
```cpp
void deliver(const SubscriptionName& name, const std::shared_ptr<Object>& subscriptionObject) const;
```

The second override adds argument filtering. It will look at the field 
arguments in the subscription `query`, and if all of the required parameters
in the `arguments` parameter are present and are an exact match it will dispatch the callback to that subscription:
```cpp
void deliver(const SubscriptionName& name, const SubscriptionArguments& arguments, const std::shared_ptr<Object>& subscriptionObject) const;
```

The last override lets you customize the the way that the required arguments
are matched. Instead of an exact match or making all of the arguments required,
it will dispatch the callback if the `apply` function parameter returns true
for every required field in the subscription `query`.
```cpp
void deliver(const SubscriptionName& name, const SubscriptionFilterCallback& apply, const std::shared_ptr<Object>& subscriptionObject) const;
```

## Handling Multiple Operation Types

Some service implementations (e.g. Apollo over HTTP) use a single pipe to
manage subscriptions and resolve queries or mutations. You can't necessarily
tell which operation type it is without parsing the query and searching for
a specific operation name, so it's hard to tell whether you should call
`resolve` or `subscribe` when the request is received that way. To help with
that, there's a public `Request::findOperationDefinition` method which returns
the operation type as a `std::string` along with a pointer to the AST node for
the selected operation in the parsed query:
```cpp
std::pair<std::string, const peg::ast_node*> findOperationDefinition(const peg::ast_node& root, const std::string& operationName) const;
```