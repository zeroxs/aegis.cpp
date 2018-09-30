//
// guild_integrations_update.hpp
// *****************************
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

/// Sent when a the guild integration is updated
struct guild_integrations_update
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake guild_id; /**< Snowflake of guild */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, guild_integrations_update& m)
{
    m.guild_id = j["guild_id"];
}
/// \endcond

}

}

}
