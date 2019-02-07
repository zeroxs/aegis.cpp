//
// channel_create.hpp
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

/// Channel object sent over the gateway on create
struct channel_create
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    objects::channel _channel; /**< gateway channel object */
};

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, channel_create& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/channel_create.cpp"
#endif
