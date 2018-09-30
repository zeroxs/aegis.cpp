//
// resumed.hpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/message.hpp"
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
struct resumed
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    std::vector<std::string> _trace; /**<\todo Needs documentation */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, resumed& m)
{
    if (j.count("_trace") && !j["_trace"].is_null())
        for (const auto & i : j["_trace"])
            m._trace.push_back(i);
}

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const resumed& m)
{
    if (!m._trace.empty())
        for (auto i : m._trace)
            j["_trace"].push_back(i);
}
/// \endcond

}

}

}
