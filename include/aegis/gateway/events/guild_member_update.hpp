//
// guild_member_update.hpp
// ***********************
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

/// Sent when a guild member is updated
struct guild_member_update
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    objects::user user; /**< User that was updated */
    snowflake guild_id; /**< Snowflake of guild */
    std::vector<snowflake> roles; /**< Array of roles the user has */
    std::string nick; /**< Nickname the user currently has (if any) */
};

}

}

}
