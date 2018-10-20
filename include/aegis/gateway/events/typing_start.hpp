//
// typing_start.hpp
// ****************
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
struct typing_start
{
    int64_t timestamp; /**<\todo Needs documentation */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    typing_start(int64_t _timestamp, aegis::channel * c, aegis::member * m) : timestamp(_timestamp), _channel(c), _member(m) {};
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    aegis::channel * const _channel = nullptr; /**<\todo Needs documentation */
    aegis::member * const _member = nullptr; /**<\todo Needs documentation */
#else
    typing_start(int64_t _timestamp) : timestamp(_timestamp) {}
#endif
};

}

}

}
