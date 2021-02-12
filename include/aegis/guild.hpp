//
// guild.hpp
// *********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/utility.hpp"
#include "aegis/gateway/objects/role.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/rest/rest_reply.hpp"
#include "aegis/ratelimit/ratelimit.hpp"
#include "aegis/gateway/objects/permission_overwrite.hpp"
#include "aegis/gateway/objects/member.hpp"
#include "aegis/gateway/objects/ban.hpp"
#include "aegis/gateway/objects/bans.hpp"
#include "aegis/gateway/objects/invite.hpp"
#include "aegis/gateway/objects/invites.hpp"
#include <future>
#include <asio.hpp>
#include <shared_mutex>

namespace aegis
{

using json = nlohmann::json;

#pragma region rest params
struct modify_guild_t
{
    modify_guild_t & name(const std::string & param) { _name = param; return *this; }
    modify_guild_t & voice_region(const std::string & param) { _voice_region = param; return *this; }
    modify_guild_t & verification_level(int param) { _verification_level = param; return *this; }
    modify_guild_t & default_message_notifications(int param) { _default_message_notifications = param; return *this; }
    modify_guild_t & explicit_content_filter(int param) { _explicit_content_filter = param; return *this; }
    modify_guild_t & afk_channel_id(snowflake param) { _afk_channel_id = param; return *this; }
    modify_guild_t & afk_timeout(int param) { _afk_timeout = param; return *this; }
    modify_guild_t & icon(const std::string & param) { _icon = param; return *this; }
    modify_guild_t & owner_id(snowflake param) { _owner_id = param; return *this; }
    modify_guild_t & splash(const std::string & param) { _splash = param; return *this; }

    std::optional<std::string> _name;
    std::optional<std::string> _voice_region;
    std::optional<int> _verification_level;
    std::optional<int> _default_message_notifications;
    std::optional<int> _explicit_content_filter;
    std::optional<snowflake> _afk_channel_id;
    std::optional<int> _afk_timeout;
    std::optional<std::string> _icon;
    std::optional<snowflake> _owner_id;
    std::optional<std::string> _splash;
};

struct create_text_channel_t
{
    create_text_channel_t & name(const std::string & param) { _name = param; return *this; }
    create_text_channel_t & parent_id(int64_t param) { _parent_id = param; return *this; }
    create_text_channel_t & nsfw(bool param) { _nsfw = param; return *this; }
    create_text_channel_t & permission_overwrites(const std::vector<gateway::objects::permission_overwrite> & param)
    { _permission_overwrites = param; return *this; }
    std::string _name;
    int64_t _parent_id = 0;
    bool _nsfw = false;
    std::vector<gateway::objects::permission_overwrite> _permission_overwrites;
};

struct create_voice_channel_t
{
    create_voice_channel_t & name(const std::string param) { _name = param; return *this; }
    create_voice_channel_t & bitrate(int32_t param) { _bitrate = param; return *this; }
    create_voice_channel_t & user_limit(int32_t param) { _user_limit = param; return *this; }
    create_voice_channel_t & parent_id(int64_t param) { _parent_id = param; return *this; }
    create_voice_channel_t & permission_overwrites(const std::vector<gateway::objects::permission_overwrite> & param)
    { _permission_overwrites = param; return *this; }
    std::string _name;
    int32_t _bitrate = 0;
    int32_t _user_limit = 0;
    int64_t _parent_id = 0;
    std::vector<gateway::objects::permission_overwrite> _permission_overwrites;
};

struct create_category_channel_t
{
    create_category_channel_t & name(const std::string & param) { _name = param; return *this; }
    create_category_channel_t & parent_id(int64_t param) { _parent_id = param; return *this; }
    create_category_channel_t & permission_overwrites(const std::vector<gateway::objects::permission_overwrite> & param)
    { _permission_overwrites = param; return *this; }
    std::string _name;
    int64_t _parent_id = 0;
    std::vector<gateway::objects::permission_overwrite> _permission_overwrites;
};

struct modify_guild_member_t
{
    modify_guild_member_t & user_id(snowflake param) { _user_id = param; return *this; }
    modify_guild_member_t & nick(const std::string & param) { _nick = param; return *this; }
    modify_guild_member_t & mute(bool param) { _mute = param; return *this; }
    modify_guild_member_t & deaf(bool param) { _deaf = param; return *this; }
    modify_guild_member_t & roles(const std::vector<snowflake> & param) { _roles = param; return *this; }
    modify_guild_member_t & channel_id(snowflake param) { _channel_id = param; return *this; }
    snowflake _user_id;
    std::optional<std::string> _nick;
    std::optional<bool> _mute;
    std::optional<bool> _deaf;
    std::optional<std::vector<snowflake>> _roles;
    std::optional<snowflake> _channel_id;
};

struct create_guild_ban_t
{
    create_guild_ban_t & user_id(snowflake param) { _user_id = param; return *this; }
    create_guild_ban_t & delete_message_days(int8_t param) { _delete_message_days = param; return *this; }
    create_guild_ban_t & reason(const std::string & param) { _reason = param; return *this; }
    snowflake _user_id;
    int8_t _delete_message_days;
    std::string _reason;
};

struct create_guild_role_t
{
    create_guild_role_t & name(const std::string & param) { _name = param; return *this; }
    create_guild_role_t & perms(permission param) { _perms = param; return *this; }
    create_guild_role_t & color(int32_t param) { _color = param; return *this; }
    create_guild_role_t & hoist(bool param) { _hoist = param; return *this; }
    create_guild_role_t & mentionable(bool param) { _mentionable = param; return *this; }
    std::string _name;
    permission _perms;
    int32_t _color = 0;
    bool _hoist = false;
    bool _mentionable = false;
};

struct modify_guild_role_t
{
    modify_guild_role_t & role_id(snowflake param) { _role_id = param; return *this; }
    modify_guild_role_t & name(const std::string & param) { _name = param; return *this; }
    modify_guild_role_t & perms(permission param) { _perms = param; return *this; }
    modify_guild_role_t & color(int32_t param) { _color = param; return *this; }
    modify_guild_role_t & hoist(bool param) { _hoist = param; return *this; }
    modify_guild_role_t & mentionable(bool param) { _mentionable = param; return *this; }
    snowflake _role_id;
    std::string _name;
    permission _perms;
    int32_t _color = 0;
    bool _hoist = false;
    bool _mentionable = false;
};
#pragma endregion

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
     * @deprecated
     */
    AEGIS_DECL user * self() const;

