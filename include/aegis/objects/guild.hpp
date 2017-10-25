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

struct guild_gw
{
    //TODO:
    snowflake guild_id;
    std::string name;
    std::string icon;
    std::string splash;
    snowflake owner_id;
    std::string region;
    snowflake afk_channel_id;
    int32_t afk_timeout = 0;
    bool embed_enabled = false;
    snowflake embed_channel_id;
    int8_t verification_level = 0;
    int8_t default_message_notifications = 0;
    int8_t explicit_content_filter = 0;
    std::vector<role> roles;
    std::vector<emoji> emojis;
    std::vector<std::string> features;
    int8_t mfa_level = 0;
    snowflake application_id;//?
    bool widget_enabled = false;
    snowflake widget_channel_id;
    //createonly
    std::string joined_at;
    bool large = false;
    bool unavailable = false;
    int32_t member_count = 0;
    //std::vector<voice_state> voice_states;
    std::vector<guild_member> members;
    std::vector<channel_gw> channels;
    //std::vector<presence> presences;
};


void from_json(const nlohmann::json& j, guild_gw& m)
{
//TODO:
}
void to_json(nlohmann::json& j, const guild_gw& m)
{

}

}

