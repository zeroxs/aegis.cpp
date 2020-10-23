//
// guild.cpp
// *********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/guild.hpp"
#include <string>
#include <memory>
#include "aegis/core.hpp"
#include "aegis/user.hpp"
#include "aegis/channel.hpp"
#include "aegis/error.hpp"
#include "aegis/shards/shard.hpp"
#include "aegis/ratelimit/ratelimit.hpp"

namespace aegis
{

using json = nlohmann::json;

AEGIS_DECL guild::guild(const int32_t _shard_id, const snowflake _id, core * _bot, asio::io_context & _io)
    : shard_id(_shard_id)
    , guild_id(_id)
    , _bot(_bot)
    , _io_context(_io)
{

}

AEGIS_DECL guild::~guild()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    //TODO: remove guilds from members elsewhere when bot is removed from guild
//     if (get_bot().get_state() != Shutdown)
//         for (auto & v : members)
//             v.second->leave(guild_id);
#endif
}

AEGIS_DECL core & guild::get_bot() const noexcept
{
    return *_bot;
}

#if !defined(AEGIS_DISABLE_ALL_CACHE)
AEGIS_DECL user * guild::self() const
{
    return get_bot().self();
}

AEGIS_DECL void guild::_add_member(user * _member) noexcept
{
    std::unique_lock<shared_mutex> l(_m);
    members.emplace(_member->_member_id, _member);
}

AEGIS_DECL void guild::_add_member_nolock(user * _member) noexcept
{
    members.emplace(_member->_member_id, _member);
}

AEGIS_DECL void guild::_remove_member(snowflake member_id) noexcept
{
    std::unique_lock<shared_mutex> l(_m);
    auto _member = members.find(member_id);
    if (_member == members.end())
    {
        AEGIS_DEBUG(get_bot().log, "Unable to remove member [{}] from guild [{}] (does not exist)", member_id, guild_id);
        return;
    }
    _member->second->leave(guild_id);
    members.erase(member_id);
}

AEGIS_DECL bool guild::member_has_role(snowflake member_id, snowflake role_id) const noexcept
{
    std::shared_lock<shared_mutex> l(_m);
    auto _member = find_member(member_id);
    if (_member == nullptr)
        return false;
    auto & gi = _member->get_guild_info(guild_id);
    auto it = std::find_if(std::begin(gi.roles), std::end(gi.roles), [&](const snowflake & id)
    {
        if (id == role_id)
            return true;
        return false;
    });
    if (it != std::end(gi.roles))
        return true;
    return false;
}

AEGIS_DECL void guild::_load_emoji(const json & obj) noexcept
{
    snowflake emoji_id = obj["id"];
    auto & _emoji = emojis[emoji_id];
    _emoji = obj;
}

AEGIS_DECL void guild::_load_voicestate(const json & obj) noexcept
{
    snowflake user_id = obj["user_id"];
    if(obj["channel_id"].is_null()) {
        voice_states.erase(user_id);
    } else {
        auto & _voicestate = voice_states[user_id];
        _voicestate = obj;
    }
}

AEGIS_DECL void guild::_load_presence(const json & obj) noexcept
{
    json user = obj["user"];

    auto _member = _find_member(user["id"]);
    if (_member == nullptr)
        return;

    using user_status = aegis::gateway::objects::presence::user_status;

    const std::string & sts = obj["status"];

    if (sts == "idle")
        _member->_status = user_status::Idle;
    else if (sts == "dnd")
        _member->_status = user_status::DoNotDisturb;
    else if (sts == "online")
        _member->_status = user_status::Online;
    else
        _member->_status = user_status::Offline;
}

AEGIS_DECL void guild::_load_role(const json & obj) noexcept
{
    snowflake role_id = obj["id"];
    auto & _role = roles[role_id];
    _role = obj;
}

AEGIS_DECL const snowflake guild::get_owner() const noexcept
{
    return owner_id;
}

