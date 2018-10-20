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
#include "aegis/snowflake.hpp"

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
AEGIS_DECL void from_json(const nlohmann::json& j, message_reaction_remove_all& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/message_reaction_remove_all.cpp"
#endif
