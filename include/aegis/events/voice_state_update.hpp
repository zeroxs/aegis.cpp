//
// voice_state_update.hpp
// **********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct voice_state_update
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake guild_id = 0;
    snowflake channel_id = 0;
    snowflake user_id = 0;
    std::string session_id;
    bool deaf = false;
    bool mute = false;
    bool self_deaf = false;
    bool self_mute = false;
    bool suppress = false;
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, voice_state_update& m)
{
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m.guild_id = j["guild_id"];
    m.channel_id = j["channel_id"];
    m.user_id = j["user_id"];
    m.session_id = j["session_id"];
    m.deaf = j["deaf"];
    m.mute = j["mute"];
    m.self_deaf = j["self_deaf"];
    m.self_mute = j["self_mute"];
    m.suppress = j["suppress"];
}

}

}

}