    /// Get name of guild
    /**
     * @returns String of guild name
     */
    std::string get_name() const noexcept
    {
        std::shared_lock<std::shared_mutex> l(_m);
        std::string _name = name;
        return std::move(_name);
    }

    /// Get icon of guild
    /**
     * @returns String of guild icon
     */
    std::string get_icon() const noexcept
    {
        std::shared_lock<std::shared_mutex> l(_m);
        std::string _icon = icon;
        return std::move(_icon);
    }

    /// Get splash of guild
    /**
     * @returns String of guild splash
     */
    std::string get_splash() const noexcept
    {
        std::shared_lock<std::shared_mutex> l(_m);
        std::string _splash = splash;
        return std::move(_splash);
    }

    /// Get region of guild
    /**
     * @returns String of region guild is in
     */
    std::string get_region() const noexcept
    {
        std::shared_lock<std::shared_mutex> l(_m);
        std::string _region = region;
        return std::move(_region);
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
    AEGIS_DECL permission get_permissions(snowflake member_id, snowflake channel_id) const;

    /// Get guild permissions for member in channel
    /**
     * @param _member Pointer to member object
     * @param _channel Pointer to channel object
     * @returns Permission object of channel
     */
    AEGIS_DECL permission get_permissions(const user * _member, const channel * _channel) const;

    /// Get base guild permissions for member
    /**
     * @param _member Pointer to member object
     */
    int64_t base_permissions() const
    {
        std::shared_lock<std::shared_mutex> l(_m);
        return base_permissions(self());
    }

    /// Get base guild permissions for self
/**
 * @param _member Pointer to member object
 */
    int64_t base_permissions(const user * _member) const noexcept
    {
        std::shared_lock<std::shared_mutex> l(_m);
        return base_permissions(*_member);
    }

    /// Get base guild permissions for member
    /**
     * @param _member Reference to member object
     */
    AEGIS_DECL int64_t base_permissions(const user & _member) const noexcept;

    /// Calculate permission overrides for member in channel
    /**
     * @param _base_permissions Base guild permissions for member in guild
     * @param _member Reference to member object
     * @param _channel Reference to channel object
     * @returns true on successful request, false for no permissions
     */
    AEGIS_DECL int64_t compute_overwrites(const int64_t _base_permissions, const user & _member, const channel & _channel) const noexcept;

    /// Get role
    /**
     * @param r Snowflake of role
     * @returns Role object
     */
    AEGIS_DECL const gateway::objects::role get_role(const int64_t r) const;

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
    snowflake get_id() const noexcept
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
     * @returns async::task<gateway::objects::guild>
     */
    AEGIS_DECL async::task<gateway::objects::guild> get_guild();

    /// Modify guild information
    /**
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
     * @returns async::task<gateway::objects::guild>
     */
    AEGIS_DECL async::task<gateway::objects::guild> modify_guild(
        std::optional<std::string> name = {},
        std::optional<std::string> voice_region = {}, std::optional<int> verification_level = {},
        std::optional<int> default_message_notifications = {}, std::optional<int> explicit_content_filter = {},
        std::optional<snowflake> afk_channel_id = {}, std::optional<int> afk_timeout = {},
        std::optional<std::string> icon = {}, std::optional<snowflake> owner_id = {},
        std::optional<std::string> splash = {}
    );

    /// Modify guild information
    /**
     * @see aegis::modify_guild_t
     * @param obj Struct of the contents of the request
     * @returns async::task<gateway::objects::guild>
     */
    AEGIS_DECL async::task<gateway::objects::guild> modify_guild(modify_guild_t obj)
    {
        return modify_guild(obj._name, obj._voice_region, obj._verification_level, obj._default_message_notifications,
                            obj._explicit_content_filter, obj._afk_channel_id, obj._afk_timeout, obj._icon,
                            obj._owner_id, obj._splash);
    }

    /// Delete a guild
    /**
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> delete_guild();

    /// Create a text channel
    /**
     * @param name String of the name for the channel
     * @param parent_id The channel or category to place this channel below
     * @param nsfw Whether the channel will be labeled as not safe for work
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     * @returns async::task<gateway::objects::channel>
     */
    AEGIS_DECL async::task<gateway::objects::channel> create_text_channel(const std::string & name, int64_t parent_id = 0, bool nsfw = false,
                                            const std::vector<gateway::objects::permission_overwrite> & permission_overwrites = {});
    
    /// Create a text channel
    /**
     * @see aegis::create_text_channel_t
     * @param obj Struct of the contents of the request
     * @returns async::task<gateway::objects::channel>
     */
    AEGIS_DECL async::task<gateway::objects::channel> create_text_channel(create_text_channel_t obj)
    {
        return create_text_channel(obj._name, obj._parent_id, obj._nsfw, obj._permission_overwrites);
    }

    /// Create a voice channel
    /**
     * @param name String of the name for the channel
     * @param bitrate The bitrate count of the channel
     * @param user_limit The max amount of members that may join the channel
     * @param parent_id The channel or category to place this channel below
     * @param nsfw Whether the channel will be labeled as not safe for work
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     * @returns async::task<gateway::objects::channel>
     */
    AEGIS_DECL async::task<gateway::objects::channel> create_voice_channel(const std::string & name, int32_t bitrate = 0, int32_t user_limit = 0, int64_t parent_id = 0,
                                             const std::vector<gateway::objects::permission_overwrite> & permission_overwrites = {});

    /// Create a voice channel
    /**
     * @see aegis::create_voice_channel_t
     * @param obj Struct of the contents of the request
     * @returns async::task<gateway::objects::channel>
     */
    AEGIS_DECL async::task<gateway::objects::channel> create_voice_channel(create_voice_channel_t obj)
    {
        return create_voice_channel(obj._name, obj._bitrate, obj._user_limit, obj._parent_id, obj._permission_overwrites);
    }

    /// Create a category
    /**
     * @param name String of the name for the channel
     * @param parent_id The channel or category to place this channel below
     * @param permission_overwrites Array of permission overwrites to apply to the channel
     * @returns async::task<gateway::objects::channel>
     */
    AEGIS_DECL async::task<gateway::objects::channel> create_category_channel(const std::string & name, int64_t parent_id,
                                                const std::vector<gateway::objects::permission_overwrite> & permission_overwrites);

    /// Create a category
    /**
     * @see aegis::create_category_channel_t
     * @param obj Struct of the contents of the request
     * @returns async::task<gateway::objects::channel>
     */
    AEGIS_DECL async::task<gateway::objects::channel> create_category_channel(create_category_channel_t obj)
    {
        return create_category_channel(obj._name, obj._parent_id, obj._permission_overwrites);
    }

    /// Modify positions of channels
    /**
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> modify_channel_positions();

    /// Modify a member
    /// All fields are optional
    /**
     * @param user_id The snowflake of the user to edit
     * @param nick String of nickname to change to
     * @param mute Whether or not to voice mute the member
     * @param deaf Whether or not to voice deafen the member
     * @param roles Array of roles to apply to the member
     * @param channel_id Snowflake of the channel to move user to
     * @returns async::task<gateway::objects::member>
     */
    AEGIS_DECL async::task<gateway::objects::member> modify_guild_member(snowflake user_id, std::optional<std::string> nick, std::optional<bool> mute,
                                            std::optional<bool> deaf, std::optional<std::vector<snowflake>> roles,
                                            std::optional<snowflake> channel_id);

    /// Modify a member
    /// All fields are optional
    /**
     * @see aegis::modify_guild_member_t
     * @param obj Struct of the contents of the request
     * @returns async::task<gateway::objects::member>
     */
    AEGIS_DECL async::task<gateway::objects::member> modify_guild_member(modify_guild_member_t obj)
    {
        return modify_guild_member(obj._user_id, obj._nick, obj._mute, obj._deaf, obj._roles, obj._channel_id);
    }

    /// Modify own nickname
    /**
     * @param newname String of the new nickname to apply
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> modify_my_nick(const std::string & newname);

    /// Add a role to guild member
    /**
     * @param user_id The snowflake of the user to add new role
     * @param role_id The snowflake of the role to add to member
     * @returns async::task<gateway::objects::role>
     */
    AEGIS_DECL async::task<gateway::objects::role> add_guild_member_role(snowflake user_id, snowflake role_id);

    /// Remove a role from guild member
    /**
     * @param user_id The snowflake of the user to remove role
     * @param role_id The snowflake of the role to remove from member
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> remove_guild_member_role(snowflake user_id, snowflake role_id);

    /// Remove guild member (kick)
    /**
     * @param user_id The snowflake of the member to kick
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> remove_guild_member(snowflake user_id);

    /// Create a new guild ban
    /**
     * @param user_id The snowflake of the member to ban
     * @param delete_message_days How many days to delete member message history (0-7)
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> create_guild_ban(snowflake user_id, int8_t delete_message_days = 0, const std::string & reason = "");

    /// Create a new guild ban
    /**
     * @see aegis::create_guild_ban_t
     * @param obj Struct of the contents of the request
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> create_guild_ban(create_guild_ban_t obj)
    {
        return create_guild_ban(obj._user_id, obj._delete_message_days, obj._reason);
    }

    /// Remove a guild ban
    /**
     * @param user_id The snowflake of the member to unban
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> remove_guild_ban(snowflake user_id);

    /// Get guild bans
    /**
     * @returns async::task<gateway::objects::bans>
     */
    AEGIS_DECL async::task<gateway::objects::bans> get_guild_bans();

    /// Get guild ban
    /**
     * @param user_id The snowflake of the user to retrieve the ban object for
     * @returns async::task<gateway::objects::ban>
    */
    AEGIS_DECL async::task<gateway::objects::ban> get_guild_ban(snowflake user_id);

    /// Create a guild role
    /**
     * @see aegis::permission
     * @param name The name of the role to create
     * @param _perms The permissions to set
     * @param color 32bit integer of the color
     * @param hoist Whether the role should be separated from other roles
     * @param mentionable Whether the role can be specifically mentioned
     * @returns async::task<gateway::objects::role>
     */
    AEGIS_DECL async::task<gateway::objects::role> create_guild_role(const std::string & name, permission _perms, int32_t color, bool hoist, bool mentionable);

    /// Create a guild role
    /**
     * @see aegis::permission
     * @see aegis::create_guild_role_t
     * @param obj Struct of the contents of the request
     * @returns async::task<gateway::objects::role>
     */
    AEGIS_DECL async::task<gateway::objects::role> create_guild_role(create_guild_role_t obj)
    {
        return create_guild_role(obj._name, obj._perms, obj._color, obj._hoist, obj._mentionable);
    }

    /// Modify the guild role positions
    /**
     * @param role_id Snowflake of role to change position
     * @param position Index of position to change role to
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> modify_guild_role_positions(snowflake role_id, int16_t position);

    /// Modify a guild role
    /**
     * @see aegis::permission
     * @param id The snowflake of the role to modify
     * @param name The name to set the role to
     * @param _perms The permissions to set
     * @param color 32bit integer of the color
     * @param hoist Whether the role should be separated from other roles
     * @param mentionable Whether the role can be specifically mentioned
     * @returns async::task<gateway::objects::role>
     */
    AEGIS_DECL async::task<gateway::objects::role> modify_guild_role(snowflake role_id, const std::string & name, permission _perms, int32_t color,
                                          bool hoist, bool mentionable);

    /// Modify a guild role
    /**
     * @see aegis::permission
     * @see aegis::modify_guild_role_t
     * @param obj Struct of the contents of the request
     * @returns async::task<gateway::objects::role>
     */
    AEGIS_DECL async::task<gateway::objects::role> modify_guild_role(modify_guild_role_t obj)
    {
        return modify_guild_role(obj._role_id, obj._name, obj._perms, obj._color, obj._hoist, obj._mentionable);
    }

    /// Delete a guild role
    /**
     * @param role_id The snowflake of the role to delete
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> delete_guild_role(snowflake role_id);

    /// Get a count of members that would be pruned
    /**
     * @param days The days of inactivity to prune the member
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> get_guild_prune_count(int16_t days);

    /// Perform a guild prune
    /**
     * @param days The days of inactivity to prune the member
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> begin_guild_prune(int16_t days);

    /// Get active guild invite
    /**
     * @param invite_code The invite code of the invite to retrieve
     * @returns async::task<gateway::objects::invite>
     */
    AEGIS_DECL async::task<gateway::objects::invite> get_guild_invite(std::string invite_code);

    /// Get active guild invites
    /**
     * @returns async::task<gateway::objects::invites>
     */
    AEGIS_DECL async::task<gateway::objects::invites> get_guild_invites();

    /// Delete active guild invite
    /**
    * @param invite_code The invite code of the invite to delete
    * @returns aegis::future<rest::rest_reply>
    */
    AEGIS_DECL async::task<rest::rest_reply> delete_guild_invite(std::string invite_code);

    /// Get guild integrations
    /**
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> get_guild_integrations();

    /// Create a new guild integration
    /**
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> create_guild_integration();

    /// Modify a guild integration
    /**
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> modify_guild_integration();

    /// Delete a guild integration
    /**
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> delete_guild_integration();

    /// Get the guild integrations
    /**
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> sync_guild_integration();

    /// Get the guild embed settings
    /**
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> get_guild_embed();

    /// Modify the guild embed settings
    /**
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> modify_guild_embed();

    /// Leave the guild this object is associated with
    /**
     * @returns async::task<rest::rest_reply>
     */
    AEGIS_DECL async::task<rest::rest_reply> leave();

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
    AEGIS_DECL user * find_member(snowflake member_id) const noexcept;

    /// Obtain a pointer to a channel by snowflake
    /**
     * @param channel_id Snowflake of channel to search for
     * @returns Pointer to channel or nullptr
     */
    AEGIS_DECL channel * find_channel(snowflake channel_id) const noexcept;

    /// Obtain a pointer to a channel by name
    /**
     * @param channel_name String of channel to search for
     * @returns Pointer to channel or nullptr
     */
    AEGIS_DECL channel * find_channel(std::string channel_name) const noexcept;

    /// Obtain a role by snowflake
    /**
     * @param role_name String of role to search for
     * @returns Pointer to gateway::objects::role or nullptr
     */
    AEGIS_DECL std::optional<gateway::objects::role> find_role(snowflake role_id) const noexcept;

    /// Obtain a role by name
    /**
     * @param role_name String of role to search for
     * @returns Pointer to gateway::objects::role or nullptr
     */
    AEGIS_DECL std::optional<gateway::objects::role> find_role(std::string role_name) const noexcept;

    /// Obtain map of channels
    /**
     * @returns unordered_map<snowflake, channel*> COPY of channels
     */
    std::unordered_map<snowflake, channel*> get_channels() const noexcept
    {
        std::shared_lock<std::shared_mutex> l(_m);
        std::unordered_map<snowflake, channel*> _list = channels;
        return std::move(_list);
    }

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    /// Obtain map of members
    /**
     * @returns unordered_map<snowflake, user*> COPY of members
     */
    std::unordered_map<snowflake, user*> get_members() const noexcept
    {
        std::shared_lock<std::shared_mutex> l(_m);
        std::unordered_map<snowflake, user*> _list = members;
        return std::move(_list);
    }

    /// Obtain map of roles
    /**
     * @returns unordered_map<snowflake, gateway::objects::role> COPY of roles
     */
    std::unordered_map<snowflake, gateway::objects::role> get_roles() const noexcept
    {
        std::shared_lock<std::shared_mutex> l(_m);
        std::unordered_map<snowflake, gateway::objects::role> _list = roles;
        return std::move(_list);
    }

    /// Obtain map of voice states
    /**
     * @returns std::unordered_map<snowflake, gateway::objects::voice_state> COPY of voice_states
     */
    std::unordered_map<snowflake, gateway::objects::voice_state> get_voicestates() const noexcept
    {
        std::shared_lock<std::shared_mutex> l(_m);
        std::unordered_map<snowflake, gateway::objects::voice_state> _list = voice_states;
        return std::move(_list);
    }

    /// Obtain map of emojis
    /**
    * @returns std::unordered_map<snowflake, gateway::objects::emoji> COPY of emojis
    */
    std::unordered_map<snowflake, gateway::objects::emoji> get_emojis() const noexcept
    {
        std::shared_lock<std::shared_mutex> l(_m);
        std::unordered_map<snowflake, gateway::objects::emoji> _list = emojis;
        return std::move(_list);
    }

    /// Obtain map of members - caller must lock guild._m to ensure no race conditions
    /**
     * @returns unordered_map<snowflake, user*> of members
     */
    const std::unordered_map<snowflake, user*> & get_members_nocopy() const noexcept
    {
        return members;
    }

    /// Obtain map of roles - caller must lock guild._m to ensure no race conditions
    /**
     * @returns unordered_map<snowflake, gateway::objects::role> of roles
     */
    const std::unordered_map<snowflake, gateway::objects::role> & get_roles_nocopy() const noexcept
    {
        return roles;
    }

    /// Obtain map of emojis - caller must lock guild._m to ensure no race conditions
    /**
    * @returns unordered_map<snowflake, gateway::objects::emoji> of emojis
    */
    const std::unordered_map<snowflake, gateway::objects::emoji>& get_emojis_nocopy() const noexcept
    {
        return emojis;
    }
#endif

    /// Obtain map of channels - caller must lock guild._m to ensure no race conditions
    /**
     * @returns unordered_map<snowflake, channel*> of channels
     */
    const std::unordered_map<snowflake, channel*> & get_channels_nocopy() const noexcept
    {
        return channels;
    }

    std::shared_mutex & mtx()
    {
        return _m;
    }

private:
    friend class core;
    friend class user;

    std::unordered_map<snowflake, channel*> channels; /**< Map of snowflakes to channel objects */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    std::unordered_map<snowflake, user*> members; /**< Map of snowflakes to member objects */
    std::unordered_map<snowflake, gateway::objects::role> roles; /**< Map of snowflakes to role objects */
    std::unordered_map<snowflake, gateway::objects::emoji> emojis; /**< Map of snowflakes to emoji objects */
    std::unordered_map<snowflake, gateway::objects::voice_state> voice_states; /**< Map of user snowflakes to voice_state objects */
#endif

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    AEGIS_DECL void _add_member(user * _member) noexcept;

    AEGIS_DECL void _add_member_nolock(user * _member) noexcept;

    AEGIS_DECL void _remove_member(snowflake member_id) noexcept;

    AEGIS_DECL void _load_presence(const json & obj) noexcept;

    AEGIS_DECL void _load_emoji(const json & obj) noexcept;

    AEGIS_DECL void _load_role(const json & obj) noexcept;

    AEGIS_DECL void _remove_role(snowflake role_id) noexcept;

    AEGIS_DECL void _load_voicestate(const json & obj) noexcept;
#endif

    AEGIS_DECL void _load(const json & obj, shards::shard * _shard);

    /// non-locking version for internal use
    AEGIS_DECL user * _find_member(snowflake member_id) const noexcept;
    
    /// non-locking version for internal use
    AEGIS_DECL channel * _find_channel(snowflake channel_id) const noexcept;

    AEGIS_DECL void _remove_channel(snowflake channel_id) noexcept;

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
    mutable std::shared_mutex _m;
};

}
