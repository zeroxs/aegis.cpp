//
// message_update.cpp
// ******************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/message_update.hpp"
#include <nlohmann/json.hpp>



namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, message_update& m)
{
    m.msg = j;
}
/// \endcond

}

}

}
