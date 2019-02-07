//
// guild_role_update.hpp
// *********************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/role.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/// Send when a guild role is updated
struct guild_role_update
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake guild_id; /**< Snowflake of guild */
    objects::role _role; /**< Role that was updated */
};

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, guild_role_update& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/guild_role_update.cpp"
#endif
