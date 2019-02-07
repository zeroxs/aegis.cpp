//
// message_create.hpp
// ******************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/fwd.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/gateway/objects/message.hpp"
#include "aegis/error.hpp"

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
    shards::shard * _shard = nullptr; /**< Pointer to shard object this message came from */
    core * bot = nullptr; /**< Pointer to the main bot object */
    objects::message msg; /**<\todo Message object */
    aegis::channel * const _channel = nullptr; /**<\todo Pointer to channel object this message was sent in */
    
    bool has_channel()
    {
        return (_channel != nullptr);
    }

    aegis::channel & get_channel()
    {
        if (_channel == nullptr)
            throw exception(make_error_code(error::channel_not_found));
        return *_channel;
    }

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    aegis::member * const _member; /**<\todo Needs documentation */

    bool has_member()
    {
        return (_member != nullptr);
    }

    aegis::member & get_member()
    {
        if (_member == nullptr)
            throw exception(make_error_code(error::member_not_found));
        return *_member;
    }

    message_create(const json & j, aegis::channel * c, aegis::member * m, aegis::core * b) : msg(j, b), _channel(c), _member(m) { msg.set_channel(c); };
    message_create(const json & j, aegis::guild * g, aegis::channel * c, aegis::member * m, aegis::core * b) : msg(j, b), _channel(c), _member(m) { msg.set_channel(c); msg.set_guild(g); };
#else
    bool has_member() { return false; }
    message_create(const json & j, aegis::channel * c, aegis::core * b) : msg(j, b), _channel(c) { msg.set_channel(c); };
    message_create(const json & j, aegis::guild * g, aegis::channel * c, aegis::core * b) : msg(j, b), _channel(c) { msg.set_channel(c); msg.set_guild(g); };
#endif
};

}

}

}
