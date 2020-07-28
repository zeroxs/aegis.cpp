//
// provider.hpp
// ************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
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

/**\todo Needs documentation
 */
struct provider
{
    std::string name; /**<\todo Needs documentation */
    std::string url; /**<\todo Needs documentation */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, provider& m)
{
    if (j.count("name") && !j["name"].is_null())
        m.name = j["name"].get<std::string>();
    if (j.count("url") && !j["url"].is_null())
        m.url = j["url"].get<std::string>();
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const provider& m)
{
    j["name"] = m.name;
    j["url"] = m.url;
}
/// \endcond

}

}

}
