//
// member.hpp
// **********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/role.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace objects
{

struct member;

/// \cond TEMPLATES
void from_json(const nlohmann::json& j, member& m);
void to_json(nlohmann::json& j, const member& m);
/// \endcond

/**\todo Needs documentation
 */
struct member
{
    member(const std::string & _json, aegis::core * bot) noexcept
    {
        from_json(nlohmann::json::parse(_json), *this);
    }

    member(const nlohmann::json & _json, aegis::core * bot) noexcept
    {
        from_json(_json, *this);
    }

    member(aegis::core * bot) noexcept {}

    member() noexcept {}

    std::vector<objects::role> roles;
    std::string nick; /**< nick of user */
    std::string joined_at;
    bool mute = false;
    bool deaf = false;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, member& m)
{
    if (j.count("roles") && !j["roles"].is_null())
        for (const auto & _role : j["roles"])
            m.roles.push_back(_role);
    if (j.count("nick") && !j["nick"].is_null())
        m.nick = j["nick"];
    if (j.count("joined_at") && !j["joined_at"].is_null())
        m.joined_at = j["joined_at"];
    if (j.count("mute") && !j["mute"].is_null())
        m.mute = j["mute"];
    if (j.count("deaf") && !j["deaf"].is_null())
        m.deaf = j["deaf"];
}

inline void to_json(nlohmann::json& j, const member& m)
{
    for (const auto & i : m.roles)
        j["roles"].push_back(i);
    j["nick"] = m.nick;
    j["joined_at"] = m.joined_at;
    j["mute"] = m.mute;
    j["deaf"] = m.deaf;
}
/// \endcond

}

}

}
