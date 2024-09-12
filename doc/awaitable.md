# Awaitable

## Launch Policy

In previous versions, this was a `std::launch` enum value used with the
`std::async` standard library function. Now, this is a C++20 `Awaitable`,
specifically a type-erased `graphql::service::await_async` class in
[GraphQLService.h](../include/graphqlservice/GraphQLService.h):
```cpp
// Type-erased awaitable.
class [[nodiscard("unnecessary construction")]] await_async final
{
private:
	struct [[nodiscard("unnecessary construction")]] Concept
	{
		virtual ~Concept() = default;

		[[nodiscard("unexpected call")]] virtual bool await_ready() const = 0;
		virtual void await_suspend(std::coroutine_handle<> h) const = 0;
		virtual void await_resume() const = 0;
	};
...

public:
	// Type-erased explicit constructor for a custom awaitable.
	template <class T>
	explicit await_async(std::shared_ptr<T> pimpl)
		: _pimpl { std::make_shared<Model<T>>(std::move(pimpl)) }
	{
	}

	// Default to immediate synchronous execution.
	await_async()
		: _pimpl { std::static_pointer_cast<const Concept>(
			std::make_shared<Model<std::suspend_never>>(std::make_shared<std::suspend_never>())) }
	{
	}

	// Implicitly convert a std::launch parameter used with std::async to an awaitable.
	await_async(std::launch launch)
		: _pimpl { ((launch & std::launch::async) == std::launch::async)
				? std::static_pointer_cast<const Concept>(std::make_shared<Model<await_worker_thread>>(
					std::make_shared<await_worker_thread>()))
				: std::static_pointer_cast<const Concept>(std::make_shared<Model<std::suspend_never>>(
					std::make_shared<std::suspend_never>())) }
	{
	}
...
};
```
For convenience, it will use `graphql::service::await_worker_thread` if you specify `std::launch::async`,
which should have the same behavior as calling `std::async(std::launch::async, ...)` did before.

If you specify any other flags for `std::launch`, it does not honor them. It will use `std::suspend_never`
(an alias for `std::suspend_never` or `std::experimental::suspend_never`), which as the name suggests,
continues executing the coroutine without suspending. In other words, `std::launch::deferred` will no
longer defer execution as in previous versions, it will execute immediately.

There is also a default constructor which also uses `std::suspend_never`, so that is the default
behavior anywhere that `await_async` is default-initialized with `{}`.

Other than simplification, the big advantage this brings is in the type-erased template constructor.
If you are using another C++20 library or thread/task pool with coroutine support, you can implement
your own `Awaitable` for it and wrap that in `graphql::service::await_async`. It should automatically
start parallelizing all of its resolvers using your custom scheduler, which can pause and resume the
coroutine when and where it likes.

## Awaitable Results

Many APIs which used to return some sort of `std::future` now return an alias for
`graphql::internal::Awaitable<...>`. This template is defined in [Awaitable.h](../include/graphqlservice/internal/Awaitable.h):
```cpp
template <typename T>
class [[nodiscard("unnecessary construction")]] Awaitable
{
public:
	Awaitable(std::future<T> value)
		: _value { std::move(value) }
	{
	}

	[[nodiscard("unnecessary construction")]] T get()
	{
		return _value.get();
	}

	struct promise_type
	{
		[[nodiscard("unnecessary construction")]] Awaitable get_return_object() noexcept
		{
			return { _promise.get_future() };
		}

		...

		void return_value(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
		{
			_promise.set_value(std::move(value));
		}

		...

	private:
		std::promise<T> _promise;

	};

	[[nodiscard("unexpected call")]] constexpr bool await_ready() const noexcept
	{
		return true;
	}

	void await_suspend(std::coroutine_handle<> h) const
	{
		h.resume();
	}

	[[nodiscard("unnecessary construction")]] T await_resume()
	{
		return _value.get();
	}

private:
	std::future<T> _value;
};
```

