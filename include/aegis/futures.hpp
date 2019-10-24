//
// futures.hpp
// ***********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 
// Adapted from https://github.com/scylladb/seastar to support asio scheduling

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/error.hpp"
#include <condition_variable>
#include <stdexcept>
#include <type_traits>
#include <memory>
#include <functional>
#include <cassert>
#include <chrono>
#include <thread>
#include <iostream>
#include <mutex>
#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <asio/bind_executor.hpp>
#include <asio/executor_work_guard.hpp>

using namespace std::literals::chrono_literals;


namespace aegis
{

template <class T>
class promise;

template <class T>
class future;

template <typename T>
class shared_future;

template <typename T, typename... A>
future<T> make_ready_future(A&&... value);

template <typename T>
future<T> make_ready_future(T&& value);

template <typename T = rest::rest_reply>
future<T> make_exception_future(std::exception_ptr value) noexcept;

template<typename T>
struct add_future
{
    using type = future<T>;
};

template<typename T>
struct add_future<future<T>>
{
    using type = future<T>;
};

template<typename T>
struct remove_future
{
    using type = T;
};

template<typename T>
struct remove_future<future<T>>
{
    using type = T;
};

template<typename T>
struct is_future : std::false_type {};

template<typename T>
struct is_future<future<T>> : std::true_type {};

template<typename F, typename... A>
struct result_of : std::result_of<F(A...)> {};

template<typename F>
struct result_of<F, void> : std::result_of<F()> {};

template<typename F, typename... A>
using result_of_t = typename result_of<F, A...>::type;


struct ready_future_marker {};
struct exception_future_marker {};

template<typename T>
using add_future_t = typename add_future<T>::type;

template<typename T>
using remove_future_t = typename remove_future<T>::type;

namespace detail
{
/// call_state<T, Func, State>
template<typename T, typename Func, typename State>
add_future_t<T> call_state(Func&& func, State&& state);

/// call_future<T, Func, Future>
template<typename T, typename Func, typename Future>
add_future_t<T> call_future(Func&& func, Future&& fut) noexcept;
}

/// future_state<T>
template <typename T>
struct future_state
{
    using type = T;
    static constexpr bool copy_noexcept = std::is_nothrow_copy_constructible<T>::value;
    static_assert(std::is_nothrow_move_constructible<T>::value,
                  "Types must be no-throw move constructible");
    static_assert(std::is_nothrow_destructible<T>::value,
                  "Types must be no-throw destructible");
    static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's copy constructor must not throw");
    static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's move constructor must not throw");
    enum class state
    {
        invalid,
        future,
        result,
        exception,
    };// _state = state::future;
    std::atomic<state> _state;
    union any
    {
        any() {}
        ~any() {}
        T value;
        std::exception_ptr ex;
    } _u;
    asio::io_context * _io_context = nullptr;
    std::recursive_mutex * _global_m = nullptr;
    future_state() noexcept {}
    future_state(asio::io_context * _io_context, std::recursive_mutex * _global_m) noexcept 
        : _state(state::future)
        , _io_context(_io_context)
        , _global_m(_global_m)
    {}
    future_state(future_state&& x) noexcept
        : _state(state::future)
        , _io_context(x._io_context)
        , _global_m(x._global_m)
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        state _x = x._state.load(std::memory_order_acquire);
        state _s = _state.load(std::memory_order_acquire);
        switch (_x)
        {
            case state::future:
                break;
            case state::result:
                new (&_u.value) T(std::move(x._u.value));
                x._u.value.~T();
                break;
            case state::exception:
                new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
                x._u.ex.~exception_ptr();
                break;
            case state::invalid:
                break;
            default:
                abort();
        }
        _state.store(_x, std::memory_order_release);
        x._state.store(state::invalid, std::memory_order_release);
        std::atomic_thread_fence(std::memory_order_release);
    }
    ~future_state() noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        switch (_state)
        {
            case state::invalid:
                break;
            case state::future:
                break;
            case state::result:
                _u.value.~T();
                break;
            case state::exception:
                _u.ex.~exception_ptr();
                break;
            default:
                abort();
        }
        std::atomic_thread_fence(std::memory_order_release);
    }
    future_state& operator=(future_state&& x) noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        state _s = _state.load(std::memory_order_consume);
        if (this != &x)
        {
            this->~future_state();
            new (this) future_state(std::move(x));
        }
        std::atomic_thread_fence(std::memory_order_release);
        return *this;
    }
    bool available() const noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        auto _s = _state.load(std::memory_order_consume);
        std::atomic_thread_fence(std::memory_order_release);
        return _s == state::result || _s == state::exception;
    }
    bool failed() const noexcept
    {
        return _state == state::exception;
    }
    void wait();
    void set(const T& value) noexcept
    {
        state _s = _state.load(std::memory_order_acquire);
        assert(_s == state::future);
        new (&_u.value) T(value);
        _state.store(state::result, std::memory_order_release);
    }
    void set(T&& value) noexcept
    {
        state _s = _state.load(std::memory_order_acquire);
        assert(_s == state::future);
        new (&_u.value) T(std::move(value));
        _state.store(state::result, std::memory_order_release);
    }
    template <typename... A>
    void set(A&&... a)
    {
        state _s = _state.load(std::memory_order_acquire);
        assert(_s == state::future);
        new (&_u.value) T(std::forward<A>(a)...);
        _state.store(state::result, std::memory_order_release);
    }
    void set_exception(std::exception_ptr ex) noexcept
    {
        state _s = _state.load(std::memory_order_acquire);
        assert(_s == state::future);
        new (&_u.ex) std::exception_ptr(ex);
        _state.store(state::exception, std::memory_order_release);
    }
    std::exception_ptr get_exception() && noexcept
    {
        state _s = _state.load(std::memory_order_acquire);
        assert(_s == state::exception);
        auto ex = std::move(_u.ex);
        _u.ex.~exception_ptr();
        _state.store(state::invalid, std::memory_order_release);
        return ex;
    }
    std::exception_ptr get_exception() const& noexcept
    {
        state _s = _state.load(std::memory_order_consume);
        assert(_s == state::exception);
        return _u.ex;
    }
    auto get_value() && noexcept
    {
        state _s = _state.load(std::memory_order_acquire);
        assert(_s == state::result);
        _state.store(_s, std::memory_order_release);
        return std::move(_u.value);
    }
    template<typename U = T>
    std::enable_if_t<std::is_copy_constructible<U>::value, U> get_value() const& noexcept(copy_noexcept)
    {
        assert(_state == state::result);
        return _u.value;
    }
    T get() &&
    {
        auto _s = _state.load(std::memory_order_acquire);
        assert(_s != state::future);
        if (_s == state::exception)
        {
            auto ex = std::move(_u.ex);
            _u.ex.~exception_ptr();
            _state.store(state::invalid, std::memory_order_release);
            std::rethrow_exception(std::move(ex));
        }
        _state.store(_s, std::memory_order_release);
        return std::move(_u.value);
    }
    T get() const&
    {
        state _s = _state.load(std::memory_order_consume);
        assert(_s != state::future);
        if (_s == state::exception)
        {
            std::rethrow_exception(_u.ex);
        }
        return _u.value;
    }
    void ignore() noexcept
    {
        state _s = _state.load(std::memory_order_acquire);
        assert(_s != state::future);
        this->~future_state();
        _state.store(state::invalid, std::memory_order_release);
    }
    void forward_to(promise<T>& pr) noexcept
    {
        state _s = _state.load(std::memory_order_acquire);
        assert(_s != state::future);
        if (_s == state::exception)
        {
            pr.set_urgent_exception(std::move(_u.ex));
            _u.ex.~exception_ptr();
        }
        else
        {
            pr.set_urgent_value(std::move(_u.value));
            _u.value.~T();
        }
        _state.store(state::invalid, std::memory_order_release);
    }
};

