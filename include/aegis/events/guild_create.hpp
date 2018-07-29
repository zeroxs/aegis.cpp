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

/**\todo Needs documentation
 */
struct guild_create
{
    shards::shard * _shard; /**< Pointer to shard object this message came from */
    core * bot; /**< Pointer to the main bot object */
    objects::guild_gw _guild; /**<\todo Needs documentation */
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, guild_create& m)
{
    m._guild = j;
}

}

}

}
