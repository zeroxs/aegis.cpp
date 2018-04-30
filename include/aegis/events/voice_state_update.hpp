//
// voice_state_update.hpp
// **********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "base_event.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct voice_state_update : public base_event
{
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, voice_state_update& m)
{
}

}

}

}
