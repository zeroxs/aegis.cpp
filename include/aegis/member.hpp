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

class member
{
public:
    explicit member(snowflake id) : m_id(id) {}
    snowflake m_id = 0;

    //std::pair<message_snowflake, time_sent>
    std::queue<std::pair<int64_t, int64_t>> m_msghistory;

    std::string m_name;
    uint16_t m_discriminator = 0;
    std::string m_avatar;
    bool m_isbot = false;
    bool m_mfa_enabled = false;

    struct guild_info
    {
        std::vector<snowflake> roles;
        std::string nickname;
        snowflake _guild;
        std::string m_joined_at;
        bool m_deaf;
        bool m_mute;
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

    std::optional<std::string> getName(snowflake guild_id)
    {
        if (m_guilds.count(guild_id))
        {
            if (m_guilds[guild_id].nickname.length() > 0)
                return m_guilds[guild_id].nickname;
        }
        return {};
    }

    std::string getFullName()
    {
        return fmt::format("{}#{}", m_name, m_discriminator);
    }

};

}
