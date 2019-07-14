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
    shards::shard & shard; /**< Reference to shard object this message came from */
    objects::guild guild; /**< guild object */
};

}

}

}
