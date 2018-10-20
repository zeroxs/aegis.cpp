//
// channel_pins_update.cpp
// ***********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/channel_pins_update.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, channel_pins_update& m)
{
    m.channel_id = j["channel_id"];
    if (j.count("last_pin_timestamp") && !j["last_pin_timestamp"].is_null())
        m.last_pin_timestamp = j["last_pin_timestamp"].get<std::string>();
}
/// \endcond

}

}

}
