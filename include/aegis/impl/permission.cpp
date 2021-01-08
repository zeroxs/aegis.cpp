//
// permission.cpp
// **************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/permission.hpp"
#include <nlohmann/json.hpp>
#include <stdint.h>

namespace aegis
{

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, permission& s)
{
    if (j.is_string())
        s = std::stoll(j.get<std::string>());
    else if (j.is_number())
        s = j.get<int64_t>();
}

AEGIS_DECL void to_json(nlohmann::json& j, const permission& s)
{
    j = std::to_string(static_cast<int64_t>(s));
}
/// \endcond

AEGIS_DECL const std::unordered_map<int64_t, const std::string> permission::perm_strs {
    {0x1, "Create invites"},
    {0x2, "Kick members"},
    {0x4, "Ban members"},
    {0x8, "Administrator"},
    {0x10, "Manage channels"},
    {0x20, "Manage server"},
    {0x40, "Add reactions"},
    {0x80, "View audit log"},
    {0x400, "Read messages"},
    {0x800, "Send messages"},
    {0x1000, "Use TTS"},
    {0x2000, "Manage messages"},
    {0x4000, "Send embeds"},
    {0x8000, "Attach files"},
    {0x10000, "Read message history"},
    {0x20000, "Mention everyone"},
    {0x40000, "Use external emoji"},
    {0x4000000, "Change nickname"},
    {0x8000000, "Manage nicknames"},
    {0x10000000, "Manage roles"},
    {0x20000000, "Manage webhooks"},
    {0x40000000, "Manage emojis"},
    {0x100000, "Connect to VC"},
    {0x400000, "Server mute"},
    {0x200000, "Speak in VC"},
    {0x800000, "Server deafen"},
    {0x1000000, "Move VC members"},
    {0x2000000, "Use voice activity"},
    {0x100, "Priority speaker"}
};

}