AEGIS_DECL user * guild::find_member(snowflake member_id) const noexcept
{
    std::shared_lock<shared_mutex> l(_m);
    auto m = members.find(member_id);
    if (m == members.end())
        return nullptr;
    return m->second;
}

AEGIS_DECL user * guild::_find_member(snowflake member_id) const noexcept
{
    auto m = members.find(member_id);
    if (m == members.end())
        return nullptr;
    return m->second;
}

AEGIS_DECL channel* guild::_find_channel(snowflake channel_id) const noexcept
{
    auto m = channels.find(channel_id);
    if (m == channels.end())
        return nullptr;
    return m->second;
}

AEGIS_DECL channel * guild::find_channel(snowflake channel_id) const noexcept
{
    std::shared_lock<shared_mutex> l(_m);
    auto m = channels.find(channel_id);
    if (m == channels.end())
        return nullptr;
    return m->second;
}

AEGIS_DECL channel * guild::find_channel(std::string channel_name) const noexcept
{
    std::shared_lock<shared_mutex> l(_m);
    for (auto & c : channels)
        if (c.second->get_name() == channel_name)
            return c.second;
    return nullptr;
}

AEGIS_DECL lib::optional<gateway::objects::role> guild::find_role(snowflake role_id) const noexcept
{
    std::shared_lock<shared_mutex> l(_m);
    auto r = roles.find(role_id);
    if (r == roles.end())
        return {};
    return r->second;
}

AEGIS_DECL lib::optional<gateway::objects::role> guild::find_role(std::string role_name) const noexcept
{
    std::shared_lock<shared_mutex> l(_m);
    for (auto & r : roles)
        if (r.second.name == role_name)
            return r.second;
    return {};
}

AEGIS_DECL permission guild::get_permissions(snowflake member_id, snowflake channel_id) const
{
    std::shared_lock<shared_mutex> l(_m);
    if (!members.count(member_id) || !channels.count(channel_id))
        return 0;
    return get_permissions(_find_member(member_id), _find_channel(channel_id));
}

AEGIS_DECL permission guild::get_permissions(const user * _member, const channel * _channel) const
{
    if (_member == nullptr || _channel == nullptr)
        return 0;

    int64_t _base_permissions = base_permissions(_member);

    return compute_overwrites(_base_permissions, *_member, *_channel);
}

AEGIS_DECL int64_t guild::base_permissions(const user & _member) const noexcept
{
    try
    {
        if (owner_id == _member._member_id)
            return ~0;

        auto & role_everyone = get_role(guild_id);
        int64_t permissions = role_everyone._permission.get_allow_perms();

        auto g = _member.get_guild_info_nocreate(guild_id);
	if (g == nullptr) {
		return 0;
	}

        for (auto & rl : g->roles)
            permissions |= get_role(rl)._permission.get_allow_perms();

        if (permissions & 0x8)//admin
            return ~0;

        return permissions;
    }
    catch (std::out_of_range &)
    {
        return 0;
    }
    catch (std::exception & e)
    {
        _bot->log->error(fmt::format("guild::base_permissions() [{}]", e.what()));
        return 0;
    }
    catch (...)
    {
        _bot->log->error("guild::base_permissions uncaught");
        return 0;
    }
}

