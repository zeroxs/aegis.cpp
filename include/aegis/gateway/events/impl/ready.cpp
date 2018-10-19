//
// ready.cpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//


#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/ready.hpp"
#include <nlohmann/json.hpp>


namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, ready& m)
{
    m._user = j["user"];
    if (j.count("private_channels") && !j["private_channels"].is_null())
        for (const auto & i : j["private_channels"])
            m.private_channels.push_back(i);
    if (j.count("guilds") && !j["guilds"].is_null())
        for (const auto & i : j["guilds"])
            m.guilds.push_back(i);
    if (j.count("_trace") && !j["_trace"].is_null())
        for (const auto & i : j["_trace"])
            m._trace.push_back(i);
}
/// \endcond

/// \cond TEMPLATES
AEGIS_DECL void to_json(nlohmann::json& j, const ready& m)
{
    j["user"] = m._user;
    if (!m.private_channels.empty())
        for (const auto & i : m.private_channels)
            j["private_channels"].push_back(i);
    if (!m.guilds.empty())
        for (const auto & i : m.guilds)
            j["guilds"].push_back(i);
    if (!m._trace.empty())
        for (const auto & i : m._trace)
            j["_trace"].push_back(i);
}
/// \endcond

}

}

}