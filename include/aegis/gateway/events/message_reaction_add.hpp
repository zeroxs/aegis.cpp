//
// message_reaction_add.hpp
// ************************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
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
struct message_reaction_add
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    snowflake user_id;
    snowflake channel_id;
    snowflake message_id;
    snowflake guild_id;
    objects::emoji emoji;
};

}

}

}
