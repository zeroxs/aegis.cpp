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


namespace aegis
{

using json = nlohmann::json;
using rest_limits::bucket_factory;

enum ChannelType
{
    TEXT = 0,
    DM = 1,
    VOICE = 2,
    GROUP_DM = 3,
    CATEGORY = 4
};

inline guild & channel::get_guild()
{
    return *_guild;
}

inline bool channel::create_message(std::string content, std::function<void(rest_reply)> callback)
{
    json obj;
    obj["content"] = content;
    m_ratelimit.push(m_id, fmt::format("/channels/{}/messages", m_id), obj.dump(), "POST", callback);
    return true;
}

inline bool channel::create_message_embed(std::string content, json embed, std::function<void(rest_reply)> callback)
{
    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;

    m_ratelimit.push(m_id, fmt::format("/channels/{:d}/messages", m_id), obj.dump(), "POST", callback);
    return true;
}

inline bool channel::edit_message(snowflake message_id, std::string content, std::function<void(rest_reply)> callback)
{
    json obj;
    obj["content"] = content;
    m_ratelimit.push(m_id, fmt::format("/channels/{}/messages/{}", m_id, message_id), obj.dump(), "PATCH", callback);
    return true;
}

inline bool channel::edit_message_embed(snowflake message_id, std::string content, json embed, std::function<void(rest_reply)> callback)
{
    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;
    obj["content"] = content;
    m_ratelimit.push(m_id, fmt::format("/channels/{}/messages/{}", m_id, message_id), obj.dump(), "PATCH", callback);
    return true;
}

inline bool channel::delete_message(snowflake message_id, std::function<void(rest_reply)> callback)
{
    m_ratelimit.push(m_id, fmt::format("/channels/{}/messages/{}", m_id, message_id), "", "DELETE", callback);
    return true;
}

inline bool channel::bulk_delete_message(snowflake message_id, std::vector<int64_t> messages, std::function<void(rest_reply)> callback)
{
    json obj = messages;
    m_ratelimit.push(m_id, fmt::format("/channels/{}/messages/bulk-delete", m_id), obj.dump(), "POST", callback);
    return true;
}

inline bool channel::modify_channel(std::optional<std::string> name, std::optional<int> position, std::optional<std::string> topic,
                                    std::optional<bool> nsfw, std::optional<int> bitrate, std::optional<int> user_limit,
                                    std::optional<std::vector<perm_overwrite>> permission_overwrites, std::optional<snowflake> parent_id, std::function<void(rest_reply)> callback)
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

    m_ratelimit.push(m_id, fmt::format("/channels/{}", m_id), std::move(obj), "PATCH", callback);
    return true;
}

inline bool channel::delete_channel(std::function<void(rest_reply)> callback)
{
    //requires MANAGE_CHANNELS
    if (!permission(_guild->base_permissions(_guild->m_self)).canManageChannels())
        return false;//no perms
    m_ratelimit.push(m_id, fmt::format("/channels/{}", m_id), "", "DELETE", callback);
    return true;
}

inline bool channel::create_reaction(snowflake message_id, std::string emoji, std::function<void(rest_reply)> callback)
{
    //requires ADD_REACTIONS
    m_emoji.push(m_id, fmt::format("/channels/{}/messages/{}/reactions/{}/@me", m_id, message_id, emoji), "", "PUT", callback);
    return true;
}

inline bool channel::delete_own_reaction(snowflake message_id, std::string emoji, std::function<void(rest_reply)> callback)
{
    //requires ADD_REACTIONS
    m_emoji.push(m_id, fmt::format("/channels/{}/messages/{}/reactions/{}/@me", m_id, message_id, emoji), "", "DELETE", callback);
    return true;
}

inline bool channel::delete_user_reaction(snowflake message_id, std::string emoji, snowflake user_id, std::function<void(rest_reply)> callback)
{
    //requires ADD_REACTIONS
    m_emoji.push(m_id, fmt::format("/channels/{}/messages/{}/reactions/{}/{}", m_id, message_id, emoji, user_id), "", "DELETE", callback);
    return true;
}

inline bool channel::get_reactions(snowflake message_id, std::string emoji, std::function<void(rest_reply)> callback)
{
    //TODO: support query params?
    //before[snowflake], after[snowflake], limit[int]
    m_ratelimit.push(m_id, fmt::format("/channels/{}/messages/{}/reactions/{}", m_id, message_id, emoji), "", "GET", callback);
    return true;
}

inline bool channel::delete_all_reactions(snowflake message_id, std::function<void(rest_reply)> callback)
{
    //requires ADD_REACTIONS
    m_emoji.push(m_id, fmt::format("/channels/{}/messages/{}/reactions", m_id, message_id), "", "DELETE", callback);
    return true;
}

inline bool channel::edit_channel_permissions(snowflake overwrite_id, int64_t allow, int64_t deny, std::string type, std::function<void(rest_reply)> callback)
{
    //requires MANAGE_ROLES
    json obj;
    obj["allow"] = allow;
    obj["deny"] = deny;
    obj["type"] = type;
    m_ratelimit.push(m_id, fmt::format("/channels/{}/permissions/{}", m_id, overwrite_id), obj.dump(), "PUT", callback);
    return true;
}

inline bool channel::get_channel_invites(std::function<void(rest_reply)> callback)
{
    //requires MANAGE_CHANNELS
    //returns
    m_ratelimit.push(m_id, fmt::format("/channels/{}/invites", m_id), "", "GET", callback);
    return true;
}

inline bool channel::create_channel_invite(std::optional<int> max_age, std::optional<int> max_uses, std::optional<bool> temporary, std::optional<bool> unique, std::function<void(rest_reply)> callback)
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
    m_ratelimit.push(m_id, fmt::format("/channels/{}/invites", m_id), obj.dump(), "POST", callback);
    return true;
}


inline bool channel::delete_channel_permission(snowflake overwrite_id, std::function<void(rest_reply)> callback)
{
    //requires MANAGE_ROLES
    m_ratelimit.push(m_id, fmt::format("/channels/{}/permissions/{}", m_id, overwrite_id), "", "DELETE", callback);
    return true;
}

inline bool channel::trigger_typing_indicator(std::function<void(rest_reply)> callback)
{
    m_ratelimit.push(m_id, fmt::format("/channels/{}/typing", m_id), "", "POST", callback);
    return true;
}

inline bool channel::get_pinned_messages(std::function<void(rest_reply)> callback)
{
    //returns
    //TODO: 
    return true;
}

inline bool channel::add_pinned_channel_message(std::function<void(rest_reply)> callback)
{
    //TODO: 
    return true;
}

inline bool channel::delete_pinned_channel_message(std::function<void(rest_reply)> callback)
{
    //TODO: 
    return true;
}

inline bool channel::group_dm_add_recipient(std::function<void(rest_reply)> callback)//will go in aegis::Aegis
{
    //TODO: 
    return true;
}

inline bool channel::group_dm_remove_recipient(std::function<void(rest_reply)> callback)//will go in aegis::Aegis
{
    //TODO: 
    return true;
}

}

