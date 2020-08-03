//
// guild_emojis_update.hpp
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
#include "aegis/gateway/objects/emoji.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when a guild's emojis change
struct guild_emojis_update
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    snowflake guild_id; /**< Snowflake of guild */
    std::vector<objects::emoji> emojis; /**< Array of emojis */
};

}

}

}
