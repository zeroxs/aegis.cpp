//
// member.hpp
// **********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/utility.hpp"
#if !defined(AEGIS_DISABLE_ALL_CACHE)
#include "aegis/snowflake.hpp"
#include "aegis/objects/presence.hpp"
#include "aegis/fwd.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <queue>
#include <memory>
#include <set>
#include <shared_mutex>

namespace aegis
{

#if (AEGIS_HAS_STD_SHARED_MUTEX == 1)
using shared_mutex = std::shared_mutex;
#else
using shared_mutex = std::shared_timed_mutex;
#endif

using json = nlohmann::json;

/// Stores user-specific and guild-specific attributes of users
///\todo Maybe rename this to the more appropriate `user`?
class member
{
public:
    using presence = aegis::gateway::objects::presence;
  
    explicit member(snowflake id) : _member_id(id) {}

    /// Member owned guild information
    struct guild_info
    {
        snowflake id;/**< Snowflake of the guild for this data */
        std::vector<snowflake> roles;
        std::string nickname;/**< Nickname of the user in this guild */
        uint64_t joined_at = 0;/**< Unix timestamp of when member joined this guild */
        bool deaf = false;/**< Whether member is deafened in a voice channel */
        bool mute = false;/**< Whether member is muted in a voice channel */
    };

    /// Get the nickname of this user
    /**
     * @param guild_id The snowflake for the guild to check if nickname is set
     * @returns string of the nickname or empty if no nickname is set
     */
    AEGIS_DECL const std::string & get_name(snowflake guild_id) noexcept;

    /// Get the nickname of this user
    /**
    * @returns string of the username
    */
    const std::string & get_username() const noexcept
    {
        return _name;
    }

    /// Get the discriminator of this user
    /**
    * @returns string of the discriminator
    */
    uint16_t get_discriminator() const noexcept
    {
        return _discriminator;
    }

    /// Get the discriminator of this user
    /**
    * @returns string of the discriminator
    */
    const std::string & get_avatar() const noexcept
    {
        return _avatar;
    }

    /// Check whether user is a bot
    /**
    * @returns bool of bot status
    */
    bool is_bot() const noexcept
    {
        return _is_bot;
    }

    /// Get the status of multi factor authentication
    /**
    * @returns bool of mfa status
    */
    bool is_mfa_enabled() const noexcept
    {
        return _mfa_enabled;
    }

    /// Builds a mention for this user
    /**
    * @returns string of member mention
    */
    AEGIS_DECL std::string get_mention() const noexcept;
    /// Get the member owned guild information object
    /**
     * @param guild_id The snowflake for the guild
     * @returns Pointer to the member owned guild information object
     */
    AEGIS_DECL guild_info & get_guild_info(snowflake guild_id) noexcept;

    /// Get the full name (username#discriminator) of this user
    /**
     * @returns string of the full username and discriminator
     */
    AEGIS_DECL std::string get_full_name() const noexcept;

    /// Get the snowflake of this user
    /**
     * @returns snowflake of the user
     */
    snowflake get_id() const noexcept
    {
        return _member_id;
    }

    /// Get the DM channel associated with this user
    /// DM channels are obtained when a DM is sent
    /**
     * @returns snowflake of DM channel
     */
    snowflake get_dm_id() const noexcept
    {
        return _dm_id;
    }

    /// Set the DM channel id for the user
    /**
     * @param _id Snowflake of DM channel for this user
     */
    void set_dm_id(snowflake _id) noexcept
    {
        _dm_id = _id;
    }

    /// 
    /**
     * @returns shared_mutex The mutex for the user
     */
    shared_mutex & mtx() noexcept
    {
        return _m;
    }

private:
    friend class core;
    friend class guild;
    friend class gateway::objects::message;

    AEGIS_DECL void load_data(gateway::objects::user mbr);

    snowflake _member_id = 0;
    snowflake _dm_id = 0;
    presence::user_status _status = presence::user_status::Offline; /**< Member _status */
    std::string _name; /**< Username of member */
    uint16_t _discriminator = 0; /**< 4 digit discriminator (1-9999) */
    std::string _avatar; /**< Hash of member avatar */
    bool _is_bot = false; /**< true if member is a bot */
    bool _mfa_enabled = false; /**< true if member has Two-factor authentication enabled */
    std::vector<guild_info> guilds; /**< Vector of snowflakes to member owned guild information */
    mutable shared_mutex _m;

    /// requires the caller to handle locking
    AEGIS_DECL void load(guild * _guild, const json & obj, shards::shard * _shard);

    /// requires the caller to handle locking
    AEGIS_DECL guild_info & join(snowflake guild_id);

    /// remove this member from the specified guild
    void leave(snowflake guild_id)
    {
        std::unique_lock<shared_mutex> l(mtx());
        guilds.erase(std::find_if(std::begin(guilds), std::end(guilds), [&guild_id](const guild_info & gi)
        {
            if (gi.id == guild_id)
                return true;
            return false;
        }));
    }
};

}

#else

namespace aegis
{

class member
{
public:
    enum member_status
    {
        Offline,
        Online,
        Idle,
        DoNotDisturb
    };
};

}
#endif
