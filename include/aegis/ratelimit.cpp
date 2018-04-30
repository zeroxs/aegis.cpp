//
// ratelimit.cpp
// *************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/ratelimit.hpp"
#include <mutex>

namespace aegis
{

namespace rest
{

using rest_call = std::function<rest_reply(const std::string & path, const std::string & content, const std::string & method, const std::string & host)>;
using namespace std::chrono;

AEGIS_DECL const bool bucket::can_async() const AEGIS_NOEXCEPT
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

AEGIS_DECL rest_reply bucket_factory::do_async(int64_t id, const std::string path, const std::string content, const std::string method, const std::string host)
{
    static bucket & use_bucket = [&]() -> bucket & {
        auto bkt = _buckets.find(id);
        if (bkt != _buckets.end())
            return *bkt->second;
        else
            return *_buckets.emplace(id, std::make_unique<bucket>()).first->second;

    }();

    {
        std::lock_guard<std::mutex> lock(use_bucket.m);
        int64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        while (!use_bucket.can_async())
        {
            time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            std::this_thread::sleep_for(std::chrono::seconds((use_bucket.reset - time) + 1));
        }
        //std::queue<std::tuple<std::string, std::string, std::string, std::function<void(rest_response)>>> query;
        rest_reply reply(_call(path, content, method, host));
        use_bucket.limit = reply.limit;
        use_bucket.remaining = reply.remaining;
        use_bucket.reset = reply.reset;
        return reply;
    }
}


AEGIS_DECL void ratelimiter::add(const uint16_t buckettype)
{
    _map.emplace(buckettype, std::make_unique<bucket_factory>(_call, global_limit));
}

AEGIS_DECL bucket_factory & ratelimiter::get(uint16_t buckettype) AEGIS_NOEXCEPT
{
    auto bkt = _map.find(buckettype);
    if (bkt != _map.end())
        return *bkt->second;

    return *_map.emplace(buckettype, std::make_unique<bucket_factory>(_call, global_limit)).first->second;
}

}

}

