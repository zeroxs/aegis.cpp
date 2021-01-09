//
// bans.hpp
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
#include "ban.hpp"
#include <nlohmann/json.hpp>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace objects
{

struct bans;

/// \cond TEMPLATES
void from_json(const nlohmann::json& j, bans& m);
void to_json(nlohmann::json& j, const bans& m);
/// \endcond

/// Stores info pertaining to guild bans
struct bans
{
    bans(const std::string& _json, aegis::core* bot) noexcept
    {
        from_json(nlohmann::json::parse(_json), *this);
    }

    bans(const nlohmann::json& _json, aegis::core* bot) noexcept
    {
        from_json(_json, *this);
    }

    bans(aegis::core* bot) noexcept {}

    bans() noexcept {}

    std::vector<aegis::gateway::objects::ban>::iterator begin()
    {
        return _bans.begin();
    }

    std::vector<aegis::gateway::objects::ban>::iterator end()
    {
        return _bans.end();
    }

    std::vector<aegis::gateway::objects::ban>::reverse_iterator rbegin()
    {
        return _bans.rbegin();
    }

    std::vector<aegis::gateway::objects::ban>::reverse_iterator rend()
    {
        return _bans.rend();
    }

    std::vector<aegis::gateway::objects::ban> _bans; /**< array of bans */

};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, bans& m)
{
    if (j.size())
        for (const auto& _ban : j)
            m._bans.push_back(_ban);
}

inline void to_json(nlohmann::json& j, const bans& m)
{
    if (!m._bans.empty())
        for (const auto& _ban : m._bans)
            j.push_back(_ban);
}
/// \endcond

}

}

}
