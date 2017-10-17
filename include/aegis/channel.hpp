//
// channel.hpp
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
#include "ratelimit.hpp"
#include "snowflake.hpp"
#include <json.hpp>



namespace aegis
{

using json = nlohmann::json;
using rest_limits::bucket_factory;

class channel
{
public:
    explicit channel(snowflake id, snowflake guild_id, bucket_factory & ratelimit, bucket_factory & emoji)
        : m_snowflake(id)
        , m_guild_snowflake(guild_id)
        , m_ratelimit(ratelimit)
        , m_emoji(emoji)
        , m_log(spdlog::get("aegis"))
    {

    }


    snowflake m_snowflake;
    snowflake m_guild_snowflake;
    bucket_factory & m_ratelimit;
    bucket_factory & m_emoji;
    std::shared_ptr<spdlog::logger> m_log;

    void create_message(std::string content)
    {
        json obj;
        obj["content"] = content;
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/messages", m_snowflake), obj.dump(), "POST");
    }

    void create_message_embed(std::string content, json embed)
    {
        json obj;
        if (!content.empty())
            obj["content"] = content;
        obj["embed"] = embed;

        m_ratelimit.push(m_snowflake, fmt::format("/channels/{:d}/messages", m_snowflake), obj.dump(), "POST");
    }

    void edit_message(snowflake message_id, std::string content)
    {
        json obj;
        obj["content"] = content;
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/messages/{}", m_snowflake, message_id), obj.dump(), "PATCH");
    }

    void edit_message_embed(snowflake message_id, std::string content, json embed)
    {
        json obj;
        if (!content.empty())
            obj["content"] = content;
        obj["embed"] = embed;
        obj["content"] = content;
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/messages/{}", m_snowflake, message_id), obj.dump(), "PATCH");
    }

    void delete_message(snowflake message_id)
    {
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/messages/{}", m_snowflake, message_id), "", "DELETE");
    }

    void bulk_delete_message(snowflake message_id, std::vector<int64_t> messages)
    {
        json obj = messages;
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/messages/bulk-delete", m_snowflake), obj.dump(), "POST");
    }

