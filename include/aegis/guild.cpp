//
// guild.cpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/guild.hpp"
#include <string>
#include <memory>
#include "aegis/core.hpp"
#include "aegis/member.hpp"
#include "aegis/channel.hpp"
#include "aegis/error.hpp"
#include "aegis/shard.hpp"

namespace aegis
{

using json = nlohmann::json;


AEGIS_DECL guild::~guild()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    //TODO: remove guilds from members elsewhere when bot is removed from guild
//     if (get_bot().get_state() != Shutdown)
//         for (auto & v : members)
//             v.second->leave(guild_id);
#endif
}

AEGIS_DECL core & guild::get_bot() const AEGIS_NOEXCEPT
{
    return *_bot;
}

#if !defined(AEGIS_DISABLE_ALL_CACHE)
AEGIS_DECL member * guild::self() const
{
    return get_bot().self();
}

AEGIS_DECL void guild::add_member(member * _member) AEGIS_NOEXCEPT
{
    members.emplace(_member->member_id, _member);
}

AEGIS_DECL void guild::remove_member(snowflake member_id) AEGIS_NOEXCEPT
{
    auto _member = members.find(member_id);
    if (_member == members.end())
    {
        get_bot().log->error("Unable to remove member [{}] from guild [{}] (does not exist)", member_id, guild_id);
        return;
    }
    _member->second->guilds.erase(guild_id);
    members.erase(member_id);
}

AEGIS_DECL bool guild::member_has_role(snowflake member_id, snowflake role_id) const AEGIS_NOEXCEPT
{
    std::shared_lock<shared_mutex> l(_m);
    auto _member = find_member(member_id);
    if (_member == nullptr)
        return false;
    auto it = _member->guilds[guild_id].roles.find(role_id);
    if (it != _member->guilds[guild_id].roles.end())
        return true;
    return false;
}

AEGIS_DECL void guild::load_presence(const json & obj) AEGIS_NOEXCEPT
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

    auto _member = _find_member(user["id"]);
    if (_member == nullptr)
    {
        //_status->core->log->error("User doesn't exist {}", user["id"].get<std::string>());
        return;
    }
    _member->status = status;
}

AEGIS_DECL void guild::load_role(const json & obj) AEGIS_NOEXCEPT
{
    snowflake role_id = obj["id"];
    if (!roles.count(role_id))
    {
        auto r = std::make_unique<gateway::objects::role>();
        //auto & _role = *r.get();
        roles.emplace(role_id, gateway::objects::role());
    }
    auto & _role = roles[role_id];
    _role.role_id = role_id;
    _role.hoist = obj["hoist"];
    _role.managed = obj["managed"];
    _role.mentionable = obj["mentionable"];
    _role._permission = permission(obj["permissions"].get<uint64_t>());
    _role.position = obj["position"];
    if (!obj["name"].is_null()) _role.name = obj["name"].get<std::string>();
    _role.color = obj["color"];
}

AEGIS_DECL const snowflake guild::get_owner() const AEGIS_NOEXCEPT
{
    return owner_id;
}

AEGIS_DECL member * guild::find_member(snowflake member_id) const AEGIS_NOEXCEPT
{
    std::shared_lock<shared_mutex> l(_m);
    auto m = members.find(member_id);
    if (m == members.end())
        return nullptr;
    return m->second;
}

AEGIS_DECL member * guild::_find_member(snowflake member_id) const AEGIS_NOEXCEPT
{
    auto m = members.find(member_id);
    if (m == members.end())
        return nullptr;
    return m->second;
}

AEGIS_DECL channel * guild::find_channel(snowflake channel_id) const AEGIS_NOEXCEPT
{
    std::shared_lock<shared_mutex> l(_m);
    auto m = channels.find(channel_id);
    if (m == channels.end())
        return nullptr;
    return m->second;
}

AEGIS_DECL channel * guild::_find_channel(snowflake channel_id) const AEGIS_NOEXCEPT
{
    auto m = channels.find(channel_id);
    if (m == channels.end())
        return nullptr;
    return m->second;
}

AEGIS_DECL permission guild::get_permissions(snowflake member_id, snowflake channel_id) AEGIS_NOEXCEPT
{
    if (!members.count(member_id) || !channels.count(channel_id))
        return 0;
    return get_permissions(find_member(member_id), find_channel(channel_id));
}

