//
// channel_impl.hpp
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

#include "config.hpp"

namespace aegis
{

using json = nlohmann::json;
using rest_limits::bucket_factory;

enum ChannelType
{
    Text = 0,
    DirectMessage = 1,
    Voice = 2,
    GroupDirectMessage = 3,
    Category = 4
};

inline aegis_guild & aegis_channel::get_guild()
{
    return *_guild;
}


inline void aegis_channel::load_with_guild(aegis_guild & _guild, json & obj, aegis_shard * shard)
{
    snowflake channel_id = obj["id"];
    aegis_channel * _channel = _guild.get_channel_create(channel_id, shard);
    _channel->guild_id = _guild.guild_id;

    try
    {
        //log->debug("Shard#{} : Channel[{}] created for guild[{}]", shard.m_shardid, channel_id, _channel.m_guild_id);
        if (!obj["name"].is_null()) _channel->name = obj["name"].get<std::string>();
        _channel->m_position = obj["position"];
        _channel->m_type = static_cast<aegis_channel::ChannelType>(obj["type"].get<int>());// 0 = text, 2 = voice

                                                                                           //voice channels
        if (!obj["bitrate"].is_null())
        {
            _channel->m_bitrate = obj["bitrate"];
            _channel->m_user_limit = obj["user_limit"];
        }
        else
        {
            //not a voice channel, so has a topic field and last message id
            if (!obj["topic"].is_null()) _channel->m_topic = obj["topic"].get<std::string>();
            if (!obj["last_message_id"].is_null()) _channel->m_last_message_id = obj["last_message_id"];
        }


        json permission_overwrites = obj["permission_overwrites"];
        for (auto & permission : permission_overwrites)
        {
            uint32_t allow = permission["allow"];
            uint32_t deny = permission["deny"];
            snowflake p_id = permission["id"];
            std::string p_type = permission["type"];

            _channel->m_overrides[p_id].allow = allow;
            _channel->m_overrides[p_id].deny = deny;
            _channel->m_overrides[p_id].id = p_id;
            if (p_type == "role")
                _channel->m_overrides[p_id].type = perm_overwrite::Role;
            else
                _channel->m_overrides[p_id].type = perm_overwrite::User;
        }

        //_channel.update_permission_cache();
    }
    catch (std::exception&e)
    {
        log->error("Shard#{} : Error processing channel[{}] of guild[{}] {}", shard->shardid, channel_id, _channel->guild_id, e.what());
    }
}

inline bool aegis_channel::create_message(std::string content, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canSendMessages())
        return false;

    json obj;
    obj["content"] = content;
    ratelimit.push(channel_id, fmt::format("/channels/{}/messages", channel_id), obj.dump(), "POST", callback);
    return true;
}

inline bool aegis_channel::create_message_embed(std::string content, json embed, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canSendMessages())
        return false;

    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;

    ratelimit.push(channel_id, fmt::format("/channels/{:d}/messages", channel_id), obj.dump(), "POST", callback);
    return true;
}

inline bool aegis_channel::edit_message(snowflake message_id, std::string content, std::function<void(rest_reply)> callback)
{
    json obj;
    obj["content"] = content;
    ratelimit.push(channel_id, fmt::format("/channels/{}/messages/{}", channel_id, message_id), obj.dump(), "PATCH", callback);
    return true;
}

inline bool aegis_channel::edit_message_embed(snowflake message_id, std::string content, json embed, std::function<void(rest_reply)> callback)
{
    json obj;
    if (!content.empty())
        obj["content"] = content;
    obj["embed"] = embed;
    obj["content"] = content;
    ratelimit.push(channel_id, fmt::format("/channels/{}/messages/{}", channel_id, message_id), obj.dump(), "PATCH", callback);
    return true;
}

//TODO: can delete your own messages freely - provide separate function or keep history of messages
inline bool aegis_channel::delete_message(snowflake message_id, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageMessages())
        return false;

    ratelimit.push(guild_id, fmt::format("/channels/{}/messages/{}", guild_id, message_id), "", "DELETE", callback);
    return true;
}

inline bool aegis_channel::bulk_delete_message(snowflake message_id, std::vector<int64_t> messages, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageMessages())
        return false;
    if (messages.size() < 2 || messages.size() > 100)
        return false;

    json obj = messages;
    ratelimit.push(channel_id, fmt::format("/channels/{}/messages/bulk-delete", channel_id), obj.dump(), "POST", callback);
    return true;
}

inline bool aegis_channel::modify_channel(std::optional<std::string> name, std::optional<int> position, std::optional<std::string> topic,
                                    std::optional<bool> nsfw, std::optional<int> bitrate, std::optional<int> user_limit,
                                    std::optional<std::vector<perm_overwrite>> permission_overwrites, std::optional<snowflake> parent_id, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageChannels())
        return false;

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
        if (!_guild->m_owner_id != _guild->self()->member_id)
            return false;


        obj["permission_overwrites"] = json::array();
        for (auto & p_ow : permission_overwrites.value())
        {
            obj["permission_overwrites"].push_back(p_ow.make());
        }
    }
    if (parent_id.has_value())//VIP only
        obj["parent_id"] = parent_id.value();

    ratelimit.push(channel_id, fmt::format("/channels/{}", channel_id), std::move(obj), "PATCH", callback);
    return true;
}

