//
// shard.cpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/config.hpp"
#include "aegis/shard.hpp"

namespace aegis
{

AEGIS_DECL shard::shard(asio::io_context & _io, websocketpp::client<websocketpp::config::asio_tls_client> & _ws)
    : write_timer(_io)
    , heartbeattime(0)
    , sequence(0)
    , connection_state(bot_status::Uninitialized)
    , ws_write(_io)
    , _io_context(_io)
    , transfer_bytes(0)
    , transfer_bytes_u(0)
    , _websocket(_ws)
{
}

AEGIS_DECL void shard::do_reset() AEGIS_NOEXCEPT
{
    heartbeat_ack = lastheartbeat = std::chrono::steady_clock::time_point();
    if (_connection != nullptr)
        _connection.reset();
    if (keepalivetimer != nullptr)
    {
        keepalivetimer->cancel();
        keepalivetimer.reset();
    }
    write_timer.cancel();
    ws_buffer.str("");
    zlib_ctx.reset();
    ++counters.reconnects;
}

AEGIS_DECL void shard::set_connected()
{
    using namespace std::chrono_literals;
    ws_buffer.str("");
    zlib_ctx = std::make_unique<zstr::istream>(ws_buffer);
    write_timer.expires_after(600ms);
    write_timer.async_wait(asio::bind_executor(ws_write, std::bind(&shard::process_writes, this, std::placeholders::_1)));
    connection_state = bot_status::Online;
}

AEGIS_DECL bool shard::is_connected() const AEGIS_NOEXCEPT
{
    if (_connection == nullptr)
        return false;

    if ((connection_state == bot_status::Connecting) || (connection_state == bot_status::Online))
        return true;

    return false;
}

AEGIS_DECL void shard::send(std::string const & payload, websocketpp::frame::opcode::value op)
{
    if (!is_connected())
        return;
    asio::post(asio::bind_executor(ws_write, [_payload = payload, op, this]()
    {
        write_queue.push(std::make_tuple(_payload, op));
    }));
}

AEGIS_DECL void shard::send_now(std::string const & payload, websocketpp::frame::opcode::value op)
{
    if (!is_connected())
        return;
    asio::post(asio::bind_executor(ws_write, [_payload = payload, op, this]()
    {
        last_ws_write = std::chrono::steady_clock::now();
        _connection->send(_payload, op);
    }));
}

AEGIS_DECL void shard::process_writes(const asio::error_code & ec)
{
    if (ec == asio::error::operation_aborted)
        return;
    using namespace std::chrono_literals;
    if ((connection_state == bot_status::Online
         || connection_state == bot_status::Connecting) && !write_queue.empty())
    {
        last_ws_write = std::chrono::steady_clock::now();

        auto msg = write_queue.front();
        write_queue.pop();

        _connection->send(std::get<0>(msg), std::get<1>(msg));
    }

    write_timer.expires_after(600ms);
    write_timer.async_wait(asio::bind_executor(ws_write, std::bind(&shard::process_writes, this, std::placeholders::_1)));
}

AEGIS_DECL std::string shard::uptime_str() const AEGIS_NOEXCEPT
{
    using seconds = std::chrono::duration<int, std::ratio<1, 1>>;
    using minutes = std::chrono::duration<int, std::ratio<60, 1>>;
    using hours = std::chrono::duration<int, std::ratio<3600, 1>>;
    using days = std::chrono::duration<int, std::ratio<24 * 3600, 1>>;

    std::stringstream ss;
    std::chrono::time_point<std::chrono::steady_clock> now_t = std::chrono::steady_clock::now();

    auto time_is = now_t - _ready_time;
    auto d = std::chrono::duration_cast<days>(time_is).count();
    time_is -= days(d);
    auto h = std::chrono::duration_cast<hours>(time_is).count();
    time_is -= hours(h);
    auto m = std::chrono::duration_cast<minutes>(time_is).count();
    time_is -= minutes(m);
    auto s = std::chrono::duration_cast<seconds>(time_is).count();

    if (d)
        ss << d << "d ";
    if (h)
        ss << h << "h ";
    if (m)
        ss << m << "m ";
    ss << s << "s ";
    return ss.str();
}

}
