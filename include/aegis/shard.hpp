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


#include "config.hpp"
#include <string>
#include <chrono>
#include <sstream>


namespace aegiscpp
{

class shard
{
public:
    /// Constructs a shard object for connecting to the websocket gateway and tracking timers
    /// 
    shard()
        : heartbeattime(0)
        , heartbeat_ack(0)
        , lastheartbeat(0)
        , lastwsevent(0)
        , last_status_time(0)
        , sequence(0)
        , connection_state(shard_status::Uninitialized)
    {
    }

    /// Resets connection, heartbeat, and timer related objects to allow reconnection
    /// 
    void do_reset();

    void start_reconnect();

    /// Get this shard's websocket message sequence counter
    /**
    * @returns Sequence counter specific to this shard
    */
    const int64_t get_sequence() const
    {
        return sequence;
    }

    /// Gets the id of the shard in the master list
    /**
    * @see aegis::shards
    *
    * @returns Shard id
    */
    const int16_t get_id() const
    {
        return shardid;
    }

    /// Contains counters of valued objects and events
    /**
    * @see guild
    * @see channel
    * @see member
    */
    struct
    {
        int64_t guilds;
        int64_t guilds_outage;
        int64_t members;
        int64_t channels;
        int64_t messages;
        int64_t presence_changes;
    } counters = { 0,0,0,0,0,0 };

    websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr connection;

    bot_state * state;

private:
    friend aegis;

    std::string session_id;
    std::map<int64_t, std::string> debug_messages;
    std::shared_ptr<asio::steady_timer> reconnect_timer;
    std::shared_ptr<asio::steady_timer> keepalivetimer;
    long heartbeattime;
    int64_t heartbeat_ack;
    int64_t lastheartbeat;
    int64_t lastwsevent;
    int64_t last_status_time;
    int64_t sequence;
    int16_t shardid;
    shard_status connection_state;
    // Websocket++ socket connection
    websocketpp::connection_hdl hdl;
};

}
