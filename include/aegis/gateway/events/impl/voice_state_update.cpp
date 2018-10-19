//
// voice_state_update.cpp
// **********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/voice_state_update.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, voice_state_update& m)
{
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m.guild_id = j["guild_id"];
    m.channel_id = j["channel_id"];
    m.user_id = j["user_id"];
    m.session_id = j["session_id"].get<std::string>();
    m.deaf = j["deaf"];
    m.mute = j["mute"];
    m.self_deaf = j["self_deaf"];
    m.self_mute = j["self_mute"];
    m.suppress = j["suppress"];
}
/// \endcond

}

}

}
