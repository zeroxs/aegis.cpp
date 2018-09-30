//
// asset.hpp
// *********
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

/**\todo Incomplete. Needs documentation
 */
struct asset
{
    std::string large_image;
    std::string large_text;
    std::string small_image;
    std::string small_text;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, asset& m)
{
    if (j.count("large_image") && !j["large_image"].is_null())
        m.large_image = j["large_image"].get<std::string>();
    if (j.count("large_text") && !j["large_text"].is_null())
        m.large_text = j["large_text"].get<std::string>();
    if (j.count("small_image") && !j["small_image"].is_null())
        m.small_image = j["small_image"].get<std::string>();
    if (j.count("small_text") && !j["small_text"].is_null())
        m.small_text = j["small_text"].get<std::string>();
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const asset& m)
{
    if (!m.large_image.empty())
        j["large_image"] = m.large_image;
    if (!m.large_text.empty())
        j["large_text"] = m.large_text;
    if (!m.small_image.empty())
        j["small_image"] = m.small_image;
    if (!m.small_text.empty())
        j["small_text"] = m.small_text;
}
/// \endcond

}

}

}
