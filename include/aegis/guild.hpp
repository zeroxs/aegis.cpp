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

#include "config.hpp"
#include "channel.hpp"
#include "member.hpp"
#include "state_c.hpp"


namespace aegiscpp
{

using rest_limits::bucket_factory;
using json = nlohmann::json;

class channel;
class member;

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
    explicit guild(int16_t shard_id, bot_state * state, snowflake id, bucket_factory & ratelimit)
        : shard_id(shard_id)
        , guild_id(id)
        , ratelimit(ratelimit)
        , log(spdlog::get("aegis"))
        , state(state)
    {
        unavailable = false;
    }

    ~guild();

    guild(const guild &) = delete;
    guild(guild &&) = delete;
    guild & operator=(const guild &) = delete;

    int16_t shard_id;
    snowflake guild_id;
    bucket_factory & ratelimit;
    std::shared_ptr<spdlog::logger> log;
    bot_state * state;
    int role_offset = 1;

    /// Get pointer to own member object
    /**
    * @returns Pointer to own member object
    */
    member * self() const
    {
        auto self_id = state->user.id;
        auto slf = get_member(self_id);
        if (slf == nullptr)
            throw std::runtime_error("guild::self() is nullptr");
        return slf;
    }

    /// Get name of guild
    /**
    * @returns String of guild name
    */
    std::string get_name() const noexcept
    {
        return name;
    }

    /// Check if member has role
    /**
    * @param member_id Snowflake of member
    *
    * @param role_id Snowflake of role
    *
    * @returns true if member has role
    */
    bool member_has_role(snowflake member_id, snowflake role_id);

    /// Add new member to guild
    /**
    * @param _member shared_ptr of member object to add to guild
    */
    void add_member(std::shared_ptr<member> _member) noexcept;

    /// Remove member from guild
    /**
    * @param member_id Snowflake of member to remove from guild
    */
    void remove_member(snowflake member_id) noexcept;

    /// Create and return new channel internally
    /**
    * @param id Snowflake of channel to add
    *
    * @param _shard Pointer to shard guild belongs to
    *
    * @returns Pointer to channel object created
    */
    channel * get_channel_create(snowflake id, shard * _shard) noexcept;

    /// Load guild from json object
    /**
    * @param obj Const json object containing guild structure
    *
    * @param _shard Pointer to shard guild belongs to
    */
    void load(const json & obj, shard * _shard) noexcept;

    /// Load member presence data
    /**
    * @param obj Const json object containing array of presence data
    */
    void load_presence(const json & obj) noexcept;

    /// Load role
    /**
    * @param obj Const json object containing array of guild roles
    */
    void load_role(const json & obj) noexcept;

    /// Get channel internally
    /**
    * @param id Snowflake of channel
    *
    * @returns Pointer to channel object created
    */
    channel * get_channel(snowflake id) const noexcept;

    /// Get member internally
    /**
    * @param member_id Snowflake of member
    *
    * @returns Pointer to member object created
    */
    member * get_member(snowflake member_id) const noexcept;

    /// Get count of members in guild (potentially inaccurate)
    /**
    * @returns Count of members in guild
    */
    int32_t get_member_count() const noexcept;

    /// Get guild permissions for member in channel
    /**
    * @param member_id Snowflake of member
    *
    * @param channel_id Snowflake of channel
    *
    * @returns Permission object of channel
    */
    permission get_permissions(snowflake member_id, snowflake channel_id) noexcept;

    /// Get guild permissions for member in channel
    /**
    * @param _member Reference to member object
    *
    * @param _channel Reference to channel object
    *
    * @returns Permission object of channel
    */
    permission get_permissions(member & _member, channel & _channel) noexcept;

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
    int64_t base_permissions(member & _member) const noexcept;

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
    int64_t compute_overwrites(int64_t _base_permissions, member & _member, channel & _channel) const noexcept;

    /// Get role
    /**
    * @param r Snowflake of role
    *
    * @returns Reference to role object
    */
    role & get_role(int64_t r) const;
    
    /// Get role
    /**
    * @param r Internal memory saving id of role
    *
    * @returns Reference to role object
    */
    role & get_role(int16_t r) const;

    /// Remove role from guild
    /**
    * @param role_id Snowflake of role to remove
    */
    void remove_role(snowflake role_id);

    /// Get owner of guild
    /**
    * @returns Snowflake of owner
    */
    snowflake get_owner() const noexcept;


    /// Create a new guild
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool create_guild(std::function<void(rest_reply)> callback = nullptr);

    /// Get guild information
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool get_guild(std::function<void(rest_reply)> callback = nullptr);

    bool modify_guild(std::optional<std::string> name, std::optional<std::string> voice_region, std::optional<int> verification_level,
                      std::optional<int> default_message_notifications, std::optional<snowflake> afk_channel_id, std::optional<int> afk_timeout,
                      std::optional<std::string> icon, std::optional<snowflake> owner_id, std::optional<std::string> splash, std::function<void(rest_reply)> callback = nullptr);

    /// Delete a guild
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool delete_guild(std::function<void(rest_reply)> callback = nullptr);

