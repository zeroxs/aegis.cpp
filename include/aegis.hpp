//
// aegis.hpp
// *********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/push.hpp"

#include "aegis/config.hpp"
#include "aegis/utility.hpp"

#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/role.hpp"
#include "aegis/error.hpp"
#include "aegis/rest/rest_reply.hpp"
#include "aegis/permission.hpp"

#if defined(AEGIS_HEADER_ONLY)

#include "aegis/ratelimit/ratelimit.hpp"
#include "aegis/rest/rest_controller.hpp"
#include "aegis/shards/shard_mgr.hpp"
#include "aegis/user.hpp"
#include "aegis/channel.hpp"
#include "aegis/guild.hpp"
#include "aegis/core.hpp"

#include "aegis/impl/core.cpp"
#include "aegis/impl/user.cpp"
#include "aegis/impl/channel.cpp"
#include "aegis/impl/guild.cpp"

#else

#include "aegis/ratelimit/ratelimit.hpp"
#include "aegis/rest/rest_controller.hpp"
#include "aegis/shards/shard_mgr.hpp"
#include "aegis/user.hpp"
#include "aegis/channel.hpp"
#include "aegis/guild.hpp"
#include "aegis/core.hpp"

#include "aegis/gateway/events/channel_create.hpp"
#include "aegis/gateway/events/channel_delete.hpp"
#include "aegis/gateway/events/channel_pins_update.hpp"
#include "aegis/gateway/events/channel_update.hpp"
#include "aegis/gateway/events/guild_ban_add.hpp"
#include "aegis/gateway/events/guild_ban_remove.hpp"
#include "aegis/gateway/events/guild_create.hpp"
#include "aegis/gateway/events/guild_delete.hpp"
#include "aegis/gateway/events/guild_emojis_update.hpp"
#include "aegis/gateway/events/guild_integrations_update.hpp"
#include "aegis/gateway/events/guild_member_add.hpp"
#include "aegis/gateway/events/guild_member_remove.hpp"
#include "aegis/gateway/events/guild_member_update.hpp"
#include "aegis/gateway/events/guild_members_chunk.hpp"
#include "aegis/gateway/events/guild_role_create.hpp"
#include "aegis/gateway/events/guild_role_delete.hpp"
#include "aegis/gateway/events/guild_role_update.hpp"
#include "aegis/gateway/events/guild_update.hpp"
#include "aegis/gateway/events/message_create.hpp"
#include "aegis/gateway/events/message_delete.hpp"
#include "aegis/gateway/events/message_delete_bulk.hpp"
#include "aegis/gateway/events/message_reaction_add.hpp"
#include "aegis/gateway/events/message_reaction_remove.hpp"
#include "aegis/gateway/events/message_reaction_remove_all.hpp"
#include "aegis/gateway/events/message_update.hpp"
#include "aegis/gateway/events/presence_update.hpp"
#include "aegis/gateway/events/ready.hpp"
#include "aegis/gateway/events/resumed.hpp"
#include "aegis/gateway/events/typing_start.hpp"
#include "aegis/gateway/events/user_update.hpp"
#include "aegis/gateway/events/voice_server_update.hpp"
#include "aegis/gateway/events/voice_state_update.hpp"
#include "aegis/gateway/events/webhooks_update.hpp"

#endif

#include "aegis/pop.hpp"
