//
// channel.hpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include <future>
#include "aegis/utility.hpp"
#include "aegis/ratelimit/ratelimit.hpp"
#include "aegis/permission.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/permission_overwrite.hpp"
#include "aegis/objects/channel.hpp"
#include <shared_mutex>

namespace aegis
{

#if (AEGIS_HAS_STD_SHARED_MUTEX == 1)
using shared_mutex = std::shared_mutex;
#else
using shared_mutex = std::shared_timed_mutex;
#endif

using json = nlohmann::json;

/// Class for performing actions pertaining to a specified channel
class channel
{
public:
    /// Constructor for the channel object
    /**
     * @param channel_id Snowflake of this channel
     * @param guild_id Snowflake of guild this channel belongs to
     * @param ratelimit Reference to bucket factory that manages ratelimits for this channel
     * @param emoji Reference to bucket factory that manages ratelimits for emoji messages
     */
    AEGIS_DECL explicit channel(const snowflake channel_id, const snowflake guild_id, core * _bot, asio::io_context & _io);

    /// Get a reference to the guild object this channel belongs to
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns Reference to the guild this channel belongs to
     */
    AEGIS_DECL guild & get_guild() const;

    /// Get a reference to the guild object this channel belongs to
    /**
     * @returns Reference to the guild this channel belongs to
     */
    AEGIS_DECL guild & get_guild(std::error_code & ec) const noexcept;

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    /// Get channel name
    /**
     * @returns String of channel name
     */
    std::string get_name() const noexcept
    {
        return name;
    }

    /// Get the type of this channel
    /**
     * @returns An channel_type enum for this channel
     */
    const gateway::objects::channel_type get_type() const noexcept
    {
        return type;
    }

    /// Get bot's permission for this channel
    /**
     * @returns Bitmask of current permissions for this channel contained within `permission` object
     */
    AEGIS_DECL permission perms();
#endif

    /// Creates a task to make a REST call - preferable to use specific functions
    /// for interactions. This function is public for the case of any functionality
    /// not implemented.
    /**
    * @param path String of the destination path
    * @param method One of POST/GET/PUT/PATCH/DELETE Defaults to POST
    * @param obj String of json to send as body
    * @param host String of remote host. Defaults to discordapp.com
    * @returns std::future<rest::rest_reply>
    */
    AEGIS_DECL std::future<rest::rest_reply> post_task(const std::string & path, const std::string & method = "POST", const std::string & obj = "", const std::string & host = "");

    /// Creates a task to make a REST call - preferable to use specific functions
    /// for interactions. This function is public for the case of any functionality
    /// not implemented.
    /**
    * @param path String of the destination path
    * @param method One of POST/GET/PUT/PATCH/DELETE Defaults to POST
    * @param obj String of json to send as body
    * @param host String of remote host. Defaults to discordapp.com
    * @returns std::future<rest::rest_reply>
    */
    AEGIS_DECL std::future<rest::rest_reply> post_emoji_task(const std::string & path, const std::string & method = "POST", const std::string & obj = "", const std::string & host = "");

