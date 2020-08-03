//
// guild_create.hpp
// ****************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
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

/// Sent when the bot is added to a new guild or when recovering from an outage
struct guild_create
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    objects::guild guild; /**< guild object */
};

}

}

}
