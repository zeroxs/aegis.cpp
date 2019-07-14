//
// channel_delete.hpp
// ******************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/objects/channel.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/// Channel object sent over the gateway on delete
struct channel_delete
{
    shards::shard & shard; /**< Reference to shard object this message came from */
    objects::channel channel; /**< gateway channel object */
};

}

}

}