/// future_state<void>
template <>
struct future_state<void>
{
    using type = void;
    static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's copy constructor must not throw");
    static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's move constructor must not throw");
    static constexpr bool copy_noexcept = true;
    enum class state : uintptr_t
    {
        invalid = 0,
        future = 1,
        result = 2,
        exception_min = 3
    };
    union any
    {
        any() { st = state::future; }
        ~any() {}
        std::atomic<state> st;
        std::exception_ptr ex;
    } _u;
    std::recursive_mutex _m;
    asio::io_context * _io_context = nullptr;
    std::recursive_mutex * _global_m = nullptr;
    future_state() noexcept {}
    future_state(asio::io_context * _io_context, std::recursive_mutex * _global_m) noexcept 
        : _io_context(_io_context)
        , _global_m(_global_m)
    {}    future_state(future_state&& x) noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        if (x._u.st < state::exception_min)
        {
            _u.st.store(x._u.st);
        }
        else
        {
            new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
            x._u.ex.~exception_ptr();
        }
        x._u.st.store(state::invalid);
        std::atomic_thread_fence(std::memory_order_release);
    }
    ~future_state() noexcept
    {
        if (_u.st >= state::exception_min)
        {
            _u.ex.~exception_ptr();
        }
    }
    future_state& operator=(future_state&& x) noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        if (this != &x)
        {
            this->~future_state();
            new (this) future_state(std::move(x));
        }
        std::atomic_thread_fence(std::memory_order_release);
        return *this;
    }
    bool available() const noexcept
    {
        return _u.st == state::result || _u.st >= state::exception_min;
    }
    bool failed() const noexcept
    {
        return _u.st >= state::exception_min;
    }
    void set()
    {
        assert(_u.st == state::future);
        _u.st.store(state::result, std::memory_order_release);
    }
    void set_exception(std::exception_ptr ex) noexcept
    {
        assert(_u.st == state::future);
        new (&_u.ex) std::exception_ptr(ex);
        assert(_u.st >= state::exception_min);
    }
    void get() &&
    {
        assert(_u.st != state::future);
        if (_u.st >= state::exception_min)
        {
            std::rethrow_exception(std::move(_u.ex));
        }
    }
    void get() const&
    {
        assert(_u.st != state::future);
        if (_u.st >= state::exception_min)
        {
            std::rethrow_exception(_u.ex);
        }
    }
    void ignore() noexcept
    {
        assert(available());
        this->~future_state();
        _u.st.store(state::invalid, std::memory_order_release);
    }
    std::exception_ptr get_exception() && noexcept
    {
        assert(_u.st >= state::exception_min);
        return std::move(_u.ex);
    }
    std::exception_ptr get_exception() const& noexcept
    {
        assert(_u.st >= state::exception_min);
        return _u.ex;
    }
    void get_value() const noexcept
    {
        assert(_u.st == state::result);
    }
    void forward_to(promise<void>& pr) noexcept;
};

