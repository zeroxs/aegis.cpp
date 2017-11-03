//
// member.hpp
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
#include "guild.hpp"
#include <string>
#include <optional>
#include <queue>
#include <memory>

namespace aegiscpp
{

class guild;
class shard;

class member : public std::enable_shared_from_this<member>
{
public:
    explicit member(snowflake id) : member_id(id) {}
    snowflake member_id = 0;


    std::string name;
    uint16_t discriminator = 0;
    std::string avatar;
    bool isbot : 1;
    bool mfa_enabled : 1;

    struct guild_info
    {
        std::vector<int16_t> roles;
        std::optional<std::string> nickname;
        //std::string joined_at;
        int64_t joined_at;
        bool deaf : 1;
        bool mute : 1;
    };

    enum member_status
    {
        Offline,
        Online,
        Idle,
        Streaming,
        DoNotDisturb
    };

    member_status status = member_status::Offline;

    void load(guild * _guild, const json & obj, shard * _shard);

    std::optional<std::string> getName(snowflake guild_id)
    {
        auto g = get_guild_info(guild_id);
        if (!g.has_value())
            return {};
        return g.value()->nickname;
    }

    std::optional<guild_info*> get_guild_info(snowflake guild_id)
    {
        auto g = guilds.find(guild_id);
        if (g == guilds.end())
            return {};
        return &g->second;
    }

    std::optional<guild_info*> join(snowflake guild_id)
    {
        auto g = get_guild_info(guild_id);
        if (g.has_value())
            return g;
        auto g2 = guilds.emplace(guild_id, guild_info());
        return &g2.first->second;
    }

    std::string getFullName()
    {
        return fmt::format("{}#{}", name, discriminator);
    }

    std::unordered_map<int64_t, guild_info> guilds;
    //std::pair<message_snowflake, time_sent>
    //std::queue<std::pair<snowflake, int64_t>> msghistory;

    void leave(snowflake guild_id)
    {
        guilds.erase(guild_id);
    }
};

}
