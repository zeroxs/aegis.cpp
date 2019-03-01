//
// fwd.hpp
// *******
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

#include <nlohmann/json_fwd.hpp>

namespace aegis
{
namespace ratelimit
{
class ratelimit_mgr;
class bucket;
}
namespace rest
{
class rest_controller;
class rest_reply;
}
namespace shards
{
class shard;
class shard_mgr;
}

class core;
class channel;
class guild;
class user;
class shard;

namespace gateway
{
namespace objects
{
struct user;
class message;
struct channel;
struct guild;
}

namespace events
{
struct typing_start;
struct message_create;
struct message_update;
struct message_delete;
struct ready;
struct resumed;
struct presence_update;
struct channel_create;
struct channel_delete;
struct channel_pins_update;
struct channel_update;
struct guild_ban_add;
struct guild_ban_remove;
struct guild_create;
struct guild_delete;
struct guild_emojis_update;
struct guild_integrations_update;
struct guild_member_add;
struct guild_member_remove;
struct guild_member_update;
struct guild_members_chunk;
struct guild_role_create;
struct guild_role_delete;
struct guild_role_update;
struct guild_update;
struct message_delete_bulk;
struct message_reaction_add;
struct message_reaction_remove;
struct message_reaction_remove_all;
struct user_update;
struct voice_server_update;
struct voice_state_update;
struct webhooks_update;
}
}
}