AEGIS_DECL int64_t guild::compute_overwrites(const int64_t _base_permissions, const user & _member, const channel & _channel) const noexcept
{
    try
    {
        if (_base_permissions & 0x8)//admin
            return ~0;

        int64_t permissions = _base_permissions;
        {
            auto it = _channel.overrides.find(guild_id);
            if (it != _channel.overrides.end())
            {
                auto & overwrite_everyone = it->second;
                permissions &= ~overwrite_everyone.deny;
                permissions |= overwrite_everyone.allow;
            }
        }

        auto & overwrites = _channel.overrides;
        int64_t allow = 0;
        int64_t deny = 0;
        auto g = _member.get_guild_info_nocreate(guild_id);
        if (g == nullptr)
        {
            //could not find guild cache within member - use base permissions
            _bot->log->warn("Member does not have guild info struct : m:[{}] g:[{}]", _member.get_id(), guild_id);
            return 0;
        }
        for (auto & rl : g->roles)
        {
            if (rl == guild_id)
                continue;
            auto it = overwrites.find(rl);
            if (it != overwrites.end())
            {
                auto & ow_role = it->second;
                allow |= ow_role.allow;
                deny |= ow_role.deny;
            }
        }

        permissions &= ~deny;
        permissions |= allow;

        {
            auto it = overwrites.find(_member._member_id);
            if (it != overwrites.end())
            {
                auto & ow_role = it->second;
                permissions &= ~ow_role.deny;
                permissions |= ow_role.allow;
            }
        }

        return permissions;
    }
    catch (std::exception &)
    {
        return 0;
    }
}

AEGIS_DECL const gateway::objects::role guild::get_role(int64_t r) const
{
    std::shared_lock<shared_mutex> l(_m);
    for (auto & kv : roles)
        if (kv.second.role_id == r)
            return kv.second;
    throw std::out_of_range(fmt::format("G: {} role:[{}] does not exist", guild_id, r));
}

AEGIS_DECL void guild::_remove_role(snowflake role_id) noexcept
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

AEGIS_DECL int32_t guild::get_member_count() const noexcept
{
    return static_cast<int32_t>(members.size());
}

