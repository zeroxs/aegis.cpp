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


#include "aegis/config.hpp"
#include <string>

namespace aegiscpp
{

using rest_limits::bucket_factory;
using json = nlohmann::json;



inline guild::~guild()
{
    for (auto &[k, v] : members)
        v->leave(guild_id);
}

inline void guild::add_member(std::shared_ptr<member> _member) noexcept
{
    members.emplace(_member->member_id, _member);
}

inline void guild::remove_member(snowflake member_id) noexcept
{
    auto _member = members.find(member_id);
    if (_member == members.end())
    {
        state->core->log->error("Unable to remove member [{}] from guild [{}] (does not exist)", member_id, guild_id);
        return;
    }
    _member->second->guilds.erase(guild_id);
    members.erase(_member);
}

inline bool guild::member_has_role(snowflake member_id, snowflake role_id)
{
    auto _member = get_member(member_id);
    if (_member == nullptr)
        return false;
    for (auto r : _member->guilds[guild_id].roles)
    {
        if (role_snowflakes[role_id] == r)
            return true;
    }
    return false;
}

inline void guild::load(const json & obj, shard * _shard) noexcept
{
    //uint64_t application_id = obj->get("application_id").convert<uint64_t>();
    snowflake g_id = obj["id"];

    shard_id = _shard->get_id();
    is_init = false;

    aegis & bot = *state->core;
    try
    {
        json voice_states;

        if (!obj["name"].is_null()) name = obj["name"];
        if (!obj["icon"].is_null()) icon = obj["icon"];
        if (!obj["splash"].is_null()) splash = obj["splash"];
        owner_id = obj["owner_id"];
        region = obj["region"];
        if (!obj["afk_channel_id"].is_null()) afk_channel_id = obj["afk_channel_id"];
        afk_timeout = obj["afk_timeout"];//in seconds
        if (obj.count("embed_enabled") && !obj["embed_enabled"].is_null()) embed_enabled = obj["embed_enabled"];
        //_guild.embed_channel_id = obj->get("embed_channel_id").convert<uint64_t>();
        verification_level = obj["verification_level"];
        default_message_notifications = obj["default_message_notifications"];
        mfa_level = obj["mfa_level"];
        if (obj.count("joined_at") && !obj["joined_at"].is_null()) joined_at = obj["joined_at"];
        if (obj.count("large") && !obj["large"].is_null()) large = obj["large"];
        if (obj.count("unavailable") && !obj["unavailable"].is_null())
            unavailable = obj["unavailable"];
        else
            unavailable = false;
        if (obj.count("member_count") && !obj["member_count"].is_null()) member_count = obj["member_count"];
        if (obj.count("voice_states") && !obj["voice_states"].is_null()) voice_states = obj["voice_states"];


        if (obj.count("roles"))
        {
            json roles = obj["roles"];

            for (auto & role : roles)
            {
                load_role(role);
            }
        }

        if (obj.count("members"))
        {
            json members = obj["members"];

            for (auto & member : members)
            {
                snowflake member_id = member["user"]["id"];
                auto _member = bot.get_member_create(member_id);
                this->members.emplace(member_id, _member);
                _member->load(this, member, _shard);
                ++_shard->counters.members;
            }
        }

        if (obj.count("channels"))
        {
            json channels = obj["channels"];

            for (auto & channel_obj : channels)
            {
                snowflake channel_id = channel_obj["id"];
                auto _channel = get_channel_create(channel_id, _shard);
                _channel->load_with_guild(*this, channel_obj, _shard);
                ++_shard->counters.channels;
            }
        }

        if (obj.count("presences"))
        {
            json presences = obj["presences"];

            for (auto & presence : presences)
            {
                load_presence(presence);
            }
        }

        if (obj.count("emojis"))
        {
            json emojis = obj["emojis"];

            /*for (auto & emoji : emojis)
            {
                //loadEmoji(emoji, _guild);
            }*/
        }

        if (obj.count("features"))
        {
            json features = obj["features"];

        }

        /*
        for (auto & feature : features)
        {
        //??
        }

        for (auto & voicestate : voice_states)
        {
        //no voice yet
        }*/



    }
    catch (std::exception&e)
    {
        log->error("Shard#{} : Error processing guild[{}] {}", _shard->get_id(), g_id, (std::string)e.what());
    }
}

inline channel * guild::get_channel_create(snowflake id, shard * _shard) noexcept
{
    auto _channel = get_channel(id);
    if (_channel == nullptr)
    {
        auto g = std::make_shared<channel>(id, guild_id, state->core->ratelimit().get(rest_limits::bucket_type::CHANNEL), state->core->ratelimit().get(rest_limits::bucket_type::EMOJI));
        auto ptr = g.get();
        channels.emplace(id, g);
        state->core->channels.emplace(id, g);
        g->_guild = this;
        ptr->channel_id = id;
        return ptr;
    }
    return _channel;
}

inline void guild::load_presence(const json & obj) noexcept
{
    json user = obj["user"];

    member::member_status status;
    if (obj["status"] == "idle")
        status = member::Idle;
    else if (obj["status"] == "dnd")
        status = member::DoNotDisturb;
    else if (obj["status"] == "online")
        status = member::Online;
    else
        status = member::Offline;

    auto _member = get_member(user["id"]);
    if (_member == nullptr)
    {
        //status->core->log->error("User doesn't exist {}", user["id"].get<std::string>());
        return;
    }
    _member->status = status;
    return;
}

inline void guild::load_role(const json & obj) noexcept
{
    snowflake role_id = obj["id"];
    if (!roles.count(role_id))
    {
        auto r = std::make_unique<role>();
        //auto & _role = *r.get();
        roles.emplace(role_id, std::move(r));
        role_snowflakes.emplace(role_id, role_offset++);
    }
    auto & _role = *roles[role_id].get();
    _role.role_id = role_id;
    _role.hoist = obj["hoist"];
    _role.managed = obj["managed"];
    _role.mentionable = obj["mentionable"];
    _role._permission = permission(obj["permissions"].get<uint64_t>());
    _role.position = obj["position"];
    if (!obj["name"].is_null()) _role.name = obj["name"];
    _role.color = obj["color"];
    return;
}

inline snowflake guild::get_owner() const noexcept
{
    return owner_id;
}

inline channel * guild::get_channel(snowflake id) const noexcept
{
    auto it = channels.find(id);
    if (it == channels.end())
        return nullptr;
    return it->second.get();
}

inline member * guild::get_member(snowflake member_id) const noexcept
{
    auto m = members.find(member_id);
    if (m == members.end())
        return nullptr;
    return m->second.get();
}

inline permission guild::get_permissions(snowflake member_id, snowflake channel_id) noexcept
{
    if (!members.count(member_id) || !channels.count(channel_id))
        return 0;
    return get_permissions(*members[guild_id], *channels[channel_id]);
}

inline permission guild::get_permissions(member & _member, channel & _channel) noexcept
{
    int64_t _base_permissions = base_permissions(_member);

    int64_t _compute_overwrites = compute_overwrites(_base_permissions, _member, _channel);

    return _base_permissions | _compute_overwrites;
}

inline int64_t guild::base_permissions(member & _member) const noexcept
{
    if (owner_id == _member.member_id)
        return ~0;

    auto & role_everyone = get_role(guild_id);
    int64_t permissions = role_everyone._permission.get_allow_perms();

    for (auto & rl : _member.get_guild_info(guild_id).value()->roles)
        permissions |= get_role(rl)._permission.get_allow_perms();

    if (permissions & 0x8)//admin
        return ~0;

    return permissions;
}

inline int64_t guild::compute_overwrites(int64_t _base_permissions, member & _member, channel & _channel) const noexcept
{
    if (_base_permissions & 0x8)//admin
        return ~0;

    int64_t permissions = _base_permissions;
    if (_channel.overrides.count(guild_id))
    {
        auto & overwrite_everyone = _channel.overrides[guild_id];
        permissions &= ~overwrite_everyone.deny;
        permissions |= overwrite_everyone.allow;
    }

    auto & overwrites = _channel.overrides;
    int64_t allow = 0;
    int64_t deny = 0;
    auto g = _member.get_guild_info(guild_id);
    if (!g.has_value()) return 0;
    for (auto & rl : g.value()->roles)
    {
        if (overwrites.count(rl))
        {
            auto & ow_role = overwrites[rl];
            allow |= ow_role.allow;
            deny |= ow_role.deny;
        }
    }

    if (overwrites.count(_member.member_id))
    {
        auto & ow_role = overwrites[_member.member_id];
        allow |= ow_role.allow;
        deny |= ow_role.deny;
    }

    permissions &= ~deny;
    permissions |= allow;

    return permissions;
}

inline role & guild::get_role(uint64_t r) const
{
    for (auto & kv : roles)
        if (kv.second->role_id == r)
            return *kv.second.get();
    throw std::out_of_range(fmt::format("G: {} role:[{}] does not exist", guild_id, r));
}

inline role & guild::get_role(uint16_t r) const
{
    int64_t realrole_id = get_role_snowflake(r);
    for (auto & kv : roles)
        if (kv.second->role_id == realrole_id)
            return *kv.second.get();
    throw std::out_of_range(fmt::format("G: {} role:[{}] shorthand:[{}] does not exist", guild_id, realrole_id, r));
}

inline void guild::remove_role(snowflake role_id)
{
    for (auto & kv : members)
    {
        auto g = kv.second->get_guild_info(guild_id);
        if (!g.has_value()) return;
        for (auto & rl : g.value()->roles)
        {
            if (rl == role_id)
            {
                g.value()->roles.erase(std::find(g.value()->roles.begin(), g.value()->roles.end(), role_id));
            }
        }
    }
    roles.erase(role_id);
}

inline int32_t guild::get_member_count() const noexcept
{
    return static_cast<int32_t>(members.size());
}


inline std::future<rest_reply> guild::post_task(std::string path, std::string method, std::string obj)
{
    auto task(std::make_shared<std::packaged_task<rest_reply()>>(
        std::bind(&aegiscpp::rest_limits::bucket_factory::do_async, &ratelimit, guild_id, path, obj, method)));

    auto fut = task->get_future();

    state->core->rest_scheduler->post([task]() { (*task)(); });

    return fut;
}

/**\todo Incomplete. Signature may change. Location may change.
*/
inline rest_api guild::create_guild()
{
    //TODO: 
    return { make_error_code(error::not_implemented),std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change. Location may change.
*/
inline rest_api guild::get_guild()
{
    //TODO: 
    return { make_error_code(error::not_implemented),std::make_optional<std::future<rest_reply>>() };
}

inline rest_api guild::modify_guild(std::optional<std::string> name, std::optional<std::string> voice_region, std::optional<int> verification_level,
                    std::optional<int> default_message_notifications, std::optional<snowflake> afk_channel_id, std::optional<int> afk_timeout,
                    std::optional<std::string> icon, std::optional<snowflake> owner_id, std::optional<std::string> splash)
{
    if (!perms().can_manage_guild())
        return { make_error_code(error::no_permission),std::make_optional<std::future<rest_reply>>() };
    if (owner_id.has_value() && owner_id != self()->member_id)
        return { make_error_code(error::no_permission),std::make_optional<std::future<rest_reply>>() };

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


    auto fut = post_task(fmt::format("/guilds/{}", guild_id), "PATCH", obj.dump());
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

inline rest_api guild::delete_guild()
{
    //requires OWNER
    if (owner_id != self()->member_id)
        return { make_error_code(error::no_permission),std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/guilds/{}", guild_id), "DELETE");
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

inline rest_api guild::create_text_channel(std::string name, int64_t parent_id, bool nsfw, std::vector<permission_overwrite> permission_overwrites)
{
    //requires MANAGE_CHANNELS
    if (!perms().can_manage_channels())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj;
    obj["name"] = name;
    obj["type"] = 0;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow);
    }

    auto fut = post_task(fmt::format("/guilds/{}/channels", guild_id), "POST", obj.dump());
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

inline rest_api guild::create_voice_channel(std::string name, int32_t bitrate, int32_t user_limit, int64_t parent_id, bool nsfw, std::vector<permission_overwrite> permission_overwrites)
{
    if (!perms().can_manage_channels())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj;
    obj["name"] = name;
    obj["type"] = 2;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow);
    }

    auto fut = post_task(fmt::format("/guilds/{}/channels", guild_id), "POST", obj.dump());
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

inline rest_api guild::create_category_channel(std::string name, int64_t parent_id, std::vector<permission_overwrite> permission_overwrites)
{
    if (!perms().can_manage_channels())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj;
    obj["name"] = name;
    obj["type"] = 4;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow);
    }

    auto fut = post_task(fmt::format("/guilds/{}/channels", guild_id), "POST", obj.dump());
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::modify_channel_positions()
{
    if (!perms().can_manage_channels())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

inline rest_api guild::modify_guild_member(snowflake user_id, std::optional<std::string> nick, std::optional<bool> mute,
                            std::optional<bool> deaf, std::optional<std::vector<snowflake>> roles, std::optional<snowflake> channel_id)
{
    permission perm = perms();
    json obj;
    if (nick.has_value())
    {
        if (!perm.can_manage_names())
            return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };
        obj["nick"] = nick.value();//requires MANAGE_NICKNAMES
    }
    if (mute.has_value())
    {
        if (!perm.can_voice_mute())
            return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };
        obj["mute"] = mute.value();//requires MUTE_MEMBERS
    }
    if (deaf.has_value())
    {
        if (!perm.can_voice_deafen())
            return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };
        obj["deaf"] = deaf.value();//requires DEAFEN_MEMBERS
    }
    if (roles.has_value())
    {
        if (!perm.can_manage_roles())
            return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };
        obj["roles"] = roles.value();//requires MANAGE_ROLES
    }
    if (channel_id.has_value())
    {
        //TODO: This needs to calculate whether or not the bot has access to the voice channel as well
        if (!perm.can_voice_move())
            return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };
        obj["channel_id"] = channel_id.value();//requires MOVE_MEMBERS
    }

    auto fut = post_task(fmt::format("/guilds/{}/members/{}", guild_id, user_id), "PATCH", obj.dump());
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

