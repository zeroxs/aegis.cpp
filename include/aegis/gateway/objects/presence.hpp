//
// presence.hpp
// ************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include <nlohmann/json.hpp>

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
    static inline std::string to_string(user_status status)
    {
        switch (status)
        {
        case Idle:
            return "idle";
            break;
        case DoNotDisturb:
            return "dnd";
            break;
        case Offline:
            return "offline";
            break;
        case Online:
        default:
            return "online";
            break;
        }
    }
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, presence& m)
{

}

inline void to_json(nlohmann::json& j, const presence& m)
{

}
/// \endcond

}

}

}
