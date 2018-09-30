//
// message_reaction_remove.hpp
// ***************************
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

/**\todo Needs documentation
 */
struct message_reaction_remove
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, message_reaction_remove& m)
{
}
/// \endcond

}

}

}
