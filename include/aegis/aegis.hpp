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
#include "member.hpp"
#include "channel.hpp"
#include "guild.hpp"
#include "client.hpp"
#include "ratelimit.hpp"
#include "utility.hpp"
#include "snowflake.hpp"

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

namespace aegis
{

namespace spd = spdlog;
using namespace std::literals;
using namespace std::chrono;
namespace placeholders = std::placeholders;
using namespace aegis::rest_limits;
using json = nlohmann::json;
using std::function;
using std::bind;
using std::ref;

struct settings;

using utility::check_setting;

void to_json(json& j, const snowflake& s)
{
    j = json{ s };
}

class Aegis
{
public:

    using c_inject = std::function<bool(json & msg, client & shard, Aegis & bot)>;

    /// Type of a pointer to the ASIO io_service
    typedef asio::io_service * io_service_ptr;

    /// Type of a pointer to the Websocket++ client
    typedef websocketpp::client<websocketpp::config::asio_tls_client> websocket_o;

    /// Type of a pointer to the Websocket++ TLS connection
    typedef websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr connection_ptr;

    /// Type of a pointer to the Websocket++ message payload
    typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

    /// Type of a shared pointer to an io_service work object
    typedef std::shared_ptr<asio::io_service::work> work_ptr;

    Aegis(std::string token)
        : m_token(token)
        , m_state(UNINITIALIZED)
        , m_shardidmax(0)
        , m_ratelimit(std::bind(&Aegis::call, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
        , m_snowflake(0)
        , m_mfa_enabled(false)
        , m_discriminator(0)
        , m_isuserset(false)
    {
        m_log = spd::stdout_color_mt("aegis");
        m_ratelimit.add(bucket_type::GUILD);
        m_ratelimit.add(bucket_type::CHANNEL);
    }

    ~Aegis()
    {
        for (auto c : m_clients)
            delete c;
        m_work.reset();
        m_websocket.reset();
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
        if (ec) { m_log->error("Websocket fail: {}", ec.message()); stop_work();  return; }
        // Connect the websocket[s]
        connect(ec);
        if (ec) { m_log->error("Connect fail: {}", ec.message()); stop_work(); return; }
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

        if constexpr (!check_setting<settings>::selfbot::test())
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

        if constexpr (!check_setting<settings>::selfbot::test())
        {
            if constexpr (check_setting<settings>::force_shard_count::test())
            {
                m_shardidmax = settings::force_shard_count;
                m_log->info("Forcing Shard count by config: {}", m_shardidmax);
            }
            else
            {
                m_shardidmax = ret["shards"];
                m_log->info("Shard count: {}", m_shardidmax);
            }
        }
        else
            m_shardidmax = 1;

        m_getgateway = ret["url"].get<std::string>();
        m_gatewayurl = m_getgateway + "/?encoding=json&v=6";

        m_websocket.clear_access_channels(websocketpp::log::alevel::all);

        m_websocket.set_tls_init_handler([](websocketpp::connection_hdl)
        {
            return websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv1);
        });

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
        m_log->info("Websocket[s] connecting");

        for (uint32_t k = 0; k < m_shardidmax; ++k)
        {
            auto shard = new client(*this);
            shard->m_connection = m_websocket.get_connection(m_gatewayurl, ec);
            shard->m_shardid = k;

            setup_callbacks(*shard);

            if (ec)
            {
                delete shard;
                m_log->error("Websocket connection failed: {0}", ec.message());
                return;
            }
            m_websocket.connect(shard->m_connection);
            m_clients.push_back(shard);
        }
    }

    void setup_callbacks(client & shard)
    {
        shard.m_connection->set_message_handler([&shard, this](websocketpp::connection_hdl hdl, message_ptr msg)
        {
            this->onMessage(hdl, msg, shard);
        });
        shard.m_connection->set_open_handler([&shard, this](websocketpp::connection_hdl hdl)
        {
            this->onConnect(hdl, shard);
        });
        shard.m_connection->set_close_handler([&shard, this](websocketpp::connection_hdl hdl)
        {
            this->onClose(hdl, shard);
        });
        shard.m_connection->set_fail_handler([&shard, this](websocketpp::connection_hdl hdl)
        {
            this->onFail(hdl, shard);
        });
    }

    void start_work()
    {
        m_work = std::make_shared<asio::io_service::work>(std::ref(io_service()));
    }

    void stop_work()
    {
        m_work.reset();
    }

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

    /// User callbacks
    c_inject i_typing_start;
    c_inject i_message_create;
    c_inject i_message_update;
    c_inject i_message_delete;
    c_inject i_message_delete_bulk;
    c_inject i_guild_create;
    c_inject i_guild_update;
    c_inject i_guild_delete;
    c_inject i_user_settings_update;
    c_inject i_user_update;
    c_inject i_ready;
    c_inject i_resumed;
    c_inject i_channel_create;
    c_inject i_channel_update;
    c_inject i_channel_delete;
    c_inject i_guild_ban_add;
    c_inject i_guild_ban_remove;
    c_inject i_guild_emojis_update;
    c_inject i_guild_integrations_update;
    c_inject i_guild_member_add;
    c_inject i_guild_member_remove;
    c_inject i_guild_member_update;
    c_inject i_guild_member_chunk;
    c_inject i_guild_role_create;
    c_inject i_guild_role_update;
    c_inject i_guild_role_delete;
    c_inject i_presence_update;
    c_inject i_voice_state_update;
    c_inject i_voice_server_update;

    ratelimiter & ratelimit() { return m_ratelimit; }
    websocket_o & websocket() { return m_websocket; }
    state get_state() const { return m_state; }
    void set_state(state s) { m_state = s; }

    uint32_t m_shardidmax;

    std::string m_getgateway;

    std::vector<client*> m_clients;
    std::map<int64_t, std::shared_ptr<member>> m_members;
    std::map<int64_t, std::unique_ptr<channel>> m_channels;//for DMs
    std::map<int64_t, std::unique_ptr<guild>> m_guilds;

    member & get_member(int64_t id)
    {
        return *m_members[id];
    }

    channel & get_channel(int64_t id)
    {
        return *m_channels[id];
    }

    guild & get_guild(int64_t id)
    {
        return *m_guilds[id];
    }

    json self_presence;

    std::chrono::steady_clock::time_point starttime;
    std::string uptime()
    {
        std::stringstream ss;
        std::chrono::steady_clock::time_point timenow = std::chrono::steady_clock::now();

        int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(timenow - starttime).count();

        uint64_t seconds = (ms / 1000) % 60;
        uint64_t minutes = (((ms / 1000) - seconds) / 60) % 60;
        uint64_t hours = (((((ms / 1000) - seconds) / 60) - minutes) / 60) % 24;
        uint64_t days = (((((((ms / 1000) - seconds) / 60) - minutes) / 60) - hours) / 24);

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

    snowflake m_snowflake;
    std::string m_username;
    bool m_mfa_enabled;
    int16_t m_discriminator;
    std::string m_mention;
    bool m_isuserset;

    std::shared_ptr<spd::logger> m_log;

private:

    //std::map<std::string, c_inject> m_cbmap;

    std::unique_ptr<std::thread> thd;

    // Gateway URL for the Discord Websocket
    std::string m_gatewayurl;

    // Websocket++ object
    websocket_o m_websocket;

    // Bot's token
    std::string m_token;

    // Work object for ASIO
    work_ptr m_work;


    state m_state;


    ratelimiter m_ratelimit;

    void onMessage(websocketpp::connection_hdl hdl, message_ptr msg, client & shard);
    void onConnect(websocketpp::connection_hdl hdl, client & shard);
    void onClose(websocketpp::connection_hdl hdl, client & shard);
    void onFail(websocketpp::connection_hdl hdl, client & shard);
    void processReady(json & d, client & shard);
    void keepAlive(const asio::error_code& error, const int ms, client & shard);

    void rest_thread();
};

}

#include "aegis_impl.hpp"


