//
// voice_server_update.hpp
// ***********************
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
struct voice_server_update
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    std::string token;
    snowflake guild_id;
    std::string endpoint;
};

}

}

}
