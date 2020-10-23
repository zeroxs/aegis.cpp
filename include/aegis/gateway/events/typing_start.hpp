//
// typing_start.hpp
// ****************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
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
struct typing_start
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    aegis::channel & channel; /**<\todo Reference to channel object this message was sent in */
    aegis::user & user; /**<\todo Reference to object of user that sent this message */
    int64_t timestamp; /**<\todo Needs documentation */
};

}

}

}
