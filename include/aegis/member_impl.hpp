//
// member_impl.hpp
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
#include "shard.hpp"
#include <string>
#include <optional>
#include <queue>


namespace aegiscpp
{




inline void member::load(guild & _guild, json & obj, shard * _shard)
{
    json & user = obj["user"];
    snowflake member_id = user["id"];

    try
    {
        if (!user["username"].is_null()) name = user["username"].get<std::string>();
        if (!user["avatar"].is_null()) avatar = user["avatar"].get<std::string>();
        if (!user["discriminator"].is_null()) discriminator = std::stoi(user["discriminator"].get<std::string>());
        isbot = user["bot"].is_null() ? false : true;

        auto g_info = get_guild_info(_guild.guild_id);
        if (g_info == nullptr)
            g_info = join(_guild.guild_id);

        if (!obj["deaf"].is_null()) g_info->deaf = obj["deaf"];
        if (!obj["mute"].is_null()) g_info->mute = obj["mute"];

        if (!obj["joined_at"].is_null()) g_info->joined_at = obj["joined_at"].get<std::string>();

        if (!obj["roles"].is_null())
        {
            if (!guilds.count(_guild.guild_id))
            {
                guilds.emplace(_guild.guild_id, std::make_unique<member::guild_info>());
            }
            g_info->roles.clear();
            g_info->guild_id = _guild.guild_id;
            g_info->roles.push_back(_guild.guild_id);//default everyone role

            json roles = obj["roles"];
            for (auto & r : roles)
                g_info->roles.push_back(r);
        }

        if (!obj["nick"].is_null())
            g_info->nickname = obj["nick"].get<std::string>();
    }
    catch (std::exception&e)
    {
        _guild.state->core->log->error("Shard#{} : Error processing member[{}] of guild[{}] {}", _shard->get_id(), member_id, _guild.guild_id, e.what());
    }
}


}

