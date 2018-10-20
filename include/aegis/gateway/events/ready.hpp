//
// ready.hpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//


#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/gateway/objects/user.hpp"
#include "aegis/gateway/objects/channel.hpp"
#include "aegis/gateway/objects/guild.hpp"

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct ready 
{
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    int8_t v = 0; /**<\todo Needs documentation */
    objects::user _user; /**<\todo Needs documentation */
    std::vector<objects::channel> private_channels; /**<\todo Needs documentation */
    std::vector<objects::guild> guilds; /**<\todo Needs documentation */
    std::string session_id; /**<\todo Needs documentation */
    std::vector<std::string> _trace; /**<\todo Needs documentation */
};

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, ready& m);

AEGIS_DECL void to_json(nlohmann::json& j, const ready& m);
/// \endcond

}

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/gateway/events/impl/ready.cpp"
#endif
