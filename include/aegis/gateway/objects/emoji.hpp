//
// emoji.hpp
// *********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
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

/// Discord Emoji Object
struct emoji
{
    snowflake id; /**< Emoji ID */
    std::string name; /**< Emoji Name */
    std::vector<snowflake> roles; /**< Roles this emoji is whitelisted to */
    snowflake user; /**< User that created this emoji */
    bool require_colons = false; /**< Whether this emoji must be wrapped in colons */
    bool managed = false; /**< Whether this emoji is managed */
    bool animated = false; /**< Whether this emoji is animated */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, emoji& m)
{
    m.id = j["id"];
    if (j.count("name") && !j["name"].is_null())
        m.name = j["name"].get<std::string>();
    if (j.count("user") && !j["user"].is_null())
        m.user = j["user"]["id"];
    if (j.count("require_colons") && !j["require_colons"].is_null())
        m.require_colons = j["require_colons"];
    if (j.count("managed") && !j["managed"].is_null())
        m.managed = j["managed"];
    if (j.count("animated") && !j["animated"].is_null())
        m.animated = j["animated"];
    if (j.count("roles") && !j["roles"].is_null())
        for (const auto & i : j["roles"])
            m.roles.push_back(i);
}

inline void to_json(nlohmann::json& j, const emoji& m)
{
    j["id"] = std::to_string(m.id);
    j["name"] = m.name;
    j["user"] = m.user;
    j["require_colons"] = m.require_colons;
    j["managed"] = m.managed;
    j["animated"] = m.animated;
    for (const auto & i : m.roles)
        j["roles"].push_back(i);
}
/// \endcond

}

}

}
