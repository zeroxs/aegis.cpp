//
// guild_emojis_update.cpp
// ***********************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/guild_emojis_update.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, guild_emojis_update& m)
{
    m.guild_id = j["guild_id"];
    if (j.count("emojis") && !j["emojis"].is_null())
        for (const auto & _emoji : j["emojis"])
            m.emojis.push_back(_emoji);
}
/// \endcond

}

}

}