AEGIS_DECL permission guild::get_permissions(member * _member, channel * _channel) AEGIS_NOEXCEPT
{
    if (_member == nullptr || _channel == nullptr)
        return 0;

    int64_t _base_permissions = base_permissions(_member);

    int64_t _compute_overwrites = compute_overwrites(_base_permissions, *_member, *_channel);

    return _base_permissions | _compute_overwrites;
}

AEGIS_DECL int64_t guild::base_permissions(member & _member) const AEGIS_NOEXCEPT
{
    try
    {
        if (owner_id == _member.member_id)
            return ~0;

        auto & role_everyone = get_role(guild_id);
        int64_t permissions = role_everyone._permission.get_allow_perms();

        auto g = _member.get_guild_info(guild_id);

        for (auto & rl : g.roles)
            permissions |= get_role(rl)._permission.get_allow_perms();

        if (permissions & 0x8)//admin
            return ~0;

        return permissions;
    }
    catch (std::out_of_range &)
    {
        return 0;
    }
}

AEGIS_DECL int64_t guild::compute_overwrites(int64_t _base_permissions, member & _member, channel & _channel) const AEGIS_NOEXCEPT
{
    try
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
        for (auto & rl : g.roles)
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
    catch (std::exception &)
    {
        return 0;
    }
}

AEGIS_DECL const gateway::objects::role & guild::get_role(int64_t r) const
{
    std::shared_lock<shared_mutex> l(_m);
    for (auto & kv : roles)
        if (kv.second.role_id == r)
            return kv.second;
    throw std::out_of_range(fmt::format("G: {} role:[{}] does not exist", guild_id, r));
}

AEGIS_DECL void guild::remove_role(snowflake role_id)
{
    std::unique_lock<shared_mutex> l(_m);
    try
    {
        for (auto & kv : members)
        {
            auto g = kv.second->get_guild_info(guild_id);
            for (auto & rl : g.roles)
            {
                if (rl == role_id)
                {
                    auto it = std::find(g.roles.begin(), g.roles.end(), role_id);
                    if (it != g.roles.end())
                        g.roles.erase(it);
                    break;
                }
            }
        }
        roles.erase(role_id);
    }
    catch (std::out_of_range &)
    {

    }
}

AEGIS_DECL int32_t guild::get_member_count() const AEGIS_NOEXCEPT
{
    return static_cast<int32_t>(members.size());
}

