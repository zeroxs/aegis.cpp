//
// guild_delete.hpp
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


namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when the bot is removed from a guild
struct guild_delete
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    snowflake guild_id; /**< Snowflake of the guild */
    bool unavailable = true; /**< Whether guild is unavailable due to an outage */
};

}

}

}
