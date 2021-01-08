//
// invite.hpp
// *********
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

namespace gateway
{

namespace objects
{

enum target_user_type {
    STREAM = 1
};

struct invite_metadata_t {
    int32_t uses; /**< # of times this invite has been used */
    int32_t max_uses; /**< Max # of times this invite can be used */
    int32_t max_age; /**< Duration (in seconds) after which the invite expires */
    bool temporary = false; /**< Whether this invite only grants temporary membership */
    std::string created_at; /**< ISO8601 timestamp of when this invite was created */
};

struct invite;

/// \cond TEMPLATES
void from_json(const nlohmann::json& j, invite& m);
void to_json(nlohmann::json& j, const invite& m);
/// \endcond

/// Discord Invite Object
struct invite
{
    invite(const std::string& _json, aegis::core* bot) noexcept
    {
        from_json(nlohmann::json::parse(_json), *this);
    }

    invite(const nlohmann::json& _json, aegis::core* bot) noexcept
    {
        from_json(_json, *this);
    }

    invite(aegis::core* bot) noexcept {}

    invite() noexcept {}

    std::string code; /**< Invite code (unique ID) */
    snowflake _guild; /**< Guild this invite is for */
    snowflake _channel; /**< Channel this invite is for */
    snowflake inviter; /**< User that created the invite */
    snowflake target_user; /**< Target user for this invite */
    target_user_type target_type = STREAM; /**< Type of user target for this invite */
    int32_t approximate_presence_count = 0; /**< Approximate count of online members of guild, requires target_user be set */
    int32_t approximate_member_count = 0; /**< Approximate count of total members of guild */
    invite_metadata_t metadata; /**< Extra information about invite */
};

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, invite& m)
{
    m.code = j["code"];
    if (j.count("guild") && !j["guild"].is_null())
        m._guild = j["guild"]["id"];
    if (j.count("channel") && !j["channel"].is_null())
        m._channel = j["channel"]["id"];
    if (j.count("inviter") && !j["inviter"].is_null())
        m.inviter = j["inviter"]["id"];
    if (j.count("target_user") && !j["target_user"].is_null())
        m.target_user = j["target_user"]["id"];
    if (j.count("target_user_type") && !j["target_user_type"].is_null())
        m.target_type = j["target_user_type"];
    if (j.count("approximate_presence_count") && !j["approximate_presence_count"].is_null())
        m.approximate_presence_count = j["approximate_presence_count"];
    if (j.count("approximate_member_count") && !j["approximate_member_count"].is_null())
        m.approximate_member_count = j["approximate_member_count"];

    // Metadata
    if (j.count("uses") && !j["uses"].is_null())
        m.metadata.uses = j["uses"];
    if (j.count("max_uses") && !j["max_uses"].is_null())
        m.metadata.max_uses = j["max_uses"];
    if (j.count("max_age") && !j["max_age"].is_null())
        m.metadata.max_age = j["max_age"];
    if (j.count("temporary") && !j["temporary"].is_null())
        m.metadata.temporary = j["temporary"];
    if (j.count("created_at") && !j["created_at"].is_null())
        m.metadata.created_at = j["created_at"];
}

inline void to_json(nlohmann::json& j, const invite& m)
{
    j["code"] = m.code;
    j["guild"] = m._guild;
    j["channel"] = m._channel;
    j["inviter"] = m.inviter;
    j["target_user"] = m.target_user;
    j["target_user_type"] = m.target_type;
    j["approximate_presence_count"] = m.approximate_presence_count;
    j["approximate_member_count"] = m.approximate_member_count;
    
    // Metadata
    j["uses"] = m.metadata.uses;
    j["max_uses"] = m.metadata.max_uses;
    j["max_age"] = m.metadata.max_age;
    j["temporary"] = m.metadata.temporary;
    j["created_at"] = m.metadata.created_at;
}
/// \endcond

}

}

}