AEGIS_DECL void guild::_load(const json & obj, shards::shard * _shard)
{
    std::unique_lock<shared_mutex> l(_m);
    //uint64_t application_id = obj->get("application_id").convert<uint64_t>();
    snowflake g_id = obj["id"];

    shard_id = _shard->get_id();
    is_init = false;

    core & bot = get_bot();
    try
    {
        json voice_states;

        if (obj.count("name") && !obj["name"].is_null()) name = obj["name"].get<std::string>();
        if (obj.count("icon") && !obj["icon"].is_null()) icon = obj["icon"].get<std::string>();
        if (obj.count("splash") && !obj["splash"].is_null()) splash = obj["splash"].get<std::string>();
        if (obj.count("owner_id") && !obj["owner_id"].is_null()) owner_id = obj["owner_id"];
        if (obj.count("region") && !obj["region"].is_null()) region = obj["region"].get<std::string>();
        if (obj.count("afk_channel_id") && !obj["afk_channel_id"].is_null()) afk_channel_id = obj["afk_channel_id"];
        if (obj.count("afk_timeout") && !obj["afk_timeout"].is_null()) afk_timeout = obj["afk_timeout"];//in seconds
        if (obj.count("embed_enabled") && !obj["embed_enabled"].is_null()) embed_enabled = obj["embed_enabled"];
        //_guild.embed_channel_id = obj->get("embed_channel_id").convert<uint64_t>();
        if (obj.count("verification_level") && !obj["verification_level"].is_null()) verification_level = obj["verification_level"];
        if (obj.count("default_message_notifications") && !obj["default_message_notifications"].is_null()) default_message_notifications = obj["default_message_notifications"];
        if (obj.count("mfa_level") && !obj["mfa_level"].is_null()) mfa_level = obj["mfa_level"];
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
                _load_role(role);
            }
        }

        if (obj.count("members"))
        {
            const json & members = obj["members"];

            for (auto & member : members)
            {
                const json & obj = member["user"];
                snowflake member_id = obj["id"];
                auto _member = bot.user_create(member_id);
                _member->_load(this, member, _shard, false);
                this->members.emplace(member_id, _member);

                {
                    auto & g_info = _member->_join(guild_id);


                    if (obj.count("deaf") && !obj["deaf"].is_null()) g_info.deaf = obj["deaf"];
                    if (obj.count("mute") && !obj["mute"].is_null()) g_info.mute = obj["mute"];

                    if (obj.count("joined_at") && !obj["joined_at"].is_null())// g_info.value()->joined_at = obj["joined_at"];
                    {
                        g_info.joined_at = utility::from_iso8601(obj["joined_at"]).time_since_epoch().count();
                    }

                    if (obj.count("roles") && !obj["roles"].is_null())
                    {
                        g_info.roles.clear();
                        g_info.roles.emplace_back(guild_id);//default everyone role

                        json roles = obj["roles"];
                        for (auto & r : roles)
                            g_info.roles.emplace_back(std::stoull(r.get<std::string>()));
                    }

                    if (obj.count("nick") && !obj["nick"].is_null())
                        g_info.nickname = obj["nick"].get<std::string>();
                }

            }
        }

        if (obj.count("channels"))
        {
            const json & channels = obj["channels"];

            for (auto & channel_obj : channels)
            {
                snowflake channel_id = channel_obj["id"];
                auto _channel = bot.channel_create(channel_id);
                _channel->_load_with_guild(*this, channel_obj, _shard);
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
                _load_presence(presence);
            }
        }

        if (obj.count("emojis"))
        {
            const json & emojis = obj["emojis"];

            for (auto & emoji : emojis)
            {
                _load_emoji(emoji);
            }
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
        */

        for (auto & voicestate : voice_states)
        {
            _load_voicestate(voicestate);
        }



    }
    catch (std::exception&e)
    {
        spdlog::get("aegis")->error("Shard#{} : Error processing guild[{}] {}", _shard->get_id(), g_id, (std::string)e.what());
        std::rethrow_exception(std::current_exception());
    }
    catch (...)
    {
        spdlog::get("aegis")->error("Shard#{} : Error processing guild[{}]", _shard->get_id(), g_id);
        std::rethrow_exception(std::current_exception());
    }
}
#else
AEGIS_DECL void guild::_load(const json & obj, shards::shard * _shard) noexcept
{
    std::unique_lock<shared_mutex> l(_m);
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
                _channel->_load_with_guild(*this, channel_obj, _shard);
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

AEGIS_DECL void guild::_remove_channel(snowflake channel_id) noexcept
{
    std::unique_lock<shared_mutex> l(_m);
     auto it = channels.find(channel_id);
    if (it == channels.end())
    {
        AEGIS_DEBUG(get_bot().log, "Unable to remove channel [{}] from guild [{}] (does not exist)", channel_id, guild_id);
        return;
    }
    channels.erase(it);
}

AEGIS_DECL channel * guild::get_channel(snowflake id) const noexcept
{
    std::shared_lock<shared_mutex> l(_m);
    auto it = channels.find(id);
    if (it == channels.end())
        return nullptr;
    return it->second;
}

/**\todo Incomplete. Signature may change. Location may change.
 */
AEGIS_DECL aegis::future<gateway::objects::guild> guild::get_guild()
{
    std::shared_lock<shared_mutex> l(_m);
    return _bot->get_ratelimit().post_task<gateway::objects::guild>({ fmt::format("/guilds/{}", guild_id), rest::Get });
}

AEGIS_DECL aegis::future<gateway::objects::guild> guild::modify_guild(lib::optional<std::string> name, lib::optional<std::string> voice_region, lib::optional<int> verification_level,
                    lib::optional<int> default_message_notifications, lib::optional<int> explicit_content_filter, lib::optional<snowflake> afk_channel_id, lib::optional<int> afk_timeout,
                    lib::optional<std::string> icon, lib::optional<snowflake> owner_id, lib::optional<std::string> splash)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if ((!perms().can_manage_guild()) || (owner_id.has_value() && owner_id != self()->_member_id))
        return aegis::make_exception_future<gateway::objects::guild>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

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

    return _bot->get_ratelimit().post_task<gateway::objects::guild>({ fmt::format("/guilds/{}", guild_id), rest::Patch, obj.dump() });
}

AEGIS_DECL aegis::future<rest::rest_reply> guild::delete_guild()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    //requires OWNER
    if (owner_id != self()->_member_id)
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);
  
    return _bot->get_ratelimit().post_task({ fmt::format("/guilds/{}", guild_id), rest::Delete });
}

