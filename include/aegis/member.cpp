//
// member.cpp
// **********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#if !defined(AEGIS_DISABLE_ALL_CACHE)
#include "aegis/member.hpp"
#include "aegis/channel.hpp"
#include "aegis/core.hpp"
#include "aegis/guild.hpp"
#include "aegis/error.hpp"
#include <string>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <spdlog/spdlog.h>

namespace aegis
{

AEGIS_DECL std::string member::get_full_name() const noexcept
{
    return fmt::format("{}#{:0=4}", std::string(_name), _discriminator);
}

AEGIS_DECL void member::load(guild * _guild, const json & obj, shards::shard * _shard)
{
    const json & user = obj["user"];
    _member_id = user["id"];

    try
    {
        if (user.count("username") && !user["username"].is_null()) _name = user["username"].get<std::string>();
        if (user.count("avatar") && !user["avatar"].is_null()) _avatar = user["avatar"].get<std::string>();
        if (user.count("discriminator") && !user["discriminator"].is_null()) _discriminator = static_cast<uint16_t>(std::stoi(user["discriminator"].get<std::string>()));
        if (user.count("bot"))
            _is_bot = user["bot"].is_null() ? false : true;

        if (_guild == nullptr)
            return;

        _guild->add_member(this);

        auto & g_info = join(_guild->guild_id);

        if (obj.count("deaf") && !obj["deaf"].is_null()) g_info.deaf = obj["deaf"];
        if (obj.count("mute") && !obj["mute"].is_null()) g_info.mute = obj["mute"];

        if (obj.count("joined_at") && !obj["joined_at"].is_null())// g_info.value()->joined_at = obj["joined_at"];
        {
            std::tm tm = {};
            std::istringstream ss(obj["joined_at"].get<std::string>());
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));

            g_info.joined_at = tp.time_since_epoch().count();
        }

        if (obj.count("roles") && !obj["roles"].is_null())
        {
            if (!guilds.count(_guild->guild_id))
            {
                guilds.emplace(_guild->guild_id, member::guild_info());
            }
            g_info.roles.clear();
            g_info.roles.emplace(_guild->guild_id);//default everyone role

            json roles = obj["roles"];
            for (auto & r : roles)
                g_info.roles.emplace(std::stoll(r.get<std::string>()));
        }

        if (obj.count("nick") && !obj["nick"].is_null())
            g_info.nickname = obj["nick"].get<std::string>();
    }
    catch (std::exception & e)
    {
        if (_guild != nullptr)
            spdlog::get("aegis")->error("Shard#{} : Error processing member[{}] of guild[{}] {}", _shard->get_id(), _member_id, _guild->get_id(), e.what());
        else
            throw exception(fmt::format("Shard#{} : Error processing member[{}] {}", _shard->get_id(), _member_id, e.what()), make_error_code(error::member_error));
    }
}

AEGIS_DECL member::guild_info & member::get_guild_info(snowflake guild_id) noexcept
{
    std::shared_lock<shared_mutex> l(_m);
    auto g = guilds.find(guild_id);
    if (g == guilds.end())
    {
        auto g2 = guilds.emplace(guild_id, guild_info());
        return g2.first->second;
    }
        //throw std::out_of_range(fmt::format("M: {} G: {} guild_info does not exist", member_id, guild_id));
    return g->second;
}

AEGIS_DECL std::string member::get_name(snowflake guild_id) noexcept
{
    auto g = get_guild_info(guild_id);
    return g.nickname;
}

AEGIS_DECL member::guild_info & member::join(snowflake guild_id)
{
    auto g = guilds.find(guild_id);
    if (g == guilds.end())
    {
        auto g2 = guilds.emplace(guild_id, guild_info());
        return g2.first->second;
    }
    return g->second;
}

AEGIS_DECL void member::load_data(gateway::objects::user mbr)
{
    if (!mbr.avatar.empty())
        _avatar = mbr.avatar;
    if (!mbr.username.empty())
        _name = mbr.username;
    if (!mbr.avatar.empty())
        _discriminator = std::stoul(mbr.discriminator);

    _is_bot = mbr.is_bot();
}


}
#endif