AEGIS_DECL void guild::load(const json & obj, shard * _shard) AEGIS_NOEXCEPT
{
    //uint64_t application_id = obj->get("application_id").convert<uint64_t>();
    snowflake g_id = obj["id"];

    shard_id = _shard->get_id();
    is_init = false;

    core & bot = get_bot();
    try
    {
        json voice_states;

        if (!obj["name"].is_null()) name = obj["name"].get<std::string>();
        if (!obj["icon"].is_null()) icon = obj["icon"].get<std::string>();
        if (!obj["splash"].is_null()) splash = obj["splash"].get<std::string>();
        owner_id = obj["owner_id"];
        region = obj["region"].get<std::string>();
        if (!obj["afk_channel_id"].is_null()) afk_channel_id = obj["afk_channel_id"];
        afk_timeout = obj["afk_timeout"];//in seconds
        if (obj.count("embed_enabled") && !obj["embed_enabled"].is_null()) embed_enabled = obj["embed_enabled"];
        //_guild.embed_channel_id = obj->get("embed_channel_id").convert<uint64_t>();
        verification_level = obj["verification_level"];
        default_message_notifications = obj["default_message_notifications"];
        mfa_level = obj["mfa_level"];
        if (obj.count("joined_at") && !obj["joined_at"].is_null()) joined_at = obj["joined_at"].get<std::string>();
        if (obj.count("large") && !obj["large"].is_null()) large = obj["large"];
        if (obj.count("unavailable") && !obj["unavailable"].is_null())
            unavailable = obj["unavailable"];
        else
            unavailable = false;
        if (obj.count("member_count") && !obj["member_count"].is_null()) member_count = obj["member_count"];
        if (obj.count("voice_states") && !obj["voice_states"].is_null()) voice_states = obj["voice_states"];


        if (obj.count("roles"))
        {
            const json & roles = obj["roles"];

            for (auto & role : roles)
            {
                load_role(role);
            }
        }

        if (obj.count("members"))
        {
            const json & members = obj["members"];

            for (auto & member : members)
            {
                snowflake member_id = member["user"]["id"];
                auto _member = bot.member_create(member_id);
                _member->load(this, member, _shard);
                this->members.emplace(member_id, _member);
            }
        }

        if (obj.count("channels"))
        {
            const json & channels = obj["channels"];

            for (auto & channel_obj : channels)
            {
                snowflake channel_id = channel_obj["id"];
                auto _channel = bot.channel_create(channel_id);
                _channel->load_with_guild(*this, channel_obj, _shard);
                _channel->guild_id = guild_id;
                _channel->_guild = this;
                this->channels.emplace(channel_id, _channel);
            }
        }

        if (obj.count("presences"))
        {
            const json & presences = obj["presences"];

            for (auto & presence : presences)
            {
                load_presence(presence);
            }
        }

        if (obj.count("emojis"))
        {
            const json & emojis = obj["emojis"];

            /*for (auto & emoji : emojis)
            {
            //loadEmoji(emoji, _guild);
            }*/
        }

        if (obj.count("features"))
        {
            const json & features = obj["features"];

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
        spdlog::get("aegis")->error("Shard#{} : Error processing guild[{}] {}", _shard->get_id(), g_id, (std::string)e.what());
    }
}
#else
AEGIS_DECL void guild::load(const json & obj, shard * _shard) AEGIS_NOEXCEPT
{
    //uint64_t application_id = obj->get("application_id").convert<uint64_t>();
    snowflake g_id = obj["id"];

    shard_id = _shard->get_id();

    core & bot = get_bot();
    try
    {
        if (obj.count("channels"))
        {
            const json & channels = obj["channels"];

            for (auto & channel_obj : channels)
            {
                snowflake channel_id = channel_obj["id"];
                auto _channel = bot.channel_create(channel_id);
                _channel->load_with_guild(*this, channel_obj, _shard);
                _channel->guild_id = guild_id;
                _channel->_guild = this;
                this->channels.emplace(channel_id, _channel);
            }
        }
    }
    catch (std::exception&e)
    {
        spdlog::get("aegis")->error("Shard#{} : Error processing guild[{}] {}", _shard->get_id(), g_id, (std::string)e.what());
    }
}
#endif

AEGIS_DECL void guild::remove_channel(snowflake channel_id) AEGIS_NOEXCEPT
{
    auto it = channels.find(channel_id);
    if (it == channels.end())
    {
        get_bot().log->debug("Unable to remove channel [{}] from guild [{}] (does not exist)", channel_id, guild_id);
        return;
    }
    channels.erase(it);
}

AEGIS_DECL channel * guild::get_channel(snowflake id) const AEGIS_NOEXCEPT
{
    std::shared_lock<shared_mutex> l(_m);
    auto it = channels.find(id);
    if (it == channels.end())
        return nullptr;
    return it->second;
}

AEGIS_DECL std::future<rest::rest_reply> guild::post_task(const std::string path, const std::string method, const std::string obj, const std::string host)
{
    using result = asio::async_result<asio::use_future_t<>, void(rest::rest_reply)>;
    using handler = typename result::completion_handler_type;

    handler exec(std::forward<decltype(asio::use_future)>(asio::use_future));
    result ret(exec);

    asio::post(_io_context, [exec, this, path, obj, method, host]() mutable
    {
        exec(_ratelimit.get(rest::bucket_type::GUILD).do_async(guild_id, path, obj, method, host));
    });
    return ret.get();
}

/**\todo Incomplete. Signature may change. Location may change.
 */
AEGIS_DECL std::future<rest::rest_reply> guild::get_guild(std::error_code & ec)
{
    ec = error_code();
    return post_task(fmt::format("/guilds/{}", guild_id), "GET");
}

AEGIS_DECL std::future<rest::rest_reply> guild::modify_guild(std::error_code & ec, std::optional<std::string> name, std::optional<std::string> voice_region, std::optional<int> verification_level,
                    std::optional<int> default_message_notifications, std::optional<int> explicit_content_filter, std::optional<snowflake> afk_channel_id, std::optional<int> afk_timeout,
                    std::optional<std::string> icon, std::optional<snowflake> owner_id, std::optional<std::string> splash)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if ((!perms().can_manage_guild()) || (owner_id.has_value() && owner_id != self()->member_id))
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    json obj;
    if (name.has_value())
        obj["name"] = name.value();
    if (voice_region.has_value())
        obj["region"] = voice_region.value();
    if (verification_level.has_value())
        obj["verification_level"] = verification_level.value();
    if (default_message_notifications.has_value())
        obj["default_message_notifications"] = default_message_notifications.value();
    if (verification_level.has_value())
        obj["explicit_content_filter"] = verification_level.value();
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

    ec = error_code();
    return post_task(fmt::format("/guilds/{}", guild_id), "PATCH", obj.dump());
}

