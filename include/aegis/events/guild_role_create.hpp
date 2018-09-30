//
// guild_role_create.hpp
// *********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/objects/role.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when a guild role is created
struct guild_role_create
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake guild_id; /**< Snowflake of guild */
    objects::role _role; /**< Role that was created */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, guild_role_create& m)
{
    m.guild_id = j["guild_id"];
    m._role = j["role"];
}
/// \endcond

}

}

}
