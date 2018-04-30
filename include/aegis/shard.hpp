//
// shard.hpp
// *********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include "aegis/utility.hpp"
#include <memory>
#include <map>
#include <string>
#include <chrono>
#include <stdint.h>
#include "aegis/push.hpp"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
#include "aegis/pop.hpp"

namespace aegis
{

/// Tracks websocket shards and their connections
class shard
{
public:
    /// Constructs a shard object for connecting to the websocket gateway and tracking timers
    AEGIS_DECL shard(asio::io_context & _io);

    /// Resets connection, heartbeat, and timer related objects to allow reconnection
    AEGIS_DECL void do_reset() AEGIS_NOEXCEPT;

    /// Get this shard's websocket message sequence counter
    /**
     * @returns Sequence counter specific to this shard
     */
    const int64_t get_sequence() const AEGIS_NOEXCEPT
    {
        return sequence;
    }

    /// Gets the id of the shard in the master list
    /**
     * @see core::shards
     * @returns Shard id
     */
    const int32_t get_id() const AEGIS_NOEXCEPT
    {
        return shardid;
    }

    /// Check if this shard has an active connection
    /**
     * @returns bool
     */
    AEGIS_DECL bool is_connected() const AEGIS_NOEXCEPT;

    /// Send a thread-safe message to this shard's websocket connection
    /**
     * @param payload String of the payload to send
     * @param op opcode to send the message
     */
    AEGIS_DECL void send(std::string const & payload, websocketpp::frame::opcode::value op);

    /// Contains counters of valued objects and events
    struct
    {
        int64_t messages;
        int64_t presence_changes;
    } counters = { 0,0 };

    /// Actual websocket connection
    websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr _connection;

private:
    friend class core;

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
    asio::io_context::strand ws_write;
    asio::io_context & _io_context;
};

}
