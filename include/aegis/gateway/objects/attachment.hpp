//
// attachment.hpp
// **************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Needs documentation
 */
struct attachment
{
    snowflake id; /**<\todo Needs documentation */
    std::string filename; /**<\todo Needs documentation */
    int32_t size = 0; /**<\todo Needs documentation */
    std::string url; /**<\todo Needs documentation */
    std::string proxy_url; /**<\todo Needs documentation */
    int32_t height = 0; /**<\todo Needs documentation */
    int32_t width = 0; /**<\todo Needs documentation */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, attachment& m)
{
    if (j.count("id"))
        m.id = j["id"];
    if (j.count("filename"))
        m.filename = j["filename"].get<std::string>();
    if (j.count("size"))
        m.size = j["size"];
    if (j.count("url"))
        m.url = j["url"].get<std::string>();
    if (j.count("proxy_url"))
        m.proxy_url = j["proxy_url"].get<std::string>();
    if (j.count("height") && !j["height"].is_null())
        m.height = j["height"];
    if (j.count("width") && !j["width"].is_null())
        m.width = j["width"];
}

inline void to_json(nlohmann::json& j, const attachment& m)
{
    j["id"] = m.id;
    j["filename"] = m.filename;
    j["size"] = m.size;
    j["url"] = m.url;
    j["proxy_url"] = m.proxy_url;
    j["height"] = m.height;
    j["width"] = m.width;
}
/// \endcond

}

}

}
