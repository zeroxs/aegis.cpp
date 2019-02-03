//
// ratelimit.hpp
// *************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/rest/rest_controller.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/ratelimit/bucket.hpp"
#include "aegis/futures.hpp"
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
public:
    /// Construct a ratelimit_mgr object for managing the bucket factories
    /**
     * @param call Function pointer to the REST API function
     */
    explicit ratelimit_mgr(rest_call call, asio::io_context & _io, core * _b)
        : global_limit(0)
        , _call(call)
        , _io_context(_io)
        , _bot(_b)
    {

    }

    ratelimit_mgr(const ratelimit_mgr &) = delete;
    ratelimit_mgr(ratelimit_mgr &&) = delete;
    ratelimit_mgr & operator=(const ratelimit_mgr &) = delete;

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
    bucket & get_bucket(const std::string & path) noexcept
    {
        // look for existing bucket
        auto bkt = _buckets.find(path);
        if (bkt != _buckets.end())
                return *bkt->second;// found

        // create new bucket and return
        return *_buckets.emplace(path, std::make_unique<bucket>(_call, _io_context, global_limit)).first->second;
    }

    template<typename ResultType, typename V = std::enable_if_t<!std::is_same<ResultType, rest::rest_reply>::value>>
    aegis::future<ResultType> post_task(rest::request_params params)
    {
        return _bot->async([=]() -> ResultType
        {
            auto & bkt = get_bucket(params.path);
            auto res = bkt.perform(params);
            return res.content.empty() ? ResultType(_bot) : ResultType(res.content, _bot);
        });
    }

    aegis::future<rest::rest_reply> post_task(rest::request_params params)
    {
        return _bot->async([=]() -> rest::rest_reply
        {
            auto & bkt = get_bucket(params.path);
            return bkt.perform(params);
        });
    }

    template<typename ResultType, typename V = std::enable_if_t<!std::is_same<ResultType, rest::rest_reply>::value>>
    aegis::future<ResultType> post_task(std::string _bucket, rest::request_params params)
    {
        return _bot->async([=]() -> ResultType
        {
            auto & bkt = get_bucket(_bucket);
            auto res = bkt.perform(params);
            return res.content.empty() ? ResultType(_bot) : ResultType(res.content, _bot);
        });
    }

    aegis::future<rest::rest_reply> post_task(std::string _bucket, rest::request_params params)
    {
        return _bot->async([=]() -> rest::rest_reply
        {
            auto & bkt = get_bucket(_bucket);
            return bkt.perform(params);
        });
    }

private:
    friend class bucket;

    std::atomic<int64_t> global_limit; /**< Timestamp in seconds when global ratelimit expires */

    std::unordered_map<std::string, std::unique_ptr<bucket>> _buckets;
    rest_call _call;
    asio::io_context & _io_context;
    core * _bot;
};

}

}
