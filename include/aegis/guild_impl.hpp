//
// guild_impl.hpp
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

namespace aegis
{

using rest_limits::bucket_factory;
using json = nlohmann::json;

inline permission guild::get_permissions(snowflake member_id, snowflake channel_id)
{
    if (!m_members.count(member_id) || !m_channels.count(channel_id))
        return 0;
    return get_permissions(*m_members[member_id], *m_channels[channel_id]);
}

inline permission guild::get_permissions(member & _member, channel & _channel)
{
    int64_t _base_permissions = base_permissions(_member);

    int64_t _compute_overwrites = compute_overwrites(_base_permissions, _member, _channel);

    return _base_permissions | _compute_overwrites;
}

inline int64_t guild::base_permissions(member & _member) const
{
    if (m_owner_id == _member.m_id)
        return ~0;

    auto & role_everyone = get_role(m_id);
    int64_t permissions = role_everyone._permission.getAllowPerms();

    for (auto & rl : _member.m_guilds[m_id].roles)
        permissions |= get_role(rl)._permission.getAllowPerms();

    if (permissions & 0x8)//admin
        return ~0;

    return permissions;
}

inline int64_t guild::compute_overwrites(int64_t _base_permissions, member & _member, channel & _channel) const
{
    if (_base_permissions & 0x8)//admin
        return ~0;

    int64_t permissions = _base_permissions;
    if (_channel.m_overrides.count(m_id))
    {
        auto & overwrite_everyone = _channel.m_overrides[m_id];
        permissions &= ~overwrite_everyone.deny;
        permissions |= overwrite_everyone.allow;
    }

    auto & overwrites = _channel.m_overrides;
    int64_t allow = 0;
    int64_t deny = 0;
    for (auto & rl : _member.m_guilds[m_id].roles)
    {
        if (overwrites.count(rl))
        {
            auto & ow_role = overwrites[rl];
            allow |= ow_role.allow;
            deny |= ow_role.deny;
        }
    }

    if (overwrites.count(_member.m_id))
    {
        auto & ow_role = overwrites[_member.m_id];
        allow |= ow_role.allow;
        deny |= ow_role.deny;
    }

    permissions &= ~deny;
    permissions |= allow;

    return permissions;
}

inline role & guild::get_role(int64_t r) const
{
    for (auto & kv : m_roles)
        if (kv.second->id == r)
            return *kv.second.get();
    throw std::out_of_range(fmt::format("G: {} role:[{}] does not exist", m_id, r));
}

inline void guild::remove_member(json & obj)
{
    snowflake member_id = std::stoll(obj["user"]["id"].get<std::string>());
    for (auto & m : m_members)
    {
        if (m.second->m_id == member_id)
        {
            m.second->m_guilds.erase(m_id);
            break;
        }
    }
    m_members.erase(member_id);
}

inline void guild::remove_role(snowflake role_id)
{
    for (auto & m : m_members)
    {
        for (auto & rl : m.second->m_guilds[m_id].roles)
        {
            if (rl == role_id)
            {
                m.second->m_guilds[m_id].roles.erase(std::find(m.second->m_guilds[m_id].roles.begin(), m.second->m_guilds[m_id].roles.end(), role_id));
            }
        }
    }
    m_roles.erase(role_id);
}

// move this to aegis::Aegis?
inline bool guild::create_guild(std::function<void(rest_reply)> callback)
{
    //TODO: 
    return true;
}

inline bool guild::get_guild(std::function<void(rest_reply)> callback)
{
    //TODO: 
    return true;
}

inline bool guild::modify_guild(std::optional<std::string> name, std::optional<std::string> voice_region, std::optional<int> verification_level,
                    std::optional<int> default_message_notifications, std::optional<snowflake> afk_channel_id, std::optional<int> afk_timeout,
                    std::optional<std::string> icon, std::optional<snowflake> owner_id, std::optional<std::string> splash, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageGuild())
        return false;
    if (owner_id.has_value() && m_owner_id != m_self.m_id)
        return false;