AEGIS_DECL aegis::future<gateway::objects::channel> guild::create_text_channel(const std::string & name,
                int64_t parent_id, bool nsfw, const std::vector<gateway::objects::permission_overwrite> & permission_overwrites)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    //requires MANAGE_CHANNELS
    if (!perms().can_manage_channels())
        return aegis::make_exception_future<gateway::objects::channel>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj;
    obj["name"] = name;
    obj["type"] = 0;
    if (parent_id != 0)
        obj["parent_id"] = parent_id;
    obj["nsfw"] = nsfw;
    if (permission_overwrites.size() > 0)
    {
        obj["permission_overwrites"] = json::array();
        for (auto & p_ow : permission_overwrites)
        {
            obj["permission_overwrites"].push_back(p_ow);
        }
    }

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    return _bot->get_ratelimit().post_task<gateway::objects::channel>({ fmt::format("/guilds/{}/channels", guild_id), rest::Post, obj.dump() }).then([&](gateway::objects::channel _ch)
    {
        channel * _t_ch = this->get_bot().channel_create(_ch.channel_id);
        _t_ch->guild_id = _ch.guild_id;
        _t_ch->_guild = this;
        _t_ch->name = _ch.name;
        _t_ch->topic = _ch.topic;
        _t_ch->_nsfw = _ch.nsfw;
        _t_ch->position = _ch.position;
        //_t_ch->overrides = _ch.permission_overwrites;
        //_t_ch->icon = _ch.icon;
        return _ch;
    });
#else
    return _bot->get_ratelimit().post_task<gateway::objects::channel>({ fmt::format("/guilds/{}/channels", guild_id), rest::Post, obj.dump() });
#endif
}

AEGIS_DECL aegis::future<gateway::objects::channel> guild::create_voice_channel(const std::string & name,
                int32_t bitrate, int32_t user_limit, int64_t parent_id,
                const std::vector<gateway::objects::permission_overwrite> & permission_overwrites)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_channels())
        return aegis::make_exception_future<gateway::objects::channel>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj;
    obj["name"] = name;
    obj["type"] = 2;
    if (bitrate > 0)
        obj["bitrate"] = bitrate;
    if (user_limit > 0)
        obj["user_limit"] = user_limit;
    if (parent_id > 0)
        obj["parent_id"] = parent_id;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow);
    }

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    return _bot->get_ratelimit().post_task<gateway::objects::channel>({ fmt::format("/guilds/{}/channels", guild_id), rest::Post, obj.dump() }).then([&](gateway::objects::channel _ch)
    {
        channel * _t_ch = this->get_bot().channel_create(_ch.channel_id);
        _t_ch->guild_id = _ch.guild_id;
        _t_ch->_guild = this;
        _t_ch->name = _ch.name;
        _t_ch->topic = _ch.topic;
        _t_ch->_nsfw = _ch.nsfw;
        _t_ch->position = _ch.position;
        //_t_ch->overrides = _ch.permission_overwrites;
        //_t_ch->icon = _ch.icon;
        return _ch;
    });
#else
    return _bot->get_ratelimit().post_task<gateway::objects::channel>({ fmt::format("/guilds/{}/channels", guild_id), rest::Post, obj.dump() });
#endif
}

AEGIS_DECL aegis::future<gateway::objects::channel> guild::create_category_channel(const std::string & name,
                int64_t parent_id, const std::vector<gateway::objects::permission_overwrite> & permission_overwrites)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_channels())
        return aegis::make_exception_future<gateway::objects::channel>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj;
    obj["name"] = name;
    obj["type"] = 4;
    obj["permission_overwrites"] = json::array();
    for (auto & p_ow : permission_overwrites)
    {
        obj["permission_overwrites"].push_back(p_ow);
    }

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    return _bot->get_ratelimit().post_task<gateway::objects::channel>({ fmt::format("/guilds/{}/channels", guild_id), rest::Post, obj.dump() }).then([&](gateway::objects::channel _ch)
    {
        channel * _t_ch = this->get_bot().channel_create(_ch.channel_id);
        _t_ch->guild_id = _ch.guild_id;
        _t_ch->_guild = this;
        _t_ch->name = _ch.name;
        _t_ch->position = _ch.position;
        //_t_ch->overrides = _ch.permission_overwrites;
        //_t_ch->icon = _ch.icon;
        return _ch;
    });
