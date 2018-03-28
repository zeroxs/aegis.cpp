//
// shard.hpp
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
#include <memory>
#include <map>
#include <string>
#include <chrono>
#include <stdint.h>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>

namespace aegiscpp
{

class aegis;
class member;


class shard
{
public:
    /// Constructs a shard object for connecting to the websocket gateway and tracking timers
    /// 
    shard(aegis * b)
        : heartbeattime(0)
        , heartbeat_ack(0)
        , lastheartbeat(0)
        , lastwsevent(0)
        , last_status_time(0)
        , sequence(0)
        , connection_state(bot_status::Uninitialized)
        , _bot(b)
    {
    }

    /// Resets connection, heartbeat, and timer related objects to allow reconnection
    /// 
    AEGIS_DECL void do_reset() noexcept;

    AEGIS_DECL void start_reconnect() noexcept;

    /// Get this shard's websocket message sequence counter
    /**
    * @returns Sequence counter specific to this shard
    */
    const int64_t get_sequence() const noexcept
    {
        return sequence;
    }

    /// Gets the id of the shard in the master list
    /**
    * @see aegis::shards
    *
    * @returns Shard id
    */
    const int16_t get_id() const noexcept
    {
        return shardid;
    }

    /// Gets the Bot object
    /**
    * @see aegis
    *
    * @returns Aegis main object
    */
    AEGIS_DECL aegis & get_bot() const noexcept;

    /// Gets the member object representing the bot
    /**
    * @see member
    *
    * @returns The bot's own member object
    */
    AEGIS_DECL member * get_bot_user() const noexcept;

    /// Contains counters of valued objects and events
    struct
    {
        int64_t messages;
        int64_t presence_changes;
    } counters = { 0,0 };

    websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr connection;

private:
    friend aegis;

    std::string session_id;
    std::map<int64_t, std::string> debug_messages;
    std::shared_ptr<asio::steady_timer> reconnect_timer;
    std::shared_ptr<asio::steady_timer> keepalivetimer;
    int32_t heartbeattime;
    int64_t heartbeat_ack;
    int64_t lastheartbeat;
    int64_t lastwsevent;
    int64_t last_status_time;
    int64_t sequence;
    int32_t shardid;
    bot_status connection_state;
    // Websocket++ socket connection
    websocketpp::connection_hdl hdl;
    aegis * _bot;
};

}

#if defined(AEGIS_HEADER_ONLY)
# include "aegis/shard.cpp"
#endif // defined(AEGIS_HEADER_ONLY)
