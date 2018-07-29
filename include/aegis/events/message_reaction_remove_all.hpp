//
// message_reaction_remove_all.hpp
// *******************************
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
struct message_reaction_remove_all
{
    shards::shard * _shard; /**< Pointer to shard object this message came from */
    core * bot; /**< Pointer to the main bot object */
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, message_reaction_remove_all& m)
{
}

}

}

}
