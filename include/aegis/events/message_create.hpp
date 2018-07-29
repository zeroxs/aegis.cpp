//
// message_create.hpp
// ******************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/objects/message.hpp"
#include "aegis/error.hpp"
#include "aegis/fwd.hpp"
#include <string>
#include <vector>

namespace aegis
{

namespace gateway
{

namespace events
{

/**\todo Needs documentation
 */
struct message_create
{
    shards::shard * _shard; /**< Pointer to shard object this message came from */
    core * bot; /**< Pointer to the main bot object */
    objects::message msg; /**<\todo Needs documentation */
    channel * const _channel; /**<\todo Needs documentation */
    
    bool has_channel()
    {
        return (_channel != nullptr);
    }

    channel & get_channel()
    {
        if (_channel == nullptr)
            throw exception(make_error_code(error::channel_not_found));
        return *_channel;
    }

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    member * const _member; /**<\todo Needs documentation */

    bool has_member()
    {
        return (_member != nullptr);
    }

    member & get_member()
    {
        if (_member == nullptr)
            throw exception(make_error_code(error::member_not_found));
        return *_member;
    }

    message_create(const json & j, channel * c, member * m) : msg(j), _channel(c), _member(m) { msg.set_channel(c); };
    message_create(const json & j, guild * g, channel * c, member * m) : msg(j), _channel(c), _member(m) { msg.set_channel(c); msg.set_guild(g); };
#else
    message_create(const json & j, channel * c) : msg(j), _channel(c) { msg.set_channel(c); };
    message_create(const json & j, guild * g, channel * c) : msg(j), _channel(c) { msg.set_channel(c); msg.set_guild(g); };
#endif
};

}

}

}
