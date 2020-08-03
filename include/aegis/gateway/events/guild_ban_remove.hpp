//
// guild_ban_remove.hpp
// ********************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/user.hpp"


namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when a new ban is removed from the guild
struct guild_ban_remove
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    snowflake guild_id; /**< Snowflake of the guild */
    objects::user user; /**< User object of the user that was unbanned */
};

}

}

}
