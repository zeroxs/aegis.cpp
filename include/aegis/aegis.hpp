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

    explicit Aegis(std::string token)
        : m_io_service(nullptr)
        , m_external_io_service(false)
        , m_token(token)
        , m_state(UNINITIALIZED)
        , m_sequence(0)
        , m_shardid(0)
        , m_shardidmax(0)
    {
        m_log = spd::stdout_color_mt("aegis");
    }

    ~Aegis()
    {
        m_work.reset();
        if (m_state != UNINITIALIZED && !m_external_io_service)
            delete m_io_service;
    }

    Aegis(const Aegis &) = delete;
    Aegis(Aegis &&) = delete;
    Aegis & operator=(const Aegis &) = delete;

    void initialize(io_service_ptr ptr, std::error_code & ec)
    {
        if (m_state != UNINITIALIZED)
        {
            m_log->critical("aegis::initialize() called in the wrong state");
            using aegis::error::make_error_code;
            ec = make_error_code(aegis::error::invalid_state);
            return;
        }

        m_log->debug("aegis::initialize()");
        m_io_service = ptr;
        m_external_io_service = true;
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
        m_external_io_service = false;
    }

    void initialize()
    {
        std::unique_ptr<asio::io_service> service(new asio::io_service());
        initialize(service.get());
        service.release();
        m_external_io_service = false;
    }

    /// Remove the logger instance from spdlog
    void remove_logger()
    {
        spd::drop("aegis");
    }

    void websocketcreate()
    {
        std::optional<std::string> res = get("/gateway/bot");

        if (!res.has_value())
        {
            throw std::runtime_error("Error retrieving gateway.");
        }

        json ret = json::parse(res.value());
        if (ret.count("message"))
            if (ret["message"] == "401: Unauthorized")
                throw std::runtime_error("Token is unauthorized.");

        m_gatewayurl = ret["url"].get<std::string>();


        m_websocket.clear_access_channels(websocketpp::log::alevel::all);

        m_websocket.set_tls_init_handler([](websocketpp::connection_hdl)
        {
            return websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv1);
        });

        m_websocket.init_asio(m_io_service);

        m_websocket.set_message_handler(std::bind(&Aegis::onMessage, this, std::placeholders::_1, std::placeholders::_2));
        m_websocket.set_open_handler(std::bind(&Aegis::onConnect, this, std::placeholders::_1));
        m_websocket.set_close_handler(std::bind(&Aegis::onClose, this, std::placeholders::_1));
        m_websocket.set_fail_handler(std::bind(&Aegis::onClose, this, std::placeholders::_1));
    }

    asio::io_service & get_io_service()
    {
        return *m_io_service;
    }

    void connect()
    {
        asio::error_code ec;
        m_connection = m_websocket.get_connection("wss://gateway.discord.gg/?encoding=json&v=6", ec);
        if (ec)
        {
            throw std::runtime_error(fmt::format("Websocket connection failed: {0}", ec.message()));
        }
        m_websocket.connect(m_connection);
    }

    void start_work()
    {
        m_work = std::make_shared<asio::io_service::work>(std::ref(*m_io_service));
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
    std::optional<std::string> get(const std::string & path);

    /// Performs a GET request on the path with content as the request body
    /**
    * @param path A string of the uri path to get
    * 
    * @param content JSON formatted string to send as the body
    *
    * @returns std::optional<std::string>
    */
    std::optional<std::string> get(const std::string & path, const std::string & content);

    /// Performs a GET request on the path with content as the request body
    /**
    * @param path A string of the uri path to get
    *
    * @param content JSON formatted string to send as the body
    *
    * @returns std::optional<std::string>
    */
    std::optional<std::string> post(const std::string & path, const std::string & content);

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
    std::optional<std::string> call(const std::string & path, const std::string & content, const std::string method);


    /// wraps the run method of the internal io_service object
    std::size_t run()
    {
        return m_io_service->run();
    }

    /// Yield execution
    void yield()
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
        RECONNECTING = 3,
        SHUTDOWN = 4
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

    // pointer to ASIO io_service object
    io_service_ptr m_io_service;

    // true if io_service object is not managed by this library
    bool m_external_io_service;

    // Websocket++ object
    websocket m_websocket;

    // Websocket++ connection
    connection_ptr m_connection;

    // Bot's token
    std::string m_token;

    // 
    std::shared_ptr<asio::steady_timer> keepalive_timer;

    // Work object for ASIO
    work_ptr m_work;


    state m_state;
    uint64_t m_sequence;
    uint32_t m_shardid, m_shardidmax;

    void onMessage(websocketpp::connection_hdl hdl, message_ptr msg);
    void onConnect(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void userMessage(json & obj);
    void processReady(json & d);
    void keepAlive(const asio::error_code& error, const long ms);
};

}

#include "aegis_impl.hpp"


