//
// resumed.cpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/events/resumed.hpp"
#include <nlohmann/json.hpp>


namespace aegis
{

namespace gateway
{

namespace events
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, resumed& m)
{
    if (j.count("_trace") && !j["_trace"].is_null())
        for (const auto & i : j["_trace"])
            m._trace.push_back(i);
}

/// \cond TEMPLATES
AEGIS_DECL void to_json(nlohmann::json& j, const resumed& m)
{
    if (!m._trace.empty())
        for (auto i : m._trace)
            j["_trace"].push_back(i);
}
/// \endcond

}

}

}