inline rest_api guild::modify_my_nick(std::string newname)
{
    if (!perms().can_change_name())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj = { "nick", newname };
    auto fut = post_task(fmt::format("/guilds/{}/members/@me/nick", guild_id), "PATCH", obj.dump());
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

inline rest_api guild::add_guild_member_role(snowflake user_id, snowflake role_id)
{
    if (!perms().can_manage_roles())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/guilds/{}/members/{}/roles/{}", guild_id, user_id, role_id), "PUT");
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

inline rest_api guild::remove_guild_member_role(snowflake user_id, snowflake role_id)
{
    if (!perms().can_manage_roles())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/guilds/{}/members/{}/roles/{}", guild_id, user_id, role_id), "DELETE");
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

inline rest_api guild::remove_guild_member(snowflake user_id)
{
    if (!perms().can_kick())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/guilds/{}/members/{}", guild_id, user_id), "DELETE");
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

inline rest_api guild::create_guild_ban(snowflake user_id, int8_t delete_message_days)
{
    if (!perms().can_ban())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    json obj = { "delete-message-days", delete_message_days };
    auto fut = post_task(fmt::format("/guilds/{}/bans/{}", guild_id, user_id), "PUT");
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

inline rest_api guild::remove_guild_ban(snowflake user_id)
{
    if (!perms().can_ban())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    auto fut = post_task(fmt::format("/guilds/{}/bans/{}", guild_id, user_id), "DELETE");
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::create_guild_role()
{
    if (!perms().can_manage_roles())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::modify_guild_role_positions()
{
    if (!perms().can_manage_roles())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::modify_guild_role(snowflake role_id)
{
    if (!perms().can_manage_roles())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::delete_guild_role(snowflake role_id)
{
    if (!perms().can_manage_roles())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::get_guild_prune_count(int16_t days)
{
    if (!perms().can_kick())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::begin_guild_prune(int16_t days)
{
    if (!perms().can_kick())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::get_guild_invites()
{
    if (!perms().can_manage_guild())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::get_guild_integrations()
{
    if (!perms().can_manage_guild())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::create_guild_integration()
{
    if (!perms().can_manage_guild())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::modify_guild_integration()
{
    if (!perms().can_manage_guild())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::delete_guild_integration()
{
    if (!perms().can_manage_guild())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::sync_guild_integration()
{
    if (!perms().can_manage_guild())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::get_guild_embed()
{
    if (!perms().can_manage_guild())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

/**\todo Incomplete. Signature may change
*/
inline rest_api guild::modify_guild_embed()
{
    if (!perms().can_manage_guild())
        return { make_error_code(error::no_permission), std::make_optional<std::future<rest_reply>>() };

    return { make_error_code(error::not_implemented), std::make_optional<std::future<rest_reply>>() };
}

inline rest_api guild::leave()
{
    auto fut = post_task(fmt::format("/users/@me/guilds/{0}", guild_id), "DELETE");
    return { std::error_code(), std::make_optional<std::future<rest_reply>>(std::move(fut)) };
}

}
