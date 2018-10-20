//
// message_reaction_remove.hpp
// ***************************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/emoji.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct message_reaction_remove
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
AEGIS_DECL void from_json(const nlohmann::json& j, message_reaction_remove& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/message_reaction_remove.cpp"
#endif
