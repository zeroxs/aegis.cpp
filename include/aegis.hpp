//
// aegis.hpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/push.hpp"

#include "aegis/config.hpp"
#include "aegis/utility.hpp"

#include "aegis/snowflake.hpp"
#include "aegis/objects/role.hpp"
#include "aegis/error.hpp"
#include "aegis/rest_reply.hpp"
#include "aegis/permission.hpp"

#if defined(AEGIS_HEADER_ONLY)

#include "aegis/ratelimit.hpp"
#include "aegis/rest_controller.hpp"
#include "aegis/shard.hpp"
#include "aegis/member.hpp"
#include "aegis/channel.hpp"
#include "aegis/guild.hpp"
#include "aegis/core.hpp"

#include "aegis/ratelimit.cpp"
#include "aegis/rest_controller.cpp"
#include "aegis/core.cpp"
#include "aegis/shard.cpp"
#include "aegis/member.cpp"
#include "aegis/channel.cpp"
#include "aegis/guild.cpp"

#else

#include "aegis/ratelimit.hpp"
#include "aegis/rest_controller.hpp"
#include "aegis/shard.hpp"
#include "aegis/member.hpp"
#include "aegis/channel.hpp"
#include "aegis/guild.hpp"
#include "aegis/core.hpp"

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
#include "aegis/events/message_create.hpp"
#include "aegis/events/message_delete.hpp"
#include "aegis/events/message_delete_bulk.hpp"
#include "aegis/events/message_reaction_add.hpp"
#include "aegis/events/message_reaction_remove.hpp"
#include "aegis/events/message_reaction_remove_all.hpp"
#include "aegis/events/message_update.hpp"
#include "aegis/events/presence_update.hpp"
#include "aegis/events/ready.hpp"
#include "aegis/events/resumed.hpp"
#include "aegis/events/typing_start.hpp"
#include "aegis/events/user_update.hpp"
#include "aegis/events/voice_server_update.hpp"
#include "aegis/events/voice_state_update.hpp"
#include "aegis/events/webhooks_update.hpp"

#endif

#include "aegis/pop.hpp"