The key details are that it implements the required `promise_type` and `await_` methods so
that you can turn any `co_return` statement into a `std::future<T>`, and it can either
`co_await` for that `std::future<T>` from a coroutine, or call `T get()` to block a regular
function until it completes.

## AwaitableScalar and AwaitableObject

In previous versions, `service::FieldResult<T>` created an abstraction over return types `T` and
`std::future<T>`, when returning from a field getter you could return either and it would
implicitly convert that to a `service::FieldResult<T>` which looked and acted like a
`std::future<T>`.

Now, `service::FieldResult<T>` is replaced with `service::AwaitableScalar` for `scalar` type
fields without a selection set of sub-fields, or `service::AwaitableObject` for `object`
type fields which must have a selection set of sub-fields. The difference between
`service::AwaitableScalar` and `service::AwaitableObject` is that `scalar` type fields can
also return `std::shared_ptr<const response::Value>` directly, which bypasses all of the
conversion logic in `service::ModifiedResult` and just validates that the shape of the
`response::Value` matches the `scalar` type with all of its modifiers. These are both defined
in [GraphQLService.h](../include/graphqlservice/GraphQLService.h):
```cpp
// Field accessors may return either a result of T, an awaitable of T, or a std::future<T>, so at
// runtime the implementer may choose to return by value or defer/parallelize expensive operations
// by returning an async future or an awaitable coroutine.
//
// If the overhead of conversion to response::Value is too expensive, scalar type field accessors
// can store and return a std::shared_ptr<const response::Value> directly.
template <typename T>
class AwaitableScalar
{
public:
	template <typename U>
	AwaitableScalar(U&& value)
		: _value { std::forward<U>(value) }
	{
	}

	struct promise_type
	{
		AwaitableScalar<T> get_return_object() noexcept
		{
			return { _promise.get_future() };
		}

		...

		void return_value(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
		{
			_promise.set_value(value);
		}

		void return_value(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
		{
			_promise.set_value(std::move(value));
		}

		...

	private:
		std::promise<T> _promise;
	};

	bool await_ready() const noexcept { ... }

	void await_suspend(std::coroutine_handle<> h) const { ... }

	T await_resume()
	{
		... // Throws std::logic_error("Cannot await std::shared_ptr<const response::Value>") if called with that alternative
	}

	std::shared_ptr<const response::Value> get_value() noexcept
	{
		... // Returns an empty std::shared_ptr if called with a different alternative
	}

private:
	std::variant<T, std::future<T>, std::shared_ptr<const response::Value>> _value;
};

// Field accessors may return either a result of T, an awaitable of T, or a std::future<T>, so at
// runtime the implementer may choose to return by value or defer/parallelize expensive operations
// by returning an async future or an awaitable coroutine.
template <typename T>
class AwaitableObject
{
public:
	template <typename U>
	AwaitableObject(U&& value)
		: _value { std::forward<U>(value) }
	{
	}

	struct promise_type
	{
		AwaitableObject<T> get_return_object() noexcept
		{
			return { _promise.get_future() };
		}

		...

		void return_value(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
		{
			_promise.set_value(value);
		}

		void return_value(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
		{
			_promise.set_value(std::move(value));
		}

		...

	private:
		std::promise<T> _promise;
	};

	bool await_ready() const noexcept { ... }

	void await_suspend(std::coroutine_handle<> h) const { ... }

	T await_resume() { ... }

private:
	std::variant<T, std::future<T>> _value;
};
```

These types both add a `promise_type` for `T`, but coroutines need their own return type to do that.
Making `service::AwaitableScalar<T>` or `service::AwaitableObject<T>` the return type of a field
getter means you can turn it into a coroutine by just replacing `return` with `co_return`, and
potentially start to `co_await` other awaitables and coroutines.

Type-erasure made it so you do not need to use a special return type, the type-erased
`Object::Model<T>` type just needs to be able to pass the return result from your field
getter into a constructor for one of these return types. So if you want to implement
your field getters as coroutines, you should still wrap the return type in
`service::AwaitableScalar<T>` or `service::AwaitableObject<T>`. Otherwise, you can remove
the template wrapper from all of your field getters.
