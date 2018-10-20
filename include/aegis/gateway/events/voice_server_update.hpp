//
// voice_server_update.hpp
// ***********************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct voice_server_update
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    std::string token;
    snowflake guild_id;
    std::string endpoint;
};

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, voice_server_update& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/voice_server_update.cpp"
#endif
