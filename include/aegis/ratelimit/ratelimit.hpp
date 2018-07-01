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
#if defined(AEGIS_HAS_STD_OPTIONAL)
#include <optional>
#else
#include "aegis/optional.hpp"
namespace std
{
using std::experimental::optional;
}
#endif

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
    AEGIS_DECL bool is_global() const AEGIS_NOEXCEPT
    {
        return global_limit > 0;
    }

    /// Get a bucket object
    /**
    * @see bucket
    * @param id Snowflake of bucket object
    * @returns Reference to a bucket object
    */
    AEGIS_DECL bucket<Callable, Result> & get_bucket(const bucket_type type, const snowflake id) AEGIS_NOEXCEPT
    {
        // look for existing bucket set by type
        auto bkt_type = _buckets.find(type);
        if (bkt_type != _buckets.end())
        {
            // found. look for existing bucket
            auto bkt = bkt_type->second.find(id);
            if (bkt != bkt_type->second.end())
                return *bkt->second;// found

            // create new bucket and return
            return *bkt_type->second.emplace(id, std::make_unique<bucket<Callable, Result>>(_call, _io_context, global_limit)).first->second;
        }

        using bkt_map = std::map<aegis::snowflake, std::unique_ptr<aegis::ratelimit::bucket<rest_call, aegis::rest::rest_reply>>>;
        // create new bucket set
        auto & bkt = _buckets.emplace(type, bkt_map()).first->second;
        // create new bucket and return
        return *bkt.emplace(id, std::make_unique<bucket<Callable, Result>>(_call, _io_context, global_limit)).first->second;
    }

private:
    friend class bucket<Callable, Result>;

    std::atomic<int64_t> global_limit; /**< Timestamp in seconds when global ratelimit expires */

    // <bucket_type, bucket_factory>
    std::map<bucket_type, std::map<snowflake, std::unique_ptr<bucket<Callable, Result>>>> _buckets;
    Callable _call;
    asio::io_context & _io_context;
};

}

}