    /// Create a text channel
    /**
    * @param name String of the name for the channel
    *
    * @param parent_id The channel or category to place this channel below
    *
    * @param nsfw Whether the channel will be labelled as not safe for work
    *
    * @param permission_overwrites Array of permission overwrites to apply to the channel
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool create_text_channel(std::string name, int64_t parent_id, bool nsfw, std::vector<permission_overwrite> permission_overwrites, std::function<void(rest_reply)> callback = nullptr);

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
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool create_voice_channel(std::string name, int32_t bitrate, int32_t user_limit, int64_t parent_id, bool nsfw, std::vector<permission_overwrite> permission_overwrites, std::function<void(rest_reply)> callback = nullptr);

    /// Create a category
    /**
    * @param name String of the name for the channel
    *
    * @param parent_id The channel or category to place this channel below
    *
    * @param permission_overwrites Array of permission overwrites to apply to the channel
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool create_category_channel(std::string name, int64_t parent_id, std::vector<permission_overwrite> permission_overwrites, std::function<void(rest_reply)> callback = nullptr);

    /// Modify positions of channels
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool modify_channel_positions(std::function<void(rest_reply)> callback);

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
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool modify_guild_member(snowflake user_id, std::optional<std::string> nick, std::optional<bool> mute,
                             std::optional<bool> deaf, std::optional<std::vector<snowflake>> roles, std::optional<snowflake> channel_id, std::function<void(rest_reply)> callback = nullptr);

    /// Modify own nickname
    /**
    * @param newname String of the new nickname to apply
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool modify_my_nick(std::string newname, std::function<void(rest_reply)> callback = nullptr);

    /// Add a role to guild member
    /**
    * @param user_id The snowflake of the user to add new role
    *
    * @param role_id The snowflake of the role to add to member
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool add_guild_member_role(snowflake user_id, snowflake role_id, std::function<void(rest_reply)> callback = nullptr);

    /// Remove a role from guild member
    /**
    * @param user_id The snowflake of the user to remove role
    *
    * @param role_id The snowflake of the role to remove from member
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool remove_guild_member_role(snowflake user_id, snowflake role_id, std::function<void(rest_reply)> callback = nullptr);

    /// Remove guild member (kick)
    /**
    * @param user_id The snowflake of the member to kick
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool remove_guild_member(snowflake user_id, std::function<void(rest_reply)> callback = nullptr);

    /// Create a new guild ban
    /**
    * @param user_id The snowflake of the member to ban
    *
    * @param delete_message_days How many days to delete member message history (0-7)
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool create_guild_ban(snowflake user_id, int8_t delete_message_days, std::function<void(rest_reply)> callback = nullptr);

    /// Remove a guild ban
    /**
    * @param user_id The snowflake of the member to unban
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool remove_guild_ban(snowflake user_id, std::function<void(rest_reply)> callback = nullptr);

    /// Create a guild role
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool create_guild_role(std::function<void(rest_reply)> callback = nullptr);

    /// Modify the guild role positions
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool modify_guild_role_positions(std::function<void(rest_reply)> callback = nullptr);

    /// Modify a guild role
    /**
    * @param role_id The snowflake of the role to edit
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool modify_guild_role(snowflake role_id, std::function<void(rest_reply)> callback = nullptr);

    /// Delete a guild role
    /**
    * @param role_id The snowflake of the role to delete
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool delete_guild_role(snowflake role_id, std::function<void(rest_reply)> callback = nullptr);

    /// Get a count of members that would be pruned
    /**
    * @param days The days of inactivity to prune the member
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool get_guild_prune_count(int16_t days, std::function<void(rest_reply)> callback = nullptr);

    /// Perform a guild prune
    /**
    * @param days The days of inactivity to prune the member
    *
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool begin_guild_prune(int16_t days, std::function<void(rest_reply)> callback = nullptr);

    /// Get active guild invites
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool get_guild_invites(std::function<void(rest_reply)> callback = nullptr);

    /// Get guild integrations
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool get_guild_integrations(std::function<void(rest_reply)> callback = nullptr);

    /// Create a new guild integration
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool create_guild_integration(std::function<void(rest_reply)> callback = nullptr);

    /// Modify a guild integration
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool modify_guild_integration(std::function<void(rest_reply)> callback = nullptr);

    /// Delete a guild integration
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool delete_guild_integration(std::function<void(rest_reply)> callback = nullptr);

    /// Get the guild integrations
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool sync_guild_integration(std::function<void(rest_reply)> callback = nullptr);

    /// Get the guild embed settings
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool get_guild_embed(std::function<void(rest_reply)> callback = nullptr);

    /// Nodify the guild embed settings
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool modify_guild_embed(std::function<void(rest_reply)> callback = nullptr);
    
    /// Leave the guild this object is associated with
    /**
    * @param callback A callback to execute after REST execution
    *
    * @returns true on successful request, false for no permissions
    */
    bool leave(std::function<void(rest_reply)> callback = nullptr);

    std::unordered_map<int64_t, std::shared_ptr<channel>> channels; /**< Map of snowflakes to channel objects */
    std::unordered_map<int64_t, std::shared_ptr<member>> members; /**< Map of snowflakes to member objects */
    std::unordered_map<int64_t, std::unique_ptr<role>> roles; /**< Map of snowflakes to role objects */
    std::unordered_map<int64_t, int16_t> role_snowflakes; /**< Map of snowflakes to the memory conserving internal role objects */

    /// Obtain the snowflake mapped to the internal role list
    /**
    * @param _role int16 role internal value
    *
    * @returns Snowflake of the mapped Role or 0
    */
    snowflake get_role_snowflake(int16_t _role) const noexcept
    {
        for (auto iter = role_snowflakes.begin(); iter != role_snowflakes.end(); ++iter)
            if (iter->second == _role)
                return iter->first;
        return { 0 };
    }

private:
    friend class aegis;
    friend class channel;
    friend class member;

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
};

}
