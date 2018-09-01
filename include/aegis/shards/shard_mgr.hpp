//
// shard_mgr.hpp
// *************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
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
    /// Type of a pointer to the asio io_service
    using io_service_ptr = asio::io_context *;

    /// Type of a pointer to the Websocket++ client
    using websocket = websocketpp::client<websocketpp::config::asio_tls_client>;

    /// Type of a pointer to the Websocket++ TLS connection
    using connection_ptr = websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr;

    /// Type of a pointer to the Websocket++ message payload
    using message_ptr = websocketpp::config::asio_client::message_type::ptr;

    /// Type of a work guard executor for keeping Asio services alive
    using asio_exec = asio::executor_work_guard<asio::io_context::executor_type>;

    /// Type of a shared pointer to an io_context work object
    using work_ptr = std::shared_ptr<asio_exec>;

    /// Constructs the aegis object that tracks all of the shards, guilds, channels, and members
    /**
     * @see shard
     * @see guild
     * @see channel
     * @see member
     * @param token A string of the authentication token
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
    AEGIS_DECL void setup_callbacks(shard * _shard);

    /// Outputs the last 5 messages received from the gateway
    ///
    AEGIS_DECL void debug_trace(shard * _shard, bool extended = false);

    /// Get the internal (or external) io_service object
    asio::io_context & get_io_context()
    {
        return _io_context;
    }

    /// Invokes a shutdown on the entire lib. Sets internal state to `Shutdown`, stops the asio::work object
    /// and propagates the Shutdown state along with closing all websockets within the shard vector
    AEGIS_DECL void shutdown();

    websocket & get_websocket() noexcept { return websocket_o; }
    bot_status get_state() const noexcept { return _status; }
    void set_state(bot_status s) noexcept { _status = s; }

    /// Helper function for posting tasks to asio
    /**
     * @param f A callable to execute within asio - signature should be void(void)
     */
    template<typename Func>
    void async(Func f)
    {
        asio::post(_io_context, std::move(f));
    }

    /// Return bot uptime as {days hours minutes seconds}
    /**
     * @returns std::string of `##h ##m ##s` formatted time
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

    AEGIS_DECL void start(std::size_t count = 0);

    using t_on_message = std::function<void(websocketpp::connection_hdl hdl, std::string msg, shard * _shard)>;
    using t_on_connect = std::function<void(websocketpp::connection_hdl hdl, shard * _shard)>;
    using t_on_close = std::function<void(websocketpp::connection_hdl hdl, shard * _shard)>;

    void set_on_message(t_on_message cb) noexcept
    {
        i_on_message = cb;
    }

    void set_on_connect(t_on_connect cb) noexcept
    {
        i_on_connect = cb;
    }

    void set_on_close(t_on_close cb) noexcept
    {
        i_on_close = cb;
    }

    void set_gateway_url(const std::string & url) noexcept
    {
        gateway_url = url;
    }

    std::string get_gateway_url() const noexcept
    {
        return gateway_url;
    }

    AEGIS_DECL void reset_shard(shard * _shard);

    AEGIS_DECL void queue_reconnect(shard * _shard);

    AEGIS_DECL void connect(shard * _shard);

    void queue_reconnect(shard & _shard)
    {
        queue_reconnect(&_shard);
    }

    AEGIS_DECL shard & get_shard(uint16_t shard_id);

    AEGIS_DECL void close(shard * _shard, int32_t code = 1001, const std::string & reason = "", shard_status connection_state = shard_status::Closed);

    void close(shard & _shard, int32_t code = 1001, const std::string & reason = "", shard_status connection_state = shard_status::Closed)
    {
        close(&_shard, code, reason, connection_state);
    }

    std::mutex m;
    std::condition_variable cv;
    asio::io_context & _io_context;

    std::string ws_gateway;

    std::unordered_map<std::string, uint64_t> message_count;

    std::string self_presence;
    uint32_t force_shard_count;
    uint32_t shard_max_count;
    bool wsdbg = false;
    //std::unique_ptr<asio::io_context::strand> ws_open_strand;
    std::shared_ptr<spdlog::logger> log;

private:
    friend core;

    std::chrono::time_point<std::chrono::steady_clock> _last_ready;
    std::chrono::time_point<std::chrono::steady_clock> _connect_time;
    shard * _connecting_shard;

    std::vector<std::unique_ptr<shard>> _shards;
  
    t_on_message i_on_message;
    t_on_connect i_on_connect;
    t_on_close i_on_close;

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
    work_ptr wrk;
    std::vector<std::thread> threads;
};

}

}
