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


#include <string>
#include <string_view>
#include <map>
#include <memory>
#include <queue>
#include <functional>
#include <optional>
#include <chrono>
#include <mutex>
#include <atomic>
#include "structs.hpp"
#include "spdlog/spdlog.h"

namespace aegis
{

namespace rest_limits
{

using rest_call = std::function<std::optional<rest_reply>(std::string path, std::string content, std::string method)>;
using namespace std::chrono;

class bucket
{
public:
    bucket()
        : limit(0)
        , remaining(1)
        , reset(0)
    {
    }
    int32_t limit;
    int32_t remaining;
    int64_t reset;

    bool can_work()
    {
        if (_queue.size() == 0)
            return false;
        int64_t time = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        if (remaining > 0)
            return true;
        if (time < reset)
            return false;
        return true;
    }

    std::queue<std::tuple<std::string, std::string, std::string, std::function<void(rest_reply)>>> _queue;
};

class bucket_factory
{
public:
    bucket_factory(rest_call call, std::atomic_int64_t * global_limit)
        : _call(call)
        , global_limit(global_limit)
    {

    }

    void run_one()
    {
        for (auto &[k, v] : _buckets)
        {
            if (!v->can_work())
                continue;

            int64_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

            spdlog::get("aegis")->info("chrono: {} reset: {}", time, v->reset);

            auto & query = v->_queue.front();

            std::optional<rest_reply> reply(_call(std::get<0>(query), std::get<1>(query), std::get<2>(query)));
            if (!reply.has_value())
            {
                //failed to call
                return;
            }
            if (reply->global)
            {
                *global_limit = (time + reply->retry);
                return;
            }
            v->_queue.pop();

            v->limit = reply->limit;
            v->remaining = reply->remaining;
            v->reset = reply->reset;
            if (std::get<3>(query) != nullptr)
            {
                std::get<3>(query)(std::move(reply.value()));
            }
        }

    }

    void push(uint64_t id, std::string path, std::string content, std::string method, std::function<void(rest_reply)> callback = nullptr)
    {

        try
        {
            auto & bkt = _buckets.at(id);
            bkt->_queue.emplace(path, content, method, callback);
        }
        catch (std::out_of_range &)
        {
            auto & bkt = _buckets.emplace(id, std::make_shared<bucket>()).first->second;
            bkt->_queue.emplace(path, content, method, callback);
        }
    }

    std::map<uint64_t, std::shared_ptr<bucket>> _buckets;
    rest_call _call;
    std::atomic_int64_t * global_limit;
};

enum bucket_type
{
    GUILD = 0,
    CHANNEL = 1
};


class ratelimiter
{
public:
    explicit ratelimiter(rest_call call)
        : _call(call)
    {
    };

    void add(const uint16_t buckettype)
    {
        _map.emplace(std::pair<uint16_t, std::shared_ptr<bucket_factory>>(buckettype, std::make_shared<bucket_factory>(_call, &global_limit)));
    }

    bucket_factory & get(uint16_t buckettype) noexcept
    {
        try
        {
            return *_map.at(buckettype);
        }
        catch (std::out_of_range &)
        {
            return *_map.emplace(buckettype, std::make_shared<bucket_factory>(_call, &global_limit)).first->second;
        }
    }

    void process_queue()
    {
        if (global_limit > 0)
        {
            if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() < global_limit)
                return;
            else
                global_limit = 0;
        }
        static std::mutex mtx;
        std::scoped_lock<std::mutex> lock(mtx);

        for (auto & [k,v] : _map)
        {
            v->run_one();
        }
    }

    bool is_global()
    {
        return global_limit > 0;
    }

    std::atomic_int64_t global_limit;


private:
    std::map<uint16_t, std::shared_ptr<bucket_factory>> _map;
    rest_call _call;


    //std::map<uint64_t, int> bucket;
};

}

}

