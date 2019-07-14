//
// presence_update.hpp
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
#include "aegis/gateway/objects/presence.hpp"
#include "aegis/gateway/objects/activity.hpp"
#include "aegis/gateway/objects/user.hpp"
#include "aegis/gateway/objects/role.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct presence_update
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    objects::user user; /**< Discord user object */
    objects::activity game; /**<\todo Needs documentation */
    std::vector<objects::role> roles; /**<\todo Needs documentation */
    snowflake guild_id; /**<\todo Needs documentation */
    objects::presence::user_status status = objects::presence::Online; /**<\todo Needs documentation */
};

}

}

}
