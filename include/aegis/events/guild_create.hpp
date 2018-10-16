//
// guild_create.hpp
// ****************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/guild.hpp"
#include "aegis/fwd.hpp"
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when the bot is added to a new guild or when recovering from an outage
struct guild_create
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    objects::guild _guild; /**< guild object */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, guild_create& m)
{
    m._guild = j;
}
/// \endcond

}

}

}
