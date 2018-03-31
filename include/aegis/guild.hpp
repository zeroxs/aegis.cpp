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

#include "aegis/config.hpp"
#include "aegis/role.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/ratelimit.hpp"
#include "aegis/objects/permission_overwrite.hpp"
#include <future>


namespace aegiscpp
{

using rest_api = std::future<rest_reply>;

using rest_limits::bucket_factory;
using json = nlohmann::json;

class channel;
class member;
class shard;

class guild
{
public:
    /// Create a new guild
    /**
     * @param shard_id Shard id this guild belongs to
     *
     * @param state Pointer to state struct holding bot data
     *
     * @param id Snowflake of this guild id
     *
     * @param ratelimit Reference to the bucket factory that handles the ratelimits for this guild
     */
    explicit guild(int16_t shard_id, snowflake id, aegis * b, bucket_factory & ratelimit)
        : shard_id(shard_id)
        , guild_id(id)
        , ratelimit(ratelimit)
        , _bot(b)
    {
        unavailable = false;
    }

    AEGIS_DECL ~guild();

    guild(const guild &) = delete;
    guild(guild &&) = delete;
    guild & operator=(const guild &) = delete;

    int16_t shard_id;
    snowflake guild_id;
    bucket_factory & ratelimit;


    AEGIS_DECL std::future<rest_reply> post_task(std::string path, std::string method = "POST", 
                                                 const nlohmann::json & obj = {});

    permission perms()
    {
        return permission(base_permissions(self()));
    }

    /// Get pointer to own member object
    /**
     * @returns Pointer to own member object
     */
    AEGIS_DECL member * self() const;

    /// Get name of guild
    /**
     * @returns String of guild name
     */
    std::string get_name() const noexcept
    {
        return name;
    }

    /// Get region of guild
    /**
     * @returns String of region guild is in
     */
    std::string get_region() const noexcept
    {
        return region;
    }

    /// Check if member has role
    /**
     * @param member_id Snowflake of member
     *
     * @param role_id Snowflake of role
     *
     * @returns true if member has role
     */
    AEGIS_DECL bool member_has_role(snowflake member_id, snowflake role_id) const noexcept;

    /// Add new member to guild
    /**
     * @param _member Pointer of member object to add to guild
     */
    AEGIS_DECL void add_member(member * _member) noexcept;

    /// Remove member from guild
    /**
     * @param member_id Snowflake of member to remove from guild
     */
    AEGIS_DECL void remove_member(snowflake member_id) noexcept;

    /// Create and return new channel internally
    /**
     * @param id Snowflake of channel to add
     *
     * @param _shard Pointer to shard guild belongs to
     *
     * @returns Pointer to channel object created
     */
    AEGIS_DECL channel * get_channel_create(snowflake id, shard * _shard) noexcept;

    /// Load guild from json object
    /**
     * @param obj Const json object containing guild structure
     *
     * @param _shard Pointer to shard guild belongs to
     */
    AEGIS_DECL void load(const json & obj, shard * _shard) noexcept;

    /// Load member presence data
    /**
     * @param obj Const json object containing array of presence data
     */
    AEGIS_DECL void load_presence(const json & obj) noexcept;

    /// Load role
    /**
     * @param obj Const json object containing array of guild roles
     */
    AEGIS_DECL void load_role(const json & obj) noexcept;

    /// Get channel internally
    /**
     * @param id Snowflake of channel
     *
     * @returns Pointer to channel object created
     */
    AEGIS_DECL channel * get_channel(snowflake id) const noexcept;

    /// Get member internally
    /**
     * @param member_id Snowflake of member
     *
     * @returns Pointer to member object created
     */
    AEGIS_DECL member * get_member(snowflake member_id) const noexcept;

    /// Get count of members in guild (potentially inaccurate)
    /**
     * @returns Count of members in guild
     */
    AEGIS_DECL int32_t get_member_count() const noexcept;

