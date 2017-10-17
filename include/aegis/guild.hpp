//
// guild.hpp
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
#include "client.hpp"
#include "ratelimit.hpp"
#include "channel.hpp"
#include "member.hpp"
#include <json.hpp>

namespace aegis
{

using rest_limits::bucket_factory;
using json = nlohmann::json;

class guild
{
public:
    explicit guild(client & shard, snowflake id, bucket_factory & ratelimit)
        : m_shard(shard)
        , m_snowflake(id)
        , m_ratelimit(ratelimit)
        , m_log(spdlog::get("aegis"))
    {

    }

    client m_shard;
    snowflake m_snowflake;
    bucket_factory & m_ratelimit;
    std::shared_ptr<spdlog::logger> m_log;

    std::map<int64_t, std::unique_ptr<channel>> m_channels;
    std::map<int64_t, std::shared_ptr<member>> m_members;

};

}