AEGIS_DECL std::future<rest::rest_reply> guild::delete_guild(std::error_code & ec)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    //requires OWNER
    if (owner_id != self()->member_id)
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    return post_task(fmt::format("/guilds/{}", guild_id), "DELETE");
}

AEGIS_DECL std::future<rest::rest_reply> guild::create_text_channel(std::error_code & ec, std::string name, int64_t parent_id, bool nsfw, std::vector<gateway::objects::permission_overwrite> permission_overwrites)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    //requires MANAGE_CHANNELS
    if (!perms().can_manage_channels())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    json obj;
    obj["name"] = name;
    obj["type"] = 0;
    obj["parent_id"] = parent_id;
    obj["nsfw"] = nsfw;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow);
    }

    return post_task(fmt::format("/guilds/{}/channels", guild_id), "POST", obj.dump());
}

AEGIS_DECL std::future<rest::rest_reply> guild::create_voice_channel(std::error_code & ec, std::string name, int32_t bitrate, int32_t user_limit, int64_t parent_id, std::vector<gateway::objects::permission_overwrite> permission_overwrites)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_channels())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    json obj;
    obj["name"] = name;
    obj["type"] = 2;
    obj["bitrate"] = bitrate;
    obj["user_limit"] = user_limit;
    obj["parent_id"] = parent_id;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow);
    }

    ec = error_code();
    return post_task(fmt::format("/guilds/{}/channels", guild_id), "POST", obj.dump());
}

AEGIS_DECL std::future<rest::rest_reply> guild::create_category_channel(std::error_code & ec, std::string name, int64_t parent_id, std::vector<gateway::objects::permission_overwrite> permission_overwrites)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_channels())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    json obj;
    obj["name"] = name;
    obj["type"] = 4;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow);
    }

    ec = error_code();
    return post_task(fmt::format("/guilds/{}/channels", guild_id), "POST", obj.dump());
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::modify_channel_positions(std::error_code & ec)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_channels())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

AEGIS_DECL std::future<rest::rest_reply> guild::modify_guild_member(std::error_code & ec, snowflake user_id, std::optional<std::string> nick, std::optional<bool> mute,
                            std::optional<bool> deaf, std::optional<std::vector<snowflake>> roles, std::optional<snowflake> channel_id)
{
    json obj;
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    permission perm = perms();
    if (nick.has_value())
    {
        if (!perm.can_manage_names())
        {
            ec = make_error_code(error::no_permission);
            return {};
        }
        obj["nick"] = nick.value();//requires MANAGE_NICKNAMES
    }
    if (mute.has_value())
    {
        if (!perm.can_voice_mute())
        {
            ec = make_error_code(error::no_permission);
            return {};
        }
        obj["mute"] = mute.value();//requires MUTE_MEMBERS
    }
    if (deaf.has_value())
    {
        if (!perm.can_voice_deafen())
        {
            ec = make_error_code(error::no_permission);
            return {};
        }
        obj["deaf"] = deaf.value();//requires DEAFEN_MEMBERS
    }
    if (roles.has_value())
    {
        if (!perm.can_manage_roles())
        {
            ec = make_error_code(error::no_permission);
            return {};
        }
        obj["roles"] = roles.value();//requires MANAGE_ROLES
    }
    if (channel_id.has_value())
    {
        //TODO: This needs to calculate whether or not the bot has access to the voice channel as well
        if (!perm.can_voice_move())
        {
            ec = make_error_code(error::no_permission);
            return {};
        }
        obj["channel_id"] = channel_id.value();//requires MOVE_MEMBERS
    }
#else
    if (nick.has_value())
        obj["nick"] = nick.value();//requires MANAGE_NICKNAMES
    if (mute.has_value())
        obj["mute"] = mute.value();//requires MUTE_MEMBERS
    if (deaf.has_value())
        obj["deaf"] = deaf.value();//requires DEAFEN_MEMBERS
    if (roles.has_value())
        obj["roles"] = roles.value();//requires MANAGE_ROLES
    if (channel_id.has_value())
        obj["channel_id"] = channel_id.value();//requires MOVE_MEMBERS
#endif

    ec = error_code();
    return post_task(fmt::format("/guilds/{}/members/{}", guild_id, user_id), "PATCH", obj.dump());
}

