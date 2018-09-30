//
// channel_pins_update.hpp
// ***********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when channel pins are updated in a channel
struct channel_pins_update
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake channel_id; /**< Snowflake of channel */
    std::string last_pin_timestamp; /**< ISO8601 timestamp of most recent pinned message */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, channel_pins_update& m)
{
    m.channel_id = j["channel_id"];
    if (j.count("last_pin_timestamp") && !j["last_pin_timestamp"].is_null())
        m.last_pin_timestamp = j["last_pin_timestamp"];
}
/// \endcond

}

}

}
