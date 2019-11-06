//
// context.hpp
// ***********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include <asio/io_context.hpp>
#include <asio/executor_work_guard.hpp>
#include <stdint.h>
#include <thread>
#include <functional>

namespace aegis
{

/// Type of a work guard executor for keeping Asio services alive
using asio_exec = asio::executor_work_guard<asio::io_context::executor_type>;

/// Type of a shared pointer to an io_context work object
using work_ptr = std::unique_ptr<asio_exec>;

enum context_type
{
    RoundRobin,
    Modulus,
    LoadBalance
};

struct thread_state
{
    std::thread thd;
    bool active = false;
    std::chrono::steady_clock::time_point start_time;
    std::function<void(void)> fn;
};

/// Utility class for thread context control
class context
{
public:
    AEGIS_DECL context(int64_t _core, int64_t _mask);
    AEGIS_DECL ~context();

    AEGIS_DECL void _thread_track(thread_state * t_state);
    AEGIS_DECL std::size_t add_run_thread() noexcept;
    AEGIS_DECL void reduce_threads(std::size_t count) noexcept;

    AEGIS_DECL asio::io_context & get_io_context();

    int64_t cpu_core() { return _cpu_core; }
    int64_t thread_mask() { return _cpu_core; }

    AEGIS_DECL void work();

    const std::vector<std::unique_ptr<thread_state>> & get_threads() const noexcept
    {
        return _threads;
    }

private:
    std::shared_ptr<asio::io_context> _io_context;
    std::vector<std::unique_ptr<thread_state>> _threads;
    int64_t _thread_mask = 0;
    int64_t _cpu_core = 0;
    work_ptr wrk;
};

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/impl/context.cpp"
#endif
