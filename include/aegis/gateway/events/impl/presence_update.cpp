//
// presence_update.cpp
// *******************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/presence_update.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, presence_update& m)
{
    m._user = j["user"];
    m.guild_id = j["guild_id"];

    const std::string & sts = j["status"];

    if (sts == "idle")
        m.status = objects::presence::user_status::Idle;
    else if (sts == "dnd")
        m.status = objects::presence::user_status::DoNotDisturb;
    else if (sts == "online")
        m.status = objects::presence::user_status::Online;
    else
        m.status = objects::presence::user_status::Offline;

    if (j.count("game") && !j["game"].is_null())
        m.game = j["game"];

    if (j.count("roles") && !j["roles"].is_null())
        for (const auto & _role : j["roles"])
            m.roles.push_back(_role);
}
/// \endcond

}

}

}
