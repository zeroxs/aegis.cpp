//
// presence_update.hpp
// *******************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/presence.hpp"
#include "aegis/gateway/objects/activity.hpp"
#include "aegis/gateway/objects/user.hpp"
#include "aegis/gateway/objects/role.hpp"

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
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    objects::user _user; /**<\todo Needs documentation */
    objects::activity game;
    std::vector<objects::role> roles;
    snowflake guild_id;
    objects::presence::user_status status = objects::presence::Online;
};

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, presence_update& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/presence_update.cpp"
#endif
