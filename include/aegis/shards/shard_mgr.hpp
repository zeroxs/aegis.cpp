//
// shard_mgr.hpp
// *************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

#include "aegis/config.hpp"
#include "aegis/utility.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/error.hpp"
#include "aegis/shards/shard.hpp"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/client.hpp>
#include "spdlog/spdlog.h"

#include <vector>
#include <iostream>
#include <string>

#include <asio/bind_executor.hpp>

namespace aegis
{

namespace shards
{

using json = nlohmann::json;
using namespace std::literals;

/// Primary class for managing a bot interface
/**
 * Only one instance of this object can exist safely
 */
class shard_mgr
{
public:
    /// Type of a pointer to the Websocket++ client
    using websocket = websocketpp::client<websocketpp::config::asio_tls_client>;

    /// Type of a pointer to the Websocket++ TLS connection
    using connection_ptr = websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr;

    /// Type of a pointer to the Websocket++ message payload
    using message_ptr = websocketpp::config::asio_client::message_type::ptr;

    /// Constructs the aegis object that tracks all of the shards, guilds, channels, and members
    /**
     * @see shard
     * @see guild
     * @see channel
     * @see member
     * @param token A string of the authentication token
     * @param _io Reference to asio::io_context
     * @param log std::shared_ptr of spdlog::logger
     */
    AEGIS_DECL shard_mgr(std::string token, asio::io_context & _io, std::shared_ptr<spdlog::logger> log);

    /// Destroys the shards, stops the asio::work object, destroys the websocket object, and attempts to join the rest_thread thread
    AEGIS_DECL ~shard_mgr();

    shard_mgr(const shard_mgr &) = delete;
    shard_mgr(shard_mgr &&) = delete;
    shard_mgr & operator=(const shard_mgr &) = delete;

    /// Assign the message, connect, and close callbacks to the websocket object
    /**
     * @param _shard The shard object this websocket belong to
     */
    AEGIS_DECL void setup_callbacks(shard * _shard) noexcept;

    /// Outputs the last 5 messages received from the gateway
    /**
     * @param _shard Pointer to shard
     * @param extended Whether extended info is output
     */
    AEGIS_DECL void debug_trace(shard * _shard, bool extended = false) noexcept;

    /// Get the internal (or external) io_service object
    /**
     * @returns Asio io_context
     */
    asio::io_context & get_io_context()
    {
        return _io_context;
    }

    /// Invokes a shutdown on the entire lib. Sets internal state to `Shutdown`, stops the asio::work object
    /// and propagates the Shutdown state along with closing all websockets within the shard vector
    AEGIS_DECL void shutdown();

    /// Get the internal Websocket++ instance
    /**
     * @returns Websocket++ internal instance
     */
    websocket & get_websocket() noexcept { return websocket_o; }

    /// Get the current state of the shard manager
    /**
     * @see aegis::bot_status
     * @returns aegis::bot_status
     */
    bot_status get_state() const noexcept { return _status; }

    /// Set the current shard manager state
    /**
     * @see aegis::bot_status
     * @param s State to set the shard manager to
     */
    void set_state(bot_status s) noexcept { _status = s; }

    /// Return bot uptime as {days hours minutes seconds}
    /**
     * @returns std::string of `## h ## m ## s` formatted time
     */
    AEGIS_DECL std::string uptime() const noexcept;

    /// Send a websocket message to a single shard
    /**
    * @param msg JSON encoded message to be sent
    */
    AEGIS_DECL void send_all_shards(const std::string & msg);

    /// Send a websocket message to a single shard
    /**
    * @param msg JSON encoded message to be sent
    */
    AEGIS_DECL void send_all_shards(const json & msg);

    /// Start shard connections and shard status timer
    AEGIS_DECL void start();

    /// Websocket on_message handler type
    using t_on_message = std::function<void(websocketpp::connection_hdl hdl, std::string msg, shard * _shard)>;
    /// Websocket on_connect handler type
    using t_on_connect = std::function<void(websocketpp::connection_hdl hdl, shard * _shard)>;
    /// Websocket on_close handler type
    using t_on_close = std::function<void(websocketpp::connection_hdl hdl, shard * _shard)>;

    /// Set handler for websocket messages
    /**
     * @see t_on_message
     * @param cb Callback
     */
    void set_on_message(t_on_message cb) noexcept
    {
        i_on_message = cb;
    }

    /// Set handler for websocket connections
    /**
     * @see t_on_connect
     * @param cb Callback
     */
    void set_on_connect(t_on_connect cb) noexcept
    {
        i_on_connect = cb;
    }

    /// Set handler for websocket disconnections
    /**
     * @see t_on_close
     * @param cb Callback
     */
    void set_on_close(t_on_close cb) noexcept
    {
        i_on_close = cb;
    }

