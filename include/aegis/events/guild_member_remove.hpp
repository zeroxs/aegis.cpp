//
// guild_member_remove.hpp
// ***********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/user.hpp"
#include "base_event.hpp"
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct guild_member_remove : public base_event
{
    objects::user _user; /**< User being removed from guild */
    snowflake guild_id; /**< snowflake of guild */
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, guild_member_remove& m)
{
    m._user = j["user"];
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m.guild_id = j["guild_id"];
}

}

}

}
