//
// src.hpp
// *******
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#error TODO: needs fixing

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
#include <aegis/member.hpp>
#include <aegis/channel.hpp>
#include <aegis/guild.hpp>
