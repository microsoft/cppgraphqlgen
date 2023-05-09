# HTTP Proxy Sample

## Dependencies

This sample requires the [Boost.Beast](https://www.boost.org/doc/libs/1_82_0/libs/beast/doc/html/index.html)
header-only library. If you are using [vcpkg](https://github.com/microsoft/vcpkg), it can install that component
for you on-demand. Otherwise, you may need to install it separately if you have a selective installation of
`Boost`.

## Server

The `server` executable listens for HTTP connections on http://127.0.0.1:8080/ (port 8080 of `localhost`). It
is expecting POST requests with a JSON payload in the request body that looks like this:
```json
{
	"query": "GraphQL query document goes here",
	"operationName": "(optional) GraphQL operation name goes here",
	"variables": "(optional) JSON object embedded in a string with GraphQL operation variables"
}
```

It has a single thread accepting requests, and it uses C++20 coroutines. The GraphQL service which actually
resolves the requests is the same Star Wars learning sample in [../learn](../learn/).

## Client

The `client` executable works like most of the other command line samples, it can take a single argument for
the operation name and it will read the GraphQL query document from standard input. It packages the query
parameters in variables for a pre-parsed [query/query.graphql](query/query.graphql), and it implements an
in-proc GraphQL service with the [schema/schema.graphql](schema/schema.graphql) schema using an asynchronous
field resolver. The field resolver packages the parameters into a JSON message and sends it to the `server`
process waiting on http://127.0.0.1:8080/.

When it gets a response, `client` uses the same pre-parsed query in [query/query.graphql](query/query.graphql)
to parse the result and return the JSON document in the field returned from the `server` process.

As long as you start the `server` process first, the behavior of `client` should be almost identical to the
`learn_star_wars` executable sample produced by [../learn](../learn/).
