//
// member.hpp
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
#include <string>
#include <optional>
#include <queue>
#include <memory>
#include <set>

namespace aegiscpp
{

class guild;
class shard;

class member : public std::enable_shared_from_this<member>
{
public:
    explicit member(snowflake id) : member_id(id) {}
    snowflake member_id = 0;


    std::string name; /**< Username of member */
    uint16_t discriminator = 0; /**< 4 digit discriminator (1-9999) */
    std::string avatar; /**< Hash of member avatar */
    bool is_bot = false; /**< true if member is a bot */
    bool mfa_enabled = false; /**< true if member has Two-factor authentication enabled */

    /**
    * Member owned guild information
    */
    struct guild_info
    {
        std::set<int64_t> roles;
        std::optional<std::string> nickname;
        //std::string joined_at;
        uint64_t joined_at = 0;
        bool deaf = false;
        bool mute = false;
    };

    /**
    * The statuses a member is able to be
    */
    enum member_status
    {
        Offline,
        Online,
        Idle,
        DoNotDisturb
    };

    member_status status = member_status::Offline; /**< Member status */

    /// Load a member object from the websocket
    /**
    * @param _guild Pointer to the guild object this member is originating from
    *
    * @param obj Const reference to the json object containing the member structure
    *
    * @param _shard Pointer to the shard this guild is managed by
    */
    AEGIS_DECL void load(guild * _guild, const json & obj, shard * _shard);

    /// Get the nickname of this user
    /**
    * @param guild_id The snowflake for the guild to check if nickname is set
    *
    * @returns A string of the nickname or empty if no nickname is set
    */
    std::optional<std::string> get_name(snowflake guild_id)
    {
        auto g = get_guild_info(guild_id);
        if (!g.has_value())
            return {};
        return g.value()->nickname;
    }

    /// Get the member owned guild information object
    /**
    * @param guild_id The snowflake for the guild
    *
    * @returns Pointer to the member owned guild information object
    */
    std::optional<guild_info*> get_guild_info(snowflake guild_id)
    {
        auto g = guilds.find(guild_id);
        if (g == guilds.end())
            return {};
        return &g->second;
    }

    /// Create a new member owned guild information object
    /**
    * @param guild_id The snowflake for the guild to create
    *
    * @returns Pointer to the member owned guild information object created or already existing
    */
    std::optional<guild_info*> join(snowflake guild_id)
    {
        auto g = get_guild_info(guild_id);
        if (g.has_value())
            return g;
        auto g2 = guilds.emplace(guild_id, guild_info());
        return &g2.first->second;
    }

    /// Get the full name (username#discriminator) of this user
    /**
    * @returns A string of the full username and discriminator
    */
    AEGIS_DECL std::string get_full_name() const noexcept;

    std::map<int64_t, guild_info> guilds; /**< Map of snowflakes to member owned guild information */
    //std::pair<message_snowflake, time_sent>
    //std::queue<std::pair<snowflake, int64_t>> msghistory;

    /// Get the nickname of this user
    /**
    * @param guild_id The snowflake for the guild to check if nickname is set
    *
    * @returns A string of the nickname or empty is no nickname is set
    */
    void leave(snowflake guild_id)
    {
        guilds.erase(guild_id);
    }

    /// Get the snowflake of this user
    /**
    * @returns A snowflake of the user
    */
    const snowflake get_id() const noexcept
    {
        return member_id;
    }
};

}

#if defined(AEGIS_HEADER_ONLY)
# include "aegis/member.cpp"
#endif // defined(AEGIS_HEADER_ONLY)
