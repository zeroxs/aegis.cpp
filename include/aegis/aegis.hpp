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
#include "guild.hpp"
#include "channel.hpp"
#include "member.hpp"
#include "ratelimit.hpp"


namespace aegiscpp
{

using namespace utility;
using namespace rest_limits;

struct typing_start;
struct message_create;
struct message_update;
struct message_delete;
struct ready;
struct resumed;
struct presence_update;
struct channel_create;
struct channel_delete;
struct channel_pins_update;
struct channel_update;
struct guild_ban_add;
struct guild_ban_remove;
struct guild_create;
struct guild_delete;
struct guild_emojis_update;
struct guild_integrations_update;
struct guild_member_add;
struct guild_member_remove;
struct guild_member_update;
struct guild_members_chunk;
struct guild_role_create;
struct guild_role_delete;
struct guild_role_update;
struct guild_update;
struct message_delete_bulk;
struct message_reaction_add;
struct message_reaction_remove;
struct message_reaction_remove_all;
struct user_update;
struct voice_server_update;
struct voice_state_update;
struct webhooks_update;

struct callbacks
{
    using c_inject = std::function<void(const nlohmann::json & msg, shard * _shard, aegis & bot)>;
    /// User callbacks
    std::function<void(typing_start obj)> i_typing_start;
    std::function<void(message_create obj)> i_message_create;
    std::function<void(message_create obj)> i_message_create_dm;
    std::function<void(message_update obj)> i_message_update;
    std::function<void(message_delete obj)> i_message_delete;
    c_inject i_message_delete_bulk;
    std::function<void(guild_create obj)> i_guild_create;
    std::function<void(guild_update obj)> i_guild_update;
    std::function<void(guild_delete obj)> i_guild_delete;
    std::function<void(user_update obj)> i_user_update;
    std::function<void(ready obj)> i_ready;
    std::function<void(resumed obj)> i_resumed;
    c_inject i_channel_create;
    c_inject i_channel_update;
    c_inject i_channel_delete;
    std::function<void(guild_ban_add obj)> i_guild_ban_add;
    std::function<void(guild_ban_remove obj)> i_guild_ban_remove;
    c_inject i_guild_emojis_update;
    c_inject i_guild_integrations_update;
    std::function<void(guild_member_add obj)> i_guild_member_add;
    std::function<void(guild_member_remove obj)> i_guild_member_remove;
    std::function<void(guild_member_update obj)> i_guild_member_update;
    std::function<void(guild_members_chunk obj)> i_guild_member_chunk;
    c_inject i_guild_role_create;
    c_inject i_guild_role_update;
    c_inject i_guild_role_delete;
    std::function<void(presence_update obj)> i_presence_update;
    c_inject i_voice_state_update;
    c_inject i_voice_server_update;
};


class aegis
{
public:
    /// Type of a pointer to the asio io_service
    typedef asio::io_service * io_service_ptr;

    /// Type of a pointer to the Websocket++ client
    typedef websocketpp::client<websocketpp::config::asio_tls_client> websocket;

    /// Type of a pointer to the Websocket++ TLS connection
    typedef websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr connection_ptr;

    /// Type of a pointer to the Websocket++ message payload
    typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

    /// Type of a shared pointer to an io_service work object
    typedef std::shared_ptr<asio::io_service::work> work_ptr;

