//
// guild.hpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/utility.hpp"
#include "aegis/objects/role.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/rest/rest_reply.hpp"
#include "aegis/ratelimit/ratelimit.hpp"
#include "aegis/objects/permission_overwrite.hpp"
#include <future>
#include <asio.hpp>
#include <shared_mutex>

namespace aegis
{

#if (AEGIS_HAS_STD_SHARED_MUTEX == 1)
using shared_mutex = std::shared_mutex;
#else
using shared_mutex = std::shared_timed_mutex;
#endif

using json = nlohmann::json;

/// Class for performing actions pertaining to a specified guild
class guild
{
public:
    /// Create a new guild
    /**
     * @param shard_id Shard id this guild belongs to
     * @param state Pointer to state struct holding bot data
     * @param id Snowflake of this guild
     * @param _ratelimit Reference to the bucket factory that handles the ratelimits for this guild
     */
    AEGIS_DECL explicit guild(const int32_t _shard_id, const snowflake _id, core * _bot, asio::io_context & _io);

    AEGIS_DECL ~guild();

    guild(const guild &) = delete;
    guild(guild &&) = delete;
    guild & operator=(const guild &) = delete;

    int32_t shard_id; /*< shard that receives this guild's messages */
    snowflake guild_id; /*< snowflake of this guild */

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
    AEGIS_DECL std::future<rest::rest_reply> post_task(const std::string & path, const std::string & method = "POST",
                                                       const std::string & obj = "", const std::string & host = "");

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    /// Get bot's current permissions for this guild
    /**
    * @returns Bitmask of current permissions for this guild contained within `permission` object
    */
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
     * @param role_id Snowflake of role
     * @returns true if member has role
     */
    AEGIS_DECL bool member_has_role(snowflake member_id, snowflake role_id) const noexcept;

    /// Get count of members in guild (potentially inaccurate)
    /**
     * @returns Count of members in guild
     */
    AEGIS_DECL int32_t get_member_count() const noexcept;

    /// Get guild permissions for member in channel
    /**
     * @param member_id Snowflake of member
     * @param channel_id Snowflake of channel
     * @returns Permission object of channel
     */
    AEGIS_DECL permission get_permissions(snowflake member_id, snowflake channel_id) noexcept;

    /// Get guild permissions for member in channel
    /**
     * @param _member Pointer to member object
     * @param _channel Pointer to channel object
     * @returns Permission object of channel
     */
    AEGIS_DECL permission get_permissions(member * _member, channel * _channel) noexcept;

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
     * @param _member Reference to member object
     * @param _channel Reference to channel object
     * @returns true on successful request, false for no permissions
     */
    AEGIS_DECL int64_t compute_overwrites(int64_t _base_permissions, member & _member, channel & _channel) const noexcept;

    /// Get role
    /**
     * @param r Snowflake of role
     * @returns Reference to role object
     */
    AEGIS_DECL const gateway::objects::role & get_role(int64_t r) const;

    /// Get owner of guild
    /**
     * @returns Snowflake of owner
     */
    AEGIS_DECL const snowflake get_owner() const noexcept;
#endif

    /// Get the snowflake of this guild
    /**
    * @returns A snowflake of this guild
    */
    const snowflake get_id() const noexcept
    {
        return guild_id;
    }

    /// Get channel internally
    /**
     * @param id Snowflake of channel
     * @returns Pointer to channel object created
     */
    AEGIS_DECL channel * get_channel(snowflake id) const noexcept;

    /// Get guild information
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> get_guild(std::error_code & ec);

    /// Get guild information
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> get_guild()
    {
        std::error_code ec;
        auto res = get_guild(ec);
        if (ec)
            throw ec;
        return res;
    }