    /// Send message to this channel
    /**
     * @param ec Indicates what error occurred, if any
     * @param content A string of the message to send
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> create_message(std::error_code & ec, const std::string & content, int64_t nonce = 0);

    /// Send message to this channel
    /**
     * @param content A string of the message to send
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> create_message(const std::string & content, int64_t nonce = 0)
    {
        std::error_code ec;
        auto res = create_message(ec, content, nonce);
        if (ec)
            throw ec;
        return res;
    }

    /// Send an embed message to this channel
    /**
     * @param ec Indicates what error occurred, if any
     * @param content A string of the message to send
     * @param embed A json object of the embed object itself
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> create_message_embed(std::error_code & ec, const std::string & content, const json & embed, int64_t nonce = 0);

    /// Send an embed message to this channel
    /**
     * @param content A string of the message to send
     * @param embed A json object of the embed object itself
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> create_message_embed(const std::string & content, const json & embed, int64_t nonce = 0)
    {
        std::error_code ec;
        auto res = create_message_embed(ec, content, embed, nonce);
        if (ec)
            throw ec;
        return res;
    }

    /// Edit a message in this channel
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of the message to replace. Must be your own message
     * @param content A string of the message to set
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> edit_message(std::error_code & ec, snowflake message_id, const std::string & content);

    /// Edit a message in this channel
    /**
     * @param message_id Snowflake of the message to replace. Must be your own message
     * @param content A string of the message to set
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> edit_message(snowflake message_id, const std::string & content)
    {
        std::error_code ec;
        auto res = edit_message(ec, message_id, content);
        if (ec)
            throw ec;
        return res;
    }

    /// Edit an embed message in this channel
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of the message to replace. Must be your own message
     * @param content A string of the message to set
     * @param embed A json object of the embed object itself
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> edit_message_embed(std::error_code & ec, snowflake message_id, const std::string & content, const json & embed);

    /// Edit an embed message in this channel
    /**
     * @param message_id Snowflake of the message to replace. Must be your own message
     * @param content A string of the message to set
     * @param embed A json object of the embed object itself
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> edit_message_embed(snowflake message_id, const std::string & content, const json & embed)
    {
        std::error_code ec;
        auto res = edit_message_embed(ec, message_id, content, embed);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete a message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of the message to delete
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> delete_message(std::error_code & ec, snowflake message_id);

    /// Delete a message
    /**
     * @param message_id Snowflake of the message to delete
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> delete_message(snowflake message_id)
    {
        std::error_code ec;
        auto res = delete_message(ec, message_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete up to 100 messages at once
    /**
     * @param ec Indicates what error occurred, if any
     * @param message Vector of up to 100 message snowflakes to delete
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> bulk_delete_message(std::error_code & ec, const std::vector<int64_t> & messages);

    /// Delete up to 100 messages at once
    /**
     * @param message Vector of up to 100 message snowflakes to delete
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> bulk_delete_message(const std::vector<int64_t> & messages)
    {
        std::error_code ec;
        auto res = bulk_delete_message(ec, messages);
        if (ec)
            throw ec;
        return res;
    }

    /// Modify this channel (all parameters optional)
    /**
     * @param ec Indicates what error occurred, if any
     * @param _name String of channel name
     * @param _position Integer of position on channel list
     * @param _topic String of channel topic
     * @param _nsfw Boolean of whether channel is NSFW or not
     * @param _bitrate Integer of the channel bitrate (VOICE CHANNEL ONLY)
     * @param _user_limit Integer of the channel max user limit (VOICE CHANNEL ONLY)
     * @param _permission_overwrites Vector of permission_overwrite objects for overriding permissions
     * @param _parent_id Snowflake of category channel belongs in (empty parent puts channel in no category)
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> modify_channel(std::error_code & ec, lib::optional<std::string> _name = {}, lib::optional<int> _position = {}, lib::optional<std::string> _topic = {},
                        lib::optional<bool> _nsfw = {}, lib::optional<int> _bitrate = {}, lib::optional<int> _user_limit = {},
                        lib::optional<std::vector<gateway::objects::permission_overwrite>> _permission_overwrites = {}, lib::optional<snowflake> _parent_id = {});

    /// Modify this channel (all parameters optional)
    /**
     * @param _name String of channel name
     * @param _position Integer of position on channel list
     * @param _topic String of channel topic
     * @param _nsfw Boolean of whether channel is NSFW or not
     * @param _bitrate Integer of the channel bitrate (VOICE CHANNEL ONLY)
     * @param _user_limit Integer of the channel max user limit (VOICE CHANNEL ONLY)
     * @param _permission_overwrites Vector of permission_overwrite objects for overriding permissions
     * @param _parent_id Snowflake of category channel belongs in (empty parent puts channel in no category)
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> modify_channel(lib::optional<std::string> _name = {}, lib::optional<int> _position = {}, lib::optional<std::string> _topic = {},
                        lib::optional<bool> _nsfw = {}, lib::optional<int> _bitrate = {}, lib::optional<int> _user_limit = {},
                        lib::optional<std::vector<gateway::objects::permission_overwrite>> _permission_overwrites = {}, lib::optional<snowflake> _parent_id = {})
    {
        std::error_code ec;
        auto res = modify_channel(ec, _name, _position, _topic, _nsfw, _bitrate, _user_limit, _permission_overwrites, _parent_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete this channel
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> delete_channel(std::error_code & ec);

    /// Delete this channel
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> delete_channel()
    {
        std::error_code ec;
        auto res = delete_channel(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Add new reaction on message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> create_reaction(std::error_code & ec, snowflake message_id, const std::string & emoji_text);

    /// Add new reaction on message
    /**
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> create_reaction(snowflake message_id, const std::string & emoji_text)
    {
        std::error_code ec;
        auto res = create_reaction(ec, message_id, emoji_text);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete own reaction on message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> delete_own_reaction(std::error_code & ec, snowflake message_id, const std::string & emoji_text);

    /// Delete own reaction on message
    /**
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> delete_own_reaction(snowflake message_id, const std::string & emoji_text)
    {
        std::error_code ec;
        auto res = delete_own_reaction(ec, message_id, emoji_text);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete specified member reaction on message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @param member_id Snowflake of member to remove emoji from
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> delete_user_reaction(std::error_code & ec, snowflake message_id, const std::string & emoji_text, snowflake member_id);

    /// Delete specified member reaction on message
    /**
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @param member_id Snowflake of member to remove emoji from
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> delete_user_reaction(snowflake message_id, const std::string & emoji_text, snowflake member_id)
    {
        std::error_code ec;
        auto res = delete_user_reaction(ec, message_id, emoji_text, member_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Get all reactions for this message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> get_reactions(std::error_code & ec, snowflake message_id, const std::string & emoji_text);

    /// Get all reactions for this message
    /**
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> get_reactions(snowflake message_id, const std::string & emoji_text)
    {
        std::error_code ec;
        auto res = get_reactions(ec, message_id, emoji_text);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete all reactions by message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of message
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> delete_all_reactions(std::error_code & ec, snowflake message_id);

    /// Delete all reactions by message
    /**
     * @param message_id Snowflake of message
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> delete_all_reactions(snowflake message_id)
    {
        std::error_code ec;
        auto res = delete_all_reactions(ec, message_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Edit channel permission override
    /**
     * @param ec Indicates what error occurred, if any
     * @param _overwrite_id Snowflake of the permission override
     * @param _allow Int64 allow flags
     * @param _deny Int64 deny flags
     * @param _type Type of override (role/user)
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> edit_channel_permissions(std::error_code & ec, snowflake _overwrite_id, int64_t _allow, int64_t _deny, const std::string & _type);

    /// Edit channel permission override
    /**
     * @param _overwrite_id Snowflake of the permission override
     * @param _allow Int64 allow flags
     * @param _deny Int64 deny flags
     * @param _type Type of override (role/user)
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> edit_channel_permissions(snowflake _overwrite_id, int64_t _allow, int64_t _deny, const std::string & _type)
    {
        std::error_code ec;
        auto res = edit_channel_permissions(ec, _overwrite_id, _allow, _deny, _type);
        if (ec)
            throw ec;
        return res;
    }

    /// Get active channel invites
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> get_channel_invites(std::error_code & ec);

    /// Get active channel invites
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> get_channel_invites()
    {
        std::error_code ec;
        auto res = get_channel_invites(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Create a new channel invite
    /**
     * @param ec Indicates what error occurred, if any
     * @param max_age How long this invite code lasts for in seconds
     * @param max_uses The max uses this invite code allows
     * @param temporary Is this invite code temporary
     * @param unique Is this invite code a unique one-use
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> create_channel_invite(std::error_code & ec, const lib::optional<int> max_age, const lib::optional<int> max_uses, const lib::optional<bool> temporary, const lib::optional<bool> unique);

    /// Create a new channel invite
    /**
     * @param max_age How long this invite code lasts for in seconds
     * @param max_uses The max uses this invite code allows
     * @param temporary Is this invite code temporary
     * @param unique Is this invite code a unique one-use
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> create_channel_invite(const lib::optional<int> max_age, const lib::optional<int> max_uses, const lib::optional<bool> temporary, const lib::optional<bool> unique)
    {
        std::error_code ec;
        auto res = create_channel_invite(ec, max_age, max_uses, temporary, unique);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete channel permission override
    /**
     * @param ec Indicates what error occurred, if any
     * @param overwrite_id Snowflake of the channel permission to delete
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> delete_channel_permission(std::error_code & ec, snowflake overwrite_id);

    /// Delete channel permission override
    /**
     * @param overwrite_id Snowflake of the channel permission to delete
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> delete_channel_permission(snowflake overwrite_id)
    {
        std::error_code ec;
        auto res = delete_channel_permission(ec, overwrite_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Trigger typing indicator in channel (lasts 10 seconds)
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> trigger_typing_indicator(std::error_code & ec);

    /// Trigger typing indicator in channel (lasts 10 seconds)
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> trigger_typing_indicator()
    {
        std::error_code ec;
        auto res = trigger_typing_indicator(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Get pinned messages in channel
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> get_pinned_messages(std::error_code & ec);

    /// Get pinned messages in channel
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> get_pinned_messages()
    {
        std::error_code ec;
        auto res = get_pinned_messages(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Add a pinned message in channel
    /**\todo
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> add_pinned_channel_message(std::error_code & ec);

    /// Add a pinned message in channel
    /**\todo
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> add_pinned_channel_message()
    {
        std::error_code ec;
        auto res = add_pinned_channel_message(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete a pinned message in channel
    /**\todo
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> delete_pinned_channel_message(std::error_code & ec);

    /// Delete a pinned message in channel
    /**\todo
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> delete_pinned_channel_message()
    {
        std::error_code ec;
        auto res = delete_pinned_channel_message(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Add member to a group direct message
    /**\todo
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> group_dm_add_recipient(std::error_code & ec);

    /// Add member to a group direct message
    /**\todo
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> group_dm_add_recipient()
    {
        std::error_code ec;
        auto res = group_dm_add_recipient(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Remove member from a group direct message
    /**\todo
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> group_dm_remove_recipient(std::error_code & ec);

    /// Remove member from a group direct message
    /**\todo
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> group_dm_remove_recipient()
    {
        std::error_code ec;
        auto res = group_dm_remove_recipient(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Get the snowflake of this channel
    /**
     * @returns A snowflake of the channel
     */
    const snowflake get_id() const noexcept
    {
        return channel_id;
    }

