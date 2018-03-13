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


#include "config.hpp"
#include "structs.hpp"


namespace aegiscpp
{

namespace rest_limits
{

using rest_call = std::function<std::optional<rest_reply>(std::string path, std::string content, std::string method)>;
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
    std::atomic_int64_t limit;
    std::atomic_int64_t remaining;
    std::atomic_int64_t reset;

    /// Check if bucket can send a message without hitting the ratelimit
    /**
    * @returns true if bucket ratelimits permit a message to be sent
    */
    bool can_async()
    {
        if (limit == 0)
            return true;
        if (remaining > 0)
            return true;
        int64_t time = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        if (time < reset)
            return false;
        return true;
    }

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

    rest_reply do_async(int64_t id, std::string path, std::string content, std::string method)
    {
        bucket * use_bucket = nullptr;

        auto bkt = _buckets.find(id);
        if (bkt != _buckets.end())
            use_bucket = bkt->second.get();
        else
            use_bucket = _buckets.emplace(id, std::make_unique<bucket>()).first->second.get();

        {
            auto log = spdlog::get("aegis");
            std::scoped_lock<std::mutex> lock(use_bucket->m);
            int64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            log->debug("Checking if can work C:{} R:{}", time, use_bucket->reset);
            while (!use_bucket->can_async())
            {
                time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                log->debug("Can't work. Waiting {}s", use_bucket->reset - time);
                std::this_thread::sleep_for(std::chrono::seconds((use_bucket->reset - time) + 1));
            }
            log->debug("Working");
            std::queue<std::tuple<std::string, std::string, std::string, std::function<void(rest_reply)>>> query;
            std::optional<rest_reply> reply(_call(path, content, method));
            use_bucket->limit = reply->limit;
            use_bucket->remaining = reply->remaining;
            use_bucket->reset = reply->reset;
            return reply.value_or(rest_reply());
        }
    }

private:
    std::unordered_map<int64_t, std::unique_ptr<bucket>> _buckets;
    rest_call _call;
    std::atomic_int64_t & global_limit;
};

/**
* Major parameter of REST API access
* Emoji is a partial major parameter
* Emojis are ratelimited per guild
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

    /// Add a new bucket factory
    /**
    * @see bucket_type
    * @param buckettype Enum value of bucket to add
    */
    void add(const uint16_t buckettype)
    {
        _map.emplace(buckettype, std::make_unique<bucket_factory>(_call, global_limit));
    }

    /// Get a bucket factory object
    /**
    * @see bucket_type
    * @param buckettype Enum value of bucket
    *
    * @returns Reference to a bucket_factory object
    */
    bucket_factory & get(uint16_t buckettype) noexcept
    {
        auto bkt = _map.find(buckettype);
        if (bkt != _map.end())
            return *bkt->second.get();

        return *_map.emplace(buckettype, std::make_unique<bucket_factory>(_call, global_limit)).first->second.get();
    }

    /// Check if globally ratelimited
    /**
    * @returns true if globally ratelimited
    */
    bool is_global()
    {
        return global_limit > 0;
    }

private:
    friend aegis;

    std::atomic_int64_t global_limit; /**< Timestamp in seconds when global ratelimit expires */

    std::unordered_map<uint16_t, std::unique_ptr<bucket_factory>> _map;
    rest_call _call;

    //std::unordered_map<uint64_t, int> bucket;
};

}

}