    /// Modify guild information
    /**
     * @param ec Indicates what error occurred, if any
     * @param name Set name of guild
     * @param voice_region Set region for voice
     * @param verification_level Set verification level from unrestricted level to verified phone level
     * (NONE=0, LOW(verified email)=1, MEDIUM(registered >5m)=2, HIGH(member of server >10m)=3 VERY_HIGH(verified phone)=4
     * @param default_message_notifications Set default notification level for new members
     * @param explicit_content_filter Set filter level for new content
     * (DISABLED=0, MEMBERS_WITHOUT_ROLES=1, ALL_MEMBERS=2)
     * @param afk_channel_id Set channel for idle voice connections to be moved to
     * @param afk_timeout Set time where voice connections are considered to be idle
     * @param icon Set icon \todo
     * @param owner_id Transfer owner to someone else
     * @param splash \todo
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> modify_guild(
        std::error_code & ec, lib::optional<std::string> name = {},
        lib::optional<std::string> voice_region = {}, lib::optional<int> verification_level = {},
        lib::optional<int> default_message_notifications = {}, lib::optional<int> explicit_content_filter = {},
        lib::optional<snowflake> afk_channel_id = {}, lib::optional<int> afk_timeout = {},
        lib::optional<std::string> icon = {}, lib::optional<snowflake> owner_id = {},
        lib::optional<std::string> splash = {}
    );


    /// Modify guild information
    /**
     * @param name Set name of guild
     * @param voice_region Set region for voice
     * @param verification_level Set verification level from unrestricted level to verified phone level
     * (NONE=0, LOW(verified email)=1, MEDIUM(registered >5m)=2, HIGH(member of server >10m)=3 VERY_HIGH(verified phone)=4
     * @param default_message_notifications Set default notification level for new members
     * (ALL_MESSAGES=0, ONLY_MENTIONS=1)
     * @param explicit_content_filter Set filter level for new content
     * (DISABLED=0, MEMBERS_WITHOUT_ROLES=1, ALL_MEMBERS=2)
     * @param afk_channel_id Set channel for idle voice connections to be moved to
     * @param afk_timeout Set time where voice connections are considered to be idle
     * @param icon Set icon \todo
     * @param owner_id Transfer owner to someone else
     * @param splash \todo
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> modify_guild(
        lib::optional<std::string> name = {},
        lib::optional<std::string> voice_region = {}, lib::optional<int> verification_level = {},
        lib::optional<int> default_message_notifications = {}, lib::optional<int> explicit_content_filter = {},
        lib::optional<snowflake> afk_channel_id = {}, lib::optional<int> afk_timeout = {},
        lib::optional<std::string> icon = {}, lib::optional<snowflake> owner_id = {},
        lib::optional<std::string> splash = {}
    )
    {
        std::error_code ec;
        auto res = modify_guild(ec, name, voice_region, verification_level, default_message_notifications,
                                explicit_content_filter, afk_channel_id, afk_timeout, icon, owner_id, splash);
        if (ec)
            throw ec;
        return res;
    }

    /// Delete a guild
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> delete_guild(std::error_code & ec);

    /// Delete a guild
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> delete_guild()
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
     * @param name String of the name for the channel
     * @param parent_id The channel or category to place this channel below
     * @param nsfw Whether the channel will be labeled as not safe for work
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> create_text_channel(std::error_code & ec, const std::string & name, int64_t parent_id = 0, bool nsfw = false,
                                            const std::vector<gateway::objects::permission_overwrite> & permission_overwrites = {});

    /// Create a text channel
    /**
     * @param name String of the name for the channel
     * @param parent_id The channel or category to place this channel below
     * @param nsfw Whether the channel will be labeled as not safe for work
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> create_text_channel(const std::string & name, int64_t parent_id = 0, bool nsfw = false ,
                                            const std::vector<gateway::objects::permission_overwrite> & permission_overwrites = {})
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
     * @param name String of the name for the channel
     * @param bitrate The bitrate count of the channel
     * @param user_limit The max amount of members that may join the channel
     * @param parent_id The channel or category to place this channel below
     * @param nsfw Whether the channel will be labeled as not safe for work
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> create_voice_channel(std::error_code & ec, const std::string & name, int32_t bitrate = 0, int32_t user_limit = 0, int64_t parent_id = 0,
                                             const std::vector<gateway::objects::permission_overwrite> & permission_overwrites = {});

    /// Create a voice channel
    /**
     * @param name String of the name for the channel
     * @param bitrate The bitrate count of the channel
     * @param user_limit The max amount of members that may join the channel
     * @param parent_id The channel or category to place this channel below
     * @param nsfw Whether the channel will be labelled as not safe for work
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> create_voice_channel(const std::string & name, int32_t bitrate = 0, int32_t user_limit = 0, int64_t parent_id = 0,
                                             const std::vector<gateway::objects::permission_overwrite> & permission_overwrites = {})
    {
        std::error_code ec;
        auto res = create_voice_channel(ec, name, bitrate, user_limit, parent_id, permission_overwrites);
        if (ec)
            throw ec;
        return res;
    }

    /// Create a category
    /**
     * @param ec Indicates what error occurred, if any
     * @param name String of the name for the channel
     * @param parent_id The channel or category to place this channel below
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> create_category_channel(std::error_code & ec, const std::string & name, int64_t parent_id,
                                                const std::vector<gateway::objects::permission_overwrite> & permission_overwrites);

    /// Create a category
    /**
     * @param name String of the name for the channel
     * @param parent_id The channel or category to place this channel below
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> create_category_channel(const std::string & name, int64_t parent_id,
                                                const std::vector<gateway::objects::permission_overwrite> & permission_overwrites)
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
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> modify_channel_positions(std::error_code & ec);

    /// Modify positions of channels
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> modify_channel_positions()
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
     * @param user_id The snowflake of the user to edit
     * @param nick String of nickname to change to
     * @param mute Whether or not to voice mute the member
     * @param deaf Whether or not to voice deafen the member
     * @param roles Array of roles to apply to the member
     * @param channel_id Snowflake of the channel to move user to
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> modify_guild_member(std::error_code & ec, snowflake user_id, lib::optional<std::string> nick, lib::optional<bool> mute,
                                            lib::optional<bool> deaf, lib::optional<std::vector<snowflake>> roles,
                                            lib::optional<snowflake> channel_id);

    /// Modify a member
    /// All fields are optional
    /**
     * @param user_id The snowflake of the user to edit
     * @param nick String of nickname to change to
     * @param mute Whether or not to voice mute the member
     * @param deaf Whether or not to voice deafen the member
     * @param roles Array of roles to apply to the member
     * @param channel_id Snowflake of the channel to move user to
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> modify_guild_member(snowflake user_id, lib::optional<std::string> nick, lib::optional<bool> mute,
                                            lib::optional<bool> deaf, lib::optional<std::vector<snowflake>> roles,
                                            lib::optional<snowflake> channel_id)
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
     * @param newname String of the new nickname to apply
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> modify_my_nick(std::error_code & ec, const std::string & newname);

    /// Modify own nickname
    /**
     * @param newname String of the new nickname to apply
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> modify_my_nick(const std::string & newname)
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
     * @param user_id The snowflake of the user to add new role
     * @param role_id The snowflake of the role to add to member
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> add_guild_member_role(std::error_code & ec, snowflake user_id, snowflake role_id);

    /// Add a role to guild member
    /**
     * @param user_id The snowflake of the user to add new role
     * @param role_id The snowflake of the role to add to member
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> add_guild_member_role(snowflake user_id, snowflake role_id)
    {
        std::error_code ec;
        auto res = add_guild_member_role(ec, user_id, role_id);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Remove a role from guild member
    /**
     * @param ec Indicates what error occurred, if any
     * @param user_id The snowflake of the user to remove role
     * @param role_id The snowflake of the role to remove from member
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> remove_guild_member_role(std::error_code & ec, snowflake user_id, snowflake role_id);

    /// Remove a role from guild member
    /**
     * @param user_id The snowflake of the user to remove role
     * @param role_id The snowflake of the role to remove from member
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> remove_guild_member_role(snowflake user_id, snowflake role_id)
    {
        std::error_code ec;
        auto res = remove_guild_member_role(ec, user_id, role_id);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Remove guild member (kick)
    /**
     * @param ec Indicates what error occurred, if any
     * @param user_id The snowflake of the member to kick
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> remove_guild_member(std::error_code & ec, snowflake user_id);

    /// Remove guild member (kick)
    /**
     * @param user_id The snowflake of the member to kick
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> remove_guild_member(snowflake user_id)
    {
        std::error_code ec;
        auto res = remove_guild_member(ec, user_id);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Create a new guild ban
    /**
     * @param ec Indicates what error occurred, if any
     * @param user_id The snowflake of the member to ban
     * @param delete_message_days How many days to delete member message history (0-7)
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> create_guild_ban(std::error_code & ec, snowflake user_id, int8_t delete_message_days, const std::string & reason = "");

    /// Create a new guild ban
    /**
     * @param user_id The snowflake of the member to ban
     * @param delete_message_days How many days to delete member message history (0-7)
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> create_guild_ban(snowflake user_id, int8_t delete_message_days, const std::string & reason = "")
    {
        std::error_code ec;
        auto res = create_guild_ban(ec, user_id, delete_message_days, reason);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Remove a guild ban
    /**
     * @param ec Indicates what error occurred, if any
     * @param user_id The snowflake of the member to unban
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> remove_guild_ban(std::error_code & ec, snowflake user_id);

    /// Remove a guild ban
    /**
     * @param user_id The snowflake of the member to unban
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> remove_guild_ban(snowflake user_id)
    {
        std::error_code ec;
        auto res = remove_guild_ban(ec, user_id);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Create a guild role
    /**
     * @see aegis::permission
     * @param ec Indicates what error occurred, if any
     * @param name The name of the role to create
     * @param _perms The permissions to set
     * @param color 32bit integer of the color
     * @param hoist Whether the role should be separated from other roles
     * @param mentionable Whether the role can be specifically mentioned
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> create_guild_role(std::error_code & ec, const std::string & name, permission _perms, int32_t color, bool hoist, bool mentionable);

    /// Create a guild role
    /**
     * @see aegis::permission
     * @param name The name of the role to create
     * @param _perms The permissions to set
     * @param color 32bit integer of the color
     * @param hoist Whether the role should be separated from other roles
     * @param mentionable Whether the role can be specifically mentioned
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> create_guild_role(const std::string & name, permission _perms, int32_t color, bool hoist, bool mentionable)
    {
        std::error_code ec;
        auto res = create_guild_role(ec, name, _perms, color, hoist, mentionable);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Modify the guild role positions
    /**
     * @param ec Indicates what error occurred, if any
     * @param role_id Snowflake of role to change position
     * @param position Index of position to change role to
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> modify_guild_role_positions(std::error_code & ec, snowflake role_id, int16_t position);

    /// Modify the guild role positions
    /**
     * @param role_id Snowflake of role to change position
     * @param position Index of position to change role to
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> modify_guild_role_positions(snowflake role_id, int16_t position)
    {
        std::error_code ec;
        auto res = modify_guild_role_positions(ec, role_id, position);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Modify a guild role
    /**
     * @see aegis::permission
     * @param ec Indicates what error occurred, if any
     * @param id The snowflake of the role to modify
     * @param name The name to set the role to
     * @param _perms The permissions to set
     * @param color 32bit integer of the color
     * @param hoist Whether the role should be separated from other roles
     * @param mentionable Whether the role can be specifically mentioned
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> modify_guild_role(std::error_code & ec, snowflake id, const std::string & name, permission _perms, int32_t color,
                                          bool hoist, bool mentionable);

    /// Modify a guild role
    /**
     * @see aegis::permission
     * @param id The snowflake of the role to modify
     * @param name The name to set the role to
     * @param _perms The permissions to set
     * @param color 32bit integer of the color
     * @param hoist Whether the role should be separated from other roles
     * @param mentionable Whether the role can be specifically mentioned
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> modify_guild_role(snowflake id, const std::string & name, permission _perms, int32_t color,
                                          bool hoist, bool mentionable)
    {
        std::error_code ec;
        auto res = modify_guild_role(ec, id, name, _perms, color, hoist, mentionable);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Delete a guild role
    /**
     * @param ec Indicates what error occurred, if any
     * @param role_id The snowflake of the role to delete
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> delete_guild_role(std::error_code & ec, snowflake role_id);

    /// Delete a guild role
    /**
     * @param role_id The snowflake of the role to delete
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> delete_guild_role(snowflake role_id)
    {
        std::error_code ec;
        auto res = delete_guild_role(ec, role_id);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Get a count of members that would be pruned
    /**
     * @param ec Indicates what error occurred, if any
     * @param days The days of inactivity to prune the member
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> get_guild_prune_count(std::error_code & ec, int16_t days);

    /// Get a count of members that would be pruned
    /**
     * @param days The days of inactivity to prune the member
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> get_guild_prune_count(int16_t days)
    {
        std::error_code ec;
        auto res = get_guild_prune_count(ec, days);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Perform a guild prune
    /**
     * @param ec Indicates what error occurred, if any
     * @param days The days of inactivity to prune the member
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> begin_guild_prune(std::error_code & ec, int16_t days);

    /// Perform a guild prune
    /**
     * @param days The days of inactivity to prune the member
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> begin_guild_prune(int16_t days)
    {
        std::error_code ec;
        auto res = begin_guild_prune(ec, days);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Get active guild invites
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> get_guild_invites(std::error_code & ec);

    /// Get active guild invites
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> get_guild_invites()
    {
        std::error_code ec;
        auto res = get_guild_invites(ec);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Get guild integrations
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> get_guild_integrations(std::error_code & ec);

    /// Get guild integrations
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> get_guild_integrations()
    {
        std::error_code ec;
        auto res = get_guild_integrations(ec);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Create a new guild integration
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> create_guild_integration(std::error_code & ec);

    /// Create a new guild integration
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> create_guild_integration()
    {
        std::error_code ec;
        auto res = create_guild_integration(ec);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Modify a guild integration
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> modify_guild_integration(std::error_code & ec);

    /// Modify a guild integration
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> modify_guild_integration()
    {
        std::error_code ec;
        auto res = modify_guild_integration(ec);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Delete a guild integration
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> delete_guild_integration(std::error_code & ec);

    /// Delete a guild integration
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> delete_guild_integration()
    {
        std::error_code ec;
        auto res = delete_guild_integration(ec);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Get the guild integrations
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> sync_guild_integration(std::error_code & ec);

    /// Get the guild integrations
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> sync_guild_integration()
    {
        std::error_code ec;
        auto res = sync_guild_integration(ec);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Get the guild embed settings
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> get_guild_embed(std::error_code & ec);

    /// Get the guild embed settings
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> get_guild_embed()
    {
        std::error_code ec;
        auto res = get_guild_embed(ec);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Modify the guild embed settings
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> modify_guild_embed(std::error_code & ec);

    /// Modify the guild embed settings
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> modify_guild_embed()
    {
        std::error_code ec;
        auto res = modify_guild_embed(ec);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Leave the guild this object is associated with
    /**
     * @param ec Indicates what error occurred, if any
     * @returns std::future<rest::rest_reply>
     */
    AEGIS_DECL std::future<rest::rest_reply> leave(std::error_code & ec);

