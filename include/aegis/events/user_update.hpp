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
#include "base_event.hpp"
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
struct user_update : public base_event
{
    objects::user _user; /**<\todo Needs documentation */
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    user_update(member * m) : _member(m) {};
    member * const _member; /**<\todo Needs documentation */
#endif
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, user_update& m)
{
    m._user = j;
}

}

}

}