AEGIS_DECL std::future<rest::rest_reply> guild::modify_my_nick(std::error_code & ec, std::string newname)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_change_name())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    json obj = { "nick", newname };
    ec = error_code();
    return post_task(fmt::format("/guilds/{}/members/@me/nick", guild_id), "PATCH", obj.dump());
}

AEGIS_DECL std::future<rest::rest_reply> guild::add_guild_member_role(std::error_code & ec, snowflake user_id, snowflake role_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = error_code();
    return post_task(fmt::format("/guilds/{}/members/{}/roles/{}", guild_id, user_id, role_id), "PUT");
}

AEGIS_DECL std::future<rest::rest_reply> guild::remove_guild_member_role(std::error_code & ec, snowflake user_id, snowflake role_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = error_code();
    return post_task(fmt::format("/guilds/{}/members/{}/roles/{}", guild_id, user_id, role_id), "DELETE");
}

AEGIS_DECL std::future<rest::rest_reply> guild::remove_guild_member(std::error_code & ec, snowflake user_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_kick())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = error_code();
    return post_task(fmt::format("/guilds/{}/members/{}", guild_id, user_id), "DELETE");
}

AEGIS_DECL std::future<rest::rest_reply> guild::create_guild_ban(std::error_code & ec, snowflake user_id, int8_t delete_message_days, std::string reason)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_ban())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    json obj;
    if (reason.empty())
        obj = { "delete-message-days", delete_message_days };
    else
        obj = { { "delete-message-days", delete_message_days }, { "reason", reason } };

    ec = error_code();
    return post_task(fmt::format("/guilds/{}/bans/{}", guild_id, user_id), "PUT", obj.dump());
}

AEGIS_DECL std::future<rest::rest_reply> guild::remove_guild_ban(std::error_code & ec, snowflake user_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_ban())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = error_code();
    return post_task(fmt::format("/guilds/{}/bans/{}", guild_id, user_id), "DELETE");
}

AEGIS_DECL std::future<rest::rest_reply> guild::create_guild_role(std::error_code & ec, std::string name, permission _perms, int32_t color, bool hoist, bool mentionable)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    json obj = { { "name", name },{ "permissions", _perms },{ "color", color },{ "hoist", hoist },{ "mentionable", mentionable } };
    ec = error_code();
    return post_task(fmt::format("/guilds/{}/roles", guild_id), "POST", obj.dump());
}

AEGIS_DECL std::future<rest::rest_reply> guild::modify_guild_role_positions(std::error_code & ec, snowflake role_id, int16_t position)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    json obj = { { "id", role_id },{ "position", position } };
    ec = error_code();
    return post_task(fmt::format("/guilds/{}/roles", guild_id), "PATCH", obj.dump());
}

AEGIS_DECL std::future<rest::rest_reply> guild::modify_guild_role(std::error_code & ec, snowflake role_id, std::string name, permission _perms, int32_t color, bool hoist, bool mentionable)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    json obj = { { "name", name },{ "permissions", _perms },{ "color", color },{ "hoist", hoist },{ "mentionable", mentionable } };
    ec = error_code();
    return post_task(fmt::format("/guilds/{}/roles/{}", guild_id, role_id), "POST", obj.dump());
}

AEGIS_DECL std::future<rest::rest_reply> guild::delete_guild_role(std::error_code & ec, snowflake role_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = error_code();
    return post_task(fmt::format("/guilds/{}/roles/{}", guild_id, role_id), "DELETE");
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::get_guild_prune_count(std::error_code & ec, int16_t days)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_kick())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::begin_guild_prune(std::error_code & ec, int16_t days)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_kick())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::get_guild_invites(std::error_code & ec)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::get_guild_integrations(std::error_code & ec)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::create_guild_integration(std::error_code & ec)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::modify_guild_integration(std::error_code & ec)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::delete_guild_integration(std::error_code & ec)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::sync_guild_integration(std::error_code & ec)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::get_guild_embed(std::error_code & ec)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL std::future<rest::rest_reply> guild::modify_guild_embed(std::error_code & ec)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
    {
        ec = make_error_code(error::no_permission);
        return {};
    }
#endif

    ec = make_error_code(error::not_implemented);
    return {};
}

AEGIS_DECL std::future<rest::rest_reply> guild::leave(std::error_code & ec)
{
    ec = error_code();
    return post_task(fmt::format("/users/@me/guilds/{0}", guild_id), "DELETE");
}

}
