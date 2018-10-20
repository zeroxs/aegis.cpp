//
// secret.hpp
// **********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Incomplete. Needs documentation
 */
struct secret
{
    std::string join;
    std::string spectate;
    std::string match;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, secret& m)
{
    if (j.count("join") && !j["join"].is_null())
        m.join = j["join"].get<std::string>();
    if (j.count("spectate") && !j["spectate"].is_null())
        m.spectate = j["spectate"].get<std::string>();
    if (j.count("match") && !j["match"].is_null())
        m.match = j["match"].get<std::string>();
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const secret& m)
{
    if (!m.join.empty())
        j["join"] = m.join;
    if (!m.spectate.empty())
        j["spectate"] = m.spectate;
    if (!m.match.empty())
        j["match"] = m.match;
}
/// \endcond

}

}

}
