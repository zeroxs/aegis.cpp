//
// guild_role_update.hpp
// *********************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/role.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/// Send when a guild role is updated
struct guild_role_update
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    snowflake guild_id; /**< Snowflake of guild */
    objects::role role; /**< Role that was updated */
};

}

}

}
