//
// user_update.hpp
// ***************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/user.hpp"
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
struct user_update
{
    objects::user _user; /**<\todo Needs documentation */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    explicit user_update(member * m) : _member(m) {}
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    member * const _member = nullptr; /**<\todo Needs documentation */
#endif
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, user_update& m)
{
    m._user = j;
}
/// \endcond

}

}

}