/// task<T>
class task
{
public:
    virtual ~task() = default;
    virtual void run() noexcept = 0;
};

/// continuation_base<T>
template <typename T>
class continuation_base : public task
{
protected:
    future_state<T> _state;
    using future_type = future<T>;
    using promise_type = promise<T>;
public:
    continuation_base() = default;
    explicit continuation_base(asio::io_context * _io_context, std::recursive_mutex * _global_m)
        : _state(_io_context, _global_m) {}

    explicit continuation_base(future_state<T>&& state) : _state(std::move(state)) {}

    void set_state(T&& state)
    {
        _state.set(std::move(state));
    }
    void set_state(future_state<T>&& state)
    {
        _state = std::move(state);
    }
    future_state<T>* state() noexcept
    {
        return &_state;
    }

    friend class promise<T>;
    friend class future<T>;
};

/// continuation_base<void>
template <>
class continuation_base<void> : public task
{
protected:
    future_state<void> _state;
    using future_type = future<void>;
    using promise_type = promise<void>;
public:
    continuation_base() = default;
    explicit continuation_base(asio::io_context * _io_context, std::recursive_mutex * _global_m)
        : _state(_io_context, _global_m) {}
    explicit continuation_base(future_state<void>&& state) : _state(std::move(state)) {}
    void set_state(future_state<void>&& state)
    {
        _state = std::move(state);
    }
    future_state<void>* state() noexcept
    {
        return &_state;
    }

    friend class promise<void>;
    friend class future<void>;
};

/// continuation<Func, T>
template <typename Func, typename T>
struct continuation final : continuation_base<T>
{
    continuation(Func&& func, future_state<T>&& state) : continuation_base<T>(std::move(state)), _func(std::move(func)) {}
    continuation(Func&& func, asio::io_context * _io_context, std::recursive_mutex * _global_m)
        : continuation_base<T>(_io_context, _global_m)
        , _func(std::move(func))
    {}
    virtual void run() noexcept override
    {
        _func(std::move(this->_state));
    }
    Func _func;
};
using task_ptr = std::unique_ptr<task>;


