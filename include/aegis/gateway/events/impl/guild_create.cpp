//
// guild_create.cpp
// ****************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/guild_create.hpp"
#include <nlohmann/json.hpp>


namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, guild_create& m)
{
    m._guild = j;
}
/// \endcond

}

}

}
