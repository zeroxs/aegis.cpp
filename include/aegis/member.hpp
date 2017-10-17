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


#include <string>
#include <optional>
#include <queue>

namespace aegis
{

class guild;

//this method of splitting the members causes some data duplication
//such as snowflake tracking, but keeps the objects separate

// class for Aegis to track
// contains info that is universal across Discord and
// tracks guilds they are in
class minimember
{
public:
    explicit minimember(snowflake id) : m_id(id) {}
    snowflake m_id = 0;
    struct guild_info
    {
        std::vector<int64_t> roles;
        std::string nickname;
        guild * _guild;
    };

    enum member_status
    {
        OFFLINE,
        ONLINE,
        IDLE,
        STREAM,
        DND
    };

    std::map<int64_t, guild_info> m_guilds;
    member_status m_status = member_status::OFFLINE;
};

// class for guild to track
// contains guild-specific data
class member
{
public:
    explicit member(snowflake id) : m_id(id) {}
    snowflake m_id = 0;

    std::vector<uint64_t> roles;
    std::map<uint64_t, perm_overwrite> overrides;//channel overrides

    //std::pair<message_snowflake, time_sent>
    std::queue<std::pair<int64_t, int64_t>> msghistory;

    std::string m_name;
    std::string m_nickname;
    uint16_t m_discriminator = 0;
    std::string m_avatar;
    bool m_isbot = false;
    bool m_deaf = false;
    bool m_mute = false;
    std::string m_joined_at;

    std::optional<std::string> getName()
    {
        if (m_nickname.length() > 0)
            return m_nickname;
        return {};
    }

    std::string getFullName()
    {
        return fmt::format("{}#{}", m_name, m_discriminator);
    }

};

}