/// promise<T>
template <typename T>
class promise
{
    enum class urgent { no, yes };
    future<T>* _future = nullptr;
    future_state<T> _local_state;
    future_state<T>* _state;
    std::unique_ptr<continuation_base<T>> _task;
    std::recursive_mutex _m;
    asio::io_context * _io_context = nullptr;
    std::recursive_mutex * _global_m = nullptr;
    static constexpr bool copy_noexcept = future_state<T>::copy_noexcept;
public:
    promise(asio::io_context * _io_context, std::recursive_mutex * _global_m) noexcept
        : _local_state(future_state<T>(_io_context, _global_m))
        , _state(&_local_state)
        , _io_context(_io_context)
        , _global_m(_global_m)
    {}

    promise(promise&& x) noexcept
        : _local_state(future_state<T>(x._io_context, x._global_m))
        , _io_context(x._io_context)
        , _global_m(x._global_m)
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        //std::lock_guard<std::recursive_mutex> gl(internal::_global_m);
        //TODO: this needs a lock or l2 lock has a chance of segfault
        if (x._future)
        {
            std::unique_lock<std::recursive_mutex> l(_m, std::defer_lock);
            std::unique_lock<std::recursive_mutex> l2(x._future->_m, std::defer_lock);
            std::lock(l, l2);
            if (!x._future)
                goto NOTREALLYTHERE;
            _future = x._future;
            _future->_promise = this;
            _state = x._state;
            _task = std::move(x._task);
            if (_state == &x._local_state)
            {
                _state = &_local_state;
                _local_state = std::move(x._local_state);
            }
            x._future = nullptr;
            x._state = nullptr;
        }
        else
        {
        NOTREALLYTHERE:;
            std::lock_guard<std::recursive_mutex> l(_m);
            _state = x._state;
            _task = std::move(x._task);
            if (_state == &x._local_state)
            {
                _state = &_local_state;
                _local_state = std::move(x._local_state);
            }
            x._state = nullptr;
        }
        std::atomic_thread_fence(std::memory_order_release);
    }
    promise(const promise&) = delete;
    ~promise() noexcept
    {
        std::unique_lock<std::recursive_mutex> l(_m, std::defer_lock);
        std::unique_lock<std::recursive_mutex> l2(*_global_m, std::defer_lock);
        std::lock(l, l2);
        abandoned();
    }
    promise& operator=(promise&& x) noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        if (this != &x)
        {
            this->~promise();
            new (this) promise(std::move(x));
        }
        std::atomic_thread_fence(std::memory_order_release);
        return *this;
    }
    void operator=(const promise&) = delete;

    future<T> get_future() noexcept;

    template <typename... A>
    void set_value(A&&... a) noexcept
    {
        assert(_state);
        _state->set(std::forward<A>(a)...);
        make_ready<urgent::no>();
    }

    void set_exception(std::exception_ptr ex) noexcept
    {
        do_set_exception<urgent::no>(std::move(ex));
    }

    template<typename Exception>
    void set_exception(Exception&& e) noexcept
    {
        set_exception(make_exception_ptr(std::forward<Exception>(e)));
    }
private:

    template<urgent Urgent, typename... A>
    void do_set_value(A... a) noexcept
    {
        assert(_state);
        _state->set(std::move(a)...);
        make_ready<Urgent>();
    }

    template<typename... A>
    void set_urgent_value(A&&... a) noexcept
    {
        set_value(std::forward<A>(a)...);
    }

    template<urgent Urgent>
    void do_set_exception(std::exception_ptr ex) noexcept
    {
        assert(_state);
        _state->set_exception(std::move(ex));
        make_ready<Urgent>();
    }

    void set_urgent_exception(std::exception_ptr ex) noexcept
    {
        do_set_exception<urgent::yes>(std::move(ex));
    }
private:
    template <typename Func>
    void schedule(Func&& func)
    {
        auto tws = std::make_unique<continuation<Func, T>>(std::move(func), _io_context, _global_m);
        _state = &tws->_state;
        _task = std::move(tws);
    }
    template<urgent Urgent>
    void make_ready() noexcept;
    void abandoned() noexcept;

    template <typename U>
    friend class future;

    friend struct future_state<T>;
    friend struct future_state<void>;
};