    /// Constructs the aegis object that tracks all of the shards, guilds, channels, and members
    /**
    * @see shard
    * @see guild
    * @see channel
    * @see member
    *
    * @param token A string of the authentication token
    */
    aegis(std::string token)
        : token(token)
        , status(Uninitialized)
        , shard_max_count(0)
        , ratelimit_o(std::bind(&aegis::call, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
        , member_id(0)
        , mfa_enabled(false)
        , discriminator(0)
    {
        spdlog::set_async_mode(32);
        log = spdlog::stdout_color_mt("aegis");
        log->set_level(spdlog::level::level_enum::trace);
        ratelimit_o.add(bucket_type::GUILD);
        ratelimit_o.add(bucket_type::CHANNEL);
        ratelimit_o.add(bucket_type::EMOJI);

        selfbot = false;
        owner_id = 0;
        control_channel = 0;
        force_shard_count = 0;
        debugmode = false;
    }

    /// Destroys the shards, stops the asio::work object, destroys the websocket object, and attempts to join the rest_thread thread
    ///
    ~aegis()
    {
        for (auto & c : shards)
            c.reset();
        stop_work();
        websocket_o.reset();
		if (thd->joinable())
			thd->join();
	}

    aegis(const aegis &) = delete;
    aegis(aegis &&) = delete;
    aegis & operator=(const aegis &) = delete;

    /// Perform basic initialization of the websocket object using the user-constructed asio::io_service
    /**
    * @param ptr A string of the uri path to get
    *
    * @param ec The error_code out value
    */
    void initialize(io_service_ptr ptr, std::error_code & ec)
    {
        log->info("Initializing");
        if (status != Uninitialized)
        {
            log->critical("aegis::initialize() called in the wrong state");
            using error::make_error_code;
            ec = make_error_code(error::invalid_state);
            return;
        }

        log->debug("aegis::initialize()");
        websocket_o.init_asio(ptr, ec);
        if (ec)
            return;
        status = Ready;
        ec.clear();
    }

    /// Perform basic initialization of the websocket object using the user-constructed asio::io_service
    /**
    * @see initialize(io_service_ptr, std::error_code&)
    *
    * @param ptr Pointer to a user-owned asio::io_service object pointer
    */
    void initialize(io_service_ptr ptr)
    {
        std::error_code ec;
        initialize(ptr, ec);
        if (ec)
            throw std::system_error(ec);
    }

    /// Perform basic initialization of the asio::io_service and websocket objects
    /**
    * @param ec The error_code out value
    */
    void initialize(std::error_code & ec)
    {
        log->info("Initializing");
        if (status != Uninitialized)
        {
            log->critical("aegis::initialize() called in the wrong state");
            using error::make_error_code;
            ec = make_error_code(error::invalid_state);
            return;
        }

        log->debug("aegis::initialize()");
        websocket_o.init_asio(ec);
        if (ec)
            return;
        status = Ready;
        ec.clear();
    }

    /// Perform basic initialization of the asio::io_service and websocket objects
    /**
    * @see initialize(std::error_code&)
    */
    void initialize()
    {
        std::error_code ec;
        initialize(ec);
    }

    /// Performs a basic startup sequence for initializing and connecting the library
    ///
    void easy_start()
    {
        std::error_code ec;
        // Pass our io_service object to bot to initialize
        initialize(ec);
        if (ec) { log->error("Initialize fail: {}", ec.message()); shutdown();  return; }
        // Start a work object so that asio won't exit prematurely
        start_work();
        // Start the REST outgoing thread
        thd = std::make_unique<std::thread>([&] { rest_thread(); });
        // Create our websocket connection
        websocketcreate(ec);
        if (ec) { log->error("Websocket fail: {}", ec.message()); shutdown();  return; }
        state.core = this;
        // Connect the websocket[s]
        starttime = std::chrono::steady_clock::now();
        std::thread make_connections([&]
        {
            connect(ec);
            if (ec) { log->error("Connect fail: {}", ec.message()); shutdown(); return; }
        });
        // Run the bot
        run();
        make_connections.join();
    }

    /// Invokes a shutdown on the entire lib. Sets internal state to `Shutdown`, stops the asio::work object
    /// and propogates the Shutdown state along with closing all websockets within the shard vector
    void shutdown()
    {
        set_state(Shutdown);
        stop_work();
        for (auto & _shard : shards)
        {
            _shard->connection_state = Shutdown;
            _shard->connection->close(1001, "");
            _shard->do_reset();
        }
    }

    /// Creates the parent websocket object
    /**
    * @param ec The error_code out value
    */
    void websocketcreate(std::error_code & ec)
    {
        log->info("Creating websocket");
        using error::make_error_code;
        if (status != Ready)
        {
            log->critical("aegis::websocketcreate() called in the wrong state");
            ec = make_error_code(error::invalid_state);
            return;
        }

        std::optional<rest_reply> res;

        if (!selfbot)
            res = get("/gateway/bot");
        else
            res = get("/gateway");

        if (!res.has_value() || res->content.size() == 0)
        {
            ec = make_error_code(error::get_gateway);
            return;
        }

        if (res->reply_code == 401)
        {
            ec = make_error_code(error::invalid_token);
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

        if (!selfbot)
        {
            if (force_shard_count)
            {
                shard_max_count = force_shard_count;
                log->info("Forcing Shard count by config: {}", shard_max_count);
            }
            else
            {
                shard_max_count = ret["shards"];
                log->info("Shard count: {}", shard_max_count);
            }
        }
        else
            shard_max_count = 1;

        ws_gateway = ret["url"];
        gateway_url = ws_gateway + "/?encoding=json&v=6";

        websocket_o.clear_access_channels(websocketpp::log::alevel::all);

        websocket_o.set_tls_init_handler([](websocketpp::connection_hdl)
        {
            return websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv1);
        });

        ec = std::error_code();
    }

    /// Get the internal (or external) io_service object
    asio::io_service & io_service()
    {
        return websocket_o.get_io_service();
    }

    /// Initiate websocket connection
    /**
    * @param ec The error_code out value
    */
    void connect(std::error_code & ec)
    {
        log->info("Websocket[s] connecting");

        for (uint32_t k = 0; k < shard_max_count; ++k)
        {
            auto _shard = std::make_unique<shard>();
            _shard->connection = websocket_o.get_connection(gateway_url, ec);
            _shard->shardid = k;
            _shard->state = &this->state;

            if (ec)
            {
                log->error("Websocket connection failed: {0}", ec.message());
                return;
            }

            setup_callbacks(_shard.get());

            websocket_o.connect(_shard->connection);
            shards.push_back(std::move(_shard));
            std::this_thread::sleep_for(6000ms);
        }
    }

    /// Assign the message, connect, and close callbacks to the websocket object
    /**
    * @param _shard The shard object this websocket belong to
    */
    void setup_callbacks(shard * _shard)
    {
        _shard->connection->set_message_handler([_shard, this](websocketpp::connection_hdl hdl, message_ptr msg)
        {
            this->onMessage(hdl, msg, _shard);
        });
        _shard->connection->set_open_handler([_shard, this](websocketpp::connection_hdl hdl)
        {
            this->onConnect(hdl, _shard);
        });
        _shard->connection->set_close_handler([_shard, this](websocketpp::connection_hdl hdl)
        {
            this->onClose(hdl, _shard);
        });
    }

    /// Starts the asio::work object
    ///
    void start_work()
    {
        websocket_o.start_perpetual();
    }

    /// Stops the asio::work object
    ///
    void stop_work()
    {
        websocket_o.stop_perpetual();
    }

    /// Outputs the last 5 messages received from the gateway
    ///
    void debug_trace(shard * _shard);

    /// Performs a GET request on the path
    /**
    * @see rest_reply
    * @param path A string of the uri path to get
    * 
    * @returns Response object
    */
    std::optional<rest_reply> get(std::string_view path);

    /// Performs a GET request on the path with content as the request body
    /**
    * @see rest_reply
    * @param path A string of the uri path to get
    * 
    * @param content JSON formatted string to send as the body
    *
    * @returns Response object
    */
    std::optional<rest_reply> get(std::string_view path, std::string_view content);

    /// Performs a GET request on the path with content as the request body
    /**
    * @see rest_reply
    * @param path A string of the uri path to get
    *
    * @param content JSON formatted string to send as the body
    *
    * @returns Response object
    */
    std::optional<rest_reply> post(std::string_view path, std::string_view content);

    /// Performs an HTTP request on the path with content as the request body using the method method
    /**
    * @see rest_reply
    * @param path A string of the uri path to get
    *
    * @param content JSON formatted string to send as the body
    *
    * @param method The HTTP method of the request
    *
    * @returns Response object
    */
    std::optional<rest_reply> call(std::string_view path, std::string_view content, std::string_view method);

    /**
    * Wraps the run method of the internal io_service object
    */
    std::size_t run()
    {
        return io_service().run();
    }

    /**
    * Yields operation of the current thread until library shutdown is detected
    */
    void yield() const noexcept
    {
        while (status != Shutdown)
        {
            std::this_thread::yield();
        }
    }

    ratelimiter & ratelimit() { return ratelimit_o; }
    websocket & get_websocket() { return websocket_o; }
    shard_status get_state() const { return status; }
    void set_state(shard_status s) { status = s; }
    bot_state & get_bot_obj() { return state; }

    uint32_t shard_max_count;

    std::string ws_gateway;

    std::vector<std::unique_ptr<shard>> shards;
    std::unordered_map<int64_t, std::shared_ptr<member>> members;
    std::unordered_map<int64_t, std::shared_ptr<channel>> channels;
    std::unordered_map<int64_t, std::unique_ptr<guild>> guilds;

    json self_presence;
    int64_t owner_id;
    int64_t control_channel;
    int16_t force_shard_count;
    bool selfbot;
    bool debugmode;
    std::string mention;
    callbacks _callbacks;

    std::shared_ptr<member> get_member(snowflake id) const noexcept
    {
        auto it = members.find(id);
        if (it == members.end())
            return nullptr;
        return it->second;
    }

    std::shared_ptr<channel> get_channel(snowflake id) const noexcept
    {
        auto it = channels.find(id);
        if (it == channels.end())
            return nullptr;
        return it->second;
    }

    guild * get_guild(snowflake id) const noexcept
    {
        auto it = guilds.find(id);
        if (it == guilds.end())
            return nullptr;
        return it->second.get();
    }

    std::shared_ptr<member> get_member_create(snowflake id)
    {
//         if (id == 362588408180375555)
//         {
//             log->critical("Trigger");
//         }
        auto it = members.find(id);
        if (it == members.end())
        {
            auto g = std::make_shared<member>(id);
            members.emplace(id, g);
            g->member_id = id;
            return g;
        }
        return it->second;
    }

    std::shared_ptr<channel> get_channel_create(snowflake id)
    {
        auto it = channels.find(id);
        if (it == channels.end())
        {
            auto g = std::make_shared<channel>(id, 0, ratelimit().get(rest_limits::bucket_type::CHANNEL), ratelimit().get(rest_limits::bucket_type::EMOJI));
            channels.emplace(id, g);
            g->channel_id = id;
            return g;
        }
        return it->second;
    }

    guild * get_guild_create(snowflake id, shard * _shard)
    {
        auto _guild = get_guild(id);
        if (_guild == nullptr)
        {
            auto g = std::make_unique<guild>(_shard->shardid, &state, id, ratelimit().get(rest_limits::bucket_type::GUILD));
            auto ptr = g.get();
            guilds.emplace(id, std::move(g));
            ptr->guild_id = id;
            return ptr;
        }
        return _guild;
    }

    //called by CHANNEL_CREATE (DirectMessage)
    void channel_create(const json & obj, shard * _shard);

    member * self() const noexcept
    {
        auto self_id = state.user.id;
        auto it = members.find(self_id);
        if (it == members.end())
            return nullptr;
        return it->second.get();
    }

    int64_t get_member_count() const noexcept
    {
        int64_t count = 0;
        for (auto & kv : guilds)
            count += kv.second->get_member_count();
        return count;
    }

    std::shared_ptr<spdlog::logger> log;

    std::chrono::steady_clock::time_point starttime;
    std::string uptime() const noexcept
    {
        using seconds = std::chrono::duration<int, std::ratio<1, 1>>;
        using minutes = std::chrono::duration<int, std::ratio<60, 1>>;
        using hours = std::chrono::duration<int, std::ratio<3600, 1>>;
        using days = std::chrono::duration<int, std::ratio<24 * 3600, 1>>;

        std::stringstream ss;
        std::chrono::time_point<std::chrono::steady_clock> now_t = std::chrono::steady_clock::now();

        auto time_is = now_t - starttime;
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

private:
    friend class guild;
    friend class channel;
    friend class shard;



    snowflake member_id;
    std::string username;
    bool mfa_enabled;
    int16_t discriminator;


    //std::unordered_map<std::string, c_inject> m_cbmap;

    std::unique_ptr<std::thread> thd;

    // Gateway URL for the Discord Websocket
    std::string gateway_url;

    // Websocket++ object
    websocket websocket_o;

    bot_state state;

    // Bot's token
    std::string token;

    shard_status status;

    ratelimiter ratelimit_o;

    void onMessage(websocketpp::connection_hdl hdl, message_ptr msg, shard * _shard);
    void onConnect(websocketpp::connection_hdl hdl, shard * _shard);
    void onClose(websocketpp::connection_hdl hdl, shard * _shard);
    void processReady(const json & d, shard * _shard);
    void keepAlive(const asio::error_code& error, const int ms, shard * _shard);

    void rest_thread();
};

}


