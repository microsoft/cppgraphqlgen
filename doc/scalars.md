# Custom Scalar Payloads with `AnyScalar`

A custom `scalar` type in GraphQL IDL (e.g. `scalar DateTime`) is represented by
`schemagen` as a plain `graphql::response::Value`, and its generated resolver
signature never changes. Sometimes, though, the natural C++ representation of a
custom scalar can't be expressed with any of `response::Value`'s built-in
alternatives &mdash; for example a `BigInt` backed by `std::int64_t`, which is
too large for the 32-bit `response::IntType`.

`response::AnyScalar` lets a hand-written resolver embed an arbitrary
`std::any` payload as the value of a scalar field, along with a type-erased
serializer callback that knows how to turn that payload into calls on a
`response::ValueVisitor`:

```c++
struct [[nodiscard("unnecessary construction")]] AnyScalar
{
	using Serializer =
		std::function<void(const std::any&, const std::shared_ptr<ValueVisitor>&)>;

	std::any value;
	Serializer serialize;
};
```

This is purely a runtime extension: it does not touch the schema IDL,
`schemagen`, or any GraphQL directive, and the wire format is still ordinary
JSON produced by whatever the serializer calls on the visitor
(`add_string`, `add_int`, `add_bool`, `start_object`, ...).

## Opting In

An implementer constructs a `response::Value` from an `AnyScalar`. The payload
and serializer are stored on the value; when the response is serialized, the
token stream calls the serializer with the visitor for the active JSON backend:

```c++
struct Int64Scalar
{
	std::int64_t value = 0;
};

response::Value getBigInt()
{
	return response::Value { response::AnyScalar {
		std::any { Int64Scalar { 9223372036854775807LL } },
		[](const std::any& payload,
			const std::shared_ptr<response::ValueVisitor>& visitor) {
			const auto& scalar = std::any_cast<const Int64Scalar&>(payload);
			// Serialize on the wire as an ordinary JSON string.
			visitor->add_string(std::to_string(scalar.value));
		},
	} };
}
```

`response::toJSON(getBigInt())` yields `"9223372036854775807"`.

## Accessors

* `bool Value::isAny() const` returns `true` if the value is a `Type::Scalar`
  holding an `AnyScalar` payload.
* `SharedAnyScalar Value::releaseAny()` moves the shared payload out of the
  value.

Copying a `Value` that holds an `AnyScalar` shares ownership of the same
payload (no deep copy), and equality compares payloads by shared-pointer
identity.
