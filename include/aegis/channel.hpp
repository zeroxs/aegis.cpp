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
#include "aegis/gateway/objects/permission_overwrite.hpp"
#include "aegis/gateway/objects/channel.hpp"
#include <shared_mutex>
#include "aegis/futures.hpp"

namespace aegis
{

#if (AEGIS_HAS_STD_SHARED_MUTEX == 1)
using shared_mutex = std::shared_mutex;
#else
using shared_mutex = std::shared_timed_mutex;
#endif

using json = nlohmann::json;

#pragma region rest params
struct create_message_t
{
    create_message_t & content(const std::string & param) { _content = param; return *this; }
    create_message_t & nonce(int64_t param) { _nonce = param; return *this; }
    std::string _content;
    int64_t _nonce = 0;
};

struct create_message_embed_t
{
    create_message_embed_t & content(const std::string & param) { _content = param; return *this; }
    create_message_embed_t & embed(const json & param) { _embed = param; return *this; }
    create_message_embed_t & nonce(int64_t param) { _nonce = param; return *this; }
    std::string _content;
    json _embed;
    int64_t _nonce = 0;
};

struct edit_message_t
{
    edit_message_t & message_id(snowflake param) { _message_id = param; return *this; }
    edit_message_t & content(const std::string & param) { _content = param; return *this; }
    snowflake _message_id;
    std::string _content;
};

struct edit_message_embed_t
{
    edit_message_embed_t & message_id(snowflake param) { _message_id = param; return *this; }
    edit_message_embed_t & content(const std::string & param) { _content = param; return *this; }
    edit_message_embed_t & embed(const json & param) { _embed = param; return *this; }
    snowflake _message_id;
    std::string _content;
    json _embed;
};

struct modify_channel_t
{
    modify_channel_t & name(const std::string & param) { _name = param; return *this; }
    modify_channel_t & position(int param) { _position = param; return *this; }
    modify_channel_t & topic(const std::string & param) { _topic = param; return *this; }
    modify_channel_t & nsfw(bool param) { _nsfw = param; return *this; }
    modify_channel_t & bitrate(int param) { _bitrate = param; return *this; }
    modify_channel_t & user_limit(int param) { _user_limit = param; return *this; }
    modify_channel_t & permission_overwrites(const std::vector<gateway::objects::permission_overwrite> & param)
    { _permission_overwrites = param; return *this; }
    modify_channel_t & parent_id(snowflake param) { _parent_id = param; return *this; }
    modify_channel_t & rate_limit_per_user(int param) { _rate_limit_per_user = param; return *this; }
    lib::optional<std::string> _name = {};
    lib::optional<int> _position = {};
    lib::optional<std::string> _topic = {};
    lib::optional<bool> _nsfw = {};
    lib::optional<int> _bitrate = {};
    lib::optional<int> _user_limit = {};
    lib::optional<std::vector<gateway::objects::permission_overwrite>> _permission_overwrites = {};
    lib::optional<snowflake> _parent_id = {};
    lib::optional<int> _rate_limit_per_user = {};
};

struct create_reaction_t
{
    create_reaction_t & message_id(snowflake param) { _message_id = param; return *this; }
    create_reaction_t & emoji_text(const std::string & param) { _emoji_text = param; return *this; }
    snowflake _message_id;
    std::string _emoji_text;
};

struct delete_own_reaction_t
{
    delete_own_reaction_t & message_id(int64_t param) { _message_id = param; return *this; }
    delete_own_reaction_t & emoji_text(const std::string & param) { _emoji_text = param; return *this; }
    snowflake _message_id;
    std::string _emoji_text;
};

struct delete_user_reaction_t
{
    delete_user_reaction_t & message_id(snowflake param) { _message_id = param; return *this; }
    delete_user_reaction_t & emoji_text(const std::string & param) { _emoji_text = param; return *this; }
    delete_user_reaction_t & member_id(snowflake param) { _member_id = param; return *this; }
    snowflake _message_id;
    std::string _emoji_text;
    snowflake _member_id;
};

struct get_reactions_t
{
    get_reactions_t & message_id(snowflake param) { _message_id = param; return *this; }
    get_reactions_t & emoji_text(const std::string & param) { _emoji_text = param; return *this; }
    snowflake _message_id;
    std::string _emoji_text;
};

struct edit_channel_permissions_t
{
    edit_channel_permissions_t & overwrite_id(snowflake param) { _overwrite_id = param; return *this; }
    edit_channel_permissions_t & allow(int64_t param) { _allow = param; return *this; }
    edit_channel_permissions_t & deny(int64_t param) { _deny = param; return *this; }
    edit_channel_permissions_t & type(const std::string & param) { _type = param; return *this; }
    snowflake _overwrite_id;
    int64_t _allow;
    int64_t _deny;
    std::string _type;
};

struct create_channel_invite_t
{
    create_channel_invite_t & max_age(int param) { _max_age = param; return *this; }
    create_channel_invite_t & max_uses(int param) { _max_uses = param; return *this; }
    create_channel_invite_t & temporary(bool param) { _temporary = param; return *this; }
    create_channel_invite_t & unique(bool param) { _unique = param; return *this; }
    lib::optional<int> _max_age;
    lib::optional<int> _max_uses;
    lib::optional<bool> _temporary;
    lib::optional<bool> _unique;
};
#pragma endregion

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
    gateway::objects::channel::channel_type get_type() const noexcept
    {
        return type;
    }

