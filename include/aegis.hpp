//
// aegis.hpp
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
#include "aegis/common.hpp"
#include "aegis/utility.hpp"
#include "aegis/state_c.hpp"

#include "aegis/snowflake.hpp"
#include "aegis/role.hpp"
#include "aegis/structs.hpp"
#include "aegis/ratelimit.hpp"
#include "aegis/error.hpp"

#include "aegis/member.hpp"
#include "aegis/shard.hpp"
#include "aegis/channel.hpp"
#include "aegis/guild.hpp"
#include "aegis/aegis.hpp"


#include "aegis/events/ready.hpp"
#include "aegis/events/resumed.hpp"
#include "aegis/events/typing_start.hpp"
#include "aegis/events/message_create.hpp"
#include "aegis/events/presence_update.hpp"
#include "aegis/events/channel_create.hpp"
#include "aegis/events/channel_delete.hpp"
#include "aegis/events/channel_pins_update.hpp"
#include "aegis/events/channel_update.hpp"
#include "aegis/events/guild_ban_add.hpp"
#include "aegis/events/guild_ban_remove.hpp"
#include "aegis/events/guild_create.hpp"
#include "aegis/events/guild_delete.hpp"
#include "aegis/events/guild_emojis_update.hpp"
#include "aegis/events/guild_integrations_update.hpp"
#include "aegis/events/guild_member_add.hpp"
#include "aegis/events/guild_member_remove.hpp"
#include "aegis/events/guild_member_update.hpp"
#include "aegis/events/guild_members_chunk.hpp"
#include "aegis/events/guild_role_create.hpp"
#include "aegis/events/guild_role_delete.hpp"
#include "aegis/events/guild_role_update.hpp"
#include "aegis/events/guild_update.hpp"
#include "aegis/events/message_delete.hpp"
#include "aegis/events/message_delete_bulk.hpp"
#include "aegis/events/message_reaction_add.hpp"
#include "aegis/events/message_reaction_remove.hpp"
#include "aegis/events/message_reaction_remove_all.hpp"
#include "aegis/events/message_update.hpp"
#include "aegis/events/user_update.hpp"
#include "aegis/events/voice_server_update.hpp"
#include "aegis/events/voice_state_update.hpp"
#include "aegis/events/webhooks_update.hpp"



#include "aegis/member_impl.hpp"
#include "aegis/shard_impl.hpp"
#include "aegis/channel_impl.hpp"
#include "aegis/guild_impl.hpp"
#include "aegis/aegis_impl.hpp"

