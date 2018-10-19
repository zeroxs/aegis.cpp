//
// message_delete_bulk.cpp
// ***********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/message_delete_bulk.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, message_delete_bulk& m)
{
    m.channel_id = j["channel_id"];
    m.guild_id = j["guild_id"];
    for (const auto & id : j["ids"])
        m.ids.push_back(id);
}
/// \endcond

}

}

}
