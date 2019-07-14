//
// voice_state_update.hpp
// **********************
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
struct voice_state_update
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    snowflake guild_id = 0;
    snowflake channel_id = 0;
    snowflake user_id = 0;
    std::string session_id;
    bool deaf = false;
    bool mute = false;
    bool self_deaf = false;
    bool self_mute = false;
    bool suppress = false;
};

}

}

}
