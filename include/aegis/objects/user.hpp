//
// user.hpp
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


#include "../config.hpp"
#include "../snowflake.hpp"
#include "../structs.hpp"
#include "attachment.hpp"
#include "embed.hpp"
#include "reaction.hpp"
#include <json.hpp>
#include <string>
#include <vector>



namespace aegiscpp
{

class member;
class channel;

/**\todo Needs documentation
*/
struct user
{
    snowflake user_id; /**<\todo Needs documentation */
    snowflake guild_id; /**<\todo Needs documentation */
    std::string username; /**<\todo Needs documentation */
    std::string discriminator; /**<\todo Needs documentation */
    std::string avatar; /**<\todo Needs documentation */
    bool isbot = false; /**<\todo Needs documentation */
    bool mfa_enabled = false; /**<\todo Needs documentation */
    bool verified = false; /**<\todo Needs documentation */
    std::string email; /**<\todo Needs documentation */
};

/**\todo Needs documentation
*/
inline void from_json(const nlohmann::json& j, user& m)
{
    m.user_id = j["id"];
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m.guild_id = j["guild_id"];
    if (j.count("username") && !j["username"].is_null())
        m.username = j["username"];
    if (j.count("discriminator") && !j["discriminator"].is_null())
        m.discriminator = j["discriminator"];
    if (j.count("avatar") && !j["avatar"].is_null())
        m.avatar = j["avatar"];
    if (j.count("bot") &&  !j["bot"].is_null())
        m.isbot = j["bot"];
    if (j.count("mfa_enabled") && !j["mfa_enabled"].is_null())
        m.mfa_enabled = j["mfa_enabled"];
    if (j.count("verified") && !j["verified"].is_null())
        m.verified = j["verified"];
}

/**\todo Needs documentation
*/
inline void to_json(nlohmann::json& j, const user& m)
{
    j["id"] = m.user_id;
    j["guild_id"] = m.guild_id;
    j["username"] = m.username;
    j["discriminator"] = m.discriminator;
    j["bot"] = m.isbot;
    j["mfa_enabled"] = m.mfa_enabled;
    j["verified"] = m.verified;
}

}

