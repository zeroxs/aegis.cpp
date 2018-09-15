//
// guild_role_delete.hpp
// *********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct guild_role_delete
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake guild_id;
    snowflake role_id;
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, guild_role_delete& m)
{
    m.guild_id = j["guild_id"];
    m.role_id = j["role_id"];
}

}

}

}
