//
// permission.hpp
// **************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include <nlohmann/json.hpp>
#include <stdint.h>

namespace aegis
{

/// Utility class for permission checks
class permission
{
public:
    permission() = default;
    permission(int64_t allow) : _allow_permissions(allow) {}
    //permission(const permission&) = delete;

    operator int64_t() const noexcept
    {
        return _allow_permissions;
    }

    int64_t get_allow_perms() const noexcept { return _allow_permissions; }
    bool can_invite() const noexcept { return (_allow_permissions & 0x1) > 0; }
    bool can_kick() const noexcept { return (_allow_permissions & 0x2) > 0; }
    bool can_ban() const noexcept { return (_allow_permissions & 0x4) > 0; }
    bool is_admin() const noexcept { return (_allow_permissions & 0x8) > 0; }
    bool can_manage_channels() const noexcept { return (_allow_permissions & 0x10) > 0; }
    bool can_manage_guild() const noexcept { return (_allow_permissions & 0x20) > 0; }
    bool can_add_reactions() const noexcept { return (_allow_permissions & 0x40) > 0; }
    bool can_view_audit_logs() const noexcept { return (_allow_permissions & 0x80) > 0; }
    bool can_read_messages() const noexcept { return (_allow_permissions & 0x400) > 0; }
    bool can_send_messages() const noexcept { return (_allow_permissions & 0x800) > 0; }
    bool can_tts() const noexcept { return (_allow_permissions & 0x1000) > 0; }
    bool can_manage_messages() const noexcept { return (_allow_permissions & 0x2000) > 0; }
    bool can_embed() const noexcept { return (_allow_permissions & 0x4000) > 0; }
    bool can_attach_files() const noexcept { return (_allow_permissions & 0x8000) > 0; }
    bool can_read_history() const noexcept { return (_allow_permissions & 0x10000) > 0; }
    bool can_mention_everyone() const noexcept { return (_allow_permissions & 0x20000) > 0; }
    bool can_external_emoiji() const noexcept { return (_allow_permissions & 0x40000) > 0; }
    bool can_change_name() const noexcept { return (_allow_permissions & 0x4000000) > 0; }
    bool can_manage_names() const noexcept { return (_allow_permissions & 0x8000000) > 0; }
    bool can_manage_roles() const noexcept { return (_allow_permissions & 0x10000000) > 0; }
    bool can_manage_webhooks() const noexcept { return (_allow_permissions & 0x20000000) > 0; }
    bool can_manage_emojis() const noexcept { return (_allow_permissions & 0x40000000) > 0; }

    bool can_voice_connect() const noexcept { return (_allow_permissions & 0x100000) > 0; }
    bool can_voice_mute() const noexcept { return (_allow_permissions & 0x400000) > 0; }
    bool can_voice_speak() const noexcept { return (_allow_permissions & 0x200000) > 0; }
    bool can_voice_deafen() const noexcept { return (_allow_permissions & 0x800000) > 0; }
    bool can_voice_move() const noexcept { return (_allow_permissions & 0x1000000) > 0; }
    bool can_voice_activity() const noexcept { return (_allow_permissions & 0x2000000) > 0; }

    void set_invite() noexcept { _allow_permissions = (_allow_permissions & 0x1); }
    void set_kick() noexcept { _allow_permissions = (_allow_permissions & 0x2); }
    void set_ban() noexcept { _allow_permissions = (_allow_permissions & 0x4); }
    void set_admin() noexcept { _allow_permissions = (_allow_permissions & 0x8); }
    void set_manage_channels() noexcept { _allow_permissions = (_allow_permissions & 0x10); }
    void set_manage_guild() noexcept { _allow_permissions = (_allow_permissions & 0x20); }
    void set_add_reactions() noexcept { _allow_permissions = (_allow_permissions & 0x40); }
    void set_view_audit_logs() noexcept { _allow_permissions = (_allow_permissions & 0x80); }
    void set_read_messages() noexcept { _allow_permissions = (_allow_permissions & 0x400); }
    void set_send_messages() noexcept { _allow_permissions = (_allow_permissions & 0x800); }
    void set_tts() noexcept { _allow_permissions = (_allow_permissions & 0x1000); }
    void set_manage_messages() noexcept { _allow_permissions = (_allow_permissions & 0x2000); }
    void set_embed() noexcept { _allow_permissions = (_allow_permissions & 0x4000); }
    void set_attach_files() noexcept { _allow_permissions = (_allow_permissions & 0x8000); }
    void set_read_history() noexcept { _allow_permissions = (_allow_permissions & 0x10000); }
    void set_mention_everyone() noexcept { _allow_permissions = (_allow_permissions & 0x20000); }
    void set_external_emoiji() noexcept { _allow_permissions = (_allow_permissions & 0x40000); }
    void set_change_name() noexcept { _allow_permissions = (_allow_permissions & 0x4000000); }
    void set_manage_names() noexcept { _allow_permissions = (_allow_permissions & 0x8000000); }
    void set_manage_roles() noexcept { _allow_permissions = (_allow_permissions & 0x10000000); }
    void set_manage_webhooks() noexcept { _allow_permissions = (_allow_permissions & 0x20000000); }
    void set_manage_emojis() noexcept { _allow_permissions = (_allow_permissions & 0x40000000); }

    void set_voice_connect() noexcept { _allow_permissions = (_allow_permissions & 0x100000); }
    void set_voice_mute() noexcept { _allow_permissions = (_allow_permissions & 0x400000); }
    void set_voice_speak() noexcept { _allow_permissions = (_allow_permissions & 0x200000); }
    void set_voice_deafen() noexcept { _allow_permissions = (_allow_permissions & 0x800000); }
    void set_voice_move() noexcept { _allow_permissions = (_allow_permissions & 0x1000000); }
    void set_voice_activity() noexcept { _allow_permissions = (_allow_permissions & 0x2000000); }

private:
    int64_t _allow_permissions = 0;
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, permission& s)
{
    if (j.is_string())
        s = std::stoll(j.get<std::string>());
    else if (j.is_number())
        s = j.get<int64_t>();
}

/**\todo Needs documentation
 */
inline void to_json(nlohmann::json& j, const permission& s)
{
    j = nlohmann::json{ static_cast<int64_t>(s) };
}

}
