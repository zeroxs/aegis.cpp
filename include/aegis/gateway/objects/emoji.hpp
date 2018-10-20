//
// emoji.hpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
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

/**\todo Needs documentation
 */
struct emoji
{
    snowflake emoji_id; /**<\todo Needs documentation */
    std::string name; /**<\todo Needs documentation */
    std::vector<snowflake> roles; /**<\todo Needs documentation */
    snowflake user; /**<\todo Needs documentation */
    bool require_colons = false; /**<\todo Needs documentation */
    bool managed = false; /**<\todo Needs documentation */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, emoji& m)
{
    m.emoji_id = j["id"];
    if (j.count("name") && !j["name"].is_null())
        m.name = j["name"];
    if (j.count("user") && !j["user"].is_null())
        m.user = j["user"]["id"];
    if (j.count("require_colons") && !j["require_colons"].is_null())
        m.require_colons = j["require_colons"];
    if (j.count("managed") && !j["managed"].is_null())
        m.managed = j["managed"];
    if (j.count("roles") && !j["roles"].is_null())
        for (const auto & i : j["roles"])
            m.roles.push_back(i);
}

inline void to_json(nlohmann::json& j, const emoji& m)
{
    j["id"] = m.emoji_id;
    j["name"] = m.name;
    j["user"] = m.user;
    j["require_colons"] = m.require_colons;
    j["managed"] = m.managed;
    for (const auto & i : m.roles)
        j["roles"].push_back(i);
}
/// \endcond

}

}

}
