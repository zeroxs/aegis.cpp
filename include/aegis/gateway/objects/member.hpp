//
// member.hpp
// **********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/role.hpp"
#include "aegis/gateway/objects/user.hpp"
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
    lib::optional<objects::user> _user;
    bool mute = false;
    bool deaf = false;

    bool has_user() const noexcept
    {
        return _user.has_value();
    }
    objects::user get_user() const
    {
        if (has_user())
            return _user.value();
#if defined(AEGIS_HAS_BUILTIN_OPTIONAL)
        throw lib::bad_optional_access("bad optional access");
#else
        throw lib::bad_optional_access();
#endif
    }
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, member& m)
{
    if (j.count("roles") && !j["roles"].is_null())
        for (const auto & _role : j["roles"])
            m.roles.push_back(_role);
    if (j.count("nick") && !j["nick"].is_null())
        m.nick = j["nick"].get<std::string>();
    if (j.count("joined_at") && !j["joined_at"].is_null())
        m.joined_at = j["joined_at"].get<std::string>();
    if (j.count("mute") && !j["mute"].is_null())
        m.mute = j["mute"];
    if (j.count("deaf") && !j["deaf"].is_null())
        m.deaf = j["deaf"];
    if (j.count("user") && !j["user"].is_null())
        m._user = j["user"].get<objects::user>();
}

inline void to_json(nlohmann::json& j, const member& m)
{
    for (const auto & i : m.roles)
        j["roles"].push_back(i);
    j["nick"] = m.nick;
    j["joined_at"] = m.joined_at;
    j["mute"] = m.mute;
    j["deaf"] = m.deaf;
    if (m._user.has_value())
        j["user"] = m._user.value();
}
/// \endcond

}

}

}
