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
#if !defined(AEGIS_DISABLE_ALL_CACHE)
#include "aegis/snowflake.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <queue>
#include <memory>
#include <set>
#include <shared_mutex>
#if defined(AEGIS_HAS_STD_OPTIONAL)
#include <optional>
#else
#include "aegis/optional.hpp"
namespace std
{
using std::experimental::optional;
}
#endif
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
class member
{
public:
    explicit member(snowflake id) : member_id(id) {}
    snowflake member_id = 0;

    std::string name; /**< Username of member */
    uint16_t discriminator = 0; /**< 4 digit discriminator (1-9999) */
    std::string avatar; /**< Hash of member avatar */
    bool is_bot = false; /**< true if member is a bot */
    bool mfa_enabled = false; /**< true if member has Two-factor authentication enabled */
    mutable shared_mutex _m;

    /// Member owned guild information
    struct guild_info
    {
        std::set<int64_t> roles;
        std::string nickname;
        //std::string joined_at;
        uint64_t joined_at = 0;
        bool deaf = false;
        bool mute = false;
    };

    /// The statuses a member is able to be
    enum member_status
    {
        Offline,
        Online,
        Idle,
        DoNotDisturb
    };

    member_status status = member_status::Offline; /**< Member _status */

    /// Get the nickname of this user
    /**
     * @param guild_id The snowflake for the guild to check if nickname is set
     * @returns A string of the nickname or empty if no nickname is set
     */
    AEGIS_DECL std::string get_name(snowflake guild_id) AEGIS_NOEXCEPT;

    /// Get the member owned guild information object
    /**
     * @param guild_id The snowflake for the guild
     * @returns Pointer to the member owned guild information object
     */
    AEGIS_DECL guild_info & get_guild_info(snowflake guild_id) AEGIS_NOEXCEPT;

    /// Get the full name (username#discriminator) of this user
    /**
     * @returns A string of the full username and discriminator
     */
    AEGIS_DECL std::string get_full_name() const AEGIS_NOEXCEPT;

    std::map<int64_t, guild_info> guilds; /**< Map of snowflakes to member owned guild information */

    /// Get the snowflake of this user
    /**
     * @returns A snowflake of the user
     */
    const snowflake get_id() const AEGIS_NOEXCEPT
    {
        return member_id;
    }
private:
    friend class core;
    friend class guild;

    /// requires the caller to handle locking
    AEGIS_DECL void load(guild * _guild, const json & obj, shard * _shard);

    /// requires the caller to handle locking
    AEGIS_DECL guild_info & join(snowflake guild_id);

    AEGIS_DECL shared_mutex & mtx() { return _m; }

    void leave(snowflake guild_id)
    {
        guilds.erase(guild_id);
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
