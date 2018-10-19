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
#include "aegis/fwd.hpp"

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
AEGIS_DECL void from_json(const nlohmann::json& j, resumed& m);

AEGIS_DECL void to_json(nlohmann::json& j, const resumed& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/resumed.cpp"
#endif
