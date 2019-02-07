//
// voice_server_update.cpp
// ***********************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/voice_server_update.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, voice_server_update& m)
{
    m.token = j["token"].get<std::string>();
    m.guild_id = j["guild_id"];
    m.endpoint = j["endpoint"].get<std::string>();
}
/// \endcond

}

}

}
