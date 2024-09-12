// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

module;

#include <coroutine>
#include <future>
#include <type_traits>

export module Internal.Awaitable;

export namespace graphql::internal {

template <typename T>
class [[nodiscard("unnecessary construction")]] Awaitable;

template <>
class [[nodiscard("unnecessary construction")]] Awaitable<void>
{
public:
	Awaitable(std::future<void> value)
		: _value { std::move(value) }
	{
	}

	void get()
	{
		_value.get();
	}

	struct promise_type
	{
		[[nodiscard("unnecessary construction")]] Awaitable get_return_object() noexcept
		{
			return { _promise.get_future() };
		}

		std::suspend_never initial_suspend() const noexcept
		{
			return {};
		}

		std::suspend_never final_suspend() const noexcept
		{
			return {};
		}

		void return_void() noexcept
		{
			_promise.set_value();
		}

		void unhandled_exception() noexcept
		{
			_promise.set_exception(std::current_exception());
		}

	private:
		std::promise<void> _promise;
	};

	[[nodiscard("unexpected call")]] constexpr bool await_ready() const noexcept
	{
		return true;
	}

	void await_suspend(std::coroutine_handle<> h) const
	{
		h.resume();
	}

	void await_resume()
	{
		_value.get();
	}

private:
	std::future<void> _value;
};

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

		std::suspend_never initial_suspend() const noexcept
		{
			return {};
		}

		std::suspend_never final_suspend() const noexcept
		{
			return {};
		}

		void return_value(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
		{
			_promise.set_value(value);
		}

		void return_value(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
		{
			_promise.set_value(std::move(value));
		}

		void unhandled_exception() noexcept
		{
			_promise.set_exception(std::current_exception());
		}

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

} // namespace graphql::internal
