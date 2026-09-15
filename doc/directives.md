# Directives

Directives in GraphQL are extensible annotations which alter the runtime
evaluation of a query or which add information to the `schema` definition.
They always begin with an `@`. There are four built-in directives which this
library automatically handles:

1. `@include(if: Boolean!)`: Only resolve this field and include it in the
results if the `if` argument evaluates to `true`.
2. `@skip(if: Boolean!)`: Only resolve this field and include it in the
results if the `if` argument evaluates to `false`.
3. `@deprecated(reason: String)`: Mark the field or enum value as deprecated
through introspection with the specified `reason` string.
4. `@specifiedBy(url: String!)`: Mark the custom scalar type through
introspection as specified by a human readable page at the specified URL.

The `schema` can also define custom `directives` which are valid on different
elements of the `query`. The library does not handle them automatically, but it
will pass them to the `getField` implementations through the optional
`graphql::service::FieldParams` struct (see [fieldparams.md](fieldparams.md)
for more information).