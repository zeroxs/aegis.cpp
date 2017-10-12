//
// aegis.hpp
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
#include "error.hpp"
#include "structs.hpp"
#include "selfbot.hpp"
#include "basebot.hpp"
#include "ratelimit.hpp"

#include <cstdio>
#include <iostream>
#include <streambuf>
#include <sstream>
#include <functional>
#include <memory>
#include <optional>
#include <set>

#include <spdlog/spdlog.h>
#include <asio.hpp>
#include <websocketpp/common/random.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <asio/steady_timer.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <json.hpp>

namespace spd = spdlog;
using json = nlohmann::json;
using namespace std::literals;
using namespace std::chrono;

namespace aegis
{

using std::function;
using std::bind;
using std::ref;
namespace placeholders = std::placeholders;
using namespace aegis::rest_limits;

template<typename bottype>
class Aegis
{
public:

    /// Type of a pointer to the ASIO io_service
    typedef asio::io_service * io_service_ptr;

    /// Type of a pointer to the Websocket++ client
    typedef websocketpp::client<websocketpp::config::asio_tls_client> websocket;

    /// Type of a pointer to the Websocket++ TLS connection
    typedef websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr connection_ptr;

    /// Type of a pointer to the Websocket++ message payload
    typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

    /// Type of a shared pointer to an io_service work object
    typedef std::shared_ptr<asio::io_service::work> work_ptr;

