//
// guild_member_add.hpp
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
#include "aegis/gateway/objects/guild_member.hpp"


namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when a new member joins the guild
struct guild_member_add
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    objects::guild_member member; /**< User being added to the guild */
};

}

}

}
