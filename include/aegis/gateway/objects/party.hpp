//
// party.hpp
// *********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
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
struct party
{
    std::string id;
    int32_t current_size = 0;
    int32_t max_size = 0;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, party& m)
{
    if (j.count("id") && !j["id"].is_null())
        m.id = j["id"].get<std::string>();
    if (j.count("size") && !j["size"].is_null() && j.is_array() && j.size() == 2)
    {
        const auto & size = j["size"];
        m.current_size = size.at(0).get<int32_t>();
        m.max_size = size.at(1).get<int32_t>();
    }
}

inline void to_json(nlohmann::json& j, const party& m)
{
    if (!m.id.empty())
        j["id"] = m.id;
    if (m.current_size != 0 && m.max_size != 0)
    {
        auto & size = j["size"] = nlohmann::json::array();
        size.at(0) = m.current_size;
        size.at(1) = m.max_size;
    }
}
/// \endcond

}

}

}
