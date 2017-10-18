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


namespace aegis
{

class permission
{
public:
    permission() {}
    permission(int64_t allow) : _allow_permissions(allow) {}
    permission(int64_t allow, int64_t deny) : _allow_permissions(allow), _deny_permissions(deny) {}
    //permission(const permission&) = delete;

    int64_t getAllowPerms() { return _allow_permissions; }
    int64_t getDenyPerms() { return _deny_permissions; }
    bool canInvite() { return (_allow_permissions & 0x1) > 0; }
    bool canKick() { return (_allow_permissions & 0x2) > 0; }
    bool canBan() { return (_allow_permissions & 0x4) > 0; }
    bool isAdmin() { return (_allow_permissions & 0x8) > 0; }
    bool canManageChannels() { return (_allow_permissions & 0x10) > 0; }
    bool canManageGuild() { return (_allow_permissions & 0x20) > 0; }
    bool canAddReactions() { return (_allow_permissions & 0x40) > 0; }
    bool canViewAuditLogs() { return (_allow_permissions & 0x80) > 0; }
    bool canReadMessages() { return (_allow_permissions & 0x400) > 0; }
    bool canSendMessages() { return (_allow_permissions & 0x800) > 0; }
    bool canTTS() { return (_allow_permissions & 0x1000) > 0; }
    bool canManageMessages() { return (_allow_permissions & 0x2000) > 0; }
    bool canEmbed() { return (_allow_permissions & 0x4000) > 0; }
    bool canAttachFiles() { return (_allow_permissions & 0x8000) > 0; }
    bool canReadHistory() { return (_allow_permissions & 0x10000) > 0; }
    bool canMentionEveryone() { return (_allow_permissions & 0x20000) > 0; }
    bool canExternalEmoiji() { return (_allow_permissions & 0x40000) > 0; }
    bool canChangeName() { return (_allow_permissions & 0x4000000) > 0; }
    bool canManageNames() { return (_allow_permissions & 0x8000000) > 0; }
    bool canManageRoles() { return (_allow_permissions & 0x10000000) > 0; }
    bool canManageWebhooks() { return (_allow_permissions & 0x20000000) > 0; }
    bool canManageEmojis() { return (_allow_permissions & 0x40000000) > 0; }

    bool canVoiceConnect() { return (_allow_permissions & 0x100000) > 0; }
    bool canVoiceMute() { return (_allow_permissions & 0x400000) > 0; }
    bool canVoiceSpeak() { return (_allow_permissions & 0x200000) > 0; }
    bool canVoiceDeafen() { return (_allow_permissions & 0x800000) > 0; }
    bool canVoiceMove() { return (_allow_permissions & 0x1000000) > 0; }
    bool canVoiceActivity() { return (_allow_permissions & 0x2000000) > 0; }

private:
    int64_t _allow_permissions = 0;
    int64_t _deny_permissions = 0;
};

}
