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

namespace aegis
{

class aegis_member
{
public:
    explicit aegis_member(snowflake id) : member_id(id) {}
    snowflake member_id = 0;


    std::string name;
    uint16_t discriminator = 0;
    std::string avatar;
    bool isbot = false;
    bool mfa_enabled = false;

    struct guild_info
    {
        std::vector<snowflake> roles;
        std::string nickname;
        snowflake guild_id;
        std::string joined_at;
        bool deaf;
        bool mute;
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

    void load(aegis_guild & _guild, json & obj, aegis_shard * shard);

    std::optional<std::string> getName(snowflake guild_id)
    {
        auto g = get_guild_info(guild_id);
        if (g == nullptr)
            return {};
        return g->nickname;
    }

    guild_info * get_guild_info(snowflake guild_id)
    {
        auto g = guilds.find(guild_id);
        if (g == guilds.end())
            return nullptr;
        return g->second.get();
    }

    guild_info * join(snowflake guild_id)
    {
        auto g = get_guild_info(guild_id);
        if (g != nullptr)
            return g;
        auto g2 = guilds.emplace(guild_id, std::make_unique<guild_info>());
        return g2.first->second.get();
    }

    std::string getFullName()
    {
        return fmt::format("{}#{}", name, discriminator);
    }

    std::map<int64_t, std::unique_ptr<guild_info>> guilds;
    //std::pair<message_snowflake, time_sent>
    std::queue<std::pair<snowflake, int64_t>> msghistory;

    void leave(snowflake guild_id)
    {
        guilds.erase(guild_id);
    }
};

}
