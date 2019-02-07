//
// guild_update.hpp
// ****************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/guild.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when a guild is updated
struct guild_update
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    objects::guild _guild; /**< guild object */
};

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, guild_update& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/guild_update.cpp"
#endif