    json obj;
    if (name.has_value())
        obj["name"] = name.value();
    if (voice_region.has_value())
        obj["region"] = voice_region.value();
    if (verification_level.has_value())
        obj["verification_level"] = verification_level.value();
    if (default_message_notifications.has_value())
        obj["default_message_notifications"] = default_message_notifications.value();
    if (afk_channel_id.has_value())
        obj["afk_channel_id"] = afk_channel_id.value();
    if (afk_timeout.has_value())
        obj["afk_timeout"] = afk_timeout.value();
    if (icon.has_value())
        obj["icon"] = icon.value();
    if (owner_id.has_value())//requires OWNER
        obj["owner_id"] = owner_id.value();
    if (splash.has_value())//VIP only
        obj["splash"] = splash.value();

    m_ratelimit.push(m_id, fmt::format("/guilds/{}", m_id), std::move(obj), "PATCH", callback);
    return true;
}

inline bool guild::delete_guild(std::function<void(rest_reply)> callback)
{
    //requires OWNER
    if (m_owner_id != m_self.m_id)
        return false;

    m_ratelimit.push(m_id, fmt::format("/guilds/{}", m_id), "", "DELETE", callback);
    return true;
}

inline bool guild::create_text_channel(std::string name, int64_t parent_id, bool nsfw, std::vector<perm_overwrite> permission_overwrites, std::function<void(rest_reply)> callback)
{
    //requires MANAGE_CHANNELS
    if (!permission(base_permissions(m_self)).canManageChannels())
        return false;

    json obj;
    obj["name"] = name;
    obj["type"] = 0;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow.make());
    }

    m_ratelimit.push(m_id, fmt::format("/guilds/{}/channels", m_id), obj.dump(), "POST", callback);
    return true;
}

inline bool guild::create_voice_channel(std::string name, int32_t bitrate, int32_t user_limit, int64_t parent_id, bool nsfw, std::vector<perm_overwrite> permission_overwrites, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageChannels())
        return false;

    json obj;
    obj["name"] = name;
    obj["type"] = 2;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow.make());
    }

    m_ratelimit.push(m_id, fmt::format("/guilds/{}/channels", m_id), obj.dump(), "POST", callback);
    return true;
}

inline bool guild::create_category_channel(std::string name, int64_t parent_id, std::vector<perm_overwrite> permission_overwrites, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageChannels())
        return false;

    json obj;
    obj["name"] = name;
    obj["type"] = 4;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow.make());
    }

    m_ratelimit.push(m_id, fmt::format("/guilds/{}/channels", m_id), obj.dump(), "POST", callback);
    return true;
}

inline bool guild::modify_channel_positions()
{
    if (!permission(base_permissions(m_self)).canManageChannels())
        return false;

    //TODO:
    return true;
}

inline bool guild::modify_guild_member(snowflake user_id, std::optional<std::string> nick, std::optional<bool> mute,
                            std::optional<bool> deaf, std::optional<std::vector<snowflake>> roles, std::optional<snowflake> channel_id, std::function<void(rest_reply)> callback)
{
    permission perm = permission(base_permissions(m_self));
    json obj;
    if (nick.has_value())
    {
        if (!perm.canManageNames())
            return false;
        obj["nick"] = nick.value();//requires MANAGE_NICKNAMES
    }
    if (mute.has_value())
    {
        if (!perm.canVoiceMute())
            return false;
        obj["mute"] = mute.value();//requires MUTE_MEMBERS
    }
    if (deaf.has_value())
    {
        if (!perm.canVoiceDeafen())
            return false;
        obj["deaf"] = deaf.value();//requires DEAFEN_MEMBERS
    }
    if (roles.has_value())
    {
        if (!perm.canManageRoles())
            return false;
        obj["roles"] = roles.value();//requires MANAGE_ROLES
    }
    if (channel_id.has_value())
    {
        if (!perm.canVoiceMove())
            return false;
        obj["channel_id"] = channel_id.value();//requires MOVE_MEMBERS
    }

    m_ratelimit.push(m_id, fmt::format("/guilds/{}/members/{}", m_id, user_id), obj.dump(), "PATCH", callback);
    return true;
}

