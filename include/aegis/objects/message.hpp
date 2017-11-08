//
// message.hpp
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
struct message
{
    snowflake message_id; /**<\todo Needs documentation */
    std::string content; /**<\todo Needs documentation */
    std::string timestamp; /**<\todo Needs documentation */
    std::string edited_timestamp; /**<\todo Needs documentation */
    bool tts = false; /**<\todo Needs documentation */
    bool mention_everyone = false; /**<\todo Needs documentation */
    std::vector<snowflake> mentions; /**<\todo Needs documentation */
    std::vector<snowflake> mention_roles; /**<\todo Needs documentation */
    std::vector<attachment> attachments; /**<\todo Needs documentation */
    std::vector<embed> embeds; /**<\todo Needs documentation */
    bool pinned = false; /**<\todo Needs documentation */
    std::vector<reaction> reactions; /**<\todo Needs documentation */
    snowflake nonce; /**<\todo Needs documentation */
    std::string webhook_id; /**<\todo Needs documentation */
    message_type type = Default; /**<\todo Needs documentation */
};

/**\todo Needs documentation
*/
inline void from_json(const nlohmann::json& j, message& m)
{
    m.message_id = j["id"];
    if (j.count("content") && !j["content"].is_null())
        m.content = j["content"];
    if (j.count("timestamp") && !j["timestamp"].is_null())
        m.timestamp = j["timestamp"];
    if (j.count("edited_timestamp") && !j["edited_timestamp"].is_null())
        m.edited_timestamp = j["edited_timestamp"];
    if (j.count("tts") && !j["tts"].is_null())
        m.tts = j["tts"];
    if (j.count("mention_everyone") && !j["mention_everyone"].is_null())
        m.mention_everyone = j["mention_everyone"];
    if (j.count("pinned") && !j["pinned"].is_null())
        m.pinned = j["pinned"];
    if (j.count("type") && !j["type"].is_null())
        m.type = j["type"];
    if (j.count("nonce") && !j["nonce"].is_null())
        m.nonce = j["nonce"];
    if (j.count("webhook_id") && !j["webhook_id"].is_null())
        m.webhook_id = j["webhook_id"];
    if (j.count("mentions") && !j["mentions"].is_null())
        for (auto i : j["mentions"])
            m.mentions.push_back(i["id"]);
    if (j.count("roles") && !j["roles"].is_null())
        for (auto i : j["roles"])
            m.mention_roles.push_back(i);
    if (j.count("attachments") && !j["attachments"].is_null())
        for (auto i : j["attachments"])
            m.attachments.push_back(i);
    if (j.count("embeds") && !j["embeds"].is_null())
        for (auto i : j["embeds"])
            m.embeds.push_back(i);
    if (j.count("reactions") && !j["reactions"].is_null())
        for (auto i : j["reactions"])
            m.reactions.push_back(i);
}

/**\todo Needs documentation
*/
inline void to_json(nlohmann::json& j, const message& m)
{
    j["id"] = m.message_id;
    j["content"] = m.content;
    j["timestamp"] = m.timestamp;
    j["edited_timestamp"] = m.edited_timestamp;
    j["tts"] = m.tts;
    j["mention_everyone"] = m.mention_everyone;
    j["pinned"] = m.pinned;
    j["type"] = m.type;
    if (m.nonce != 0)
        j["nonce"] = m.nonce;
    if (m.webhook_id.size() > 0)
        j["webhook_id"] = m.webhook_id;
    for (auto i : m.mentions)
        j["mentions"].push_back(i);
    for (auto i : m.mention_roles)
        j["roles"].push_back(i);
    for (auto i : m.attachments)
        j["attachments"].push_back(i);
    for (auto i : m.embeds)
        j["embeds"].push_back(i);
    if (m.reactions.size() > 0)
        for (auto i : m.reactions)
            j["reactions"].push_back(i);
}



}

