//
// user.hpp
// ********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "attachment.hpp"
#include "embed.hpp"
#include "reaction.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace aegis
{

class member;
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

    snowflake user_id; /**< snowflake of user */
    snowflake guild_id; /**< snowflake of guild of the event this user is attached to */
    std::string username; /**< username of user */
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
    m.user_id = j["id"];
    if (j.count("guild_id") && !j["guild_id"].is_null())
        m.guild_id = j["guild_id"];
    if (j.count("username") && !j["username"].is_null())
        m.username = j["username"];
    if (j.count("discriminator") && !j["discriminator"].is_null())
        m.discriminator = j["discriminator"];
    if (j.count("avatar") && !j["avatar"].is_null())
        m.avatar = j["avatar"];
    if (j.count("bot") &&  !j["bot"].is_null())
        m._is_bot = j["bot"];
    if (j.count("mfa_enabled") && !j["mfa_enabled"].is_null())
        m.mfa_enabled = j["mfa_enabled"];
    if (j.count("verified") && !j["verified"].is_null())
        m.verified = j["verified"];
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const user& m)
{
    j["id"] = m.user_id;
    j["guild_id"] = m.guild_id;
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
