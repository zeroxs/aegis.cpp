//
// user.hpp
// ********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include <nlohmann/json.hpp>

namespace aegis
{

class user;
class channel;

}

namespace aegis
{

namespace gateway
{

namespace objects
{

/**\todo Needs documentation
 */
struct user
{
    /// Returns whether user is a bot
    /**
     * @returns true if user is a bot
     */
    bool is_bot() const noexcept
    {
        return _is_bot;
    }

    /// Set user bot flag
    /**
     * @param isbot true if user is a bot
     */
    void is_bot(bool isbot) noexcept
    {
        _is_bot = isbot;
    }

    /// Returns whether user is a webhook
    /**
     * @returns true if user is a webhook
     */
    bool is_webhook() const noexcept
    {
        return (_is_bot) && (discriminator == "0000" || discriminator == "0");
    }

    snowflake id; /**< snowflake of user */
    snowflake guild_id; /**< snowflake of guild of the event this user is attached to */
    std::string username; /**< username of user */
    //todo: make discriminator an integer
    std::string discriminator; /**< discriminator of user */
    std::string avatar; /**< Hash of user's avatar */
private:
    friend void from_json(const nlohmann::json& j, user& m);
    friend void to_json(nlohmann::json& j, const user& m);
    bool _is_bot = false;
    bool mfa_enabled = false;
    bool verified = false;
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, user& m)
{
    m.id = std::stoull(j["id"].get<std::string>());
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m.guild_id = std::stoull(j["guild_id"].get<std::string>());
    if (j.count("username") && !j["username"].is_null())
        m.username = j["username"].get<std::string>();
    if (j.count("discriminator") && !j["discriminator"].is_null())
        m.discriminator = j["discriminator"].get<std::string>();
    if (j.count("avatar") && !j["avatar"].is_null())
        m.avatar = j["avatar"].get<std::string>();
    if (j.count("bot") &&  !j["bot"].is_null())
        m._is_bot = j["bot"].get<bool>();
    if (j.count("mfa_enabled") && !j["mfa_enabled"].is_null())
        m.mfa_enabled = j["mfa_enabled"];
    if (j.count("verified") && !j["verified"].is_null())
        m.verified = j["verified"].get<bool>();
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const user& m)
{
    j["id"] = std::to_string(m.id);
    j["guild_id"] = std::to_string(m.guild_id);
    j["username"] = m.username;
    j["discriminator"] = m.discriminator;
    j["bot"] = m._is_bot;
    j["mfa_enabled"] = m.mfa_enabled;
    j["verified"] = m.verified;
}
/// \endcond

}

}

}