    /// Get guild permissions for member in channel
    /**
     * @param member_id Snowflake of member
     *
     * @param channel_id Snowflake of channel
     *
     * @returns Permission object of channel
     */
    AEGIS_DECL permission get_permissions(snowflake member_id, snowflake channel_id) noexcept;

    /// Get guild permissions for member in channel
    /**
     * @param _member Reference to member object
     *
     * @param _channel Reference to channel object
     *
     * @returns Permission object of channel
     */
    AEGIS_DECL permission get_permissions(member & _member, channel & _channel) noexcept;

    /// Get base guild permissions for member
    /**
     * @param _member Pointer to member object
     */
    int64_t base_permissions(member * _member) const noexcept
    {
        return base_permissions(*_member);
    }

    /// Get base guild permissions for member
    /**
     * @param _member Reference to member object
     */
    AEGIS_DECL int64_t base_permissions(member & _member) const noexcept;

    /// Calculate permission overrides for member in channel
    /**
     * @param _base_permissions Base guild permissions for member in guild
     *
     * @param _member Reference to member object
     *
     * @param _channel Reference to channel object
     *
     * @returns true on successful request, false for no permissions
     */
    AEGIS_DECL int64_t compute_overwrites(int64_t _base_permissions, member & _member, channel & _channel) const noexcept;

    /// Get role
    /**
     * @param r Snowflake of role
     *
     * @returns Reference to role object
     */
    AEGIS_DECL const role & get_role(int64_t r) const;

    /// Remove role from guild
    /**
     * @param role_id Snowflake of role to remove
     */
    AEGIS_DECL void remove_role(snowflake role_id);

    /// Get owner of guild
    /**
     * @returns Snowflake of owner
     */
    AEGIS_DECL const snowflake get_owner() const noexcept;

    /// Create a new guild
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_guild(std::error_code & ec);

