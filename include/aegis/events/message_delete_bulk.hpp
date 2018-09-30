//
// message_delete_bulk.hpp
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

/// Sent when many messages are deleted at once
struct message_delete_bulk
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake channel_id; /**< Snowflake of channel */
    snowflake guild_id; /**< Snowflake of guild */
    std::vector<snowflake> ids; /**< Array of snowflake of deleted messages */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, message_delete_bulk& m)
{
    m.channel_id = j["channel_id"];
    m.guild_id = j["guild_id"];
    for (const auto & id : j["ids"])
        m.ids.push_back(id);
}
/// \endcond

}

}

}
