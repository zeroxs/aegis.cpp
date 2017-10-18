//
// structs.hpp
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
#include <functional>
#include <websocketpp/http/constants.hpp>
#include "snowflake.hpp"
#include <json.hpp>

namespace aegis
{


enum message_type
{
    Default = 0,
    RecipientAdd = 1,
    RecipientRemove = 2,
    Call = 3,
    ChannelNameChange = 4,
    ChannelIconChange = 5,
    ChannelPinnedMessage = 6,
    GuildMemberJoin = 7
};

struct perm_overwrite
{
    enum OverwriteType
    {
        User,
        Role
    };

    snowflake id;
    //either "role" or "member"
    OverwriteType type;
    int64_t allow;
    int64_t deny;
    nlohmann::json make()
    {
        return { { "id", id }, { "type", type }, { "allow", allow }, { "deny", deny } };
    }
};

struct rest_reply
{
    websocketpp::http::status_code::value reply_code;
    bool global = false;
    int32_t limit = 0;
    int32_t remaining = 0;
    int64_t reset = 0;
    int32_t retry = 0;
    std::string content;
};

}
