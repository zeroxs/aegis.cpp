//
// ratelimit.hpp
// aegis.cpp
//
// Copyright (c) 2017 Sara W (sara at xandium dot net)
//
// This file is part of aegis.cpp .
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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
#include <optional>
#include <thread>
#include <mutex>

namespace aegiscpp
{

class aegis;

namespace rest_limits
{

using rest_call = std::function<rest_reply(std::string path, std::string content, std::string method, std::string host)>;
using namespace std::chrono;

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
    std::atomic_int64_t limit; /**< Rate limit current endpoint call limit */
    std::atomic_int64_t remaining; /**< Rate limit remaining count */
    std::atomic_int64_t reset; /**< Rate limit reset time */

    /// Check if bucket can send a message without hitting the ratelimit
    /**
    * @returns true if bucket ratelimits permit a message to be sent
    */
    AEGIS_DECL const bool can_async() const noexcept;

    std::mutex m;
    std::queue<std::tuple<std::string, std::string, std::string, std::function<void(rest_reply)>>> _queue;
};

/**
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
    *
    * @param global_limit Pointer to an atomic_int64 for tracking the global ratelimit
    *
    * @returns true on successful request, false for no permissions
    */
    bucket_factory(rest_call call, std::atomic_int64_t & global_limit)
        : _call(call)
        , global_limit(global_limit)
    {

    }

    bucket_factory(const bucket_factory &) = delete;
    bucket_factory(bucket_factory &&) = delete;
    bucket_factory & operator=(const bucket_factory &) = delete;

    AEGIS_DECL rest_reply do_async(int64_t id, std::string path, std::string content, std::string method, std::string host = "");

private:
    std::map<int64_t, std::unique_ptr<bucket>> _buckets;
    rest_call _call;
    std::atomic_int64_t & global_limit;
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

/**
* Bucket factory class for mapping snowflakes to specific buckets.
* Each bucket_factory belongs to one of the major REST API parameters
* Current major parameters are GUILD, CHANNEL, and EMOJI
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
    *
    * @returns Reference to a bucket_factory object
    */
    AEGIS_DECL bucket_factory & get(uint16_t buckettype) noexcept;

    /// Check if globally ratelimited
    /**
    * @returns true if globally ratelimited
    */
    const bool is_global() const noexcept
    {
        return global_limit > 0;
    }

private:
    friend class aegis;

    std::atomic_int64_t global_limit; /**< Timestamp in seconds when global ratelimit expires */

    std::map<uint16_t, std::unique_ptr<bucket_factory>> _map;
    rest_call _call;
};

}

}

#if defined(AEGIS_HEADER_ONLY)
# include "aegis/ratelimit.cpp"
#endif // defined(AEGIS_HEADER_ONLY)
