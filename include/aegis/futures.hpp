//
// futures.hpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 
// Adapted from https://github.com/scylladb/seastar to support asio scheduling

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include <condition_variable>
#include <stdexcept>
#include <type_traits>
#include <memory>
#include <cassert>
#include <chrono>
#include <thread>
#include <iostream>
#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <asio/bind_executor.hpp>
#include <asio/executor_work_guard.hpp>

using namespace std::literals::chrono_literals;


namespace aegis
{
/// Type of a work guard executor for keeping Asio services alive
using asio_exec = asio::executor_work_guard<asio::io_context::executor_type>;

/// Type of a shared pointer to an io_context work object
using work_ptr = std::unique_ptr<asio_exec>;

#if !defined(AEGIS_CXX17)
template<class T>
struct V
{
    static core * bot;
    static std::unique_ptr<asio::io_context> _io_context;
    static std::vector<std::thread> threads;
    static work_ptr wrk;
    static std::condition_variable cv;
    static void shutdown()
    {
        cv.notify_all();
    }
};
#endif

#if defined(AEGIS_CXX17)
struct internal
{
    static inline core * bot = nullptr;
    static inline std::unique_ptr<asio::io_context> _io_context = nullptr;
    static inline std::vector<std::thread> threads;
    static inline work_ptr wrk = nullptr;
    static inline std::condition_variable cv;
    static inline void shutdown()
    {
        cv.notify_all();
    }
};
#endif

#if !defined(AEGIS_CXX17)
template<class T>
core * V<T>::bot = nullptr;
template<class T>
std::unique_ptr<asio::io_context> V<T>::_io_context = nullptr;
template<class T>
std::vector<std::thread> V<T>::threads;
template<class T>
work_ptr V<T>::wrk = nullptr;
template<class T>
std::condition_variable V<T>::cv;

using internal = V<void>;
#endif

template <class T>
class promise;

template <class T>
class future;

template <typename T>
class shared_future;

template <typename T, typename... A>
future<T> make_ready_future(A&&... value);

template <typename T>
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
template<typename T, typename Func, typename State>
add_future_t<T> call_state(Func&& func, State&& state);

template<typename T, typename Func, typename Future>
add_future_t<T> call_future(Func&& func, Future&& fut) noexcept;
}

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
    } _state = state::future;
    union any
    {
        any() {}
        ~any() {}
        T value;
        std::exception_ptr ex;
    } _u;
    future_state() noexcept {}
    future_state(future_state&& x) noexcept
        : _state(x._state)
    {
        switch (_state)
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
        x._state = state::invalid;
    }
    ~future_state() noexcept
    {
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
    }
    future_state& operator=(future_state&& x) noexcept
    {
        if (this != &x)
        {
            this->~future_state();
            new (this) future_state(std::move(x));
        }
        return *this;
    }
    bool available() const noexcept
    {
        return _state == state::result || _state == state::exception;
    }
    bool failed() const noexcept
    {
        return _state == state::exception;
    }
    void wait();
    void set(const T& value) noexcept
    {
        assert(_state == state::future);
        new (&_u.value) T(value);
        _state = state::result;
    }
    void set(T&& value) noexcept
    {
        assert(_state == state::future);
        new (&_u.value) T(std::move(value));
        _state = state::result;
    }
    template <typename... A>
    void set(A&&... a)
    {
        assert(_state == state::future);
        new (&_u.value) T(std::forward<A>(a)...);
        _state = state::result;
    }
    void set_exception(std::exception_ptr ex) noexcept
    {
        assert(_state == state::future);
        new (&_u.ex) std::exception_ptr(ex);
        _state = state::exception;
    }
    std::exception_ptr get_exception() && noexcept
    {
        assert(_state == state::exception);
        _state = state::invalid;
        auto ex = std::move(_u.ex);
        _u.ex.~exception_ptr();
        return ex;
    }
    std::exception_ptr get_exception() const& noexcept
    {
        assert(_state == state::exception);
        return _u.ex;
    }
    auto get_value() && noexcept
    {
        assert(_state == state::result);
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
        assert(_state != state::future);
        if (_state == state::exception)
        {
            _state = state::invalid;
            auto ex = std::move(_u.ex);
            _u.ex.~exception_ptr();
            std::rethrow_exception(std::move(ex));
        }
        return std::move(_u.value);
    }
    T get() const&
    {
        assert(_state != state::future);
        if (_state == state::exception)
        {
            std::rethrow_exception(_u.ex);
        }
        return _u.value;
    }
    void ignore() noexcept
    {
        assert(_state != state::future);
        this->~future_state();
        _state = state::invalid;
    }
    void forward_to(promise<T>& pr) noexcept
    {
        assert(_state != state::future);
        if (_state == state::exception)
        {
            pr.set_urgent_exception(std::move(_u.ex));
            _u.ex.~exception_ptr();
        }
        else
        {
            pr.set_urgent_value(std::move(_u.value));
            _u.value.~T();
        }
        _state = state::invalid;
    }
};

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
        state st;
        std::exception_ptr ex;
    } _u;
    future_state() noexcept {}
    future_state(future_state&& x) noexcept
    {
        if (x._u.st < state::exception_min)
        {
            _u.st = x._u.st;
        }
        else
        {
            new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
            x._u.ex.~exception_ptr();
        }
        x._u.st = state::invalid;
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
        if (this != &x)
        {
            this->~future_state();
            new (this) future_state(std::move(x));
        }
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
        _u.st = state::result;
    }
    void set_exception(std::exception_ptr ex) noexcept
    {
        assert(_u.st == state::future);
        new (&_u.ex) std::exception_ptr(ex);
        assert(_u.st >= state::exception_min);
    }
    void get() &&
    {
        assert(available());
        if (_u.st >= state::exception_min)
        {
            std::rethrow_exception(std::move(_u.ex));
        }
    }
    void get() const&
    {
        assert(available());
        if (_u.st >= state::exception_min)
        {
            std::rethrow_exception(_u.ex);
        }
    }
    void ignore() noexcept
    {
        assert(available());
        this->~future_state();
        _u.st = state::invalid;
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

class task
{
public:
    virtual ~task() = default;
    virtual void run() noexcept = 0;
    virtual void run_and_dispose() noexcept = 0;
};

template <typename T>
class continuation_base : public task
{
protected:
    future_state<T> _state;
    using future_type = future<T>;
    using promise_type = promise<T>;
public:
    continuation_base() = default;
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

template <>
class continuation_base<void> : public task
{
protected:
    future_state<void> _state;
    using future_type = future<void>;
    using promise_type = promise<void>;
public:
    continuation_base() = default;
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

template <typename Func, typename T>
struct continuation final : continuation_base<T>
{
    continuation(Func&& func, future_state<T>&& state) : continuation_base<T>(std::move(state)), _func(std::move(func)) {}
    continuation(Func&& func) : _func(std::move(func)) {}
    virtual void run_and_dispose() noexcept override
    {
        _func(std::move(this->_state));
        delete this;
    }
    virtual void run() noexcept override
    {
        _func(std::move(this->_state));
    }
    Func _func;
};
using task_ptr = std::unique_ptr<task>;


template <typename T>
class promise
{
    enum class urgent { no, yes };
    future<T>* _future = nullptr;
    future_state<T> _local_state;
    future_state<T>* _state;
    std::unique_ptr<continuation_base<T>> _task;
    static constexpr bool copy_noexcept = future_state<T>::copy_noexcept;
public:
    promise() noexcept : _state(&_local_state) {}

    promise(promise&& x) noexcept : _future(x._future), _state(x._state), _task(std::move(x._task))
    {
        if (_state == &x._local_state)
        {
            _state = &_local_state;
            _local_state = std::move(x._local_state);
        }
        x._future = nullptr;
        x._state = nullptr;
        migrated();
    }
    promise(const promise&) = delete;
    ~promise() noexcept
    {
        abandoned();
    }
    promise& operator=(promise&& x) noexcept
    {
        if (this != &x)
        {
            this->~promise();
            new (this) promise(std::move(x));
        }
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
        auto tws = std::make_unique<continuation<Func, T>>(std::move(func));
        _state = &tws->_state;
        _task = std::move(tws);
    }
    template<urgent Urgent>
    void make_ready() noexcept;
    void migrated() noexcept;
    void abandoned() noexcept;

    template <typename U>
    friend class future;

    friend struct future_state<T>;
};

template <typename T>
class future
{
    promise<T>* _promise;
    future_state<T> _local_state;
    static constexpr bool copy_noexcept = future_state<T>::copy_noexcept;
private:
    future(promise<T>* pr) noexcept : _promise(pr)
    {
        _promise->_future = this;
    }
    template <typename... A>
    future(ready_future_marker, A&&... a) : _promise(nullptr)
    {
        _local_state.set(std::forward<A>(a)...);
    }
    future(exception_future_marker, std::exception_ptr ex) noexcept : _promise(nullptr)
    {
        _local_state.set_exception(std::move(ex));
    }
    explicit future(future_state<T>&& state) noexcept
        : _promise(nullptr), _local_state(std::move(state))
    {
    }
    future_state<T>* state() noexcept
    {
        return (_promise && _promise->_state) ? _promise->_state : &_local_state;
    }
    const future_state<T>* state() const noexcept
    {
        return (_promise && _promise->_state) ? _promise->_state : &_local_state;
    }
    template <typename Func>
    void schedule(Func&& func)
    {
        auto t_st = state();
        if (t_st->available())
        {
            //asio::post(*aegis::internal::_io_context, [func = std::move(func), state = std::move(*t_st)]() mutable
            {
                //func(state);
                func(std::move(*t_st));
            }//);
        }
        else
        {
            assert(_promise);
            _promise->schedule(std::move(func));
            _promise->_future = nullptr;
            _promise = nullptr;
        }
    }

    future_state<T> get_available_state() noexcept
    {
        auto st = state();
        if (_promise)
        {
            _promise->_future = nullptr;
            _promise = nullptr;
        }
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
    future(future&& x) noexcept : _promise(x._promise)
    {
        if (!_promise)
        {
            _local_state = std::move(x._local_state);
        }
        x._promise = nullptr;
        if (_promise)
        {
            _promise->_future = this;
        }
    }
    future(const future&) = delete;
    future& operator=(future&& x) noexcept
    {
        if (this != &x)
        {
            this->~future();
            new (this) future(std::move(x));
        }
        return *this;
    }
    void operator=(const future&) = delete;
    ~future()
    {
        if (_promise)
        {
            _promise->_future = nullptr;
        }
        if (failed())
        {
        }
    }

    auto get()
    {
        if (!state()->available())
        {
            do_wait();
        }
        return get_available_state().get();
    }

    std::exception_ptr get_exception()
    {
        return get_available_state().get_exception();
    }

    void wait() noexcept
    {
        if (!state()->available())
        {
            do_wait();
        }
    }
private:
    void do_wait() noexcept
    {
        // fake wait
        // maybe execute something in the asio queue?

        while (!available())
            std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }

public:
    bool available() const noexcept
    {
        assert(state());
        return state()->available();
    }

    bool failed() noexcept
    {
        return state()->failed();
    }

    template <typename Func, typename Result = result_of_t<Func, T>>
    add_future_t<Result> then(Func&& func) noexcept
    {
        using inner_type = remove_future_t<Result>;
        if (available())
        {
            if (failed())
            {
                return make_exception_future<inner_type>(get_available_state().get_exception());
            }
            else
            {
                return detail::call_state<inner_type>(std::forward<Func>(func), get_available_state());
            }
        }
        promise<inner_type> pr;
        auto fut = pr.get_future();
        try
        {
            this->schedule([pr = std::move(pr), func = std::forward<Func>(func)](auto&& state) mutable {
                if (state.failed())
                {
                    pr.set_exception(std::move(state).get_exception());
                }
                else
                {
                    detail::call_state<inner_type>(std::forward<Func>(func), std::move(state)).forward_to(std::move(pr));
                }
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
        if (available())
        {
            return detail::call_future<inner_type>(std::forward<Func>(func), future(get_available_state()));
        }
        promise<inner_type> pr;
        auto fut = pr.get_future();
        try
        {
            this->schedule([pr = std::move(pr), func = std::forward<Func>(func)](auto&& state) mutable {
                detail::call_future<inner_type>(std::forward<Func>(func), future(std::move(state))).forward_to(std::move(pr));
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
    friend future<U> make_exception_future(std::exception_ptr ex) noexcept;
};

template <typename T>
inline future<T> promise<T>::get_future() noexcept
{
    assert(!_future && _state && !_task);
    return future<T>(this);
}

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
            //asio::post(*aegis::internal::_io_context, [_task = std::move(_task)]
            {
                _task->run();
            }//);
        }
    }
}

template <typename T>
inline void promise<T>::migrated() noexcept
{
    if (_future)
    {
        _future->_promise = this;
    }
}

template <typename T>
inline void promise<T>::abandoned() noexcept
{
    if (_future)
    {
        assert(_state);
        assert(_state->available() || !_task);
        _future->_local_state = std::move(*_state);
        _future->_promise = nullptr;
    }
    else if (_state && _state->failed())
    {
    }
}

template <typename T, typename... A>
inline future<T> make_ready_future(A&&... value)
{
    return { ready_future_marker(), std::forward<A>(value)... };
}

template <typename T>
inline future<T> make_exception_future(std::exception_ptr ex) noexcept
{
    return { exception_future_marker(), std::move(ex) };
}


namespace detail
{
template<typename T>
inline add_future_t<T> to_future(T&& value)
{
    return make_ready_future<T>(std::forward<T>(value));
}

template<typename T>
inline future<T> to_future(future<T>&& fut)
{
    return std::move(fut);
}

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

template<typename T, typename Func, typename State>
inline add_future_t<T> call_from_state(std::true_type, Func&& func, State&&)
{
    return call_function<T>(std::is_void<T>{}, std::forward<Func>(func));
}

template<typename T, typename Func, typename State>
inline add_future_t<T> call_from_state(std::false_type, Func&& func, State&& state)
{
    return call_function<T>(std::is_void<T>{}, std::forward<Func>(func), std::forward<State>(state).get_value());
}

template<typename T, typename Func, typename State>
inline add_future_t<T> call_state(Func&& func, State&& state)
{
    return call_from_state<T>(std::is_void<typename State::type>{}, std::forward<Func>(func), std::forward<State>(state));
}

template<typename T, typename Func, typename Future>
inline add_future_t<T> call_future(Func&& func, Future&& fut) noexcept
{
    return call_function<T>(std::is_void<T>{}, std::forward<Func>(func), std::forward<Future>(fut));
}
}

inline void future_state<void>::forward_to(promise<void>& pr) noexcept
{
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
}

template<typename Duration>
inline aegis::future<void> sleep(const Duration& dur)
{
    aegis::promise<void> pr;
    auto fut = pr.get_future();
    std::thread t1([pr = std::move(pr), dur = dur]() mutable {
        std::this_thread::sleep_for(dur);
        pr.set_value();
    });
    t1.detach();
    return fut;
}

template<typename T, typename V = std::result_of_t<T()>, typename = std::enable_if_t<!std::is_void<V>::value>>
aegis::future<V> async(T f)
{
    aegis::promise<V> pr;
    auto fut = pr.get_future();

    aegis::promise<void> pc;
    auto fut_c = pc.get_future();

    asio::post(*aegis::internal::_io_context, [pc = std::move(pc), pr = std::move(pr), f = std::move(f)]() mutable
    {
        pc.set_value();
        pr.set_value(f());
    });
    fut_c.get();
    return fut;
}

template<typename T, typename V = std::enable_if_t<std::is_void<std::result_of_t<T()>>::value>>
aegis::future<V> async(T f)
{
    aegis::promise<V> pr;
    auto fut = pr.get_future();

    aegis::promise<void> pc;
    auto fut_c = pc.get_future();

    asio::post(*aegis::internal::_io_context, [pc = std::move(pc), pr = std::move(pr), f = std::move(f)]() mutable
    {
        pc.set_value();
        f();
        pr.set_value();
    });
    fut_c.get();
    return fut;
}

}
