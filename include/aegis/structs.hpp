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

namespace aegiscpp
{


/**\todo Needs documentation
*/
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

/**\todo Needs documentation
*/
enum channel_type
{
    Text = 0,
    DirectMessage = 1,
    Voice = 2,
    GroupDirectMessage = 3,
    Category = 4
};

/**\todo Needs documentation
*/
enum shard_status
{
    Uninitialized = 0,
    Ready = 1,
    Connecting = 2,
    Online = 3,
    Reconnecting = 4,
    Shutdown = 5
};

/**\todo Needs documentation
*/
enum overwrite_type
{
    User,
    Role
};

/**\todo Needs documentation
*/
struct rest_reply
{
    explicit rest_reply() {}
    explicit rest_reply(bool p) { permissions = p; }
    explicit rest_reply(bool p, std::string msg) { permissions = p; reason = msg; }
    rest_reply(websocketpp::http::status_code::value reply_code, bool global, int32_t limit, int32_t remaining, int64_t reset, int32_t retry, std::string content)
        : reply_code(reply_code)
        , global(global)
        , limit(limit)
        , remaining(remaining)
        , reset(reset)
        , retry(retry)
        , content(content)
    { }
    websocketpp::http::status_code::value reply_code; /**< REST HTTP reply code */
    bool global = false; /**< Is global ratelimited */
    int32_t limit = 0; /**< Rate limit current endpoint call limit */
    int32_t remaining = 0; /**< Rate limit remaining time */
    int64_t reset = 0; /**< Rate limit reset time */
    int32_t retry = 0; /**< Rate limit retry time */
    std::string content; /**< REST call's reply body */
    bool permissions = true; /**< Whether the call had proper permissions */
    std::string reason;
};

}
