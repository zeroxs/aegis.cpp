//
// guild_member_remove.hpp
// ***********************
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

/// Sent when a new member is removed from the guild
struct guild_member_remove
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    objects::user _user; /**< User being removed from guild */
    snowflake guild_id; /**< Snowflake of guild */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, guild_member_remove& m)
{
    m._user = j["user"];
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m.guild_id = j["guild_id"];
}
/// \endcond

}

}

}
