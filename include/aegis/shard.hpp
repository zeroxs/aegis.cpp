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
    shard()
        : heartbeat_ack(0)
        , lastheartbeat(0)
        , sequence(0)
        , connection_state(shard_status::Uninitialized)
    {
    }

    void do_reset();

    bool conn_test(std::function<void()> func);

    const uint64_t get_sequence() const
    {
        return sequence;
    }

    const uint64_t get_id() const
    {
        return shardid;
    }


    struct
    {
        uint32_t guilds;
        uint32_t guilds_outage;
        uint32_t members;
        uint32_t channels;
        uint32_t messages;
        uint64_t presence_changes;
    } counters = { 0,0,0,0 };

private:
    friend aegis;

    bot_state * state;
    shard_status connection_state;
    std::string session_id;
    std::map<int64_t, std::string> debug_messages;
    std::shared_ptr<asio::steady_timer> reconnect_timer;
    std::shared_ptr<asio::steady_timer> keepalivetimer;
    uint64_t sequence;
    int16_t shardid;
    int64_t heartbeat_ack;
    int64_t lastheartbeat;
    // Websocket++ socket connection
    websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr connection;
    websocketpp::connection_hdl hdl;
};

}
