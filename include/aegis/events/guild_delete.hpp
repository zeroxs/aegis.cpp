//
// guild_delete.hpp
// ****************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/fwd.hpp"
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when the bot is removed from a guild
struct guild_delete
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake guild_id = 0; /**< Snowflake of the guild */
    bool unavailable = true; /**< Whether guild is unavailable due to an outage */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, guild_delete& m)
{
    m.guild_id = j["id"];
    m.unavailable = j["unavailable"];
}
/// \endcond

}

}

}