#else
    return _bot->get_ratelimit().post_task<gateway::objects::channel>({ fmt::format("/guilds/{}/channels", guild_id), rest::Post, obj.dump() });
#endif
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::modify_channel_positions()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_channels())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

AEGIS_DECL aegis::future<gateway::objects::member> guild::modify_guild_member(snowflake user_id, lib::optional<std::string> nick, lib::optional<bool> mute,
                            lib::optional<bool> deaf, lib::optional<std::vector<snowflake>> roles, lib::optional<snowflake> channel_id)
{
    json obj;
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    permission perm = perms();
    if (nick.has_value())
    {
        if (!perm.can_manage_names())
            return aegis::make_exception_future<gateway::objects::member>(error::no_permission);
        obj["nick"] = nick.value();//requires MANAGE_NICKNAMES
    }
    if (mute.has_value())
    {
        if (!perm.can_voice_mute())
            return aegis::make_exception_future<gateway::objects::member>(error::no_permission);
        obj["mute"] = mute.value();//requires MUTE_MEMBERS
    }
    if (deaf.has_value())
    {
        if (!perm.can_voice_deafen())
            return aegis::make_exception_future<gateway::objects::member>(error::no_permission);
        obj["deaf"] = deaf.value();//requires DEAFEN_MEMBERS
    }
    if (roles.has_value())
    {
        if (!perm.can_manage_roles())
            return aegis::make_exception_future<gateway::objects::member>(error::no_permission);
        obj["roles"] = roles.value();//requires MANAGE_ROLES
    }
    if (channel_id.has_value())
    {
        //TODO: This needs to calculate whether or not the bot has access to the voice channel as well
        if (!perm.can_voice_move())
            return aegis::make_exception_future<gateway::objects::member>(error::no_permission);
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

    std::shared_lock<shared_mutex> l(_m);

    return _bot->get_ratelimit().post_task<gateway::objects::member>({ fmt::format("/guilds/{}/members/{}", guild_id, user_id), rest::Patch, obj.dump() });
}

AEGIS_DECL aegis::future<rest::rest_reply> guild::modify_my_nick(const std::string & newname)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_change_name())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj = { { "nick", newname } };
    return _bot->get_ratelimit().post_task({ fmt::format("/guilds/{}/members/@me/nick", guild_id), rest::Patch, obj.dump() });
}

AEGIS_DECL aegis::future<gateway::objects::role> guild::add_guild_member_role(snowflake user_id, snowflake role_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
        return aegis::make_exception_future<gateway::objects::role>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    return _bot->get_ratelimit().post_task<gateway::objects::role>({ fmt::format("/guilds/{}/members/{}/roles/{}", guild_id, user_id, role_id), rest::Put });
}

AEGIS_DECL aegis::future<rest::rest_reply> guild::remove_guild_member_role(snowflake user_id, snowflake role_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    return _bot->get_ratelimit().post_task({ fmt::format("/guilds/{}/members/{}/roles/{}", guild_id, user_id, role_id), rest::Delete });
}

AEGIS_DECL aegis::future<rest::rest_reply> guild::remove_guild_member(snowflake user_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_kick())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    return _bot->get_ratelimit().post_task({ fmt::format("/guilds/{}/members/{}", guild_id, user_id), rest::Delete });
}