    Aegis(std::string token)
        : m_token(token)
        , m_state(UNINITIALIZED)
        , m_sequence(0)
        , m_shardid(0)
        , m_shardidmax(0)
        , m_heartbeatack(0)
        , m_ratelimit(std::bind(&Aegis::call, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    {
        m_log = spd::stdout_color_mt("aegis");
        m_ratelimit.add(bucket_type::GUILD);
        m_ratelimit.add(bucket_type::CHANNEL);
    }

    ~Aegis()
    {
        m_work.reset();
        m_websocket.reset();
        if (m_keepalivetimer)
            m_keepalivetimer->cancel();
    }

    Aegis(const Aegis &) = delete;
    Aegis(Aegis &&) = delete;
    Aegis & operator=(const Aegis &) = delete;

    void initialize(io_service_ptr ptr, std::error_code & ec)
    {
        m_log->info("Initializing");
        if (m_state != UNINITIALIZED)
        {
            m_log->critical("aegis::initialize() called in the wrong state");
            using error::make_error_code;
            ec = make_error_code(error::invalid_state);
            return;
        }

        m_log->debug("aegis::initialize()");
        m_websocket.init_asio(ptr, ec);
        m_state = READY;
        ec = std::error_code();
    }

    void initialize(io_service_ptr ptr)
    {
        std::error_code ec;
        initialize(ptr, ec);
        if (ec)
            throw exception(ec);
    }

    //
    void initialize(std::error_code & ec)
    {
        std::unique_ptr<asio::io_service> service(new asio::io_service());
        initialize(service.get(), ec);
        if (!ec) service.release();
    }

    void initialize()
    {
        std::unique_ptr<asio::io_service> service(new asio::io_service());
        initialize(service.get());
        service.release();
    }

    void easy_start()
    {
        std::error_code ec;
        // Pass our io_service object to bot to initialize
        initialize(ec);
        if (ec) { m_log->error("Initialize fail: {}", ec.message()); return; }
        // Start a work object so that asio won't exit prematurely
        start_work();
        // Start the REST outgoing thread
        thd = std::make_unique<std::thread>([&] { rest_thread(); });
        // Create our websocket connection
        websocketcreate(ec);
        if (ec) { m_log->error("Websocket fail: {}", ec.message()); return; }
        // Connect the websocket
        connect(ec);
        if (ec) { m_log->error("Connect fail: {}", ec.message()); return; }
        starttime = std::chrono::steady_clock::now();
        // Run the bot
        run();
    } 

    /// Remove the logger instance from spdlog
    void remove_logger() const
    {
        spd::drop("aegis");
    }

    void websocketcreate(std::error_code & ec)
    {
        m_log->info("Creating websocket");
        using error::make_error_code;
        if (m_state != READY)
        {
            m_log->critical("aegis::websocketcreate() called in the wrong state");
            ec = make_error_code(error::invalid_state);
            return;
        }

        std::optional<rest_reply> res;

        if constexpr (std::is_same<bottype, basebot>::value)
            res = get("/gateway/bot");
        else
            res = get("/gateway");

        if (!res.has_value() || res->content.size() == 0)
        {
            ec = make_error_code(error::get_gateway);
            return;
        }


        json ret = json::parse(res->content);
        if (ret.count("message"))
        {
            if (ret["message"] == "401: Unauthorized")
            {
                ec = make_error_code(error::invalid_token);
                return;
            }
        }

        if constexpr (std::is_same<bottype, basebot>::value)
        {
            m_shardidmax = ret["shards"];
            m_log->info("Shard count: {}", m_shardidmax);
        }
        else
            m_shardidmax = 1;

        m_gatewayurl = ret["url"].get<std::string>();

        m_websocket.clear_access_channels(websocketpp::log::alevel::all);

        m_websocket.set_tls_init_handler([](websocketpp::connection_hdl)
        {
            return websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv1);
        });

        m_websocket.set_message_handler(std::bind(&Aegis::onMessage, this, std::placeholders::_1, std::placeholders::_2));
        m_websocket.set_open_handler(std::bind(&Aegis::onConnect, this, std::placeholders::_1));
        m_websocket.set_close_handler(std::bind(&Aegis::onClose, this, std::placeholders::_1));
        m_websocket.set_fail_handler(std::bind(&Aegis::onClose, this, std::placeholders::_1));
        ec = std::error_code();
    }

    /// Get the internal (or external) io_service object
    asio::io_service & io_service()
    {
        return m_websocket.get_io_service();
    }

    /// Initiate websocket connection
    void connect(std::error_code & ec)
    {
        m_log->info("Websocket connecting");
        m_connection = m_websocket.get_connection("wss://gateway.discord.gg/?encoding=json&v=6", ec);
        if (ec)
        {
            m_log->error("Websocket connection failed: {0}", ec.message());
            return;
        }
        m_websocket.connect(m_connection);
    }

    void start_work()
    {
        m_work = std::make_shared<asio::io_service::work>(std::ref(io_service()));
    }

    void stop_work()
    {
        m_work.reset();
    }

    enum AegisOption
    {

    };

    void set_option(AegisOption opt, bool val);

    /// Performs a GET request on the path
    /**
     * @param path A string of the uri path to get
     * 
     * @returns std::optional<std::string>
     */
    std::optional<rest_reply> get(std::string_view path);

    /// Performs a GET request on the path with content as the request body
    /**
    * @param path A string of the uri path to get
    * 
    * @param content JSON formatted string to send as the body
    *
    * @returns std::optional<std::string>
    */
    std::optional<rest_reply> get(std::string_view path, std::string_view content);

    /// Performs a GET request on the path with content as the request body
    /**
    * @param path A string of the uri path to get
    *
    * @param content JSON formatted string to send as the body
    *
    * @returns std::optional<std::string>
    */
    std::optional<rest_reply> post(std::string_view path, std::string_view content);

    /// Performs an HTTP request on the path with content as the request body using the method method
    /**
    * @param path A string of the uri path to get
    *
    * @param content JSON formatted string to send as the body
    *
    * @param method The HTTP method of the request
    *
    * @returns std::optional<std::string>
    */
    std::optional<rest_reply> call(std::string_view path, std::string_view content, std::string_view method);


    /// wraps the run method of the internal io_service object
    std::size_t run()
    {
        return io_service().run();
    }

    /// Yield execution
    void yield() const
    {
        while (m_state != SHUTDOWN)
        {
            std::this_thread::yield();
        }
    }

    enum state
    {
        UNINITIALIZED = 0,
        READY = 1,
        CONNECTED = 2,
        ONLINE = 3,
        RECONNECTING = 4,
        SHUTDOWN = 5
    };

    state get_state() const
    {
        return m_state;
    }

private:

    std::unique_ptr<std::thread> thd;
    std::shared_ptr<spd::logger> m_log;

    // Gateway URL for the Discord Websocket
    std::string m_gatewayurl;

    // Websocket++ object
    websocket m_websocket;

    // Websocket++ connection
    connection_ptr m_connection;

    // Bot's token
    std::string m_token;

    // 
    std::shared_ptr<asio::steady_timer> m_keepalivetimer;

    // Work object for ASIO
    work_ptr m_work;


    state m_state;
    uint64_t m_sequence;
    uint32_t m_shardid, m_shardidmax;

    int64_t m_heartbeatack;
    int64_t m_lastheartbeat;

    //std::vector<std::unique_ptr<client>> m_clients;

    ratelimit m_ratelimit;
    void onMessage(const websocketpp::connection_hdl hdl, const message_ptr msg);
    void onConnect(const websocketpp::connection_hdl hdl);
    void onClose(const websocketpp::connection_hdl hdl);
    void userMessage(json & obj);
    void processReady(json & d);
    void keepAlive(const asio::error_code& error, const int ms);

    void rest_thread();

    std::chrono::steady_clock::time_point starttime;
    std::string uptime()
    {
        std::stringstream ss;
        std::chrono::steady_clock::time_point timenow = std::chrono::steady_clock::now();

        int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(timenow - starttime).count();

        uint32_t seconds = (ms / 1000) % 60;
        uint32_t minutes = (((ms / 1000) - seconds) / 60) % 60;
        uint32_t hours = (((((ms / 1000) - seconds) / 60) - minutes) / 60) % 24;
        uint32_t days = (((((((ms / 1000) - seconds) / 60) - minutes) / 60) - hours) / 24);

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

#include "aegis_impl.hpp"