    /// Leave the guild this object is associated with
    /**
     * @throws aegis::exception Thrown on failure.
     * @returns std::future<rest::rest_reply>
     */
    std::future<rest::rest_reply> leave()
    {
        std::error_code ec;
        auto res = leave(ec);
        if (ec)
            throw exception(ec);
        return res;
    }

    /// Gets the Bot object
    /**
     * @see aegis
     * @throws aegis::exception Thrown on failure.
     * @returns Aegis main object
     */
    AEGIS_DECL core & get_bot() const noexcept;

    /// Obtain a pointer to a member by snowflake
    /**
     * @param member_id Snowflake of member to search for
     * @returns Pointer to member or nullptr
     */
    AEGIS_DECL member * find_member(snowflake member_id) const noexcept;

    /// Obtain a pointer to a channel by snowflake
    /**
     * @param channel_id Snowflake of channel to search for
     * @returns Pointer to channel or nullptr
     */
    AEGIS_DECL channel * find_channel(snowflake channel_id) const noexcept;

    std::unordered_map<snowflake, channel*> channels; /**< Map of snowflakes to channel objects */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    std::unordered_map<snowflake, member*> members; /**< Map of snowflakes to member objects */
    std::unordered_map<snowflake, gateway::objects::role> roles; /**< Map of snowflakes to role objects */
#endif

private:
    friend class core;
    friend class member;

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    AEGIS_DECL void add_member(member * _member) noexcept;

    AEGIS_DECL void remove_member(snowflake member_id) noexcept;

    AEGIS_DECL void load_presence(const json & obj) noexcept;

    AEGIS_DECL void load_role(const json & obj) noexcept;

    AEGIS_DECL void remove_role(snowflake role_id);
#endif

    AEGIS_DECL void load(const json & obj, shards::shard * _shard) noexcept;

    AEGIS_DECL shared_mutex & mtx() { return _m; }

    /// non-locking version for internal use
    AEGIS_DECL member * _find_member(snowflake member_id) const noexcept;
    
    /// non-locking version for internal use
    AEGIS_DECL channel * _find_channel(snowflake channel_id) const noexcept;

    AEGIS_DECL void remove_channel(snowflake channel_id) noexcept;

#if !defined(AEGIS_DISABLE_ALL_CACHE)
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
#endif
    core * _bot;
    asio::io_context & _io_context;
    mutable shared_mutex _m;
};

}
