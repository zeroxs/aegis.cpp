//
// voice_state.hpp
// ***************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/member.hpp"
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
struct voice_state
{
    snowflake guild_id;
    snowflake channel_id;
    snowflake user_id;
    objects::member * _member = nullptr;
    std::string session_id;
    bool deaf = false;
    bool mute = false;
    bool self_deaf = false;
    bool self_mute = false;
    bool suppress = false;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, voice_state& m)
{

}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const voice_state& m)
{

}
/// \endcond

}

}

}
