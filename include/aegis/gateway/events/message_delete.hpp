//
// message_delete.hpp
// ******************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/message.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct message_delete
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    aegis::channel & channel; /**<\todo Needs documentation */
    snowflake id; /**< Snowflake of deleted message */
};

}

}

}
