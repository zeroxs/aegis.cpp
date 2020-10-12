//
// bucket.hpp
// **********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/rest/rest_controller.hpp"
#include "aegis/snowflake.hpp"
#include <mutex>
#include <future>
#include <chrono>
#include <queue>
#include <atomic>
#include <spdlog/spdlog.h>

namespace aegis
{

    using rest_call = std::function<aegis::rest::rest_reply(aegis::rest::request_params)>;

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
        class bucket
        {
            asio::io_context& _io_context;
            std::atomic<int64_t>& _global_limit;
            std::atomic<int64_t> _time_delay;
        public:
            bool ignore_rates = false;
            std::mutex m;
            std::function<aegis::rest::rest_reply(aegis::rest::request_params)>& _call;
            std::queue<std::tuple<std::string, std::string, std::string, std::function<void(aegis::rest::rest_reply)>>> _queue;
            int32_t reset_bypass = 0;
            std::atomic<int64_t> limit; /**< Rate limit current endpoint call limit */
            std::atomic<int64_t> remaining; /**< Rate limit remaining count */
            std::atomic<int64_t> reset; /**< Rate limit reset time */

            /**
             * Construct a bucket object for tracking ratelimits per major parameter of the REST API (guild/channel/emoji)
             */
            bucket(std::function<aegis::rest::rest_reply(aegis::rest::request_params)>& call, asio::io_context& _io_context, std::atomic<int64_t>& global_limit)
                : limit(0)
                , remaining(1)
                , reset(0)
                , _call(call)
                , _io_context(_io_context)
                , _global_limit(global_limit)
            {

            }


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
                if (limit == 0)
                    return true;
                if (remaining > 0)
                    return true;
                int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                if (time < (reset/* + _time_delay*/))
                    return false;
                return true;
            }

            aegis::rest::rest_reply perform(aegis::rest::request_params params)
            {
                std::lock_guard<std::mutex> lock(m);
                while (!can_perform())
                {
                    //TODO: find a better solution - wrap asio execution handling, poll ratelimit object to track ordering and execution
                    // not an ideal scenario, but by current design rescheduling a message that would be ratelimited
                    // would cause out of order messages
                    std::chrono::milliseconds waitfor = std::chrono::milliseconds((reset.load(std::memory_order_relaxed)
                        - std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())/* + _time_delay*/);
                    spdlog::get("aegis")->debug("Ratelimit almost hit: {}({}) - waiting {}ms", aegis::rest::rest_controller::get_method(params.method), params.path, waitfor.count());
                    std::this_thread::sleep_for(waitfor);
                }
                aegis::rest::rest_reply reply(_call(params));
                auto _now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
                if (reply.reply_code == 429)
                {
                    if (reset_bypass)
                    {
                        spdlog::get("aegis")->warn("Ratelimit hit - retrying in {}ms...", reset_bypass);
                        std::this_thread::sleep_for(std::chrono::milliseconds(reset_bypass));
                    }
                    else
                    {
                        spdlog::get("aegis")->warn("Ratelimit hit - retrying in {}s...", reply.retry / 1000);
                        std::this_thread::sleep_for(std::chrono::milliseconds(reply.retry));
                    }
                    reply = _call(params);
                    if (reply.reply_code == 429)
                        spdlog::get("aegis")->error("Ratelimit hit twice. Giving up.");
                }

                limit.store(reply.limit, std::memory_order_relaxed);
                remaining.store(reply.remaining, std::memory_order_relaxed);
                auto http_date = std::chrono::duration_cast<std::chrono::milliseconds>(reply.date.time_since_epoch());
                if (reset_bypass)
                    reset.store((_now + std::chrono::milliseconds(reset_bypass)).count(), std::memory_order_relaxed);
                else
                {
                    reset.store(reply.reset * 1000, std::memory_order_relaxed);
                    _time_delay = (http_date - _now).count();
                }
                return reply;
            }

        };

    }

}
