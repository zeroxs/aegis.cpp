//
// user_update.hpp
// ***************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/user.hpp"


namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct user_update
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    objects::user _user; /**< Discord User object */
};

}

}

}
