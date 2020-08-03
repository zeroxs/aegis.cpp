//
// channel_pins_update.hpp
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

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when channel pins are updated in a channel
struct channel_pins_update
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    snowflake channel_id; /**< Snowflake of channel */
    std::string last_pin_timestamp; /**< ISO8601 timestamp of most recent pinned message */
};

}

}

}
