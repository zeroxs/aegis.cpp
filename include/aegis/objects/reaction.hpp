//
// reaction.hpp
// ************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "emoji.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Needs documentation
 */
struct reaction
{
    int32_t count = 0; /**<\todo Needs documentation */
    bool me = false; /**<\todo Needs documentation */
    emoji emoji_; /**<\todo Needs documentation */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, reaction& m)
{
    m.count = j["count"];
    m.me = j["me"];
    m.emoji_ = j["emoji_"];
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const reaction& m)
{
    j["count"] = m.count;
    j["me"] = m.me;
    j["emoji_"] = m.emoji_;
}
/// \endcond

}

}

}
