//
// guild.hpp
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
#include "client.hpp"
#include "ratelimit.hpp"
#include "channel.hpp"
#include "member.hpp"
#include <json.hpp>

namespace aegis
{

using rest_limits::bucket_factory;
using json = nlohmann::json;

class guild
{
public:
    explicit guild(client & shard, snowflake id, bucket_factory & ratelimit)
        : m_shard(shard)
        , m_snowflake(id)
        , m_ratelimit(ratelimit)
        , m_log(spdlog::get("aegis"))
    {

    }

    client m_shard;
    snowflake m_snowflake;
    bucket_factory & m_ratelimit;
    std::shared_ptr<spdlog::logger> m_log;

    std::map<int64_t, std::unique_ptr<channel>> m_channels;
    std::map<int64_t, std::shared_ptr<member>> m_members;

    // move this to aegis?
    void create_guild() const
    {
        //TODO: 
    }

    void get_guild() const
    {
        //TODO: 
    }

    void modify_guild(std::optional<std::string> name, std::optional<std::string> voice_region, std::optional<int> verification_level,
                      std::optional<int> default_message_notifications, std::optional<snowflake> afk_channel_id, std::optional<int> afk_timeout,
                      std::optional<std::string> icon, std::optional<snowflake> owner_id, std::optional<std::string> splash) const
    {
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

        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}", m_snowflake), std::move(obj) , "PATCH", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("modify_guild success : {}", m_snowflake);
            else
                m_log->debug("modify_guild fail response: " + reply.content);
        });
    }

    void delete_guild() const
    {
        //requires OWNER
        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}", m_snowflake), "", "DELETE", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("delete_guild success : {}", m_snowflake);
            else
                m_log->debug("delete_guild fail response: " + reply.content);
        });
    }

    void create_text_channel(std::string name, int64_t parent_id, bool nsfw, std::vector<perm_overwrite> permission_overwrites) const
    {
        //requires MANAGE_CHANNELS

        json obj;
        obj["name"] = name;
        obj["type"] = 0;
        obj["permission_overwrites"] = json::array();
        for (auto & p_ow : permission_overwrites)
        {
            obj["permission_overwrites"].push_back(p_ow.make());
        }

        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}/channels", m_snowflake), obj.dump(), "POST", [&](rest_reply reply)
        {
        });
    }

    void create_voice_channel(std::string name, int32_t bitrate, int32_t user_limit, int64_t parent_id, bool nsfw, std::vector<perm_overwrite> permission_overwrites) const
    {
        json obj;
        obj["name"] = name;
        obj["type"] = 2;
        obj["permission_overwrites"] = json::array();
        for (auto & p_ow : permission_overwrites)
        {
            obj["permission_overwrites"].push_back(p_ow.make());
        }

        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}/channels", m_snowflake), obj.dump(), "POST", [&](rest_reply reply)
        {
        });
    }

    void create_category_channel(std::string name, int64_t parent_id, std::vector<perm_overwrite> permission_overwrites) const
    {
        json obj;
        obj["name"] = name;
        obj["type"] = 4;
        obj["permission_overwrites"] = json::array();
        for (auto & p_ow : permission_overwrites)
        {
            obj["permission_overwrites"].push_back(p_ow.make());
        }

        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}/channels", m_snowflake), obj.dump(), "POST", [&](rest_reply reply)
        {
        });
    }

    void modify_channel_positions()
    {
        //TODO:
    }

    void modify_guild_member(snowflake user_id, std::optional<std::string> nick, std::optional<bool> mute,
                             std::optional<bool> deaf, std::optional<std::vector<snowflake>> roles, std::optional<snowflake> channel_id) const
    {
        json obj;
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

        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}/members/{}", m_snowflake, user_id), obj.dump(), "PATCH", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("modify_guild_member success : {}", m_snowflake);
            else
                m_log->debug("modify_guild_member fail response: " + reply.content);
        });
    }

    void modify_my_nick(std::string newname) const
    {
        //requires CHANGE_NICKNAME
        json obj = { "nick", newname };
        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}/members/@me/nick", m_snowflake), obj.dump(), "PATCH", [&](rest_reply reply)
        {
            if (reply.reply_code == 200)
                m_log->debug("modify_my_nick success : {}", m_snowflake);
            else
                m_log->debug("modify_my_nick fail response: " + reply.content);
        });
    }

    void add_guild_member_role(snowflake user_id, snowflake role_id) const
    {
        //requires MANAGE_ROLES
        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}/members/{}/roles/{}", m_snowflake, user_id, role_id), "", "PUT", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("add_guild_member_role success : {}", m_snowflake);
            else
                m_log->debug("add_guild_member_role fail response: " + reply.content);
        });
    }

    void remove_guild_member_role(snowflake user_id, snowflake role_id) const
    {
        //requires MANAGE_ROLES
        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}/members/{}/roles/{}", m_snowflake, user_id, role_id), "", "DELETE", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("remove_guild_member_role success : {}", m_snowflake);
            else
                m_log->debug("remove_guild_member_role fail response: " + reply.content);
        });
    }

    void remove_guild_member(snowflake user_id) const
    {
        //requires KICK_MEMBERS
        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}/members/{}", m_snowflake, user_id), "", "DELETE", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("remove_guild_member success : {}", m_snowflake);
            else
                m_log->debug("remove_guild_member fail response: " + reply.content);
        });
    }

    void create_guild_ban(snowflake user_id, int8_t delete_message_days) const
    {
        //requires BAN_MEMBERS
        json obj = { "delete-message-days", delete_message_days };
        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}/bans/{}", m_snowflake, user_id), obj.dump(), "PUT", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("create_guild_ban success : {}", m_snowflake);
            else
                m_log->debug("create_guild_ban fail response: " + reply.content);
        });
    }

    void remove_guild_ban(snowflake user_id) const
    {
        //requires BAN_MEMBERS
        m_ratelimit.push(m_snowflake, fmt::format("/guilds/{}/bans/{}", m_snowflake, user_id), "", "DELETE", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("remove_guild_ban success : {}", m_snowflake);
            else
                m_log->debug("remove_guild_ban fail response: " + reply.content);
        });
    }

    void create_guild_role() const
    {
        //TODO: 
    }

    void modify_guild_role_positions() const
    {
        //TODO: 
    }

    void modify_guild_role(snowflake role_id) const
    {
        //TODO: 
    }

    void delete_guild_role(snowflake role_id) const
    {
        //TODO: 
    }

    void get_guild_prune_count(int16_t days) const
    {
        //TODO: 
    }

    void begin_guild_prune(int16_t days) const
    {
        //TODO: 
    }

    void get_guild_invites() const
    {
        //TODO: 
    }

    void get_guild_integrations() const
    {
        //TODO: 
    }

    void create_guild_integration() const
    {
        //TODO: 
    }

    void modify_guild_integration() const
    {
        //TODO: 
    }

    void delete_guild_integration() const
    {
        //TODO: 
    }

    void sync_guild_integration() const
    {
        //TODO: 
    }

    void get_guild_embed() const
    {
        //TODO: 
    }

    void modify_guild_embed() const
    {
        //TODO: 
    }



    void leave() const
    {
        m_ratelimit.push(m_snowflake, fmt::format("/users/@me/guilds/{0}", m_snowflake), "", "DELETE", [](rest_reply reply)
        {
            json response = json::parse(reply.content);
        });
    }
};

}
