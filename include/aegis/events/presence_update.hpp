//
// presence_update.hpp
// *******************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/presence.hpp"
#include "aegis/objects/activity.hpp"
#include "aegis/fwd.hpp"
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct presence_update
{
    using presence = aegis::gateway::objects::presence;
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    objects::user _user; /**<\todo Needs documentation */
    objects::activity game;
    std::vector<aegis::gateway::objects::role> roles;
    snowflake guild_id = 0;
    presence::user_status status = presence::Online;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, presence_update& m)
{
    m._user = j["user"];
    m.guild_id = j["guild_id"];

    using presence = aegis::gateway::objects::presence;
    const std::string & sts = j["status"];

    if (sts == "idle")
        m.status = presence::user_status::Idle;
    else if (sts == "dnd")
        m.status = presence::user_status::DoNotDisturb;
    else if (sts == "online")
        m.status = presence::user_status::Online;
    else
        m.status = presence::user_status::Offline;

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
