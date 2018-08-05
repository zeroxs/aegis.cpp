//
// shard.cpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/shards/shard.hpp"
#include "aegis/error.hpp"
#include <websocketpp/connection.hpp>

namespace aegis
{

namespace shards
{

AEGIS_DECL shard::shard(asio::io_context & _io, websocketpp::client<websocketpp::config::asio_tls_client> & _ws, int32_t id)
    : keepalivetimer(_io)
    , delayedauth(_io)
    , write_timer(_io)
    , heartbeattime(0)
    , connection_state(shard_status::Uninitialized)
    , _sequence(0)
    , _id(id)
    , _io_context(_io)
    , transfer_bytes(0)
    , transfer_bytes_u(0)
    , _websocket(_ws)
{
}

AEGIS_DECL void shard::do_reset(shard_status _status) AEGIS_NOEXCEPT
{
    if (_connection == nullptr)
    {
        _reset();
        return;
    }

    asio::post(asio::bind_executor(*_connection->get_strand(), [this, _status]()
    {
        try
        {
            connection_state = _status;
            if (_connection != nullptr)
            {
                if (_connection->get_state() == websocketpp::session::state::open)
                {
                    _connection->close(1001, "");
                    std::cout << "Shard#" << get_id() << ": had to close socket on a reset\n";
                }
            }

            _reset();

            ++counters.reconnects;
        }
        catch (std::exception & e)
        {
            std::cout << "Shard#" << get_id() << ": worst happened do_reset() - std::exception " << e.what() << '\n';
            _reset();
            ++counters.reconnects;
        }
        catch (asio::error_code & e)
        {
            std::cout << "Shard#" << get_id() << ": worst happened do_reset() - asio::error_code " << e.value() << ':' << e.message() << '\n';
            _reset();
            ++counters.reconnects;
        }
        catch (...)
        {
            std::cout << "error in shard::do_reset()\n";
        }
    }));
}

AEGIS_DECL void shard::_reset()
{
    last_status_time = lastwsevent = std::chrono::steady_clock::now();
    heartbeat_ack = lastheartbeat = std::chrono::steady_clock::time_point();

    delayedauth.cancel();
    keepalivetimer.cancel();
    write_timer.cancel();
    ws_buffer.str("");
    zlib_ctx.reset();
}

AEGIS_DECL void shard::connect()
{
    if (_connection == nullptr)
        return;

    asio::post(asio::bind_executor(*_connection->get_strand(), [this]()
    {
        try
        {
            _reset();
            _websocket.connect(_connection);
            connection_state = shard_status::Connecting;
        }
        catch (std::exception & e)
        {
            std::cout << "Shard#" << get_id() << ": worst happened connect() - std::exception " << e.what() << '\n';
            _reset();
        }
        catch (asio::error_code & e)
        {
            std::cout << "Shard#" << get_id() << ": worst happened connect() - asio::error_code " << e.value() << ':' << e.message() << '\n';
            _reset();
        }
        catch (...)
        {
            std::cout << "error in shard::connect()\n";
        }
    }));
}

AEGIS_DECL void shard::set_connected()
{
    using namespace std::chrono_literals;
    if (zlib_ctx)
    {
        //already has an existing context
        throw aegis::exception("set_connected() zlib context already exists");
    }
    if (_connection == nullptr)
    {
        //error
        throw aegis::exception("set_connected() connection = nullptr");
    }
    connection_state = shard_status::PreReady;
    ws_buffer.str("");
    zlib_ctx = std::make_unique<zstr::istream>(ws_buffer);
    write_timer.cancel();
    write_timer.expires_after(600ms);
    write_timer.async_wait(asio::bind_executor(*_connection->get_strand(), std::bind(&shard::process_writes, this, std::placeholders::_1)));
}

AEGIS_DECL bool shard::is_connected() const AEGIS_NOEXCEPT
{
    if ((_connection == nullptr) || (!_connection->get_raw_socket().is_open()))
        return false;

    if (connection_state == shard_status::PreReady || connection_state == shard_status::Online)
        return true;

    return false;
}

AEGIS_DECL bool shard::is_online() const AEGIS_NOEXCEPT
{
    if ((_connection == nullptr) || (!_connection->get_raw_socket().is_open()))
        return false;

    if (connection_state == shard_status::Online)
        return true;

    return false;
}

AEGIS_DECL void shard::send(const std::string & payload, websocketpp::frame::opcode::value op)
{
    if (!is_connected())
        return;
    asio::post(asio::bind_executor(*_connection->get_strand(), [=]()
    {
        write_queue.push(std::make_tuple(payload, op));
    }));
}

AEGIS_DECL void shard::send_now(const std::string & payload, websocketpp::frame::opcode::value op)
{
    if (!is_connected())
        return;
    asio::post(asio::bind_executor(*_connection->get_strand(), [=]()
    {
        last_ws_write = std::chrono::steady_clock::now();
        _connection->send(payload, op);
    }));
}

AEGIS_DECL void shard::process_writes(const asio::error_code & ec)
{
    if (ec == asio::error::operation_aborted)
        return;

    if (_connection == nullptr)
        return;

    using namespace std::chrono_literals;
    if ((connection_state == shard_status::Online || connection_state == shard_status::PreReady) && !write_queue.empty())
    {
        last_ws_write = std::chrono::steady_clock::now();

        auto msg = write_queue.front();
        write_queue.pop();

        _connection->send(std::get<0>(msg), std::get<1>(msg));
    }

    write_timer.expires_after(600ms);
    write_timer.async_wait(asio::bind_executor(*_connection->get_strand(), std::bind(&shard::process_writes, this, std::placeholders::_1)));
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

}
