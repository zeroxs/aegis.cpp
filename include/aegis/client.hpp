//
// client.hpp
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


#include <string>
#include <chrono>
#include <sstream>
#include "config.hpp"

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/connection_hdl.hpp>

namespace aegis
{

class client
{
public:
    client()
        : m_heartbeatack(0)
        , m_lastheartbeat(0)
        , m_sequence(0)
        , m_state(aegis::UNINITIALIZED)
    {
        starttime = std::chrono::steady_clock::now();
    }

    std::function<void()> cb;

    // Websocket++ socket connection
    websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr m_connection;
    websocketpp::connection_hdl hdl;
    
    int64_t m_heartbeatack;
    int64_t m_lastheartbeat;
    
    int16_t m_shardid;
    
    // Timer for heartbeats
    std::shared_ptr<asio::steady_timer> m_keepalivetimer;

    uint64_t m_sequence;

    int16_t m_state;

    std::shared_ptr<asio::steady_timer> m_reconnect_timer;



    std::chrono::steady_clock::time_point starttime;
    std::string uptime()
    {
        std::stringstream ss;
        std::chrono::steady_clock::time_point timenow = std::chrono::steady_clock::now();

        int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(timenow - starttime).count();

        int64_t seconds = (ms / 1000) % 60;
        int64_t minutes = (((ms / 1000) - seconds) / 60) % 60;
        int64_t hours = (((((ms / 1000) - seconds) / 60) - minutes) / 60) % 24;
        int64_t days = (((((((ms / 1000) - seconds) / 60) - minutes) / 60) - hours) / 24);

        if (days > 0)
            ss << days << "d ";
        if (hours > 0)
            ss << hours << "h ";
        if (minutes > 0)
            ss << minutes << "m ";
        if (seconds > 0)
            ss << seconds << "s";
        return ss.str();
    }

};

}
