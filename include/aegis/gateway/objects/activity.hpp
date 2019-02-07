//
// activity.hpp
// ************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/timestamp.hpp"
#include "aegis/gateway/objects/party.hpp"
#include "aegis/gateway/objects/asset.hpp"
#include "aegis/gateway/objects/secret.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Incomplete. Needs documentation
 */
struct activity
{
    enum activity_type
    {
        Game = 0,
        Streaming = 1,
        Listening = 2,
        Watching = 3
    };
    enum activity_flags
    {
        Null = 0,
        Instance = 1,
        Join = 1 << 1,
        Spectate = 1 << 2,
        JoinRequest = 1 << 3,
        Sync = 1 << 4,
        Play = 1 << 5
    };
    std::string name;
    activity_type type = activity_type::Game;
    std::string url;
    objects::timestamp timestamps;
    snowflake application_id;
    std::string details;
    std::string state;
    objects::party party_data;
    objects::asset assets;
    objects::secret secrets;
    bool instance;
    activity_flags flags;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, activity& m)
{
    if (j.count("name") && !j["name"].is_null())
        m.name = j["name"].get<std::string>();
    if (j.count("type") && !j["type"].is_null())
        m.type = static_cast<objects::activity::activity_type>(j["type"].get<int32_t>());
    if (j.count("url") && !j["url"].is_null())
        m.url = j["url"].get<std::string>();
    if (j.count("timestamps") && !j["timestamps"].is_null())
        m.timestamps = j["timestamps"];
    if (j.count("party") && !j["party"].is_null())
        m.party_data = j["party"];
    if (j.count("assets") && !j["assets"].is_null())
        m.assets = j["assets"];
    if (j.count("secrets") && !j["secrets"].is_null())
        m.secrets = j["secrets"];
    if (j.count("application_id") && !j["application_id"].is_null())
        m.application_id = j["application_id"];
    if (j.count("details") && !j["details"].is_null())
        m.details = j["details"].get<std::string>();
    if (j.count("state") && !j["state"].is_null())
        m.state = j["state"].get<std::string>();
    if (j.count("instance") && !j["instance"].is_null())
        m.instance = j["instance"];
    if (j.count("flags") && !j["flags"].is_null())
        m.flags = j["flags"];
}

inline void to_json(nlohmann::json& j, const activity& m)
{
    j["name"] = m.name;
    j["type"] = static_cast<int32_t>(m.type);
    if (!m.name.empty())
    {
        j["flags"] = static_cast<int32_t>(m.flags);
        if (!m.url.empty())
            if (m.type == activity::Streaming || m.type == activity::Watching)
                j["url"] = m.url;
        if (m.type == activity::Game)
        {
            if (!m.details.empty())
                j["details"] = m.details;
            if (!m.state.empty())
                j["state"] = m.state;
            j["party"] = m.party_data;
            j["assets"] = m.assets;
            j["secrets"] = m.secrets;
            j["application_id"] = m.application_id;
            j["instance"] = m.instance;
        }
    }
}
/// \endcond

}

}

}