    /// Set the gateway url the shards will connect to
    /**
     * @param url String to gateway url
     */
    void set_gateway_url(const std::string & url) noexcept
    {
        gateway_url = url;
    }

    /// Get the gateway url the shards will connect to
    /**
     * @returns std::string Shard connection url
     */
    std::string get_gateway_url() const noexcept
    {
        return gateway_url;
    }

    /// Resets the shard's state
    /**
     * @param _shard Pointer to shard
     */
    AEGIS_DECL void reset_shard(shard * _shard, shard_status _status = shard_status::closing) noexcept;

    /// Queue the shard for reconnection. Typically only called internally
    /**
     * @param _shard Pointer to shard
     */
    AEGIS_DECL void queue_reconnect(shard * _shard) noexcept;

    /// Queue the shard for reconnection. Typically only called internally
    /**
     * @param _shard Reference to shard
     */
    void queue_reconnect(shard & _shard)
    {
        queue_reconnect(&_shard);
    }

    /// Connect the shard to the gateway. Typically only called internally
    /**
     * @param _shard Pointer to shard
     */
    AEGIS_DECL void connect(shard * _shard) noexcept;

    /// Get the shard object
    /**
     * @see aegis::shards::shard
     * @param shard_id Internal index of shard to retrieve
     * @returns reference to aegis::shards::shard object
     */
    AEGIS_DECL shard & get_shard(uint16_t shard_id);

    /// Get a const vector of all the shards
    /**
     * @returns const std::vector of all the shards as an std::unique_ptr
     */
    AEGIS_DECL const std::vector<std::unique_ptr<shard>> & get_shards() const noexcept
    {
        return _shards;
    }

    /// Close the shard's websocket connection
    /**
     * @see aegis::shard_status
     * @param _shard Pointer to shard
     * @param code Websocket close code (default: 1001)
     * @param reason Websocket close reason (default: "")
     * @param connection_state State the set the shard to after close (default: shard_status::Closing)
     */
    AEGIS_DECL void close(shard * _shard, int32_t code = 1001, const std::string & reason = "", shard_status connection_state = shard_status::closing) noexcept;

    /// Close the shard's websocket connection
    /**
     * @see aegis::shard_status
     * @param _shard Reference to shard
     * @param code Websocket close code (default: 1001)
     * @param reason Websocket close reason (default: "")
     * @param connection_state State the set the shard to after close (default: shard_status::Closing)
     */
    void close(shard & _shard, int32_t code = 1001, const std::string & reason = "", shard_status connection_state = shard_status::closing) noexcept
    {
        close(&_shard, code, reason, connection_state);
    }

    /// Get the amount of shards that exist
    /**
     * @returns uint32_t of shard count
     */
    uint32_t shard_count() const noexcept
    {
        return _shards.size();
    }

    /// Asio context
    asio::io_context & _io_context;

    /// Gateway URL
    std::string ws_gateway;

    /// Websocket event message counters
    std::unordered_map<std::string, uint64_t> message_count;

    /// Shard count to force manager to use
    uint32_t force_shard_count;
    /// Shard count retrieved from gateway
    uint32_t shard_max_count;
    /// Logging instance
    std::shared_ptr<spdlog::logger> log;

private:
    friend aegis::core;

    std::chrono::time_point<std::chrono::steady_clock> _last_ready;
    std::chrono::time_point<std::chrono::steady_clock> _last_identify;
    std::chrono::time_point<std::chrono::steady_clock> _connect_time;
    shard * _connecting_shard;

    std::vector<std::unique_ptr<shard>> _shards;
  
    t_on_message i_on_message;
    t_on_connect i_on_connect;
    t_on_close i_on_close;

    std::function<void(aegis::shards::shard*)> i_shard_disconnect;
    std::function<void(aegis::shards::shard*)> i_shard_connect;

    AEGIS_DECL void _on_message(websocketpp::connection_hdl hdl, message_ptr msg, shard * _shard);
    AEGIS_DECL void _on_connect(websocketpp::connection_hdl hdl, shard * _shard);
    AEGIS_DECL void _on_close(websocketpp::connection_hdl hdl, shard * _shard);
    AEGIS_DECL void ws_status(const asio::error_code & ec);

    std::chrono::steady_clock::time_point starttime;

    // Gateway URL for the Discord Websocket
    std::string gateway_url;

    // Websocket++ object
    websocket websocket_o;

    // Bot's token
    std::string _token;

    bot_status _status;

    std::shared_ptr<asio::steady_timer> ws_timer;
    std::shared_ptr<asio::steady_timer> ws_connect_timer;
    std::deque<shard*> _shards_to_connect;
};

}

}

#if defined(AEGIS_HEADER_ONLY)
#include "aegis/shards/impl/shard_mgr.cpp"
#endif
