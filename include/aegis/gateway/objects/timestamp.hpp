//
// timestamp.hpp
// *************
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

/**\todo Incomplete. Needs documentation
 */
struct timestamp
{
    int64_t start;
    int64_t end;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, timestamp& m)
{
    if (j.count("start") && !j["start"].is_null())
        m.start = j["start"];
    if (j.count("end") && !j["end"].is_null())
        m.end = j["end"];
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const timestamp& m)
{
    if (m.start != 0)
        j["start"] = m.start;
    if (m.end != 0)
        j["end"] = m.end;
}
/// \endcond

}

}

}
