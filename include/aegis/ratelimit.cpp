//
// ratelimit.cpp
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
#include "ratelimit.hpp"

namespace aegiscpp
{

class aegis;

namespace rest_limits
{

using rest_call = std::function<std::optional<rest_reply>(std::string path, std::string content, std::string method)>;
using namespace std::chrono;

AEGIS_DECL const bool bucket::can_async() const noexcept
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

AEGIS_DECL rest_reply bucket_factory::do_async(int64_t id, std::string path, std::string content, std::string method)
{
    static bucket & use_bucket = [&]() -> bucket & {
        auto bkt = _buckets.find(id);
        if (bkt != _buckets.end())
            return *bkt->second;
        else
            return *_buckets.emplace(id, std::make_unique<bucket>()).first->second;

    }();

    {
        std::scoped_lock<std::mutex> lock(use_bucket.m);
        int64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        while (!use_bucket.can_async())
        {
            time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            std::this_thread::sleep_for(std::chrono::seconds((use_bucket.reset - time) + 1));
        }
        std::queue<std::tuple<std::string, std::string, std::string, std::function<void(rest_reply)>>> query;
        std::optional<rest_reply> reply(_call(path, content, method));
        use_bucket.limit = reply->limit;
        use_bucket.remaining = reply->remaining;
        use_bucket.reset = reply->reset;
        return reply.value_or(rest_reply());
    }
}


AEGIS_DECL void ratelimiter::add(const uint16_t buckettype)
{
    _map.emplace(buckettype, std::make_unique<bucket_factory>(_call, global_limit));
}

AEGIS_DECL bucket_factory & ratelimiter::get(uint16_t buckettype) noexcept
{
    auto bkt = _map.find(buckettype);
    if (bkt != _map.end())
        return *bkt->second;

    return *_map.emplace(buckettype, std::make_unique<bucket_factory>(_call, global_limit)).first->second;
}

}

}

