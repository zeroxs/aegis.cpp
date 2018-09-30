//
// guild_members_chunk.hpp
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
#include "aegis/objects/guild_member.hpp"
#include "aegis/fwd.hpp"
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace events
{

/// Reply to a Request Guild Members request
struct guild_members_chunk
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    snowflake guild_id; /**< Snowflake of guild */
    std::vector<objects::guild_member> members; /** Array of guild members */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, guild_members_chunk& m)
{
    m.guild_id = j["guild_id"];
    if (j.count("members") && !j["members"].is_null())
        for (const auto & i : j["members"])
            m.members.push_back(i);
}
/// \endcond

}

}

}
