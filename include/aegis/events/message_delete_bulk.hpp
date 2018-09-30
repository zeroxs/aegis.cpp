//
// message_delete_bulk.hpp
// ***********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/// Sent when many messages are deleted at once
struct message_delete_bulk
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, message_delete_bulk& m)
{
}
/// \endcond

}

}

}
