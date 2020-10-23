//
// channel.hpp
// ***********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/user.hpp"
#include "permission_overwrite.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace objects
{

/// \cond TEMPLATES
void from_json(const nlohmann::json& j, channel& m);
void to_json(nlohmann::json& j, const channel& m);
/// \endcond

/// Represents a guild or DM channel within Discord
struct channel
{
    channel(const std::string & _json, aegis::core * bot) noexcept
    {
        from_json(nlohmann::json::parse(_json), *this);
    }

    channel(const nlohmann::json & _json, aegis::core * bot) noexcept
    {
        from_json(_json, *this);
    }

    channel(aegis::core * bot) noexcept {}

    channel() noexcept {}

    /// Enumeration of the channel types
    enum channel_type
    {
        Text = 0, /**< Text channel within a guild */
        DirectMessage = 1, /**< Direct message channel between a pair of users */
        Voice = 2, /**< Voice channel within a guild */
        GroupDirectMessage = 3, /**< Direct message channel between a group of users */
        Category = 4 /**< Category containing channels */
    };

    snowflake id; /**< Snowflake */
    snowflake channel_id; /**<\deprecated Snowflake */
    channel_type type = Text; /**< Channel type - Text | DirectMessage | Voice | GroupDirectMessage | Category */
    snowflake guild_id = 0; /**< Snowflake if channel belongs to a guild (not a DM) */
    int position = 0; /**< Sorting position of the channel */
    std::vector<objects::permission_overwrite> permission_overwrites; /**< Explicit permission overwrites for members and roles */
    std::string name; /**< Name of the channel (must be 2-100 characters) */
    std::string topic; /**< Topic of the channel (up to 1024 characters) */
    bool nsfw = false; /**< Whether the channel is NSFW */
    snowflake last_message_id = 0; /**< Snowflake of the last message sent in the channel (message may be non-existent) */
    int bitrate = 0; /**< Bitrate (in bits), if this is a voice channel */
    int userlimit = 0; /**< User limit, if this is a voice channel */
    std::vector<objects::user> recipients; /**< Channel message recipients, if this is a DM */
    std::string icon; /**< Icon hash */
    snowflake owner_id; /**< Snowflake of the DM creator (if the creator is a user) */
    snowflake application_id; /**< Application ID of the DM creator (if the creator is a bot) */
    snowflake parent_id; /**< Snowflake of the parent category */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, channel& m)
{
    m.id = m.channel_id = j["id"];
    m.type = j["type"];
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m.guild_id = j["guild_id"];
    if (j.count("position") && !j["position"].is_null())
        m.position = j["position"];
    if (j.count("permission_overwrites") && !j["permission_overwrites"].is_null())
        for (const auto & i : j["permission_overwrites"])
            m.permission_overwrites.push_back(i);
    if (j.count("name") && !j["name"].is_null())
        m.name = j["name"].get<std::string>();
    if (j.count("topic") && !j["topic"].is_null())
        m.topic = j["topic"].get<std::string>();
    if (j.count("nsfw") && !j["nsfw"].is_null())
        m.nsfw = j["nsfw"];
    if (j.count("last_message_id") && !j["last_message_id"].is_null())
        m.last_message_id = j["last_message_id"];
    if (j.count("bitrate") && !j["bitrate"].is_null())
        m.bitrate = j["bitrate"];
    if (j.count("userlimit") && !j["userlimit"].is_null())
        m.userlimit = j["userlimit"];
    if (j.count("icon") && !j["icon"].is_null())
        m.icon = j["icon"].get<std::string>();
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

inline void to_json(nlohmann::json& j, const channel& m)
{
    if (m.channel_id)
        j["id"] = std::to_string(m.id);
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
/// \endcond

}

}

}
