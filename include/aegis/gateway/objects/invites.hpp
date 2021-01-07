//
// invites.hpp
// ********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/permission.hpp"
#include "aegis/user.hpp"
#include "invite.hpp"
#include <nlohmann/json.hpp>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace objects
{

struct invites;

/// \cond TEMPLATES
void from_json(const nlohmann::json& j, invites& m);
void to_json(nlohmann::json& j, const invites& m);
/// \endcond

/// Stores info pertaining to guild invites
struct invites
{
    invites(const std::string& _json, aegis::core* bot) noexcept
    {
        from_json(nlohmann::json::parse(_json), *this);
    }

    invites(const nlohmann::json& _json, aegis::core* bot) noexcept
    {
        from_json(_json, *this);
    }

    invites(aegis::core* bot) noexcept {}

    invites() noexcept {}

    std::vector<aegis::gateway::objects::invite>::iterator begin()
    {
        return _invites.begin();
    }

    std::vector<aegis::gateway::objects::invite>::iterator end()
    {
        return _invites.end();
    }

    std::vector<aegis::gateway::objects::invite>::reverse_iterator rbegin()
    {
        return _invites.rbegin();
    }

    std::vector<aegis::gateway::objects::invite>::reverse_iterator rend()
    {
        return _invites.rend();
    }

    std::vector<aegis::gateway::objects::invite> _invites; /**< array of invites */

};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, invites& m)
{
    if (j.size())
        for (const auto& _invite : j)
            m._invites.push_back(_invite);
}

inline void to_json(nlohmann::json& j, const invites& m)
{
    if (!m._invites.empty())
        for (const auto& _invite : m._invites)
            j.push_back(_invite);
}
/// \endcond

}

}

}