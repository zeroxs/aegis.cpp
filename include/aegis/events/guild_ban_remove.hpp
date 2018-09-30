//
// guild_ban_remove.hpp
// ********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/user.hpp"
#include "aegis/fwd.hpp"
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when a new ban is removed from the guild
struct guild_ban_remove
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake guild_id; /**< Snowflake of the guild */
    objects::user _user; /**< User object of the user that was unbanned */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, guild_ban_remove& m)
{
    m.guild_id = j["guild_id"];
    m._user = j["user"];
}
/// \endcond

}

}

}
