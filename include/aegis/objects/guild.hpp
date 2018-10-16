//
// guild.hpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "guild_member.hpp"
#include "channel.hpp"
#include "role.hpp"
#include "voice_state.hpp"
#include "presence.hpp"
#include "emoji.hpp"
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
struct guild
{
    //TODO:
    snowflake guild_id; /**<\todo Needs documentation */
    std::string name; /**<\todo Needs documentation */
    std::string icon; /**<\todo Needs documentation */
    std::string splash; /**<\todo Needs documentation */
    snowflake owner_id = 0; /**<\todo Needs documentation */
    std::string region; /**<\todo Needs documentation */
    snowflake afk_channel_id; /**<\todo Needs documentation */
    int32_t afk_timeout = 0; /**<\todo Needs documentation */
    bool embed_enabled = false; /**<\todo Needs documentation */
    snowflake embed_channel_id; /**<\todo Needs documentation */
    int8_t verification_level = 0; /**<\todo Needs documentation */
    int8_t default_message_notifications = 0; /**<\todo Needs documentation */
    int8_t explicit_content_filter = 0; /**<\todo Needs documentation */
    std::vector<objects::role> roles; /**<\todo Needs documentation */
    std::vector<objects::emoji> emojis; /**<\todo Needs documentation */
    std::vector<std::string> features; /**<\todo Needs documentation */
    int8_t mfa_level = 0; /**<\todo Needs documentation */
    snowflake application_id;//? /**<\todo Needs documentation */
    bool widget_enabled = false; /**<\todo Needs documentation */
    snowflake widget_channel_id; /**<\todo Needs documentation */
    //createonly
    std::string joined_at; /**<\todo Needs documentation */
    bool large = false; /**<\todo Needs documentation */
    bool unavailable = false; /**<\todo Needs documentation */
    int32_t member_count = 0; /**<\todo Needs documentation */
    std::vector<objects::voice_state> voice_states; /**<\todo Needs documentation */
    std::vector<objects::guild_member> members; /**<\todo Needs documentation */
    std::vector<objects::channel> channels; /**<\todo Needs documentation */
    std::vector<objects::presence> presences; /**<\todo Needs documentation */
};


/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, guild& m)
{
    if (j.count("id") && !j["id"].is_null())
        m.guild_id = j["id"];
    if (j.count("name") && !j["name"].is_null())
        m.name = j["name"];
    if (j.count("icon") && !j["icon"].is_null())
        m.icon = j["icon"];
    if (j.count("splash") && !j["splash"].is_null())
        m.splash = j["splash"];
    if (j.count("owner_id") && !j["owner_id"].is_null())
        m.owner_id = j["owner_id"];
    if (j.count("region") && !j["region"].is_null())
        m.region = j["region"];
    if (j.count("afk_channel_id") && !j["afk_channel_id"].is_null())
        m.afk_channel_id = j["afk_channel_id"];
    if (j.count("afk_timeout") && !j["afk_timeout"].is_null())
        m.afk_timeout = j["afk_timeout"];
    if (j.count("embed_enabled") && !j["embed_enabled"].is_null())
        m.embed_enabled = j["embed_enabled"];
    if (j.count("embed_channel_id") && !j["embed_channel_id"].is_null())
        m.embed_channel_id = j["embed_channel_id"];
    if (j.count("verification_level") && !j["verification_level"].is_null())
        m.verification_level = j["verification_level"];
    if (j.count("default_message_notifications") && !j["default_message_notifications"].is_null())
        m.default_message_notifications = j["default_message_notifications"];
    if (j.count("mfa_level") && !j["mfa_level"].is_null())
        m.mfa_level = j["mfa_level"];
    if (j.count("joined_at") && !j["joined_at"].is_null())
        m.joined_at = j["joined_at"];
    if (j.count("large") && !j["large"].is_null())
        m.large = j["large"];
    if (j.count("unavailable") && !j["unavailable"].is_null())
        m.unavailable = j["unavailable"];
    else
        m.unavailable = false;
    if (j.count("member_count") && !j["member_count"].is_null())
        m.member_count = j["member_count"];

    if (j.count("roles"))
        for (const auto & _role : j["roles"])
            m.roles.push_back(_role);

    if (j.count("members"))
        for (const auto & _member : j["members"])
            m.members.push_back(_member);

    if (j.count("channels"))
        for (const auto & _channel : j["channels"])
            m.channels.push_back(_channel);

    if (j.count("presences"))
        for (const auto & _presence : j["presences"])
            m.presences.push_back(_presence);

    if (j.count("emojis"))
        for (const auto & _emoji : j["emojis"])
            m.emojis.push_back(_emoji);

    if (j.count("voice_states"))
        for (const auto & _voicestate : j["voice_states"])
            m.voice_states.push_back(_voicestate);

    if (j.count("features"))
        for (const auto & _feature : j["features"])
            m.features.push_back(_feature);
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const guild& m)
{

}
/// \endcond

}

}

}
