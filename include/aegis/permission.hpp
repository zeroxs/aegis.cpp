//
// permission.hpp
// aegis.cpp
//
// Copyright (c) 2017 Sara W (sara at xandium dot net)
//
// This file is part of aegis.cpp .
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once


#include "aegis/config.hpp"
#include <stdint.h>


namespace aegiscpp
{

/**\todo Needs documentation
*/
class permission
{
public:
    permission() = default;
    permission(int64_t allow) : _allow_permissions(allow) {}
    //permission(const permission&) = delete;

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

private:
    int64_t _allow_permissions = 0;
};

}