    /// Create a new guild
    /**
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_guild()
    {
        std::error_code ec;
        auto res = create_guild(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Get guild information
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api get_guild(std::error_code & ec);

    /// Get guild information
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api get_guild()
    {
        std::error_code ec;
        auto res = get_guild(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Modify guild information
    /**\todo better elaborate
     * @param ec Indicates what error occurred, if any
     * 
     * @param name Set name of guild
     * 
     * @param voice_region Set region for voice
     * 
     * @param verification_level Set verification level from unrestricted level to verified phone level
     * 
     * @param default_message_notifications Set default notification level for new members
     * 
     * @param afk_channel_id Set channel for idle voice connections to be moved to
     * 
     * @param afk_timeout Set time where voice connections are considered to be idle
     * 
     * @param icon Set icon \todo
     * 
     * @param owner_id Transfer owner to someone else
     * 
     * @param splash \todo
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild(std::error_code & ec, std::optional<std::string> name = {}, std::optional<std::string> voice_region = {},
                                     std::optional<int> verification_level = {}, std::optional<int> default_message_notifications = {},
                                     std::optional<snowflake> afk_channel_id = {}, std::optional<int> afk_timeout = {},
                                     std::optional<std::string> icon = {}, std::optional<snowflake> owner_id = {},
                                     std::optional<std::string> splash = {});


    /// Modify guild information
    /**\todo better elaborate
     * @param name Set name of guild
     * 
     * @param voice_region Set region for voice
     * 
     * @param verification_level Set verification level from unrestricted level to verified phone level
     * 
     * @param default_message_notifications Set default notification level for new members
     * 
     * @param afk_channel_id Set channel for idle voice connections to be moved to
     * 
     * @param afk_timeout Set time where voice connections are considered to be idle
     * 
     * @param icon Set icon \todo
     * 
     * @param owner_id Transfer owner to someone else
     * 
     * @param splash \todo
     * 
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild(std::optional<std::string> name = {}, std::optional<std::string> voice_region = {},
                                     std::optional<int> verification_level = {}, std::optional<int> default_message_notifications = {},
                                     std::optional<snowflake> afk_channel_id = {}, std::optional<int> afk_timeout = {},
                                     std::optional<std::string> icon = {}, std::optional<snowflake> owner_id = {},
                                     std::optional<std::string> splash = {})
    {
        std::error_code ec;
        auto res = modify_guild(ec, name, voice_region, verification_level, default_message_notifications,
                                afk_channel_id, afk_timeout, icon, owner_id, splash);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete a guild
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api delete_guild(std::error_code & ec);

    /// Delete a guild
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api delete_guild()
    {
        std::error_code ec;
        auto res = delete_guild(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Create a text channel
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param name String of the name for the channel
     *
     * @param parent_id The channel or category to place this channel below
     *
     * @param nsfw Whether the channel will be labeled as not safe for work
     *
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_text_channel(std::error_code & ec, std::string name, int64_t parent_id, bool nsfw,
                                            std::vector<permission_overwrite> permission_overwrites);

    /// Create a text channel
    /**
     * @param name String of the name for the channel
     *
     * @param parent_id The channel or category to place this channel below
     *
     * @param nsfw Whether the channel will be labeled as not safe for work
     *
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     *
     * @throws aegiscpp::exception Thrown on failure.
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_text_channel(std::string name, int64_t parent_id, bool nsfw,
                                            std::vector<permission_overwrite> permission_overwrites)
    {
        std::error_code ec;
        auto res = create_text_channel(ec, name, parent_id, nsfw, permission_overwrites);
        if (ec)
            throw ec;
        return res;
    }

    /// Create a voice channel
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param name String of the name for the channel
     *
     * @param bitrate The bitrate count of the channel
     *
     * @param user_limit The max amount of members that may join the channel
     *
     * @param parent_id The channel or category to place this channel below
     *
     * @param nsfw Whether the channel will be labeled as not safe for work
     *
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_voice_channel(std::error_code & ec, std::string name, int32_t bitrate, int32_t user_limit, int64_t parent_id,
                                             bool nsfw, std::vector<permission_overwrite> permission_overwrites);

    /// Create a voice channel
    /**
     * @param name String of the name for the channel
     *
     * @param bitrate The bitrate count of the channel
     *
     * @param user_limit The max amount of members that may join the channel
     *
     * @param parent_id The channel or category to place this channel below
     *
     * @param nsfw Whether the channel will be labelled as not safe for work
     *
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     *
     * @throws aegiscpp::exception Thrown on failure.
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_voice_channel(std::string name, int32_t bitrate, int32_t user_limit, int64_t parent_id,
                                             bool nsfw, std::vector<permission_overwrite> permission_overwrites)
    {
        std::error_code ec;
        auto res = create_voice_channel(ec, name, bitrate, user_limit, parent_id, nsfw, permission_overwrites);
        if (ec)
            throw ec;
        return res;
    }

    /// Create a category
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param name String of the name for the channel
     *
     * @param parent_id The channel or category to place this channel below
     *
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_category_channel(std::error_code & ec, std::string name, int64_t parent_id,
                                                std::vector<permission_overwrite> permission_overwrites);

    /// Create a category
    /**
     * @param name String of the name for the channel
     *
     * @param parent_id The channel or category to place this channel below
     *
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     *
     * @throws aegiscpp::exception Thrown on failure.
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_category_channel(std::string name, int64_t parent_id,
                                                std::vector<permission_overwrite> permission_overwrites)
    {
        std::error_code ec;
        auto res = create_category_channel(ec, name, parent_id, permission_overwrites);
        if (ec)
            throw ec;
        return res;
    }

    /// Modify positions of channels
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_channel_positions(std::error_code & ec);

    /// Modify positions of channels
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_channel_positions()
    {
        std::error_code ec;
        auto res = modify_channel_positions(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Modify a member
    /// All fields are optional
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param user_id The snowflake of the user to edit
     *
     * @param nick String of nickname to change to
     *
     * @param mute Whether or not to voice mute the member
     *
     * @param deaf Whether or not to voice deafen the member
     *
     * @param roles Array of roles to apply to the member
     *
     * @param channel_id Snowflake of the channel to move user to
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild_member(std::error_code & ec, snowflake user_id, std::optional<std::string> nick, std::optional<bool> mute,
                                            std::optional<bool> deaf, std::optional<std::vector<snowflake>> roles,
                                            std::optional<snowflake> channel_id);

    /// Modify a member
    /// All fields are optional
    /**
     * @param user_id The snowflake of the user to edit
     *
     * @param nick String of nickname to change to
     *
     * @param mute Whether or not to voice mute the member
     *
     * @param deaf Whether or not to voice deafen the member
     *
     * @param roles Array of roles to apply to the member
     *
     * @param channel_id Snowflake of the channel to move user to
     *
     * @throws aegiscpp::exception Thrown on failure.
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild_member(snowflake user_id, std::optional<std::string> nick, std::optional<bool> mute,
                                            std::optional<bool> deaf, std::optional<std::vector<snowflake>> roles,
                                            std::optional<snowflake> channel_id)
    {
        std::error_code ec;
        auto res = modify_guild_member(ec, user_id, nick, mute, deaf, roles, channel_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Modify own nickname
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param newname String of the new nickname to apply
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_my_nick(std::error_code & ec, std::string newname);

    /// Modify own nickname
    /**
     * @param newname String of the new nickname to apply
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_my_nick(std::string newname)
    {
        std::error_code ec;
        auto res = modify_my_nick(ec, newname);
        if (ec)
            throw ec;
        return res;
    }

    /// Add a role to guild member
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param user_id The snowflake of the user to add new role
     *
     * @param role_id The snowflake of the role to add to member
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api add_guild_member_role(std::error_code & ec, snowflake user_id, snowflake role_id);

    /// Add a role to guild member
    /**
     * @param user_id The snowflake of the user to add new role
     *
     * @param role_id The snowflake of the role to add to member
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api add_guild_member_role(snowflake user_id, snowflake role_id)
    {
        std::error_code ec;
        auto res = add_guild_member_role(ec, user_id, role_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Remove a role from guild member
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param user_id The snowflake of the user to remove role
     *
     * @param role_id The snowflake of the role to remove from member
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api remove_guild_member_role(std::error_code & ec, snowflake user_id, snowflake role_id);

    /// Remove a role from guild member
    /**
     * @param user_id The snowflake of the user to remove role
     *
     * @param role_id The snowflake of the role to remove from member
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api remove_guild_member_role(snowflake user_id, snowflake role_id)
    {
        std::error_code ec;
        auto res = remove_guild_member_role(ec, user_id, role_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Remove guild member (kick)
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param user_id The snowflake of the member to kick
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api remove_guild_member(std::error_code & ec, snowflake user_id);

    /// Remove guild member (kick)
    /**
     * @param user_id The snowflake of the member to kick
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api remove_guild_member(snowflake user_id)
    {
        std::error_code ec;
        auto res = remove_guild_member(ec, user_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Create a new guild ban
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param user_id The snowflake of the member to ban
     *
     * @param delete_message_days How many days to delete member message history (0-7)
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_guild_ban(std::error_code & ec, snowflake user_id, int8_t delete_message_days, std::string reason = "");

    /// Create a new guild ban
    /**
     * @param user_id The snowflake of the member to ban
     *
     * @param delete_message_days How many days to delete member message history (0-7)
     *
     * @throws aegiscpp::exception Thrown on failure.

     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_guild_ban(snowflake user_id, int8_t delete_message_days, std::string reason = "")
    {
        std::error_code ec;
        auto res = create_guild_ban(ec, user_id, delete_message_days, reason);
        if (ec)
            throw ec;
        return res;
    }

    /// Remove a guild ban
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param user_id The snowflake of the member to unban
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api remove_guild_ban(std::error_code & ec, snowflake user_id);

    /// Remove a guild ban
    /**
     * @param user_id The snowflake of the member to unban
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api remove_guild_ban(snowflake user_id)
    {
        std::error_code ec;
        auto res = remove_guild_ban(ec, user_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Create a guild role
    /**
     * @see aegiscpp::permission
     *
     * @param ec Indicates what error occurred, if any
     *
     * @param name The name of the role to create
     *
     * @param _perms The permissions to set
     *
     * @param color 32bit integer of the color
     *
     * @param hoist Whether the role should be separated from other roles
     *
     * @param mentionable Whether the role can be specifically mentioned
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_guild_role(std::error_code & ec, std::string name, permission _perms, int32_t color, bool hoist, bool mentionable);

    /// Create a guild role
    /**
     * @see aegiscpp::permission
     *
     * @param name The name of the role to create
     *
     * @param _perms The permissions to set
     *
     * @param color 32bit integer of the color
     *
     * @param hoist Whether the role should be separated from other roles
     *
     * @param mentionable Whether the role can be specifically mentioned
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_guild_role(std::string name, permission _perms, int32_t color, bool hoist, bool mentionable)
    {
        std::error_code ec;
        auto res = create_guild_role(ec, name, _perms, color, hoist, mentionable);
        if (ec)
            throw ec;
        return res;
    }

    /// Modify the guild role positions
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param role_id Snowflake of role to change position
     * 
     * @param position Index of position to change role to
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild_role_positions(std::error_code & ec, snowflake role_id, int16_t position);

    /// Modify the guild role positions
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @param role_id Snowflake of role to change position
     *
     * @param position Index of position to change role to
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild_role_positions(snowflake role_id, int16_t position)
    {
        std::error_code ec;
        auto res = modify_guild_role_positions(ec, role_id, position);
        if (ec)
            throw ec;
        return res;
    }

    /// Modify a guild role
    /**
     * @see aegiscpp::permission
     *
     * @param ec Indicates what error occurred, if any
     *
     * @param id The snowflake of the role to modify
     *
     * @param name The name to set the role to
     *
     * @param _perms The permissions to set
     *
     * @param color 32bit integer of the color
     *
     * @param hoist Whether the role should be separated from other roles
     *
     * @param mentionable Whether the role can be specifically mentioned
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild_role(std::error_code & ec, snowflake id, std::string name, permission _perms, int32_t color,
                                          bool hoist, bool mentionable);

    /// Modify a guild role
    /**
     * @see aegiscpp::permission
     *
     * @param id The snowflake of the role to modify
     *
     * @param name The name to set the role to
     *
     * @param _perms The permissions to set
     *
     * @param color 32bit integer of the color
     *
     * @param hoist Whether the role should be separated from other roles
     *
     * @param mentionable Whether the role can be specifically mentioned
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild_role(snowflake id, std::string name, permission _perms, int32_t color,
                                          bool hoist, bool mentionable)
    {
        std::error_code ec;
        auto res = modify_guild_role(ec, id, name, _perms, color, hoist, mentionable);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete a guild role
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param role_id The snowflake of the role to delete
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api delete_guild_role(std::error_code & ec, snowflake role_id);

    /// Delete a guild role
    /**
     * @param role_id The snowflake of the role to delete
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api delete_guild_role(snowflake role_id)
    {
        std::error_code ec;
        auto res = delete_guild_role(ec, role_id);
        if (ec)
            throw ec;
        return res;
    }

    /// Get a count of members that would be pruned
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param days The days of inactivity to prune the member
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api get_guild_prune_count(std::error_code & ec, int16_t days);

    /// Get a count of members that would be pruned
    /**
     * @param days The days of inactivity to prune the member
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api get_guild_prune_count(int16_t days)
    {
        std::error_code ec;
        auto res = get_guild_prune_count(ec, days);
        if (ec)
            throw ec;
        return res;
    }

    /// Perform a guild prune
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @param days The days of inactivity to prune the member
     *
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api begin_guild_prune(std::error_code & ec, int16_t days);

    /// Perform a guild prune
    /**
     * @param days The days of inactivity to prune the member
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api begin_guild_prune(int16_t days)
    {
        std::error_code ec;
        auto res = begin_guild_prune(ec, days);
        if (ec)
            throw ec;
        return res;
    }

    /// Get active guild invites
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api get_guild_invites(std::error_code & ec);

    /// Get active guild invites
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api get_guild_invites()
    {
        std::error_code ec;
        auto res = get_guild_invites(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Get guild integrations
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api get_guild_integrations(std::error_code & ec);

    /// Get guild integrations
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api get_guild_integrations()
    {
        std::error_code ec;
        auto res = get_guild_integrations(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Create a new guild integration
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_guild_integration(std::error_code & ec);

    /// Create a new guild integration
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api create_guild_integration()
    {
        std::error_code ec;
        auto res = create_guild_integration(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Modify a guild integration
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild_integration(std::error_code & ec);

    /// Modify a guild integration
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild_integration()
    {
        std::error_code ec;
        auto res = modify_guild_integration(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete a guild integration
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api delete_guild_integration(std::error_code & ec);

    /// Delete a guild integration
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api delete_guild_integration()
    {
        std::error_code ec;
        auto res = delete_guild_integration(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Get the guild integrations
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api sync_guild_integration(std::error_code & ec);

    /// Get the guild integrations
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api sync_guild_integration()
    {
        std::error_code ec;
        auto res = sync_guild_integration(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Get the guild embed settings
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api get_guild_embed(std::error_code & ec);

    /// Get the guild embed settings
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api get_guild_embed()
    {
        std::error_code ec;
        auto res = get_guild_embed(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Modify the guild embed settings
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild_embed(std::error_code & ec);

    /// Modify the guild embed settings
    /**
     * @throws aegiscpp::exception Thrown on failure.

     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api modify_guild_embed()
    {
        std::error_code ec;
        auto res = modify_guild_embed(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Leave the guild this object is associated with
    /**
     * @param ec Indicates what error occurred, if any
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api leave(std::error_code & ec);

    /// Leave the guild this object is associated with
    /**
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns std::future<rest_reply>
     */
    AEGIS_DECL rest_api leave()
    {
        std::error_code ec;
        auto res = leave(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Gets the Bot object
    /**
     * @see aegis
     *
     * @throws aegiscpp::exception Thrown on failure.
     * 
     * @returns Aegis main object
     */
    AEGIS_DECL aegis & get_bot() const noexcept;

    /// Get the snowflake of this guild
    /**
     * @returns A snowflake of this guild
     */
    const snowflake get_id() const noexcept
    {
        return guild_id;
    }

    std::map<snowflake, channel*> channels; /**< Map of snowflakes to channel objects */
    std::map<snowflake, member*> members; /**< Map of snowflakes to member objects */
    std::map<snowflake, role> roles; /**< Map of snowflakes to role objects */

private:
    friend class aegis;
    friend class channel;
    friend class member;
    friend class message;

    std::string name;
    std::string icon;
    std::string splash;
    snowflake owner_id = 0;
    std::string region;
    snowflake afk_channel_id = 0;
    uint32_t afk_timeout = 0;//in seconds
    bool embed_enabled = false;
    snowflake embed_channel_id = 0;
    uint32_t verification_level = 0;
    uint32_t default_message_notifications = 0;
    uint32_t mfa_level = 0;
    std::string joined_at;
    bool large = false;
    bool unavailable = false;
    uint32_t member_count = 0;
    //std::string m_voice_states;//this is really an array
    bool is_init = true;
    aegis * _bot;
};

}

#if defined(AEGIS_HEADER_ONLY)
# include "aegis/guild.cpp"
#endif // defined(AEGIS_HEADER_ONLY)
