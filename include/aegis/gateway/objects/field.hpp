//
// field.hpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Needs documentation
 */
struct field
{
    field(std::string n, std::string v) : name(n), value(v) {}
    field(std::string n, std::string v, bool il) : name(n), value(v), is_inline(il) {}
    field() = default;
    std::string name; /**<\todo Needs documentation */
    std::string value; /**<\todo Needs documentation */
    bool is_inline = false; /**<\todo Needs documentation */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, field& m)
{
    if (j.count("name"))
        m.name = j["name"];
    if (j.count("value"))
        m.value = j["value"];
    if (j.count("inline"))
        m.is_inline = j["inline"];
}

inline void to_json(nlohmann::json& j, const field& m)
{
    j["name"] = m.name;
    j["value"] = m.value;
    j["inline"] = m.is_inline;
}
/// \endcond

}

}

}
