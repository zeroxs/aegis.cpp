//
// ready.hpp
// *********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//


#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/objects/user.hpp"
#include "aegis/gateway/objects/channel.hpp"
#include "aegis/gateway/objects/guild.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct ready 
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    int8_t v = 0; /**< Gateway protocol version */
    objects::user user; /**< Discord user object */
    std::vector<objects::channel> private_channels; /**< Direct Message channels (empty for bots) */
    std::vector<objects::guild> guilds; /**< Guilds currently in */
    std::string session_id; /**< Session ID for resuming */
    std::vector<std::string> _trace; /**< Debug information for Discord */
};

}

}

}