/// future<T>
template <typename T>
class future
{
    promise<T>* _promise;
    future_state<T> _local_state;
    mutable std::recursive_mutex _m;
    asio::io_context * _io_context = nullptr;
    std::recursive_mutex * _global_m = nullptr;
    static constexpr bool copy_noexcept = future_state<T>::copy_noexcept;
private:
    future(promise<T>* pr) noexcept
        : _local_state(future_state<T>(pr->_io_context, pr->_global_m))
    {
        _io_context = pr->_io_context;
        _global_m = pr->_global_m;
        std::atomic_thread_fence(std::memory_order_acquire);
        std::unique_lock<std::recursive_mutex> l(*_global_m, std::defer_lock);
        std::unique_lock<std::recursive_mutex> l2(_m, std::defer_lock);
        std::unique_lock<std::recursive_mutex> l3(pr->_m, std::defer_lock);
        std::lock(l, l2, l3);

        _promise = pr;
        _promise->_future = this;
        std::atomic_thread_fence(std::memory_order_release);
    }
    template <typename... A>
    future(ready_future_marker, A&&... a)
        : _promise(nullptr)
        , _local_state(future_state<T>(nullptr, nullptr))
    {
        _local_state.set(std::forward<A>(a)...);
    }
    future(exception_future_marker, std::exception_ptr ex) noexcept
        : _promise(nullptr)
        , _local_state(future_state<T>(nullptr, nullptr))
    {
        _local_state.set_exception(std::move(ex));
    }
    explicit future(future_state<T>&& state, asio::io_context * _io_context, std::recursive_mutex * _global_m) noexcept
        : _local_state(future_state<T>(_io_context, _global_m))
        , _io_context(_io_context)
        , _global_m(_global_m)
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        _local_state = std::move(state);
        _promise = nullptr;
        std::atomic_thread_fence(std::memory_order_release);
    }
    future_state<T> * state() noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        if (_promise)
        {
            std::unique_lock<std::recursive_mutex> l(_m, std::defer_lock);
            std::unique_lock<std::recursive_mutex> l2(*_global_m, std::defer_lock);
            std::lock(l, l2);
            
            //std::lock_guard<std::recursive_mutex> l(_promise->_m);
            future_state<T> * _st = _promise->_state;
            std::atomic_thread_fence(std::memory_order_release);
            return _st;
        }
        else
        {
            future_state<T> * _st = &_local_state;
            std::atomic_thread_fence(std::memory_order_release);
            return _st;
        }
    }
    const future_state<T> * state() const noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        if (_promise)
        {
            std::unique_lock<std::recursive_mutex> l(_m, std::defer_lock);
            std::unique_lock<std::recursive_mutex> l2(*_global_m, std::defer_lock);
            std::lock(l, l2);

            //std::lock_guard<std::recursive_mutex> l(_promise->_m);
            const future_state<T> * _st = _promise->_state;
            std::atomic_thread_fence(std::memory_order_release);
            return _st;
        }
        else
        {
            const future_state<T> * _st = &_local_state;
            std::atomic_thread_fence(std::memory_order_release);
            return _st;
        }
    }
    template <typename Func>
    void schedule(Func&& func)
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        future_state<T> * _st = state();
        if (state()->available())
        {
            asio::post(*_io_context, [func = std::move(func), _state = std::move(*state())]() mutable
            {
                func(std::move(_state));
            });
        }
        else
        {
            std::unique_lock<std::recursive_mutex> l(_m, std::defer_lock);
            std::unique_lock<std::recursive_mutex> l2(_promise->_m, std::defer_lock);
            std::lock(l, l2);
            assert(_promise);
            _promise->schedule(std::move(func));
            _promise->_future = nullptr;
            _promise = nullptr;
        }
        std::atomic_thread_fence(std::memory_order_release);
    }
    future_state<T> get_available_state() noexcept
    {
        auto st = state();
        if (_promise)
        {
            std::lock_guard<std::recursive_mutex> l(_promise->_m);
            if (!_promise)
                goto NOTREALLYTHERE;
            _promise->_future = nullptr;
            _promise = nullptr;
        }
    NOTREALLYTHERE:;
        return std::move(*st);
    }

    future<T> rethrow_with_nested()
    {
        if (!failed())
        {
            return make_exception_future<T>(std::current_exception());
        }
        else
        {
            std::nested_exception f_ex;
            try
            {
                get();
            }
            catch (...)
            {
                std::throw_with_nested(f_ex);
            }
            throw std::runtime_error("unreachable");
        }
    }

    template<typename U>
    friend class shared_future;
