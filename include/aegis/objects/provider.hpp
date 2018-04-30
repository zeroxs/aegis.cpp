//
// provider.hpp
// ************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

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

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, provider& m)
{
    if (j.count("name") && !j["name"].is_null())
        m.name = j["name"];
    if (j.count("url") && !j["url"].is_null())
        m.url = j["url"];
}

/**\todo Needs documentation
 */
inline void to_json(nlohmann::json& j, const provider& m)
{
    j["name"] = m.name;
    j["url"] = m.url;
}

}

}

}