    /// Get the snowflake of this channel's guild - 0 if DM
    /**
     * @returns A snowflake of this channel's guild
     */
    const snowflake get_guild_id() const noexcept
    {
        return guild_id;
    }

    /// Is this channel a DM or in a guild
    /**
     * @returns bool whether channel is a DM or belongs to a guild
     */
    const bool is_dm() const noexcept
    {
        return _guild == nullptr;
    }

    shared_mutex & mtx() noexcept
    {
        return _m;
    }

private:
    friend class guild;
    friend class core;

    /// requires the caller to handle locking
    AEGIS_DECL void load_with_guild(guild & _guild, const json & obj, shards::shard * _shard);

    snowflake channel_id; /**< snowflake of this channel */
    snowflake guild_id; /**< snowflake of the guild this channel belongs to */
    guild * _guild; /**< Pointer to the guild this channel belongs to */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake last_message_id = 0; /**< Snowflake of the last message sent in this channel */
    std::string name; /**< String of the name of this channel */
    std::string topic; /**< String of the topic of this channel */
    uint32_t position = 0; /**< Position of channel in guild channel list */
    gateway::objects::channel_type type = gateway::objects::channel_type::Text; /**< Type of channel */
    uint16_t bitrate = 0; /**< Bit rate of voice channel */
    uint16_t user_limit = 0; /**< User limit of voice channel */
    std::unordered_map<int64_t, gateway::objects::permission_overwrite> overrides; /**< Snowflake map of user/role to permission overrides */
#endif
    asio::io_context & _io_context;
    mutable shared_mutex _m;
    core * _bot = nullptr;
};

}
