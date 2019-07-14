//
// snowflake.cpp
// *************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include <nlohmann/json.hpp>

#include "aegis/guild.hpp"
#include "aegis/channel.hpp"
#include "aegis/gateway/objects/role.hpp"
#include "aegis/gateway/objects/message.hpp"
#include "aegis/gateway/objects/emoji.hpp"
#include "aegis/gateway/objects/attachment.hpp"

namespace aegis
{

snowflake::snowflake(const aegis::guild & _guild) noexcept : _id(_guild.get_id()) {}
snowflake::snowflake(const aegis::channel & _channel) noexcept : _id(_channel.get_id()) {}
snowflake::snowflake(const aegis::gateway::objects::role & _role) noexcept : _id(_role.role_id) {}
snowflake::snowflake(const aegis::gateway::objects::message & _message) noexcept : _id(_message.get_id()) {}
snowflake::snowflake(const aegis::gateway::objects::emoji & _emoji) noexcept : _id(_emoji.id) {}
snowflake::snowflake(const aegis::gateway::objects::attachment & _attachment) noexcept : _id(_attachment.id) {}

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, snowflake& s)
{
    if (j.is_string())
        s = std::stoll(j.get<std::string>());
    else if (j.is_number())
        s = j.get<int64_t>();
}

AEGIS_DECL void to_json(nlohmann::json& j, const snowflake& s)
{
    j = nlohmann::json{ static_cast<int64_t>(s) };
}
/// \endcond

}
