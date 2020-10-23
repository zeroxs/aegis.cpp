//
// src.hpp
// *******
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include <aegis/config.hpp>

#if defined(AEGIS_HEADER_ONLY)
# error Do not compile Aegis library source with AEGIS_HEADER_ONLY defined
#endif

#include <aegis/error.hpp>

#include <aegis/config.hpp>
#include <aegis/utility.hpp>

#include <aegis/snowflake.hpp>
#include <aegis/gateway/objects/role.hpp>
#include <aegis/error.hpp>
#include <aegis/rest/rest_reply.hpp>

#include <aegis/ratelimit/ratelimit.hpp>
#include <aegis/rest/rest_controller.hpp>
#include <aegis/core.hpp>
#include <aegis/shards/shard_mgr.hpp>
#include <aegis/user.hpp>
#include <aegis/channel.hpp>
#include <aegis/guild.hpp>

#include <aegis/impl/core.cpp>
#include <aegis/impl/user.cpp>
#include <aegis/impl/channel.cpp>
#include <aegis/impl/guild.cpp>
#include <aegis/impl/permission.cpp>
#include <aegis/impl/snowflake.cpp>

#include <aegis/shards/impl/shard.cpp>
#include <aegis/shards/impl/shard_mgr.cpp>

#include <aegis/rest/impl/rest_controller.cpp>

#include <aegis/gateway/objects/impl/message.cpp>
