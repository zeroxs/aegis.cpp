//
// presence.hpp
// ************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Incomplete. Needs documentation
 */
struct presence
{
    enum user_status
    {
        Online,
        Idle,
        DoNotDisturb,
        Offline
    };
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, presence& m)
{

}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const presence& m)
{

}
/// \endcond

}

}

}
