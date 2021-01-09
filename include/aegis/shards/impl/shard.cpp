//
// shard.cpp
// *********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/shards/shard.hpp"
#include "aegis/error.hpp"
#include "aegis/async++.hpp"

namespace aegis
{

namespace shards
{

AEGIS_DECL shard::shard(asio::io_context & _io, websocketpp::client<websocketpp::config::asio_tls_client> & _ws, int32_t id)
    : keepalivetimer(_io)
    , delayedauth(_io)
    , write_timer(_io)
    , heartbeattime(0)
    , connection_state(shard_status::uninitialized)
    , _sequence(0)
    , _id(id)
    , _io_context(_io)
    , transfer_bytes(0)
    , transfer_bytes_u(0)
    , _websocket(_ws)
{
}

AEGIS_DECL void shard::do_reset(shard_status _status) noexcept
{
    if (!state_valid())
    {
        connection_state = _status;
        _reset();
        ++counters.reconnects;
        return;
    }
    async::spawn([this, _status]()
    {
        try
        {
            connection_state = _status;
            if (_connection != nullptr)
            {
                // if socket is being set to a closed/shutdown state then it may be intentional to be closing an open socket
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
    });
}

AEGIS_DECL void shard::_reset()
{
    last_status_time = lastwsevent =
    heartbeat_ack = lastheartbeat = connect_time = std::chrono::steady_clock::time_point();
    _connection.reset();

    write_queue = std::queue<std::tuple<std::string, websocketpp::frame::opcode::value>>();
    delayedauth.cancel();
    keepalivetimer.cancel();
    write_timer.cancel();
    ws_buffer.str("");
    zlib_ctx.reset();
    _trace.clear();
}

AEGIS_DECL void shard::connect()
{
    if (!state_valid())
        return;

    async::spawn([this]()
    {
        try
        {
            //_reset();
            _websocket.connect(_connection);
            connection_state = shard_status::connecting;
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
    });
}

AEGIS_DECL void shard::set_connected()
{
    if (!state_valid())
        return;
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
    ws_buffer.str("");
    zlib_ctx = std::make_unique<zstr::istream>(ws_buffer);
    write_timer.cancel();
    write_timer.expires_after(600ms);
    write_timer.async_wait(asio::bind_executor(*_connection->get_strand(), std::bind(&shard::process_writes, this, std::placeholders::_1)));
    connection_state = shard_status::preready;
}

AEGIS_DECL bool shard::is_connected() const noexcept
{
    if ((_connection == nullptr) || (!_connection->get_raw_socket().is_open()))
        return false;

    if (connection_state == shard_status::preready || connection_state == shard_status::online)
        return true;

    return false;
}

AEGIS_DECL bool shard::is_online() const noexcept
{
    if ((_connection == nullptr) || (!_connection->get_raw_socket().is_open()))
        return false;

    if (connection_state == shard_status::online)
        return true;

    return false;
}

AEGIS_DECL void shard::start_heartbeat(int32_t heartbeat) noexcept
{
    if (!state_valid())
        return;
    heartbeattime = heartbeat;

    if (!keepalivefunc || !_connection)
        return;

    keepalivetimer.expires_after(std::chrono::milliseconds(heartbeat));
    keepalivetimer.async_wait([=](const asio::error_code & ec)
    {
        if (ec == asio::error::operation_aborted)
            return;

        keepalivefunc(ec, std::chrono::milliseconds(heartbeat), this);
        start_heartbeat(heartbeat);
    });
}

AEGIS_DECL void shard::send(const std::string & payload, websocketpp::frame::opcode::value op)
{
    if (!state_valid())
        return;
    if (!is_connected())
        return;
    async::spawn([=]()
    {
        write_queue.push(std::make_tuple(payload, op));
    });
}

AEGIS_DECL void shard::send_now(const std::string & payload, websocketpp::frame::opcode::value op)
{
    if (!state_valid())
        return;
    if (!is_connected())
        return;
    async::spawn([=]()
    {
        last_ws_write = std::chrono::steady_clock::now();
        if (!_connection)
            return;
        _connection->send(payload, op);
    });
}

AEGIS_DECL void shard::process_writes(const asio::error_code & ec)
{
    if (!state_valid())
        return;
    if (ec == asio::error::operation_aborted)
        return;

    if (_connection == nullptr)
        return;

    using namespace std::chrono_literals;
    if ((connection_state == shard_status::online || connection_state == shard_status::preready) && !write_queue.empty())
    {
        last_ws_write = std::chrono::steady_clock::now();

        auto msg = write_queue.front();
        write_queue.pop();

        _connection->send(std::get<0>(msg), std::get<1>(msg));
    }

    write_timer.expires_after(600ms);
    write_timer.async_wait(asio::bind_executor(*_connection->get_strand(), std::bind(&shard::process_writes, this, std::placeholders::_1)));
}

AEGIS_DECL void shard::update_presence(const std::string& text, gateway::objects::activity::activity_type type, gateway::objects::presence::user_status status)
{
    nlohmann::json j = {
        { "op", 3 },
        {
            "d",
            {
                { "game",
                    {
                        { "name", text },
                        { "type", static_cast<int32_t>(type) }
                    }
                },
                { "status", gateway::objects::presence::to_string(status) },
                { "since", nlohmann::json::value_t::null },
                { "afk", false }
            }
        }
    };
    send(j.dump());
    return;
}


}

}
