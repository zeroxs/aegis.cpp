//
// ready.hpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//


#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/user.hpp"
#include "aegis/objects/channel.hpp"
#include "aegis/objects/guild.hpp"
#include "aegis/fwd.hpp"
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct ready 
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    int8_t v = 0; /**<\todo Needs documentation */
    objects::user _user; /**<\todo Needs documentation */
    std::vector<objects::channel> private_channels; /**<\todo Needs documentation */
    std::vector<objects::guild> guilds; /**<\todo Needs documentation */
    std::string session_id; /**<\todo Needs documentation */
    std::vector<std::string> _trace; /**<\todo Needs documentation */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, ready& m)
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
inline void to_json(nlohmann::json& j, const ready& m)
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
