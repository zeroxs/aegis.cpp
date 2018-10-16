//
// message_reaction_add.hpp
// ************************
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
struct message_reaction_add
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake user_id;
    snowflake channel_id;
    snowflake message_id;
    snowflake guild_id;
    objects::emoji _emoji;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, message_reaction_add& m)
{
    m.user_id = j["user_id"];
    m.channel_id = j["channel_id"];
    m.message_id = j["message_id"];
    m.guild_id = j["guild_id"];
    m._emoji = j["emoji"];
}
/// \endcond

}

}

}
