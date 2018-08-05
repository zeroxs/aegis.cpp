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
#include <chrono>
#include <functional>
#include <string>
#include <queue>
#include <map>
#include <memory>
#include <atomic>
#include "aegis/rest/rest_reply.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/ratelimit/bucket.hpp"
#include <future>
#include <thread>
#include <mutex>

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
template<typename Callable, typename Result>
class ratelimit_mgr
{
public:
    /// Construct a ratelimit_mgr object for managing the bucket factories
    /**
     * @param call Function pointer to the REST API function
     */
    explicit ratelimit_mgr(Callable call, asio::io_context & _io)
        : global_limit(0)
        , _call(call)
        , _io_context(_io)
    {

    }

    ratelimit_mgr(const ratelimit_mgr &) = delete;
    ratelimit_mgr(ratelimit_mgr &&) = delete;
    ratelimit_mgr & operator=(const ratelimit_mgr &) = delete;

    /// Check if globally ratelimited
    /**
     * @returns true if globally ratelimited
     */
    bool is_global() const AEGIS_NOEXCEPT
    {
        return global_limit > 0;
    }

    /// Get a bucket object
    /**
    * @see bucket
    * @param id Snowflake of bucket object
    * @returns Reference to a bucket object
    */
    bucket<Callable, Result> & get_bucket(const std::string & path) AEGIS_NOEXCEPT
    {
        // look for existing bucket
        auto bkt = _buckets.find(path);
        if (bkt != _buckets.end())
                return *bkt->second;// found

        // create new bucket and return
        return *_buckets.emplace(path, std::make_unique<bucket<Callable, Result>>(_call, _io_context, global_limit)).first->second;
    }

    std::future<Result> post_task(const std::string & path, const std::string & method = "POST", const std::string & obj = "", const std::string & host = "")
    {
        auto & bkt = get_bucket(path);
        using result = asio::async_result<asio::use_future_t<>, void(Result)>;
        using handler = typename result::completion_handler_type;

        handler exec(asio::use_future);
        result ret(exec);

        asio::post(_io_context, [=, &bkt]() mutable
        {
            exec(bkt.perform(path, obj, method, host));
        });
        return ret.get();
    }

private:
    friend class bucket<Callable, Result>;

    std::atomic<int64_t> global_limit; /**< Timestamp in seconds when global ratelimit expires */

    std::unordered_map<std::string, std::unique_ptr<bucket<Callable, Result>>> _buckets;
    Callable _call;
    asio::io_context & _io_context;
};

}

}