inline bool guild::modify_my_nick(std::string newname, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canChangeName())
        return false;

    json obj = { "nick", newname };
    m_ratelimit.push(m_id, fmt::format("/guilds/{}/members/@me/nick", m_id), obj.dump(), "PATCH", callback);
    return true;
}

inline bool guild::add_guild_member_role(snowflake user_id, snowflake role_id, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageRoles())
        return false;

    m_ratelimit.push(m_id, fmt::format("/guilds/{}/members/{}/roles/{}", m_id, user_id, role_id), "", "PUT", callback);
    return true;
}

inline bool guild::remove_guild_member_role(snowflake user_id, snowflake role_id, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageRoles())
        return false;

    m_ratelimit.push(m_id, fmt::format("/guilds/{}/members/{}/roles/{}", m_id, user_id, role_id), "", "DELETE", callback);
    return true;
}

inline bool guild::remove_guild_member(snowflake user_id, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canKick())
        return false;

    m_ratelimit.push(m_id, fmt::format("/guilds/{}/members/{}", m_id, user_id), "", "DELETE", callback);
    return true;
}

inline bool guild::create_guild_ban(snowflake user_id, int8_t delete_message_days, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canBan())
        return false;

    json obj = { "delete-message-days", delete_message_days };
    m_ratelimit.push(m_id, fmt::format("/guilds/{}/bans/{}", m_id, user_id), obj.dump(), "PUT", callback);
    return true;
}

inline bool guild::remove_guild_ban(snowflake user_id, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canBan())
        return false;

    m_ratelimit.push(m_id, fmt::format("/guilds/{}/bans/{}", m_id, user_id), "", "DELETE", callback);
    return true;
}

inline bool guild::create_guild_role(std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageRoles())
        return false;

    //TODO: 
    return true;
}

inline bool guild::modify_guild_role_positions(std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageRoles())
        return false;

    //TODO: 
    return true;
}

inline bool guild::modify_guild_role(snowflake role_id, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageRoles())
        return false;

    //TODO: 
    return true;
}

inline bool guild::delete_guild_role(snowflake role_id, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageRoles())
        return false;

    //TODO: 
    return true;
}

inline bool guild::get_guild_prune_count(int16_t days, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canKick())
        return false;

    //TODO: 
    return true;
}

inline bool guild::begin_guild_prune(int16_t days, std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canKick())
        return false;

    //TODO: 
    return true;
}

inline bool guild::get_guild_invites(std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageGuild())
        return false;

    //TODO: 
    return true;
}

inline bool guild::get_guild_integrations(std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageGuild())
        return false;

    //TODO: 
    return true;
}

inline bool guild::create_guild_integration(std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageGuild())
        return false;

    //TODO: 
    return true;
}

inline bool guild::modify_guild_integration(std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageGuild())
        return false;

    //TODO: 
    return true;
}

inline bool guild::delete_guild_integration(std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageGuild())
        return false;

    //TODO: 
    return true;
}

inline bool guild::sync_guild_integration(std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageGuild())
        return false;

    //TODO: 
    return true;
}

inline bool guild::get_guild_embed(std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageGuild())
        return false;

    //TODO: 
    return true;
}

inline bool guild::modify_guild_embed(std::function<void(rest_reply)> callback)
{
    if (!permission(base_permissions(m_self)).canManageGuild())
        return false;

    //TODO: 
    return true;
}

inline bool guild::leave(std::function<void(rest_reply)> callback)
{
    m_ratelimit.push(m_id, fmt::format("/users/@me/guilds/{0}", m_id), "", "DELETE", callback);
    return true;
}

}
