//
// ratelimit.hpp
// *************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/rest/rest_controller.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/ratelimit/bucket.hpp"
//#include "aegis/futures.hpp"
#include "lsw/future_mod.h"
#include "aegis/core.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <queue>
#include <map>
#include <atomic>
#include <mutex>
#include <type_traits>
namespace aegis
{

    namespace ratelimit
    {

        using namespace std::chrono;

        /// Factory class for managing ratelimit bucket factory objects
        /**
         * Ratelimit manager class for tracking and handling ratelimit checks and dispatches
         * Different callables and results require different instances. Global limit is not
         * shared between instances.
         */
        class ratelimit_mgr
        {
            friend class bucket;

            std::atomic<int64_t> global_limit; /**< Timestamp in seconds when global ratelimit expires */

            std::unordered_map<std::string, std::unique_ptr<aegis::ratelimit::bucket>> _buckets;
            aegis::rest_call _call;
            asio::io_context& _io_context;
            core* _bot;
        public:
            /// Construct a ratelimit_mgr object for managing the bucket factories
            /**
             * @param call Function pointer to the REST API function
             */
            explicit ratelimit_mgr(aegis::rest_call call, asio::io_context& _io, core* _b)
                : global_limit(0)
                , _call(call)
                , _io_context(_io)
                , _bot(_b)
            {

            }

            ratelimit_mgr(const ratelimit_mgr&) = delete;
            ratelimit_mgr(ratelimit_mgr&&) = delete;
            ratelimit_mgr& operator=(const ratelimit_mgr&) = delete;

            /// Check if globally ratelimited
            /**
             * @returns true if globally ratelimited
             */
            bool is_global() const noexcept
            {
                return global_limit > 0;
            }

            /// Get a bucket object
            /**
             * @see bucket
             * @param id Snowflake of bucket object
             * @returns Reference to a bucket object
             */
            aegis::ratelimit::bucket& get_bucket(const std::string& path) noexcept
            {
                // look for existing bucket
                auto bkt = _buckets.find(path);
                if (bkt != _buckets.end())
                    return *bkt->second;// found

            // create new bucket and return
                return *_buckets.emplace(path, std::make_unique<aegis::ratelimit::bucket>(_call, _io_context, global_limit)).first->second;
            }

            template<typename ResultType, typename V = std::enable_if_t<!std::is_same<ResultType, aegis::rest::rest_reply>::value>>
            LSW::v5::Tools::Future<ResultType> post_task(aegis::rest::request_params params) noexcept
            {
                return _bot->async([=]() -> ResultType
                    {
                        auto& bkt = get_bucket(params.path);
                        auto res = bkt.perform(params);
                        if (res.reply_code < rest::ok || res.reply_code >= rest::multiple_choices)//error
                            throw aegis::exception(fmt::format("REST Reply Code: {}", static_cast<int>(res.reply_code)), bad_request);
                        return res.content.empty() ? ResultType(_bot) : ResultType(res.content, _bot);
                    });
            }

            LSW::v5::Tools::Future<aegis::rest::rest_reply> post_task(aegis::rest::request_params params) noexcept
            {
                return _bot->async([=]() -> aegis::rest::rest_reply
                    {
                        auto& bkt = get_bucket(params.path);
                        return bkt.perform(params);
                    });
            }

            template<typename ResultType, typename V = std::enable_if_t<!std::is_same<ResultType, aegis::rest::rest_reply>::value>>
            LSW::v5::Tools::Future<ResultType> post_task(std::string _bucket, aegis::rest::request_params params) noexcept
            {
                return _bot->async([=]() -> ResultType
                    {
                        auto& bkt = get_bucket(_bucket);
                        auto res = bkt.perform(params);
                        if (res.reply_code < rest::ok || res.reply_code >= rest::multiple_choices)//error
                            throw aegis::exception(fmt::format("REST Reply Code: {}", static_cast<int>(res.reply_code)), bad_request);
                        return res.content.empty() ? ResultType(_bot) : ResultType(res.content, _bot);
                    });
            }

            LSW::v5::Tools::Future<aegis::rest::rest_reply> post_task(std::string _bucket, aegis::rest::request_params params) noexcept
            {
                return _bot->async([=]() -> aegis::rest::rest_reply
                    {
                        auto& bkt = get_bucket(_bucket);
                        return bkt.perform(params);
                    });
            }
        };

    }

}
