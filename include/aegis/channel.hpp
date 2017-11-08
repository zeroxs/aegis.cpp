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


#include "config.hpp"
#include "objects/permission_overwrite.hpp"


namespace aegiscpp
{

using json = nlohmann::json;
using rest_limits::bucket_factory;

class guild;
class shard;

/**
* Channel class for performing actions pertaining to the specified channel
*/
class channel
{
public:
    /// Constructor for the channel object
    /**
    * @param channel_id Snowflake of this channel
    *
    * @param guild_id Snowflake of guild this channel belongs to
    *
    * @param ratelimit Reference to bucket factory that manages ratelimits for this channel
    *
    * @param emoji Reference to bucket factory that manages ratelimits for emoiji messages
    */
    explicit channel(snowflake channel_id, snowflake guild_id, bucket_factory & ratelimit, bucket_factory & emoji)
        : channel_id(channel_id)
        , guild_id(guild_id)
        , ratelimit(ratelimit)
        , emoji(emoji)
        , log(spdlog::get("aegis"))
        , _guild(nullptr)
    {

    }

    snowflake channel_id; /**< snowflake of this channel */
    snowflake guild_id; /**< snowflake of the guild this channel belongs to */
    bucket_factory & ratelimit; /**< Bucket factory for tracking the regular ratelimits */
    bucket_factory & emoji; /**< Bucket factory for tracking emoji actions */
    std::shared_ptr<spdlog::logger> log; /**< shared_ptr to global logger */

    guild * _guild; /**< Pointer to the guild this channel belongs to */

    snowflake last_message_id = 0; /**< Snowflake of the last message sent in this channel */
    std::string name; /**< String of the name of this channel */
    std::string topic; /**< String of the topic of this channel */
    uint32_t position = 0; /**< Position of channel in guild channel list */
    channel_type type = channel_type::Text; /**< Type of channel */

    uint16_t bitrate = 0; /**< Bitrate of voice channel */
    uint16_t user_limit = 0; /**< User limit of voice channel */

    std::unordered_map<int64_t, permission_overwrite> overrides; /**< Snowflake map of user/role to permission overrides */

    /// Get a reference to the guild object this channel belongs to
    /**
    * @returns Reference to the guild this channel belongs to
    */
    guild & get_guild();

    /// Load this channel with guild data
    /**
    * @param _guild Reference of the guild this channel belongs to
    *
    * @param obj Const reference to the object conmtaining this channel object
    *
    * @param _shard Pointer to the shard that tracks this channel
    */
    void load_with_guild(guild & _guild, const json & obj, shard * _shard);

    bool create_debug_message(std::string content, std::function<void(rest_reply)> callback = nullptr);

    bool create_message(std::string content, std::function<void(rest_reply)> callback = nullptr);

    bool create_message_embed(std::string content, const json embed, std::function<void(rest_reply)> callback = nullptr);

    bool edit_message(snowflake message_id, std::string content, std::function<void(rest_reply)> callback = nullptr);

    bool edit_message_embed(snowflake message_id, std::string content, json embed, std::function<void(rest_reply)> callback = nullptr);

    bool delete_message(snowflake message_id, std::function<void(rest_reply)> callback = nullptr);

    bool bulk_delete_message(snowflake message_id, std::vector<int64_t> messages, std::function<void(rest_reply)> callback = nullptr);

    bool modify_channel(std::optional<std::string> name, std::optional<int> position, std::optional<std::string> topic,
                        std::optional<bool> nsfw, std::optional<int> bitrate, std::optional<int> user_limit,
                        std::optional<std::vector<permission_overwrite>> permission_overwrites, std::optional<snowflake> parent_id, std::function<void(rest_reply)> callback = nullptr);

    /// Delete this channel
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool delete_channel(std::function<void(rest_reply)> callback = nullptr);

    /// Add new reaction on message
    /**
    * @param message_id Snowflake of message
    *
    * @param emoji_text Text of emoji being added `name:snowflake`
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool create_reaction(snowflake message_id, std::string emoji_text, std::function<void(rest_reply)> callback = nullptr);

    /// Delete own reaction on message
    /**
    * @param message_id Snowflake of message
    *
    * @param emoji_text Text of emoji being added `name:snowflake`
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool delete_own_reaction(snowflake message_id, std::string emoji_text, std::function<void(rest_reply)> callback = nullptr);

    /// Delete specified member reaction on message
    /**
    * @param message_id Snowflake of message
    *
    * @param emoji_text Text of emoji being added `name:snowflake`
    *
    * @param member_id Snowflake of member to remove emoji from
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool delete_user_reaction(snowflake message_id, std::string emoji_text, snowflake member_id, std::function<void(rest_reply)> callback = nullptr);

    /// Get all reactions for this message
    /**
    * @param message_id Snowflake of message
    *
    * @param emoji_text Text of emoji being added `name:snowflake`
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool get_reactions(snowflake message_id, std::string emoji_text, std::function<void(rest_reply)> callback = nullptr);

    /// Delete all reactions by message
    /**
    * @param message_id Snowflake of message
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool delete_all_reactions(snowflake message_id, std::function<void(rest_reply)> callback = nullptr);

    /// Edit channel permission override
    /**
    * @param overwrite_id Snowflake of the permission override
    *
    * @param allow Int64 allow flags
    *
    * @param deny Int64 deny flags
    *
    * @param type Type of override (role/user)
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool edit_channel_permissions(snowflake overwrite_id, int64_t allow, int64_t deny, std::string type, std::function<void(rest_reply)> callback = nullptr);

    /// Get active channel invites
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool get_channel_invites(std::function<void(rest_reply)> callback = nullptr);

    /// Create a new channel invite
    /**
    * @param max_age How long this invite code lasts for in seconds
    *
    * @param max_uses The max uses this invite code allows
    *
    * @param temporary Is this invite code temporary
    *
    * @param unique Is this invite code a unique one-use
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool create_channel_invite(std::optional<int> max_age, std::optional<int> max_uses, std::optional<bool> temporary, std::optional<bool> unique, std::function<void(rest_reply)> callback = nullptr);

    /// Delete channel permission override
    /**
    * @param overwrite_id Snowflake of the channel permission to delete
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool delete_channel_permission(snowflake overwrite_id, std::function<void(rest_reply)> callback = nullptr);

    /// Trigger typing indicator in channel (lasts 10 seconds)
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool trigger_typing_indicator(std::function<void(rest_reply)> callback = nullptr);

    /// Get pinned messages in channel
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool get_pinned_messages(std::function<void(rest_reply)> callback = nullptr);

    /// Add a pinned message in channel
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool add_pinned_channel_message(std::function<void(rest_reply)> callback = nullptr);

    /// Delete a pinned message in channel
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool delete_pinned_channel_message(std::function<void(rest_reply)> callback = nullptr);

    /// Add member to a group direct message
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool group_dm_add_recipient(std::function<void(rest_reply)> callback = nullptr);

    /// Remove member from a group direct message
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool group_dm_remove_recipient(std::function<void(rest_reply)> callback = nullptr);

};

}