inline bool aegis_channel::delete_channel(std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageChannels())
        return false;

    ratelimit.push(channel_id, fmt::format("/channels/{}", channel_id), "", "DELETE", callback);
    return true;
}

inline bool aegis_channel::create_reaction(snowflake message_id, std::string emoji_text, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canAddReactions())
        return false;

    emoji.push(channel_id, fmt::format("/channels/{}/messages/{}/reactions/{}/@me", channel_id, message_id, emoji_text), "", "PUT", callback);
    return true;
}

inline bool aegis_channel::delete_own_reaction(snowflake message_id, std::string emoji_text, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canAddReactions())
        return false;

    emoji.push(channel_id, fmt::format("/channels/{}/messages/{}/reactions/{}/@me", channel_id, message_id, emoji_text), "", "DELETE", callback);
    return true;
}

inline bool aegis_channel::delete_user_reaction(snowflake message_id, std::string emoji_text, snowflake user_id, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageMessages())
        return false;

    emoji.push(channel_id, fmt::format("/channels/{}/messages/{}/reactions/{}/{}", channel_id, message_id, emoji_text, user_id), "", "DELETE", callback);
    return true;
}

inline bool aegis_channel::get_reactions(snowflake message_id, std::string emoji_text, std::function<void(rest_reply)> callback)
{
    //TODO: support query params?
    //before[snowflake], after[snowflake], limit[int]
    ratelimit.push(channel_id, fmt::format("/channels/{}/messages/{}/reactions/{}", channel_id, message_id, emoji_text), "", "GET", callback);
    return true;
}

inline bool aegis_channel::delete_all_reactions(snowflake message_id, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageMessages())
        return false;

    emoji.push(channel_id, fmt::format("/channels/{}/messages/{}/reactions", channel_id, message_id), "", "DELETE", callback);
    return true;
}

inline bool aegis_channel::edit_channel_permissions(snowflake overwrite_id, int64_t allow, int64_t deny, std::string type, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageRoles())
        return false;

    json obj;
    obj["allow"] = allow;
    obj["deny"] = deny;
    obj["type"] = type;
    ratelimit.push(channel_id, fmt::format("/channels/{}/permissions/{}", channel_id, overwrite_id), obj.dump(), "PUT", callback);
    return true;
}

inline bool aegis_channel::get_channel_invites(std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageChannels())
        return false;

    ratelimit.push(channel_id, fmt::format("/channels/{}/invites", channel_id), "", "GET", callback);
    return true;
}

inline bool aegis_channel::create_channel_invite(std::optional<int> max_age, std::optional<int> max_uses, std::optional<bool> temporary, std::optional<bool> unique, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canInvite())
        return false;

    json obj;
    if (max_age.has_value())
        obj["max_age"] = max_age.value();
    if (max_uses.has_value())
        obj["max_uses"] = max_uses.value();
    if (temporary.has_value())
        obj["temporary"] = temporary.value();
    if (unique.has_value())
        obj["unique"] = unique.value();
    ratelimit.push(channel_id, fmt::format("/channels/{}/invites", channel_id), obj.dump(), "POST", callback);
    return true;
}


inline bool aegis_channel::delete_channel_permission(snowflake overwrite_id, std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageRoles())
        return false;

    ratelimit.push(channel_id, fmt::format("/channels/{}/permissions/{}", channel_id, overwrite_id), "", "DELETE", callback);
    return true;
}

inline bool aegis_channel::trigger_typing_indicator(std::function<void(rest_reply)> callback)
{
    ratelimit.push(channel_id, fmt::format("/channels/{}/typing", channel_id), "", "POST", callback);
    return true;
}

inline bool aegis_channel::get_pinned_messages(std::function<void(rest_reply)> callback)
{
    //returns
    //TODO: 
    return true;
}

inline bool aegis_channel::add_pinned_channel_message(std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageMessages())
        return false;

    return true;
}

inline bool aegis_channel::delete_pinned_channel_message(std::function<void(rest_reply)> callback)
{
    if (!permission(_guild->base_permissions(_guild->self())).canManageMessages())
        return false;

    return true;
}

inline bool aegis_channel::group_dm_add_recipient(std::function<void(rest_reply)> callback)//will go in aegis::aegis_core
{
    //TODO: 
    return true;
}

inline bool aegis_channel::group_dm_remove_recipient(std::function<void(rest_reply)> callback)//will go in aegis::aegis_core
{
    //TODO: 
    return true;
}

}

