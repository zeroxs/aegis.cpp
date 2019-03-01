//
// webhooks_update.hpp
// *******************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
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
struct webhooks_update
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    snowflake guild_id;
    snowflake channel_id;
};

}

}

}
