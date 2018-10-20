//
// guild_member_update.hpp
// ***********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/user.hpp"


namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when a guild member is updated
struct guild_member_update
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    objects::user _user; /**< User that was updated */
    snowflake guild_id; /**< Snowflake of guild */
    std::vector<snowflake> roles; /**< Array of roles the user has */
    std::string nick; /**< Nickname the user currently has (if any) */
};

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, guild_member_update& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/guild_member_update.cpp"
#endif
