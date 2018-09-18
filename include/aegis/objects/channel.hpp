//
// channel.hpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/user.hpp"
#include "aegis/objects/user.hpp"
#include "permission_overwrite.hpp"
#include "user.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Needs documentation
 */
struct channel_gw
{
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

    snowflake channel_id; /**<\todo Needs documentation */
    channel_type type = Text; /**<\todo Needs documentation */
    snowflake guild_id = 0;//? /**<\todo Needs documentation */
    int position = 0;//? /**<\todo Needs documentation */
    std::vector<permission_overwrite> permission_overwrites;//? /**<\todo Needs documentation */
    std::string name;//? /**<\todo Needs documentation */
    std::string topic;//? /**<\todo Needs documentation */
    bool nsfw = false;//? /**<\todo Needs documentation */
    snowflake last_message_id = 0;//? /**<\todo Needs documentation */
    int bitrate = 0;//? /**<\todo Needs documentation */
    int userlimit = 0;//? /**<\todo Needs documentation */
    std::vector<user> recipients;//? /**<\todo Needs documentation */
    std::string icon;//? /**<\todo Needs documentation */
    snowflake owner_id;//? /**<\todo Needs documentation */
    snowflake application_id;//? /**<\todo Needs documentation */
    snowflake parent_id;//? /**<\todo Needs documentation */
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, channel_gw& m)
{
    m.channel_id = j["id"];
    m.type = j["type"];
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m.guild_id = j["guild_id"];
    if (j.count("position") && !j["position"].is_null())
        m.position = j["position"];
    if (j.count("permission_overwrites") && !j["permission_overwrites"].is_null())
        for (const auto & i : j["permission_overwrites"])
            m.permission_overwrites.push_back(i);
    if (j.count("name") && !j["name"].is_null())
        m.name = j["name"];
    if (j.count("topic") && !j["topic"].is_null())
        m.topic = j["topic"];
    if (j.count("nsfw") && !j["nsfw"].is_null())
        m.nsfw = j["nsfw"];
    if (j.count("last_message_id") && !j["last_message_id"].is_null())
        m.last_message_id = j["last_message_id"];
    if (j.count("bitrate") && !j["bitrate"].is_null())
        m.bitrate = j["bitrate"];
    if (j.count("userlimit") && !j["userlimit"].is_null())
        m.userlimit = j["userlimit"];
    if (j.count("icon") && !j["icon"].is_null())
        m.icon = j["icon"];
    if (j.count("owner_id") && !j["owner_id"].is_null())
        m.owner_id = j["owner_id"];
    if (j.count("application_id") && !j["application_id"].is_null())
        m.application_id = j["application_id"];
    if (j.count("parent_id") && !j["parent_id"].is_null())
        m.parent_id = j["parent_id"];
    if (j.count("recipients") && !j["recipients"].is_null())
        for (const auto & i : j["recipients"])
            m.recipients.push_back(i);
}

/**\todo Needs documentation
 */
inline void to_json(nlohmann::json& j, const channel_gw& m)
{
    if (m.channel_id)
        j["id"] = m.channel_id;
    j["type"] = m.type;
    if (m.guild_id)
        j["guild_id"] = m.guild_id;
    if (m.position)
        j["position"] = m.position;
    j["name"] = m.name;
    if (!m.topic.empty())
        j["topic"] = m.topic;
    if (m.nsfw)
        j["nsfw"] = m.nsfw;
    if (m.last_message_id)
        j["last_message_id"] = m.last_message_id;
    if (m.bitrate)
        j["bitrate"] = m.bitrate;
    if (m.userlimit)
        j["userlimit"] = m.userlimit;
    if (!m.icon.empty())
        j["icon"] = m.icon;
    if (m.bitrate)
        j["owner_id"] = m.owner_id;
    if (m.application_id)
        j["application_id"] = m.application_id;
    if (m.parent_id)
        j["parent_id"] = m.parent_id;

    if (!m.permission_overwrites.empty())
        for (auto i : m.permission_overwrites)
            j["permission_overwrites"].push_back(i);

    if (!m.recipients.empty())
        for (auto i : m.recipients)
            j["recipients"].push_back(i);
}

}

}

}
