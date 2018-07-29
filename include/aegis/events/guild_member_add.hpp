//
// guild_member_add.hpp
// ********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/guild_member.hpp"
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
struct guild_member_add
{
    shards::shard * _shard; /**< Pointer to shard object this message came from */
    core * bot; /**< Pointer to the main bot object */
    objects::guild_member _member_data; /**<\todo Needs documentation */
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, guild_member_add& m)
{
    m._member_data = j;
}

}

}

}