public:
    using value_type = T;
    using promise_type = promise<T>;
    future(future&& x) noexcept
        : _local_state(future_state<T>(x._io_context, x._global_m))
        , _io_context(x._io_context)
        , _global_m(x._global_m)
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        std::unique_lock<std::recursive_mutex> l(_m, std::defer_lock);
        std::unique_lock<std::recursive_mutex> l2(x._m, std::defer_lock);
        std::lock(l, l2);

        _promise = x._promise;
        if (!_promise)
        {
            _local_state = std::move(x._local_state);
        }
        x._promise = nullptr;
        if (_promise)
        {
            std::lock_guard<std::recursive_mutex> l(_promise->_m);
            _promise->_future = this;
        }
        std::atomic_thread_fence(std::memory_order_release);
    }
    future(const future&) = delete;
    future& operator=(future&& x) noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        if (this != &x)
        {
            this->~future();
            new (this) future(std::move(x));
        }
        std::atomic_thread_fence(std::memory_order_release);
        return *this;
    }
    void operator=(const future&) = delete;
    ~future()
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        if (_promise)
        {
            std::lock_guard<std::recursive_mutex> l(*_global_m);
            if (_promise)
            {
                std::lock_guard<std::recursive_mutex> l(_promise->_m);
                _promise->_future = nullptr;
            }
        }
        if (failed())
        {
        }
        std::atomic_thread_fence(std::memory_order_release);
    }

    T get()
    {
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            future_state<T> * _st = state();
            std::atomic_thread_fence(std::memory_order_release);
            if (!_st->available())
            {
                do_wait();
            }
        }

        {
            std::atomic_thread_fence(std::memory_order_acquire);

            std::unique_lock<std::recursive_mutex> l(_m, std::defer_lock);
            std::unique_lock<std::recursive_mutex> l2(*_global_m, std::defer_lock);
            std::lock(l, l2);

            future_state<T> _st(get_available_state());
            std::atomic_thread_fence(std::memory_order_release);
            return _st.get();
        }
    }

    std::exception_ptr get_exception()
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        future_state<T> _st(get_available_state());
        std::atomic_thread_fence(std::memory_order_release);
        return _st.get_exception();
    }

    void wait() const noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        const future_state<T> * _st = state();
        std::atomic_thread_fence(std::memory_order_release);
        if (!_st->available())
        {
            do_wait();
        }
    }
private:
    void do_wait() const noexcept
    {
        // fake wait
        // maybe execute something in the asio queue?

        while (!available())
            std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }

