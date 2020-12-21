//
// permission.hpp
// **************
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include <stdint.h>

namespace aegis
{

/*
I made this, but then i didn't need it. It is here now, if it becomes needed in the future
enum permissions : int64_t {
    CREATE_INSTANT_INVITE = 0x00000001,
    KICK_MEMBERS = 0x00000002,
    BAN_MEMBERS = 0x00000004,
    ADMINISTRATOR = 0x00000008,
    MANAGE_CHANNELS = 0x00000010,
    MANAGE_GUILD = 0x00000020,
    ADD_REACTIONS = 0x00000040,
    VIEW_AUDIT_LOG = 0x00000080,
    VIEW_CHANNEL = 0x00000400,
    SEND_MESSAGES = 0x00000800,
    SEND_TTS_MESSAGES = 0x00001000,
    MANAGE_MESSAGES = 0x00002000,
    EMBED_LINKS = 0x00004000,
    ATTACH_FILES = 0x00008000,
    READ_MESSAGE_HISTORY = 0x00010000,
    MENTION_EVERYONE = 0x00020000,
    USE_EXTERNAL_EMOJIS = 0x00040000,
    VIEW_GUILD_INSIGHTS = 0x00080000,
    CONNECT = 0x00100000,
    SPEAK = 0x00200000,
    MUTE_MEMBERS = 0x00400000,
    DEAFEN_MEMBERS = 0x00800000,
    MOVE_MEMBERS = 0x01000000,
    USE_VAD = 0x02000000,
    PRIORITY_SPEAKER = 0x00000100,
    STREAM = 0x00000200,
    CHANGE_NICKNAME = 0x04000000,
    MANAGE_NICKNAMES = 0x08000000,
    MANAGE_ROLES = 0x10000000,
    MANAGE_WEBHOOKS = 0x20000000,
    MANAGE_EMOJIS = 0x40000000
};
*/

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

    /// Gets the names of allowed permissions
    /**
    * @returns string vector of allowed permissions' names
    */
    std::vector<std::string> perms_to_strs() {
        std::vector<std::string> out;
        for (auto& pair : perm_strs) {
            if ((_allow_permissions & pair.first) > 0) out.push_back(pair.second);
        }
        return out;
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
    bool has_priority_speaker() const noexcept { return (_allow_permissions & 0x100) > 0; }

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
    static const std::unordered_map<int64_t, const std::string> perm_strs;
};

const std::unordered_map<int64_t, const std::string> permission::perm_strs {
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

/// \cond TEMPLATES
AEGIS_DECL void from_json(const nlohmann::json& j, permission& s);

AEGIS_DECL void to_json(nlohmann::json& j, const permission& s);
/// \endcond

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/impl/permission.cpp"
#endif
