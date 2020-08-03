//
// voice_state.hpp
// ***************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/member.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Incomplete. Needs documentation
 */
struct voice_state
{
    snowflake guild_id;
    snowflake channel_id;
    snowflake user_id;
    objects::member * _member = nullptr;
    std::string session_id;
    bool deaf = false;
    bool mute = false;
    bool self_deaf = false;
    bool self_mute = false;
    bool suppress = false;
    bool self_stream = false;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, voice_state& m)
{
    if(j.count("guild_id") && !j["guild_id"].is_null()) {
        m.guild_id = j["guild_id"];
    }
    // todo: member
    if(j.count("channel_id") && !j["channel_id"].is_null()) {
        m.channel_id = j["channel_id"];
    }
    if(j.count("user_id") && !j["user_id"].is_null()) {
        m.user_id = j["user_id"];
    }
    if(j.count("session_id") && !j["session_id"].is_null()) {
        m.session_id = j["session_id"];
    }
    if(j.count("deaf") && !j["deaf"].is_null()) {
        m.deaf = j["deaf"];
    }
    if(j.count("mute") && !j["mute"].is_null()) {
        m.mute = j["mute"];
    }
    if(j.count("self_deaf") && !j["self_deaf"].is_null()) {
        m.self_deaf = j["self_deaf"];
    }
    if(j.count("self_mute") && !j["self_mute"].is_null()) {
        m.self_mute = j["self_mute"];
    }
    if(j.count("suppress") && !j["suppress"].is_null()) {
        m.suppress = j["suppress"];
    }
    if(j.count("self_stream") && !j["self_stream"].is_null()) {
        m.self_stream = j["self_stream"];
    }

}

inline void to_json(nlohmann::json& j, const voice_state& m)
{
    j["guild_id"] = m.guild_id;
    j["chanel_id"] = m.channel_id;
    j["user_id"] = m.user_id;
    j["member"] = NULL;
    j["session_id"] = m.session_id;
    j["deaf"] = m.deaf;
    j["mute"] = m.mute;
    j["self_deaf"] = m.self_deaf;
    j["self_mute"] = m.self_mute;
    j["suppress"] = m.suppress;
    j["self_stream"] = m.self_stream;
}
/// \endcond

}

}

}