public:
    bool available() const noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        const future_state<T> * _st = state();
        assert(_st);
        auto res = _st->available();
        std::atomic_thread_fence(std::memory_order_release);
        return res;
    }

    bool failed() const noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        auto _st = state();
        if (!_st)
            return false;
        bool f = _st->failed();
        std::atomic_thread_fence(std::memory_order_release);
        return f;
    }

    template <typename Func, typename Result = result_of_t<Func, T>>
    add_future_t<Result> then(Func&& func) noexcept
    {
        using inner_type = remove_future_t<Result>;
        std::atomic_thread_fence(std::memory_order_acquire);
        if (available())
        {
            if (failed())
            {
                return make_exception_future<inner_type>(get_exception());
            }
            else
            {
                return detail::call_state<inner_type>(std::forward<Func>(func), get_available_state());
            }
        }
        std::atomic_thread_fence(std::memory_order_release);
        promise<inner_type> pr(_io_context, _global_m);
        auto fut = pr.get_future();
        try
        {
            this->schedule([pr = std::move(pr), func = std::forward<Func>(func)](future_state<T> && state) mutable {
                std::atomic_thread_fence(std::memory_order_acquire);
                if (state.failed())
                {
                    pr.set_exception(std::move(state).get_exception());
                }
                else
                {
                    detail::call_state<inner_type>(std::forward<Func>(func), std::move(state)).forward_to(std::move(pr));
                }
                std::atomic_thread_fence(std::memory_order_release);
            });
        }
        catch (...)
        {
            abort();
        }
        return fut;
    }

    template <typename Func, typename Result = std::result_of_t<Func(future)>>
    add_future_t<Result> then_wrapped(Func&& func) noexcept
    {
        using inner_type = remove_future_t<Result>;
        std::atomic_thread_fence(std::memory_order_acquire);
        if (available())
        {
            return detail::call_future<inner_type>(std::forward<Func>(func), future(get_available_state(), _io_context, _global_m));
        }
        std::atomic_thread_fence(std::memory_order_release);
        promise<inner_type> pr(_io_context, _global_m);
        auto fut = pr.get_future();
        try
        {
            this->schedule([pr = std::move(pr), func = std::forward<Func>(func), this](auto&& state) mutable {
                std::atomic_thread_fence(std::memory_order_acquire);
                detail::call_future<inner_type>(std::forward<Func>(func), future(std::move(state), _io_context, _global_m)).forward_to(std::move(pr));
                std::atomic_thread_fence(std::memory_order_release);
            });
        }
        catch (...)
        {
            abort();
        }
        return fut;
    }

    void forward_to(promise<T>&& pr) noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        if (state()->available())
        {
            state()->forward_to(pr);
        }
        else
        {
            _promise->_future = nullptr;
            *_promise = std::move(pr);
            _promise = nullptr;
        }
        std::atomic_thread_fence(std::memory_order_release);
    }

    template <typename Func>
    future<T> finally(Func&& func) noexcept
    {
        return then_wrapped(finally_body<Func, is_future<std::result_of_t<Func()>>::value>(std::forward<Func>(func)));
    }

    template <typename Func, bool FuncReturnsFuture>
    struct finally_body;

    template <typename Func>
    struct finally_body<Func, true>
    {
        Func _func;

        using inner_type = remove_future_t<std::result_of_t<Func(T)>>;

        finally_body(Func&& func) : _func(std::forward<Func>(func))
        {
        }

        future<T> operator()(future<T>&& result)
        {
            return detail::call_state<inner_type>(_func).then_wrapped([result = std::move(result)](auto f_res) mutable {
                if (!f_res.failed())
                {
                    return std::move(result);
                }
                else
                {
                    try
                    {
                        f_res.get();
                    }
                    catch (...)
                    {
                        return result.rethrow_with_nested();
                    }
                    throw std::runtime_error("unreachable");
                }
            });
        }
    };

    template <typename Func>
    struct finally_body<Func, false>
    {
        Func _func;

        finally_body(Func&& func) : _func(std::forward<Func>(func))
        {
        }

        future<T> operator()(future<T>&& result)
        {
            try
            {
                _func();
                return std::move(result);
            }
            catch (...)
            {
                return result.rethrow_with_nested();
            }
        };
    };


    template <typename Func>
    future<T> handle_exception(Func&& func) noexcept
    {
        using func_ret = std::result_of_t<Func(std::exception_ptr)>;
        return then_wrapped([func = std::forward<Func>(func)]
        (auto&& fut) mutable->future<T> {
            if (!fut.failed())
                return make_ready_future<T>(fut.get());
            else
                return detail::call_future<func_ret>(func, fut.get_exception());
        });
    }

    void ignore_ready_future() noexcept
    {
        state()->ignore();
    }

private:
    template <typename U>
    friend class promise;
    template <typename U, typename... A>
    friend future<U> make_ready_future(A&&... value);
    template <typename U>
    friend future<U> make_ready_future(U&& value);
    template <typename U>
    friend future<U> make_exception_future(std::exception_ptr ex) noexcept;
};

/// promise<T>::get_future()
template <typename T>
inline future<T> promise<T>::get_future() noexcept
{
    assert(!_future && _state && !_task);
    return future<T>(this);
}

/// promise<T>::make_ready()
template <typename T>
template<typename promise<T>::urgent Urgent>
inline void promise<T>::make_ready() noexcept
{
    if (_task)
    {
        _state = nullptr;
        if (Urgent == urgent::yes)
        {
            _task->run();
        }
        else
        {
            asio::post(*_io_context, [_task = std::move(_task)]
            {
                _task->run();
            });
        }
    }
}

