//
// permission_overwrite.hpp
// ************************
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

class member;
class channel;

}

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Needs documentation
 */
enum overwrite_type
{
    User,
    Role
};

/**\todo Needs documentation
 */
struct permission_overwrite
{
    snowflake id; /**<\todo Needs documentation */
    //either "role" or "member"
    overwrite_type type = User; /**<\todo Needs documentation */
    int64_t allow = 0; /**<\todo Needs documentation */
    int64_t deny = 0; /**<\todo Needs documentation */
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, permission_overwrite& m)
{
    m.id = j["id"];
    if (j.count("type"))
        m.type = (j["type"] == "role") ? (overwrite_type::Role) : (overwrite_type::User);
    m.allow = j["allow"];
    m.deny = j["deny"];
}

/**\todo Needs documentation
 */
inline void to_json(nlohmann::json& j, const permission_overwrite& m)
{
    j["id"] = m.id;
    j["type"] = m.type;
    j["allow"] = m.allow;
    j["deny"] = m.deny;
}

}

}

}
