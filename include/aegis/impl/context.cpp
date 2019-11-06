//
// context.cpp
// ***********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/context.hpp"
#include <stdint.h>
#include <iostream>
#include <asio/post.hpp>

#if defined(WIN32)
//#include <WinBase.h>
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
//#include <pthread.h>
#endif

namespace aegis
{

using asio_return_type = asio::io_context::count_type(asio::io_context::*)();

AEGIS_DECL context::context(int64_t _core, int64_t _mask)
    : _thread_mask(_mask)
    , _cpu_core(_core)
{
    _io_context = std::make_shared<asio::io_context>();
}

AEGIS_DECL context::~context()
{
    wrk->reset();
    _io_context->stop();
    for (auto & t : _threads)
        t->thd.join();
}

AEGIS_DECL void context::_thread_track(thread_state * t_state)
{
    try
    {
#if defined(WIN32)
        HANDLE threadh = GetCurrentThread();
        if (SetThreadAffinityMask(threadh, 1LL << _thread_mask) == FALSE)
        {
            throw 1; //throw stuff at the wall
        }
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(_thread_mask, &cpuset);
        int rc = pthread_setaffinity_np(t_state->thd.native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            throw std::runtime_error("Error calling pthread_setaffinity_np");
        }
#endif

        //        while (t_state->active)
        t_state->fn();
    }
    catch (std::exception & e)
    {
        //log->critical("Scheduler thread exit due to exception: {}", e.what());
        std::cout << "Thread exception : " << e.what() << std::endl;
    }
    catch (int)
    {
        std::cout << "Thread exited" << std::endl;
    }
    t_state->active = false;
}

AEGIS_DECL std::size_t context::add_run_thread() noexcept
{
    std::unique_ptr<thread_state> t = std::make_unique<thread_state>();
    t->active = true;
    t->start_time = std::chrono::steady_clock::now();
    t->fn = std::bind(static_cast<asio_return_type>(&asio::io_context::run), _io_context);
    t->thd = std::thread(std::bind(&context::_thread_track, this, t.get()));
    _threads.emplace_back(std::move(t));
    return _threads.size();
}

AEGIS_DECL void context::reduce_threads(std::size_t count) noexcept
{
    for (uint32_t i = 0; i < count; ++i)
        asio::post(*_io_context, [] { throw 1; });
}

AEGIS_DECL asio::io_context & context::get_io_context()
{
    return *_io_context;
}

AEGIS_DECL void context::work()
{
    wrk = std::make_unique<asio_exec>(asio::make_work_guard(*_io_context));
}

}
