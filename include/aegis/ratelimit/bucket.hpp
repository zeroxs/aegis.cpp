//
// bucket.hpp
// **********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include <mutex>
#include <future>
#include <chrono>
#include <functional>
#include <string>
#include <queue>
#include <atomic>
#include "aegis/rest/rest_reply.hpp"
#include "aegis/snowflake.hpp"
#include <asio/io_context.hpp>
#include <asio/use_future.hpp>
#include <asio/post.hpp>
#include "aegis/rest/rest_controller.hpp"

namespace aegis
{

using rest_call = std::function<rest::rest_reply(rest::request_params)>;

namespace ratelimit
{

using namespace std::chrono;

/**
* Major parameter of REST API access
* Emoji is a partial major parameter and is ratelimited per guild across all members
*/
enum bucket_type
{
    Guild = 0,
    Channel = 1,
    Emoji = 2
};

/// Buckets store ratelimit data per major parameter
/**
 * Bucket class for tracking the ratelimits per snowflake per major parameter.
 * Each bucket tracks a single major parameter and a single snowflake
 * Current major parameters are GUILD, CHANNEL, and EMOJI
 */
template<typename Callable, typename Result>
class bucket
{
public:
    /**
     * Construct a bucket object for tracking ratelimits per major parameter of the REST API (guild/channel/emoji)
     */
    bucket(Callable & call, asio::io_context & _io_context, std::atomic<int64_t> & global_limit)
        : limit(0)
        , remaining(1)
        , reset(0)
        , _call(call)
        , _io_context(_io_context)
        , _global_limit(global_limit)
    {

    }

    std::atomic<int64_t> limit; /**< Rate limit current endpoint call limit */
    std::atomic<int64_t> remaining; /**< Rate limit remaining count */
    std::atomic<int64_t> reset; /**< Rate limit reset time */

    /// Check if globally ratelimited
    /**
     * @returns true if globally ratelimited
     */
    bool is_global() const noexcept
    {
        return _global_limit > 0;
    }


    /// Check if bucket can send a message without hitting the ratelimit
    /**
     * @returns true if bucket ratelimits permit a message to be sent
     */
    bool can_perform() const noexcept
    {
        if (ignore_rates)
            return true;
        if (limit.load(std::memory_order_relaxed) == 0)
            return true;
        if (remaining.load(std::memory_order_relaxed) > 0)
            return true;
        int64_t time = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        if (time < reset.load(std::memory_order_relaxed))
            return false;
        return true;
    }

    Result perform(rest::request_params params)
    {
        std::lock_guard<std::mutex> lock(m);
        while (!can_perform())
        {
            //TODO: find a better solution - wrap asio execution handling, poll ratelimit object to track ordering and execution
            // not an ideal scenario, but by current design rescheduling a message that would be ratelimited
            // would cause out of order messages
            std::this_thread::sleep_for(seconds((reset.load(std::memory_order_relaxed)
                                                 - std::chrono::duration_cast<seconds>(std::chrono::system_clock::now().time_since_epoch()).count()) + 1));
        }
        Result reply(_call(params));
        limit.store(reply.limit, std::memory_order_relaxed);
        remaining.store(reply.remaining, std::memory_order_relaxed);
        reset.store(reply.reset, std::memory_order_relaxed);
        return reply;
    }

    std::future<Result> post_task(rest::request_params params)
    {
        using result = asio::async_result<asio::use_future_t<>, void(Result)>;
        using handler = typename result::completion_handler_type;

        handler exec(asio::use_future);
        result ret(exec);

        asio::post(_io_context, [=]() mutable
        {
            exec(perform(params));
        });
        return ret.get();
    }

    bool ignore_rates = false;
    std::mutex m;
    Callable & _call;
    std::queue<std::tuple<std::string, std::string, std::string, std::function<void(Result)>>> _queue;

private:
    asio::io_context & _io_context;
    std::atomic<int64_t> & _global_limit;
};

}

}