    /// Get bot's permission for this channel
    /**
     * @returns Bitmask of current permissions for this channel contained within `permission` object
     */
    AEGIS_DECL permission perms();
#endif

    /// Send message to this channel
    /**
     * @param ec Indicates what error occurred, if any
     * @param content A string of the message to send
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> create_message(const std::string & content, int64_t nonce = 0);

    /// Send message to this channel
    /**
     * @see aegis::create_message_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> create_message(create_message_t obj)
    {
        return create_message(obj._content, obj._nonce);
    };

    /// Send an embed message to this channel
    /**
     * @param ec Indicates what error occurred, if any
     * @param content A string of the message to send
     * @param embed A json object of the embed object itself
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> create_message_embed(const std::string & content, const json & embed, int64_t nonce = 0);

    /// Send an embed message to this channel
    /**
     * @see aegis::create_message_embed_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> create_message_embed(create_message_embed_t obj)
    {
        return create_message_embed(obj._content, obj._embed, obj._nonce);
    }

    /// Edit a message in this channel
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of the message to replace. Must be your own message
     * @param content A string of the message to set
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> edit_message(snowflake message_id, const std::string & content);

    /// Edit a message in this channel
    /**
     * @see aegis::edit_message_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> edit_message(edit_message_t obj)
    {
        return edit_message(obj._message_id, obj._content);
    }

    /// Edit an embed message in this channel
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of the message to replace. Must be your own message
     * @param content A string of the message to set
     * @param embed A json object of the embed object itself
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> edit_message_embed(snowflake message_id, const std::string & content, const json & embed);

    /// Edit an embed message in this channel
    /**
     * @see aegis::edit_message_embed_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> edit_message_embed(edit_message_embed_t obj)
    {
        return edit_message_embed(obj._message_id, obj._content, obj._embed);
    }

    /// Delete a message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of the message to delete
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> delete_message(snowflake message_id);

    /// Delete up to 100 messages at once
    /**
     * @param ec Indicates what error occurred, if any
     * @param message Vector of up to 100 message snowflakes to delete
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> bulk_delete_message(const std::vector<int64_t> & messages);

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
    AEGIS_DECL aegis::future<rest::rest_reply> modify_channel(lib::optional<std::string> _name = {},
                        lib::optional<int> _position = {}, lib::optional<std::string> _topic = {},
                        lib::optional<bool> _nsfw = {}, lib::optional<int> _bitrate = {},
                        lib::optional<int> _user_limit = {},
                        lib::optional<std::vector<gateway::objects::permission_overwrite>> _permission_overwrites = {},
                        lib::optional<snowflake> _parent_id = {}, lib::optional<int> _rate_limit_per_user = {});

    /// Modify this channel (all parameters optional)
    /**
     * @see aegis::modify_channel_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> modify_channel(modify_channel_t obj)
    {
        return modify_channel(obj._name, obj._position, obj._topic, obj._nsfw,
                              obj._bitrate, obj._user_limit, obj._permission_overwrites,
                              obj._parent_id, obj._rate_limit_per_user);
    }

    /// Delete this channel
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> delete_channel();

    /// Add new reaction on message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> create_reaction(snowflake message_id, const std::string & emoji_text);

    /// Add new reaction on message
    /**
     * @see aegis::create_reaction_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> create_reaction(create_reaction_t obj)
    {
        return create_reaction(obj._message_id, obj._emoji_text);
    }

    /// Delete own reaction on message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> delete_own_reaction(snowflake message_id, const std::string & emoji_text);

    /// Delete own reaction on message
    /**
     * @see aegis::delete_own_reaction_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> delete_own_reaction(delete_own_reaction_t obj)
    {
        return delete_own_reaction(obj._message_id, obj._emoji_text);
    }

    /// Delete specified member reaction on message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @param member_id Snowflake of member to remove emoji from
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> delete_user_reaction(snowflake message_id, const std::string & emoji_text, snowflake member_id);

    /// Delete specified member reaction on message
    /**
     * @see aegis::delete_user_reaction_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> delete_user_reaction(delete_user_reaction_t obj)
    {
        return delete_user_reaction(obj._message_id, obj._emoji_text, obj._member_id);
    }

    /// Get all reactions for this message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of message
     * @param emoji_text Text of emoji being added `name:snowflake`
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> get_reactions(snowflake message_id, const std::string & emoji_text);

    /// Get all reactions for this message
    /**
     * @see aegis::get_reactions_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> get_reactions(get_reactions_t obj)
    {
        return get_reactions(obj._message_id, obj._emoji_text);
    }

    /// Delete all reactions by message
    /**
     * @param ec Indicates what error occurred, if any
     * @param message_id Snowflake of message
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> delete_all_reactions(snowflake message_id);

    /// Edit channel permission override
    /**
     * @param ec Indicates what error occurred, if any
     * @param _overwrite_id Snowflake of the permission override
     * @param _allow Int64 allow flags
     * @param _deny Int64 deny flags
     * @param _type Type of override (role/user)
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> edit_channel_permissions(snowflake _overwrite_id, int64_t _allow, int64_t _deny, const std::string & _type);

    /// Edit channel permission override
    /**
     * @see aegis::edit_channel_permissions_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> edit_channel_permissions(edit_channel_permissions_t obj)
    {
        return edit_channel_permissions(obj._overwrite_id, obj._allow, obj._deny, obj._type);
    }

    /// Get active channel invites
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> get_channel_invites();

    /// Create a new channel invite
    /**
     * @param ec Indicates what error occurred, if any
     * @param max_age How long this invite code lasts for in seconds
     * @param max_uses The max uses this invite code allows
     * @param temporary Is this invite code temporary
     * @param unique Is this invite code a unique one-use
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> create_channel_invite(const lib::optional<int> max_age, const lib::optional<int> max_uses, const lib::optional<bool> temporary, const lib::optional<bool> unique);

    /// Create a new channel invite
    /**
     * @see aegis::create_channel_invite_t
     * @param obj Struct of the contents of the request
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> create_channel_invite(create_channel_invite_t obj)
    {
        return create_channel_invite(obj._max_age, obj._max_uses, obj._temporary, obj._unique);
    }

    /// Delete channel permission override
    /**
     * @param ec Indicates what error occurred, if any
     * @param overwrite_id Snowflake of the channel permission to delete
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> delete_channel_permission(snowflake overwrite_id);

    /// Trigger typing indicator in channel (lasts 10 seconds)
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> trigger_typing_indicator();

    /// Get pinned messages in channel
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> get_pinned_messages();

    /// Add a pinned message in channel
    /**\todo
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> add_pinned_channel_message(snowflake message_id);

    /// Delete a pinned message in channel
    /**\todo
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> delete_pinned_channel_message(snowflake message_id);

    /// Add member to a group direct message
    /**\todo
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> group_dm_add_recipient(snowflake user_id);

    /// Remove member from a group direct message
    /**\todo
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL aegis::future<rest::rest_reply> group_dm_remove_recipient(snowflake user_id);

    /// Get the snowflake of this channel
    /**
     * @returns A snowflake of the channel
     */
    snowflake get_id() const noexcept
    {
        return channel_id;
    }

    /// Get the snowflake of this channel's guild - 0 if DM
    /**
     * @returns A snowflake of this channel's guild
     */
    snowflake get_guild_id() const noexcept
    {
        return guild_id;
    }

    /// Is this channel a DM or in a guild
    /**
     * @returns bool whether channel is a DM or belongs to a guild
     */
    bool is_dm() const noexcept
    {
        return _guild == nullptr;
    }

    shared_mutex & mtx() noexcept
    {
        return _m;
    }

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    bool nsfw() const noexcept
    {
        return _nsfw;
    }
#endif

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
    bool _nsfw = false;
    uint32_t position = 0; /**< Position of channel in guild channel list */
    gateway::objects::channel::channel_type type = gateway::objects::channel::channel_type::Text; /**< Type of channel */
    uint16_t bitrate = 0; /**< Bit rate of voice channel */
    uint16_t user_limit = 0; /**< User limit of voice channel */
    std::unordered_map<int64_t, gateway::objects::permission_overwrite> overrides; /**< Snowflake map of user/role to permission overrides */
    uint16_t rate_limit_per_user = 0; /**< Limit of how many seconds sent messages must have between each */
#endif
    asio::io_context & _io_context;
    mutable shared_mutex _m;
    core * _bot = nullptr;
};

}