AEGIS_DECL aegis::future<rest::rest_reply> guild::create_guild_ban(snowflake user_id, int8_t delete_message_days, const std::string & reason)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_ban())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj = { { "delete_message_days", delete_message_days }, { "reason", reason } };

    return _bot->get_ratelimit().post_task({ fmt::format("/guilds/{}/bans/{}", guild_id, user_id), rest::Put, obj.dump()});
}

AEGIS_DECL aegis::future<rest::rest_reply> guild::remove_guild_ban(snowflake user_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_ban())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    return _bot->get_ratelimit().post_task({ fmt::format("/guilds/{}/bans/{}", guild_id, user_id), rest::Delete });
}

AEGIS_DECL aegis::future<gateway::objects::bans> guild::get_guild_bans()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_ban())
        return aegis::make_exception_future<gateway::objects::bans>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    return _bot->get_ratelimit().post_task<gateway::objects::bans>({ fmt::format("/guilds/{}/bans", guild_id), rest::Get });
}

AEGIS_DECL aegis::future<gateway::objects::ban> guild::get_guild_ban(snowflake user_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_ban())
        return aegis::make_exception_future<gateway::objects::ban>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    return _bot->get_ratelimit().post_task<gateway::objects::ban>({ fmt::format("/guilds/{}/bans/{}", guild_id, user_id), rest::Get });
}

AEGIS_DECL aegis::future<gateway::objects::role> guild::create_guild_role(const std::string & name, permission _perms, int32_t color, bool hoist, bool mentionable)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
        return aegis::make_exception_future<gateway::objects::role>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj = { { "name", name },{ "permissions", _perms },{ "color", color },{ "hoist", hoist },{ "mentionable", mentionable } };
    return _bot->get_ratelimit().post_task<gateway::objects::role>({ fmt::format("/guilds/{}/roles", guild_id), rest::Post, obj.dump() });
}

AEGIS_DECL aegis::future<rest::rest_reply> guild::modify_guild_role_positions(snowflake role_id, int16_t position)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj = { { "id", role_id },{ "position", position } };
    return _bot->get_ratelimit().post_task({ fmt::format("/guilds/{}/roles", guild_id), rest::Patch, obj.dump() });
}

AEGIS_DECL aegis::future<gateway::objects::role> guild::modify_guild_role(snowflake role_id, const std::string & name, permission _perms, int32_t color, bool hoist, bool mentionable)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
        return aegis::make_exception_future<gateway::objects::role>(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    json obj = { { "name", name },{ "permissions", _perms },{ "color", color },{ "hoist", hoist },{ "mentionable", mentionable } };
    return _bot->get_ratelimit().post_task<gateway::objects::role>({ fmt::format("/guilds/{}/roles/{}", guild_id, role_id), rest::Post, obj.dump() });
}

AEGIS_DECL aegis::future<rest::rest_reply> guild::delete_guild_role(snowflake role_id)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_roles())
        return aegis::make_exception_future(error::no_permission);
#endif

    std::shared_lock<shared_mutex> l(_m);

    return _bot->get_ratelimit().post_task({ fmt::format("/guilds/{}/roles/{}", guild_id, role_id), rest::Delete });
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::get_guild_prune_count(int16_t days)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_kick())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::begin_guild_prune(int16_t days)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_kick())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::get_guild_invites()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::get_guild_integrations()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::create_guild_integration()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::modify_guild_integration()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::delete_guild_integration()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::sync_guild_integration()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::get_guild_embed()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

/**\todo Incomplete. Signature may change
 */
AEGIS_DECL aegis::future<rest::rest_reply> guild::modify_guild_embed()
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (!perms().can_manage_guild())
        return aegis::make_exception_future(error::no_permission);
#endif

    return aegis::make_exception_future(error::not_implemented);
}

AEGIS_DECL aegis::future<rest::rest_reply> guild::leave()
{
    std::unique_lock<shared_mutex> l(_m);

    return _bot->get_ratelimit().post_task({ fmt::format("/users/@me/guilds/{0}", guild_id), rest::Delete });
}

}
