//
// guild_member_remove.hpp
// ***********************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
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

/// Sent when a new member is removed from the guild
struct guild_member_remove
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    objects::user user; /**< User being removed from guild */
    snowflake guild_id; /**< Snowflake of guild */
};

}

}

}
