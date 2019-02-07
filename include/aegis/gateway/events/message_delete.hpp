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
    snowflake id; /**< Snowflake of deleted message */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    explicit message_delete(snowflake m, aegis::channel * c) : id(m), _channel(c) {};
    channel * const _channel = nullptr; /**<\todo Needs documentation */
#else
    explicit message_delete(snowflake m) : id(m) {}
#endif
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
};

}

}

}
