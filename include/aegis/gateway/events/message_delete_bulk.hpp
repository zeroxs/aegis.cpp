//
// message_delete_bulk.hpp
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

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when many messages are deleted at once
struct message_delete_bulk
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    snowflake channel_id; /**< Snowflake of channel */
    snowflake guild_id; /**< Snowflake of guild */
    std::vector<snowflake> ids; /**< Array of snowflake of deleted messages */
};

}

}

}