/// promise<T>::abandoned()
template <typename T>
inline void promise<T>::abandoned() noexcept
{
    std::atomic_thread_fence(std::memory_order_acquire);
    if (_future)
    {
        std::lock_guard<std::recursive_mutex> l(_future->_m);
        assert(_state);
        assert(_state->available() || !_task);
        _future->_local_state = std::move(*_state);
        _future->_promise = nullptr;
    }
    else if (_state && _state->failed())
    {
    }
    std::atomic_thread_fence(std::memory_order_release);
}

/// make_ready_future<T, ...A>()
template <typename T, typename... A>
inline future<T> make_ready_future(A&&... value)
{
    return { ready_future_marker(), std::forward<A>(value)... };
}

/// make_ready_future<T>()
template <typename T>
inline future<T> make_ready_future(T&& value)
{
    return { ready_future_marker(), std::forward<T>(value) };
}

/// make_exception_future<T>()
template <typename T>
inline future<T> make_exception_future(std::exception_ptr ex) noexcept
{
    return { exception_future_marker(), std::move(ex) };
}


namespace detail
{
/// to_future<T>()
template<typename T>
inline add_future_t<T> to_future(T&& value)
{
    return make_ready_future<T>(std::forward<T>(value));
}

/// to_future<T>()
template<typename T>
inline future<T> to_future(future<T>&& fut)
{
    return std::move(fut);
}

/// call_function<T, Func, ...A, Ret>()
template<typename T, typename Func, typename... A, typename Ret = std::result_of_t<Func(A&&...)>>
inline std::enable_if_t<is_future<Ret>::value, add_future_t<Ret>> call_function(std::true_type, Func&& func, A&&... args)
{
    try
    {
        return func(std::forward<A>(args)...);
    }
    catch (...)
    {
        return make_exception_future<T>(std::current_exception());
    }
}

/// call_function<T, Func, ...A, Ret>()
template<typename T, typename Func, typename... A, typename Ret = std::result_of_t<Func(A&&...)>>
inline std::enable_if_t<!is_future<Ret>::value, add_future_t<T>> call_function(std::true_type, Func&& func, A&&... args)
{
    try
    {
        func(std::forward<A>(args)...);
        return make_ready_future<T>();
    }
    catch (...)
    {
        return make_exception_future<T>(std::current_exception());
    }
}

/// call_function<T, Func, ...A>()
template<typename T, typename Func, typename... A>
inline add_future_t<T> call_function(std::false_type, Func&& func, A&&... args)
{
    try
    {
        return to_future(func(std::forward<A>(args)...));
    }
    catch (...)
    {
        return make_exception_future<T>(std::current_exception());
    }
}

/// call_from_state<T, Func, State>()
template<typename T, typename Func, typename State>
inline add_future_t<T> call_from_state(std::true_type, Func&& func, State&&)
{
    return call_function<T>(std::is_void<T>{}, std::forward<Func>(func));
}

/// call_from_state<T, Func, State>()
template<typename T, typename Func, typename State>
inline add_future_t<T> call_from_state(std::false_type, Func&& func, State&& state)
{
    return call_function<T>(std::is_void<T>{}, std::forward<Func>(func), std::forward<State>(state).get_value());
}

/// call_state<T, Func, State>()
template<typename T, typename Func, typename State>
inline add_future_t<T> call_state(Func&& func, State&& state)
{
    return call_from_state<T>(std::is_void<typename State::type>{}, std::forward<Func>(func), std::forward<State>(state));
}

/// call_future<T, Func, Future>
template<typename T, typename Func, typename Future>
inline add_future_t<T> call_future(Func&& func, Future&& fut) noexcept
{
    return call_function<T>(std::is_void<T>{}, std::forward<Func>(func), std::forward<Future>(fut));
}
}

/// future_state<void>::forward_to()
inline void future_state<void>::forward_to(promise<void>& pr) noexcept
{
    std::atomic_thread_fence(std::memory_order_acquire);
    assert(available());
    if (_u.st == state::exception_min)
    {
        pr.set_urgent_exception(std::move(_u.ex));
    }
    else
    {
        pr.set_urgent_value();
    }
    _u.st = state::invalid;
    std::atomic_thread_fence(std::memory_order_release);
}

/// make_exception_future()
template <typename T = rest::rest_reply>
inline future<T> make_exception_future(aegis::error ec)
{
    return aegis::make_exception_future<T>(std::make_exception_ptr(aegis::exception(make_error_code(ec))));
}

}