    void modify_channel(std::optional<std::string> name, std::optional<int> position, std::optional<std::string> topic,
                        std::optional<bool> nsfw, std::optional<int> bitrate, std::optional<int> user_limit,
                        std::optional<std::vector<perm_overwrite>> permission_overwrites, std::optional<snowflake> parent_id) const
    {
        json obj;
        if (name.has_value())
            obj["name"] = name.value();
        if (position.has_value())
            obj["position"] = position.value();
        if (topic.has_value())
            obj["topic"] = topic.value();
        if (nsfw.has_value())
            obj["nsfw"] = nsfw.value();
        if (bitrate.has_value())//voice only
            obj["bitrate"] = bitrate.value();
        if (user_limit.has_value())//voice only
            obj["user_limit"] = user_limit.value();
        if (permission_overwrites.has_value())//requires OWNER
        {
            obj["permission_overwrites"] = json::array();
            for (auto & p_ow : permission_overwrites.value())
            {
                obj["permission_overwrites"].push_back(p_ow.make());
            }
        }
        if (parent_id.has_value())//VIP only
            obj["parent_id"] = parent_id.value();

        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}", m_snowflake), std::move(obj), "PATCH", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("modify_channel success : {}", m_snowflake);
            else
                m_log->debug("modify_channel fail response: " + reply.content);
        });
    }

    void delete_channel() const
    {
        //requires MANAGE_CHANNELS
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}", m_snowflake), "", "DELETE", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("delete_channel success : {}", m_snowflake);
            else
                m_log->debug("delete_channel fail response: " + reply.content);
        });
    }

    void create_reaction(snowflake message_id, std::string emoji) const
    {
        //requires ADD_REACTIONS
        m_emoji.push(m_snowflake, fmt::format("/channels/{}/messages/{}/reactions/{}/@me", m_snowflake, message_id, emoji), "", "PUT", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("create_reaction success : {}", m_snowflake);
            else
                m_log->debug("create_reaction fail response: " + reply.content);
        });
    }

    void delete_own_reaction(snowflake message_id, std::string emoji) const
    {
        //requires ADD_REACTIONS
        m_emoji.push(m_snowflake, fmt::format("/channels/{}/messages/{}/reactions/{}/@me", m_snowflake, message_id, emoji), "", "DELETE", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("delete_own_reaction success : {}", m_snowflake);
            else
                m_log->debug("delete_own_reaction fail response: " + reply.content);
        });
    }

    void delete_user_reaction(snowflake message_id, std::string emoji, snowflake user_id) const
    {
        //requires ADD_REACTIONS
        m_emoji.push(m_snowflake, fmt::format("/channels/{}/messages/{}/reactions/{}/{}", m_snowflake, message_id, emoji, user_id), "", "DELETE", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("delete_user_reaction success : {}", m_snowflake);
            else
                m_log->debug("delete_user_reaction fail response: " + reply.content);
        });
    }

    void get_reactions(snowflake message_id, std::string emoji)
    {
        //TODO: support query params?
        //before[snowflake], after[snowflake], limit[int]
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/messages/{}/reactions/{}", m_snowflake, message_id, emoji), "", "GET", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("get_reactions success : {}", m_snowflake);
            else
                m_log->debug("get_reactions fail response: " + reply.content);
        });
    }

    void delete_all_reactions(snowflake message_id) const
    {
        //requires ADD_REACTIONS
        m_emoji.push(m_snowflake, fmt::format("/channels/{}/messages/{}/reactions", m_snowflake, message_id), "", "DELETE", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("delete_all_reactions success : {}", m_snowflake);
            else
                m_log->debug("delete_all_reactions fail response: " + reply.content);
        });
    }

    void edit_channel_permissions(snowflake overwrite_id, int64_t allow, int64_t deny, std::string type) const
    {
        //requires MANAGE_ROLES
        json obj;
        obj["allow"] = allow;
        obj["deny"] = deny;
        obj["type"] = type;
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/permissions/{}", m_snowflake, overwrite_id), obj.dump(), "PUT", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("edit_channel_permissions success : {}", m_snowflake);
            else
                m_log->debug("edit_channel_permissions fail response: " + reply.content);
        });
    }

    void get_channel_invites()
    {
        //requires MANAGE_CHANNELS
        //returns
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/invites", m_snowflake), "", "GET", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("get_channel_invites success : {}", m_snowflake);
            else
                m_log->debug("get_channel_invites fail response: " + reply.content);
        });
    }

    void create_channel_invite(std::optional<int> max_age, std::optional<int> max_uses, std::optional<bool> temporary, std::optional<bool> unique)
    {
        //requires CREATE_INSTANT_INVITE
        //returns
        json obj;
        if (max_age.has_value())
            obj["max_age"] = max_age.value();
        if (max_uses.has_value())
            obj["max_uses"] = max_uses.value();
        if (temporary.has_value())
            obj["temporary"] = temporary.value();
        if (unique.has_value())
            obj["unique"] = unique.value();
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/invites", m_snowflake), obj.dump(), "POST", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("create_channel_invite success : {}", m_snowflake);
            else
                m_log->debug("create_channel_invite fail response: " + reply.content);
        });
    }


    void delete_channel_permission(snowflake overwrite_id) const
    {
        //requires MANAGE_ROLES
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/permissions/{}", m_snowflake, overwrite_id), "", "DELETE", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("edit_channel_permissions success : {}", m_snowflake);
            else
                m_log->debug("edit_channel_permissions fail response: " + reply.content);
        });
    }

    void trigger_typing_indicator()
    {
        m_ratelimit.push(m_snowflake, fmt::format("/channels/{}/typing", m_snowflake), "", "POST", [&](rest_reply reply)
        {
            if (reply.reply_code == 204)
                m_log->debug("trigger_typing_indicator success : {}", m_snowflake);
            else
                m_log->debug("trigger_typing_indicator fail response: " + reply.content);
        });
    }

    void get_pinned_messages()
    {
        //returns
        //TODO: 
    }

    void add_pinned_channel_message()
    {
        //TODO: 
    }

    void delete_pinned_channel_message()
    {
        //TODO: 
    }

    void group_dm_add_recipient()//will go in aegis::Aegis
    {
        //TODO: 
    }

    void group_dm_remove_recipient()//will go in aegis::Aegis
    {
        //TODO: 
    }

};

}

