//
// user.cpp
// ********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#if !defined(AEGIS_DISABLE_ALL_CACHE)
#include "aegis/user.hpp"
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

AEGIS_DECL std::string user::get_full_name() const noexcept
{
    std::shared_lock<shared_mutex> l(_m);

    return fmt::format("{}#{:0=4}", std::string(_name), _discriminator);
}

AEGIS_DECL void user::_load(guild * _guild, const json & obj, shards::shard * _shard, bool self_add)
{
    std::unique_lock<shared_mutex> l(_m);
    _load_nolock(_guild, obj, _shard, self_add);
}

AEGIS_DECL void user::_load_nolock(guild * _guild, const json & obj, shards::shard * _shard, bool self_add, bool guild_lock)
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

        if (self_add)
        {
            guild_info * g_info = nullptr;
            if (guild_lock)
            {
                _guild->_add_member(this);
                g_info = &_join(_guild->guild_id);

            }
            else
            {
                _guild->_add_member_nolock(this);
                g_info = &_join_nolock(_guild->guild_id);
            }

            if (obj.count("deaf") && !obj["deaf"].is_null()) g_info->deaf = obj["deaf"];
            if (obj.count("mute") && !obj["mute"].is_null()) g_info->mute = obj["mute"];

            if (obj.count("joined_at") && !obj["joined_at"].is_null())// g_info.value()->joined_at = obj["joined_at"];
            {
                g_info->joined_at = utility::from_iso8601(obj["joined_at"]).time_since_epoch().count();
            }

            if (obj.count("roles") && !obj["roles"].is_null())
            {
                g_info->roles.clear();
                g_info->roles.emplace_back(_guild->guild_id);//default everyone role

                json roles = obj["roles"];
                for (auto & r : roles)
                    g_info->roles.emplace_back(std::stoull(r.get<std::string>()));
            }

            if (obj.count("nick") && !obj["nick"].is_null())
                g_info->nickname = obj["nick"].get<std::string>();
            else
                g_info->nickname.reset();
        }
    }
    catch (std::exception & e)
    {
        if (_guild != nullptr)
            spdlog::get("aegis")->error("Shard#{} : Error processing member[{}] of guild[{}] {}", _shard->get_id(), _member_id, _guild->get_id(), e.what());
        else
            throw exception(fmt::format("Shard#{} : Error processing member[{}] {}", _shard->get_id(), _member_id, e.what()), make_error_code(error::member_error));
    }
}

AEGIS_DECL user::guild_info & user::get_guild_info(snowflake guild_id) noexcept
{
    std::unique_lock<shared_mutex> l(_m);

    auto g = std::find_if(std::begin(guilds), std::end(guilds), [&guild_id](const std::unique_ptr<guild_info> & gi)
    {
        if (gi->id == guild_id)
            return true;
        return false;
    });
    if (g == guilds.end())
    {
#if defined(AEGIS_CXX17)
        return *guilds.emplace_back(std::make_unique<guild_info>(guild_id));
#else
        return **guilds.insert(guilds.end(), std::make_unique<guild_info>(guild_id));
#endif
    }
    return **g;
}

AEGIS_DECL std::string user::get_name(snowflake guild_id) noexcept
{
    std::shared_lock<shared_mutex> l(_m);

    const auto & def = get_guild_info(guild_id).nickname;
    return def.has_value() ? def.value() : "";
}

AEGIS_DECL user::guild_info & user::_join(snowflake guild_id)
{
    std::unique_lock<shared_mutex> l(_m);
    return _join_nolock(guild_id);
}

AEGIS_DECL user::guild_info & user::_join_nolock(snowflake guild_id)
{
    auto g = std::find_if(std::begin(guilds), std::end(guilds), [&guild_id](const std::unique_ptr<guild_info> & gi)
    {
        if (gi->id == guild_id)
            return true;
        return false;
    });
    if (g == guilds.end())
    {
#if defined(AEGIS_CXX17)
        return *guilds.emplace_back(std::make_unique<guild_info>(guild_id));
#else
        return **guilds.insert(guilds.end(), std::make_unique<guild_info>(guild_id));
#endif
    }
    return **g;
}

AEGIS_DECL void user::_load_data(gateway::objects::user mbr)
{
    std::unique_lock<shared_mutex> l(_m);

    if (!mbr.avatar.empty())
        _avatar = mbr.avatar;
    if (!mbr.username.empty())
        _name = mbr.username;
    if (!mbr.avatar.empty())
        _discriminator = static_cast<uint16_t>(std::stoi(mbr.discriminator));

    _is_bot = mbr.is_bot();
}

AEGIS_DECL std::string user::get_mention() const noexcept
{
    return fmt::format("<@{}>", _member_id);
}

}
#endif
