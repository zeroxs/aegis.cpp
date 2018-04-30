//
// base_event.hpp
// **************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

class member;
class channel;
class shard;
class core;

}

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct base_event
{
    shard * _shard; /**< Pointer to shard object this message came from */
    core * bot; /**< Pointer to the main bot object */
};

}

}

}
