//
// guild_members_chunk.cpp
// ***********************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/guild_members_chunk.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, guild_members_chunk& m)
{
    m.guild_id = j["guild_id"];
    if (j.count("members") && !j["members"].is_null())
        for (const auto & i : j["members"])
            m.members.push_back(i);
}
/// \endcond

}

}

}
