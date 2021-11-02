// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef AWAITABLEFUTURE_H
#define AWAITABLEFUTURE_H

// clang-format off
#ifdef USE_STD_EXPERIMENTAL_COROUTINE
	#include <experimental/coroutine>
	namespace coro = std::experimental;
#else // !USE_STD_EXPERIMENTAL_COROUTINE
	#include <coroutine>
	namespace coro = std;
#endif
// clang-format on

#include <future>
#include <thread>
#include <type_traits>

namespace graphql::internal {

class AwaitableVoid
{
public:
	AwaitableVoid(std::future<void>&& value)
		: _value { std::move(value) }
	{
	}

	void get()
	{
		using namespace std::literals;

		if (_value.wait_for(0s) != std::future_status::timeout)
		{
			_value.get();
		}

		std::promise<void> promise;
		auto future = promise.get_future();

		std::thread([&promise, self = std::move(*this)]() mutable -> AwaitableVoid {
			co_await self;
			promise.set_value();
		}).detach();

		return future.get();
	}

	struct promise_type
	{
		AwaitableVoid get_return_object() noexcept
		{
			return { _promise.get_future() };
		}

		coro::suspend_never initial_suspend() const noexcept
		{
			return {};
		}

		coro::suspend_never final_suspend() const noexcept
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

	bool await_ready() const noexcept
	{
		using namespace std::literals;

		return (_value.wait_for(0s) != std::future_status::timeout);
	}

	void await_suspend(coro::coroutine_handle<> h) const
	{
		std::thread([this, h]() mutable {
			_value.wait();
			h.resume();
		}).detach();
	}

	void await_resume()
	{
		_value.get();
	}

private:
	std::future<void> _value;
};

template <typename T>
class AwaitableFuture
{
public:
	AwaitableFuture(std::future<T>&& value)
		: _value { std::move(value) }
	{
	}

	T get()
	{
		using namespace std::literals;

		if (_value.wait_for(0s) != std::future_status::timeout)
		{
			return _value.get();
		}

		std::promise<T> promise;
		auto future = promise.get_future();

		std::thread([&promise, self = std::move(*this)]() mutable -> AwaitableVoid {
			promise.set_value(co_await self);
		}).detach();

		return future.get();
	}

	struct promise_type
	{
		AwaitableFuture get_return_object() noexcept
		{
			return { _promise.get_future() };
		}

		coro::suspend_never initial_suspend() const noexcept
		{
			return {};
		}

		coro::suspend_never final_suspend() const noexcept
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

	bool await_ready() const noexcept
	{
		using namespace std::literals;

		return (_value.wait_for(0s) != std::future_status::timeout);
	}

	void await_suspend(coro::coroutine_handle<> h) const
	{
		std::thread([this, h]() mutable {
			_value.wait();
			h.resume();
		}).detach();
	}

	T await_resume()
	{
		return _value.get();
	}

private:
	std::future<T> _value;
};

} // namespace graphql::internal

#endif // AWAITABLEFUTURE_H
