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


#include "aegis/config.hpp"
#include <future>
#include "aegis/ratelimit.hpp"
#include "aegis/permission.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/permission_overwrite.hpp"
#include "aegis/objects/channel.hpp"


namespace aegiscpp
{

using rest_api = std::tuple<std::error_code, std::optional<std::future<rest_reply>>>;

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
    * @param emoji Reference to bucket factory that manages ratelimits for emoji messages
    */
    explicit channel(snowflake channel_id, snowflake guild_id, aegis * b, bucket_factory & ratelimit, bucket_factory & emojilimit)
        : channel_id(channel_id)
        , guild_id(guild_id)
        , ratelimit(ratelimit)
        , emojilimit(emojilimit)
        , _guild(nullptr)
        , _bot(b)
    {

    }

    /// Get a reference to the guild object this channel belongs to
    /**
    * @returns Reference to the guild this channel belongs to
    */
    AEGIS_DECL guild & get_guild() const;

    AEGIS_DECL permission perms();

    AEGIS_DECL std::future<rest_reply> post_task(std::string path, std::string method = "POST", std::string obj = {});

    AEGIS_DECL std::future<rest_reply> post_emoji_task(std::string path, std::string method = "POST", std::string obj = {});

    /// Load this channel with guild data
    /**
    * @param _guild Reference of the guild this channel belongs to
    *
    * @param obj Const reference to the object containing this channel object
    *
    * @param _shard Pointer to the shard that tracks this channel
    */
    AEGIS_DECL void load_with_guild(guild & _guild, const json & obj, shard * _shard);

    /// Send message to this channel
    /**
    * @param content A string of the message to send
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api create_message(std::string content);

    /// Send an embed message to this channel
    /**
    * @param content A string of the message to send
    *
    * @param embed A json object of the embed object itself
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api create_message_embed(std::string content, const json embed);

    /// Edit a message in this channel
    /**
    * @param message_id Snowflake of the message to replace. Must be your own message
    *
    * @param content A string of the message to set
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api edit_message(snowflake message_id, std::string content);

    /// Edit an embed message in this channel
    /**
    * @param message_id Snowflake of the message to replace. Must be your own message
    *
    * @param content A string of the message to set
    *
    * @param embed A json object of the embed object itself
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api edit_message_embed(snowflake message_id, std::string content, json embed);

    /// Delete a message
    /**
    * @param message_id Snowflake of the message to delete
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api delete_message(snowflake message_id);

    /// Delete up to 100 messages at once
    /**
    * @param message Vector of up to 100 message snowflakes to delete
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api bulk_delete_message(std::vector<int64_t> messages);

    /// Modify this channel (all parameters optional)
    /**
    * @param name String of channel name
    *
    * @param position Integer of position on channel list
    *
    * @param topic String of channel topic
    *
    * @param nsfw Boolean of whether channel is NSFW or not
    *
    * @param bitrate Integer of the channel bitrate (VOICE CHANNEL ONLY)
    *
    * @param user_limit Integer of the channel max user limit (VOICE CHANNEL ONLY)
    *
    * @param permission_overwrites Vector of permission_overwrite objects for overriding permissions
    *
    * @param parent_id Snowflake of category channel belongs in (empty parent puts channel in no category)
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api modify_channel(std::optional<std::string> name, std::optional<int> position, std::optional<std::string> topic,
                        std::optional<bool> nsfw, std::optional<int> bitrate, std::optional<int> user_limit,
                        std::optional<std::vector<permission_overwrite>> permission_overwrites, std::optional<snowflake> parent_id);

    /// Delete this channel
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api delete_channel();

    /// Add new reaction on message
    /**
    * @param message_id Snowflake of message
    *
    * @param emoji_text Text of emoji being added `name:snowflake`
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api create_reaction(snowflake message_id, std::string emoji_text);

    /// Delete own reaction on message
    /**
    * @param message_id Snowflake of message
    *
    * @param emoji_text Text of emoji being added `name:snowflake`
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api delete_own_reaction(snowflake message_id, std::string emoji_text);

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
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api delete_user_reaction(snowflake message_id, std::string emoji_text, snowflake member_id);

    /// Get all reactions for this message
    /**
    * @param message_id Snowflake of message
    *
    * @param emoji_text Text of emoji being added `name:snowflake`
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api get_reactions(snowflake message_id, std::string emoji_text);

    /// Delete all reactions by message
    /**
    * @param message_id Snowflake of message
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api delete_all_reactions(snowflake message_id);

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
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api edit_channel_permissions(snowflake overwrite_id, int64_t allow, int64_t deny, std::string type);

    /// Get active channel invites
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api get_channel_invites();

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
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api create_channel_invite(std::optional<int> max_age, std::optional<int> max_uses, std::optional<bool> temporary, std::optional<bool> unique);

    /// Delete channel permission override
    /**
    * @param overwrite_id Snowflake of the channel permission to delete
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api delete_channel_permission(snowflake overwrite_id);

    /// Trigger typing indicator in channel (lasts 10 seconds)
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api trigger_typing_indicator();

    /// Get pinned messages in channel
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api get_pinned_messages();

    /// Add a pinned message in channel
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api add_pinned_channel_message();

    /// Delete a pinned message in channel
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api delete_pinned_channel_message();

    /// Add member to a group direct message
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api group_dm_add_recipient();

    /// Remove member from a group direct message
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns std::tuple<std::error_code,std::shared_future<rest_reply>>
    */
    AEGIS_DECL rest_api group_dm_remove_recipient();

    /// Get the type of this channel
    /**
    * @returns An channel_type enum for this channel
    */
    const channel_type get_type() const noexcept
    {
        return type;
    }

    /// Gets the Bot object
    /**
    * @see aegis
    *
    * @returns Aegis main object
    */
    AEGIS_DECL aegis & get_bot() const noexcept;

    /// Get the snowflake of this channel
    /**
    * @returns A snowflake of the channel
    */
    const snowflake get_id() const noexcept
    {
        return channel_id;
    }

    /// Get the snowflake of this channel's guild
    /**
    * @returns A snowflake of this channel's guild
    */
    const snowflake get_guild_id() const noexcept
    {
        return guild_id;
    }

private:
    friend class aegis;
    friend class guild;
    friend class message;
    snowflake channel_id; /**< snowflake of this channel */
    snowflake guild_id; /**< snowflake of the guild this channel belongs to */
    bucket_factory & ratelimit; /**< Bucket factory for tracking the regular ratelimits */
    bucket_factory & emojilimit; /**< Bucket factory for tracking emoji actions */
    guild * _guild; /**< Pointer to the guild this channel belongs to */
    snowflake last_message_id = 0; /**< Snowflake of the last message sent in this channel */
    std::string name; /**< String of the name of this channel */
    std::string topic; /**< String of the topic of this channel */
    uint32_t position = 0; /**< Position of channel in guild channel list */
    channel_type type = channel_type::Text; /**< Type of channel */
    uint16_t bitrate = 0; /**< Bit rate of voice channel */
    uint16_t user_limit = 0; /**< User limit of voice channel */
    std::map<int64_t, permission_overwrite> overrides; /**< Snowflake map of user/role to permission overrides */
    aegis * _bot;
};

}

#if defined(AEGIS_HEADER_ONLY)
# include "aegis/channel.cpp"
#endif // defined(AEGIS_HEADER_ONLY)
