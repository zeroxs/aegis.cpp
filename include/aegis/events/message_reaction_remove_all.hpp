//
// message_reaction_remove_all.hpp
// *******************************
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

/**\todo Needs documentation
 */
struct message_reaction_remove_all
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake channel_id;
    snowflake message_id;
    snowflake guild_id;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, message_reaction_remove_all& m)
{
    m.channel_id = j["channel_id"];
    m.message_id = j["message_id"];
    m.guild_id = j["guild_id"];
}
/// \endcond

}

}

}
