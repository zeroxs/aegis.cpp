//
// guild_members_chunk.hpp
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
#include "aegis/gateway/objects/guild_member.hpp"


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
AEGIS_DECL void from_json(const nlohmann::json& j, guild_members_chunk& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/guild_members_chunk.cpp"
#endif
