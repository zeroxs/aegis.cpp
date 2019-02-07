// ***********************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/guild_member_update.hpp"
#include <nlohmann/json.hpp>


namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, guild_member_update& m)
{
    m._user = j["user"];
    if (j.count("nick") && !j["nick"].is_null())
        m.nick = j["nick"];
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m.guild_id = j["guild_id"];
    if (j.count("roles") && !j["roles"].is_null())
        for (const auto & i : j["roles"])
            m.roles.push_back(i);
}
/// \endcond

}

}

}
