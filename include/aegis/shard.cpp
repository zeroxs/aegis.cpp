//
// shard.cpp
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

#include "shard.hpp"
#include "aegis.hpp"
#include "utility.hpp"



namespace aegiscpp
{

AEGIS_DECL void shard::do_reset() noexcept
{
    heartbeat_ack = 0;
    lastheartbeat = 0;
    if (connection != nullptr)
        connection.reset();
    if (keepalivetimer != nullptr)
    {
        keepalivetimer->cancel();
        keepalivetimer.reset();
    }
}

AEGIS_DECL void shard::start_reconnect() noexcept
{
    reconnect_timer = get_bot().websocket_o.set_timer(10000, [this](const asio::error_code & ec)
    {
        if (ec == asio::error::operation_aborted)
            return;
        connection_state = Connecting;
        asio::error_code wsec;
        connection = get_bot().websocket_o.get_connection(get_bot().gateway_url, wsec);
        get_bot().setup_callbacks(this);
        get_bot().websocket_o.connect(connection);
    });
}

AEGIS_DECL member * shard::get_bot_user() const noexcept
{
    return get_bot().self();
}

AEGIS_DECL aegis & shard::get_bot() const noexcept
{
    return *_bot;
}

}
