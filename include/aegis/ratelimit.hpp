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
#include "aegis/rest_reply.hpp"
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

namespace rest
{

using rest_call = std::function<rest_reply(const std::string & path, const std::string & content, const std::string & method, const std::string & host)>;
using namespace std::chrono;

/// Buckets store ratelimit data per major parameter
/**
 * Bucket class for tracking the ratelimits per snowflake per major parameter.
 * Each bucket tracks a single major parameter and a single snowflake
 * Current major parameters are GUILD, CHANNEL, and EMOJI
 */
class bucket
{
public:
    /**
     * Construct a bucket object for tracking ratelimits per major parameter of the REST API (guild/channel/emoji)
     */
    bucket()
        : limit(0)
        , remaining(1)
        , reset(0)
    {
    }
    std::atomic<int64_t> limit; /**< Rate limit current endpoint call limit */
    std::atomic<int64_t> remaining; /**< Rate limit remaining count */
    std::atomic<int64_t> reset; /**< Rate limit reset time */

    /// Check if bucket can send a message without hitting the ratelimit
    /**
     * @returns true if bucket ratelimits permit a message to be sent
     */
    AEGIS_DECL const bool can_async() const AEGIS_NOEXCEPT;

    std::mutex m;
    std::queue<std::tuple<std::string, std::string, std::string, std::function<void(rest_reply)>>> _queue;
};


/// Bucket factory exists to manage buckets per major parameter.
/**
 * This could probably be incorporated into the ratelimiter class
 * since only 3 bucket_factory objects (currently) will exist

 * Bucket factory class for mapping snowflakes to specific buckets.
 * Each bucket_factory belongs to one of the major REST API parameters
 * Current major parameters are GUILD, CHANNEL, and EMOJI
 */
class bucket_factory
{
public:
    /// Construct a bucket_factory object which tracks individual bucket objects
    /**
     * @param call Function pointer to the REST API function
     * @param global_limit Pointer to an atomic_int64 for tracking the global ratelimit
     * @returns true on successful request, false for no permissions
     */
    bucket_factory(rest_call call, std::atomic<int64_t> & global_limit)
        : _call(call)
        , global_limit(global_limit)
    {

    }

    bucket_factory(const bucket_factory &) = delete;
    bucket_factory(bucket_factory &&) = delete;
    bucket_factory & operator=(const bucket_factory &) = delete;

    AEGIS_DECL rest_reply do_async(int64_t id, const std::string path, const std::string content, const std::string method, const std::string host = "");

private:
    std::map<int64_t, std::unique_ptr<bucket>> _buckets;
    rest_call _call;
    std::atomic<int64_t> & global_limit;
};

/**
 * Major parameter of REST API access
 * Emoji is a partial major parameter and is ratelimited per guild across all members
 */
enum bucket_type
{
    GUILD = 0,
    CHANNEL = 1,
    EMOJI = 2
};

/// Factory class for managing ratelimit bucket factory objects
/**
 * Bucket factory factory class for mapping snowflakes to specific buckets.
 * Yes, double factory. This object handles the different major parameter
 * distinctions.
 */
class ratelimiter
{
public:
    /// Construct a ratelimiter object for managing the bucket factories
    /**
     * @param call Function pointer to the REST API function
     */
    explicit ratelimiter(rest_call call)
        : global_limit(0)
        , _call(call)
    {
    };

    ratelimiter(const ratelimiter &) = delete;
    ratelimiter(ratelimiter &&) = delete;
    ratelimiter & operator=(const ratelimiter &) = delete;

    /// Add a new bucket factory
    /**
     * @see bucket_type
     * @param buckettype Enum value of bucket to add
     */
    AEGIS_DECL void add(const uint16_t buckettype);

    /// Get a bucket factory object
    /**
     * @see bucket_type
     * @param buckettype Enum value of bucket
     * @returns Reference to a bucket_factory object
     */
    AEGIS_DECL bucket_factory & get(uint16_t buckettype) AEGIS_NOEXCEPT;

    /// Check if globally ratelimited
    /**
     * @returns true if globally ratelimited
     */
    const bool is_global() const AEGIS_NOEXCEPT
    {
        return global_limit > 0;
    }

private:
    friend class core;

    std::atomic<int64_t> global_limit; /**< Timestamp in seconds when global ratelimit expires */

    std::map<uint16_t, std::unique_ptr<bucket_factory>> _map;
    rest_call _call;
};

}

}
