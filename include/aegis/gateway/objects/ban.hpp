//
// ban.hpp
// ********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/permission.hpp"
#include "aegis/user.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace aegis
{

    namespace gateway
    {

        namespace objects
        {

            struct ban;

            /// \cond TEMPLATES
            void from_json(const nlohmann::json& j, ban& m);
            void to_json(nlohmann::json& j, const ban& m);
            /// \endcond

            /// Stores info pertaining to guild ban
            struct ban
            {
                ban(const std::string& _json, aegis::core* bot) noexcept
                {
                    from_json(nlohmann::json::parse(_json), *this);
                }

                ban(const nlohmann::json& _json, aegis::core* bot) noexcept
                {
                    from_json(_json, *this);
                }

                ban(aegis::core* bot) noexcept {}

                ban() noexcept {}

                std::string reason;
                user _user;
            };

            /// \cond TEMPLATES
            inline void from_json(const nlohmann::json& j, ban& m)
            {
                if (j.count("reason") && j["reason"].is_string())
                    m.reason = j["reason"];
                if (j.count("user") && j["user"].is_object())
                    m._user = j["user"];
            }

            inline void to_json(nlohmann::json& j, const ban& m)
            {
                j["reason"] = m.reason;
                j["user"] = m._user;
            }
            /// \endcond

        }

    }

}
