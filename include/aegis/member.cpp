//
// member.cpp
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
#include "aegis/member.hpp"
#include "aegis/guild.hpp"
#include "aegis/shard.hpp"
#include "aegis/error.hpp"
#include <string>
#include <optional>
#include <queue>
#include <spdlog/fmt/fmt.h>


namespace aegiscpp
{

AEGIS_DECL std::string member::get_full_name() const noexcept
{
    return fmt::format("{}#{:0=4}", name, discriminator);
}

AEGIS_DECL void member::load(guild * _guild, const json & obj, shard * _shard)
{
    const json & user = obj["user"];
    snowflake member_id = user["id"];

    try
    {
        if (user.count("username") && !user["username"].is_null()) name = user["username"];
        if (user.count("avatar") && !user["avatar"].is_null()) avatar = user["avatar"];
        if (user.count("discriminator") && !user["discriminator"].is_null()) discriminator = static_cast<uint16_t>(std::stoi(user["discriminator"].get<std::string>()));
        if (user.count("bot"))
            is_bot = user["bot"].is_null() ? false : true;

        if (_guild == nullptr)
            return;

        _guild->add_member(this);

        auto g_info = get_guild_info(_guild->guild_id);
        if (!g_info.has_value())
            g_info = join(_guild->guild_id);

        if (obj.count("deaf") && !obj["deaf"].is_null()) g_info.value()->deaf = obj["deaf"];
        if (obj.count("mute") && !obj["mute"].is_null()) g_info.value()->mute = obj["mute"];

        if (obj.count("joined_at") && !obj["joined_at"].is_null())// g_info.value()->joined_at = obj["joined_at"];
        {
            std::tm tm = {};
            std::istringstream ss(obj["joined_at"].get<std::string>());
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));

            g_info.value()->joined_at = tp.time_since_epoch().count();
        }

        if (obj.count("roles") && !obj["roles"].is_null())
        {
            if (!guilds.count(_guild->guild_id))
            {
                guilds.emplace(_guild->guild_id, member::guild_info());
            }
            g_info.value()->roles.clear();
            g_info.value()->roles.emplace(_guild->guild_id);//default everyone role

            json roles = obj["roles"];
            for (auto & r : roles)
                g_info.value()->roles.emplace(std::stoll(r.get<std::string>()));
        }

        if (obj.count("nick") && !obj["nick"].is_null())
            g_info.value()->nickname = obj["nick"].get<std::string>();
    }
    catch (std::exception&e)
    {
        if (_guild != nullptr)
            _guild->get_bot().log->error("Shard#{} : Error processing member[{}] of guild[{}] {}", _shard->get_id(), member_id, _guild->get_id(), e.what());
        else
            throw aegiscpp::exception(fmt::format("Shard#{} : Error processing member[{}] {}", _shard->get_id(), member_id, e.what()), make_error_code(aegiscpp::member_error));
    }
}


}

