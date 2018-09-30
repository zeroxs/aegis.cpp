//
// role.hpp
// ********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/permission.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace objects
{

/// Stores info pertaining to guild roles
struct role
{
    uint32_t color = 0;
    snowflake role_id;
    std::string name;
    permission _permission;
    uint16_t position = 0;
    bool hoist = false;
    bool managed = false;
    bool mentionable = false;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, role& m)
{
    if (j.count("color") && !j["color"].is_null())
        m.color = j["color"];
    if (j.count("id") && !j["id"].is_null())
        m.role_id = j["id"];
    if (j.count("name") && !j["name"].is_null())
        m.name = j["name"];
    if (j.count("permissions") && !j["permissions"].is_null())
        m._permission = j["permissions"];
    if (j.count("position") && !j["position"].is_null())
        m.position = j["position"];
    if (j.count("hoist") && !j["hoist"].is_null())
        m.hoist = j["hoist"];
    if (j.count("mentionable") && !j["mentionable"].is_null())
        m.mentionable = j["mentionable"];
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const role& m)
{
    j["color"] = m.color;
    j["id"] = m.role_id;
    j["name"] = m.name;
    j["permissions"] = m._permission;
    j["position"] = m.position;
    j["hoist"] = m.hoist;
    j["mentionable"] = m.mentionable;
}
/// \endcond

}

}

}
