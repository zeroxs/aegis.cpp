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

AEGIS_DECL shard::shard(asio::io_context & _io)
    : heartbeattime(0)
    , heartbeat_ack(0)
    , lastheartbeat(0)
    , lastwsevent(0)
    , last_status_time(0)
    , sequence(0)
    , connection_state(bot_status::Uninitialized)
    , ws_write(_io)
    , _io_context(_io)
{
}

AEGIS_DECL void shard::do_reset() AEGIS_NOEXCEPT
{
    heartbeat_ack = 0;
    lastheartbeat = 0;
    if (_connection != nullptr)
        _connection.reset();
    if (keepalivetimer != nullptr)
    {
        keepalivetimer->cancel();
        keepalivetimer.reset();
    }
}

AEGIS_DECL bool shard::is_connected() const AEGIS_NOEXCEPT
{
    if (_connection == nullptr)
        return false;

    if (_connection->get_state() == websocketpp::session::state::value::open || _connection->get_state() == websocketpp::session::state::value::connecting)
        return true;

    return false;
}

AEGIS_DECL void shard::send(std::string const & payload, websocketpp::frame::opcode::value op)
{
    asio::post(asio::bind_executor(ws_write, [payload, op, this]()
    {
        _connection->send(payload, op);
    }));
}

}
