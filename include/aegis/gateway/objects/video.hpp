//
// video.hpp
// *********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
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
struct video
{
    std::string url; /**<\todo Needs documentation */
    int32_t height = 0; /**<\todo Needs documentation */
    int32_t width = 0; /**<\todo Needs documentation */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, video& m)
{
    if (j.count("url"))
        m.url = j["url"].get<std::string>();
    if (j.count("height") && !j["height"].is_null())
        m.height = j["height"];
    if (j.count("width") && !j["width"].is_null())
        m.width = j["width"];
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const video& m)
{
    j["url"] = m.url;
    j["height"] = m.height;
    j["width"] = m.width;
}
/// \endcond

}

}

}
