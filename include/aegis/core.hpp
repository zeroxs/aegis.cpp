//
// core.hpp
// ********
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
#include "aegis/ratelimit.hpp"
#include "aegis/objects/role.hpp"
#include "aegis/guild.hpp"
#if defined(AEGIS_HAS_STD_OPTIONAL)
#include <optional>
#else
#include "aegis/optional.hpp"
#endif

//#include <algorithm>
//#include <functional>
//#include <memory>
#include <vector>
#include <iostream>
#include <string>

#include "spdlog/spdlog.h"

// #ifdef WIN32
// # include "aegis/push.hpp"
// # include "websocketpp/config/asio_client.hpp"
// # include "websocketpp/client.hpp"
// # include "aegis/pop.hpp"
// #endif

#ifdef REDIS
# include "aegis/push.hpp"
# ifdef WIN32
#  include <redisclient/redissyncclient.h>
# else
#  include <redisclient/redissyncclient.h>
# endif
# include "aegis/pop.hpp"
#else
# include <asio.hpp>
#endif

#include <asio/bind_executor.hpp>

#if !defined(AEGIS_HAS_STD_OPTIONAL)
namespace std
{
using std::experimental::optional;
}
#endif

namespace aegis
{

#if (AEGIS_HAS_STD_SHARED_MUTEX == 1)
using shared_mutex = std::shared_mutex;
#else
using shared_mutex = std::shared_timed_mutex;
#endif

namespace rest
{
class rest_controller;

}

using json = nlohmann::json;
using namespace std::literals;

namespace gateway
{

namespace events
{
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
}

namespace objects
{
struct channel_gw;
struct guild_gw;
}

}

class shard;
class member;
class channel;
class guild;

/// DEBUG ONLY
class redis_sink : public spdlog::sinks::base_sink<std::mutex>
{
public:
#ifdef REDIS
    redis_sink(asio::io_context & _io_context, redisclient::RedisSyncClient & _redis)
        : strand(_io_context)
        , redis(_redis)
#else
    redis_sink(asio::io_context & _io_context)
        : strand(_io_context)
#endif
    {
    }
    void _sink_it(const spdlog::details::log_msg& msg) override
    {
#ifdef REDIS
        if (redis.state() == redisclient::RedisClientImpl::State::Connected)
        {
            asio::post(asio::bind_executor(strand, [msg = msg.formatted.str(), this]()
            {
                redis.command("PUBLISH", { "aegis:log", msg });
            }));
        }
#endif
    }

    void _flush() override
    {
    }
    asio::io_context::strand strand;
#ifdef REDIS
    redisclient::RedisSyncClient & redis;
#endif
};

/// Primary class for managing a bot interface
/**
 * Only one instance of this object can exist safely
 */
class core
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
    AEGIS_DECL explicit core(spdlog::level::level_enum loglevel = spdlog::level::level_enum::warn);

    /// Destroys the shards, stops the asio::work object, destroys the websocket object, and attempts to join the rest_thread thread
    AEGIS_DECL ~core();

    core(const core &) = delete;
    core(core &&) = delete;
    core & operator=(const core &) = delete;

    /// Perform basic initialization of the websocket object using the user-constructed asio::io_service
    /**
     * @param ptr A string of the uri path to get
     * @param ec The error_code out value
     */
    void initialize(io_service_ptr ptr, std::error_code & ec)
    {
        _io_context = std::shared_ptr<asio::io_context>(ptr);
        _init();
        log->info("Initializing");
        if (_status != Uninitialized)
        {
            log->error("aegis::initialize() called in the wrong state");
            ec = make_error_code(error::invalid_state);
            return;
        }

        websocket_o.init_asio(ptr, ec);
        if (ec)
            return;
        starttime = std::chrono::steady_clock::now();
        set_state(Ready);
        ec.clear();
        ws_open_strand = std::make_unique<asio::io_context::strand>(io_service());
    }

    /// Perform basic initialization of the websocket object using the user-constructed asio::io_service
    /**
     * @see initialize(io_service_ptr, std::error_code&)
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
        _io_context = std::make_shared<asio::io_context>();
        _init();
        log->info("Initializing");
        if (_status != Uninitialized)
        {
            log->error("aegis::initialize() called in the wrong state");
            ec = make_error_code(aegis::error::invalid_state);
            return;
        }

        websocket_o.init_asio(_io_context.get(), ec);
        if (ec)
            return;
        starttime = std::chrono::steady_clock::now();
        set_state(Ready);
        ec.clear();
        ws_open_strand = std::make_unique<asio::io_context::strand>(io_service());
    }

    /// Perform basic initialization of the asio::io_service and websocket objects
    /**
     * @see initialize(std::error_code&)
     */
    void initialize()
    {
        std::error_code ec;
        initialize(ec);
        if (ec)
            throw std::system_error(ec);
    }

    /// Assign the message, connect, and close callbacks to the websocket object
    /**
     * @param _shard The shard object this websocket belong to
     */
    AEGIS_DECL void setup_callbacks(shard * _shard);

    /// Outputs the last 5 messages received from the gateway
    ///
    AEGIS_DECL void debug_trace(shard * _shard, bool extended = false);

    /// Get the internal (or external) io_service object
    asio::io_context & io_service()
    {
        return *_io_context;
    }

    /// Invokes a shutdown on the entire lib. Sets internal state to `Shutdown`, stops the asio::work object
    /// and propagates the Shutdown state along with closing all websockets within the shard vector
    AEGIS_DECL void shutdown();

    /// Create new guild - Unique case. Does not belong to any ratelimit bucket so it is run
    /// directly on the same thread and does not attempt to manage ratelimits due to the already
    /// existing requirement that the bot must be in less than 10 guilds for this call to succeed
    /**
     * @param ec Indicates what error occurred, if any
     * @param name Set name of guild
     * @param voice_region Set region for voice
     * @param verification_level Set verification level from unrestricted level to verified phone level
     * (NONE=0, LOW(verified email)=1, MEDIUM(registered >5m)=2, HIGH(member of server >10m)=3 VERY_HIGH(verified phone)=4
     * @param default_message_notifications Set default notification level for new members
     * @param explicit_content_filter Set filter level for new content
     * (DISABLED=0, MEMBERS_WITHOUT_ROLES=1, ALL_MEMBERS=2)
     * @param afk_channel_id Set channel for idle voice connections to be moved to
     * @param afk_timeout Set time where voice connections are considered to be idle
     * @param icon Set icon \todo
     * @param roles vector of roles to create
     * @param channels vector of channels to create
     * @returns rest_reply
     */
    AEGIS_DECL rest::rest_reply create_guild(
        std::error_code & ec, std::string name,
        std::optional<std::string> voice_region = {}, std::optional<int> verification_level = {},
        std::optional<int> default_message_notifications = {}, std::optional<int> explicit_content_filter = {},
        std::optional<std::string> icon = {}, std::optional<std::vector<gateway::objects::role>> roles = {},
        std::optional<std::vector<std::tuple<std::string, int>>> channels = {}
    );

    /// Create new guild - Unique case. Does not belong to any ratelimit bucket so it is run
    /// directly on the same thread and does not attempt to manage ratelimits due to the already
    /// existing requirement that the bot must be in less than 10 guilds for this call to succeed
    /**
     * @param name Set name of guild
     * @param voice_region Set region for voice
     * @param verification_level Set verification level from unrestricted level to verified phone level
     * (NONE=0, LOW(verified email)=1, MEDIUM(registered >5m)=2, HIGH(member of server >10m)=3 VERY_HIGH(verified phone)=4
     * @param default_message_notifications Set default notification level for new members
     * @param explicit_content_filter Set filter level for new content
     * (DISABLED=0, MEMBERS_WITHOUT_ROLES=1, ALL_MEMBERS=2)
     * @param afk_channel_id Set channel for idle voice connections to be moved to
     * @param afk_timeout Set time where voice connections are considered to be idle
     * @param icon Set icon \todo
     * @param roles vector of roles to create
     * @param channels vector of channels to create
     * @throws aegiscpp::exception Thrown on failure.
     * @returns rest_reply
     */
    AEGIS_DECL rest::rest_reply create_guild(
        std::string name,
        std::optional<std::string> voice_region = {}, std::optional<int> verification_level = {},
        std::optional<int> default_message_notifications = {}, std::optional<int> explicit_content_filter = {},
        std::optional<std::string> icon = {}, std::optional<std::vector<gateway::objects::role>> roles = {},
        std::optional<std::vector<std::tuple<std::string, int>>> channels = {}
    );

    /// Spawns and starts the specified amount of threads on the io_context
    /// Will automatically run core::initialize() if it has not been executed
    /**
     * @param count Spawn `count` threads and run all on io_context object
     */
    AEGIS_DECL void run(std::size_t count = 0);

    /// Spawns and starts the default hardware hinted threads on the io_context
    /// and executes the provided functor prior to connecting the gateway
    /// Will automatically run core::initialize() if it has not been executed
    /**
     * @param count Spawn `count` threads and run io_service object
     */
    AEGIS_DECL void run(std::function<void(void)> f);

    /// Spawns and starts the specified amount of threads on the io_context
    /// and executes the provided functor prior to connecting the gateways
    /// Will automatically run core::initialize() if it has not been executed
    /**
     * @param count Spawn `count` threads and run io_service object
     */
    AEGIS_DECL void run(std::size_t count, std::function<void(void)> f);

    /**
     * Yields operation of the current thread until library shutdown is detected
     */
    void yield() const AEGIS_NOEXCEPT
    {
        while (_status != Shutdown)
        {
            std::this_thread::yield();
        }
    }

    rest::ratelimiter & ratelimit() AEGIS_NOEXCEPT { return ratelimit_o; }
    websocket & get_websocket() AEGIS_NOEXCEPT { return websocket_o; }
    bot_status get_state() const AEGIS_NOEXCEPT { return _status; }
    void set_state(bot_status s) AEGIS_NOEXCEPT { _status = s; }

    /// Helper function for posting tasks to asio
    /**
     * @param f A callable to execute within asio - signature should be void(void)
     */
    template<typename T>
    AEGIS_DECL void async(T f)
    {
        asio::post(io_service(), std::move(f));
    }

#if !defined(AEGIS_DISABLE_ALL_CACHE)

    member * self() const
    {
        if (_self == nullptr)
            throw exception("Self not found", make_error_code(error::member_not_found));
        return _self;
    }

    AEGIS_DECL int64_t get_member_count() const AEGIS_NOEXCEPT;

    /// Obtain a pointer to a member by snowflake
    /**
     * @param id Snowflake of member to search for
     * @returns Pointer to member or nullptr
     */
    AEGIS_DECL member * find_member(snowflake id) const AEGIS_NOEXCEPT;

    /// Obtain a pointer to a member by snowflake. If none exists, creates the object.
    /**
     * @param id Snowflake of member to search for
     * @returns Pointer to member
     */
    AEGIS_DECL member * member_create(snowflake id) AEGIS_NOEXCEPT;
#endif

    /// Obtain a pointer to a channel by snowflake
    /**
     * @param id Snowflake of channel to search for
     * @returns Pointer to channel or nullptr
     */
    AEGIS_DECL channel * find_channel(snowflake id) const AEGIS_NOEXCEPT;

    /// Obtain a pointer to a channel by snowflake. If none exists, creates the object.
    /**
     * @param id Snowflake of channel to search for
     * @returns Pointer to channel
     */
    AEGIS_DECL channel * channel_create(snowflake id) AEGIS_NOEXCEPT;

    /// Obtain a pointer to a guild by snowflake
    /**
     * @param id Snowflake of guild to search for
     * @returns Pointer to guild or nullptr
     */
    AEGIS_DECL guild * find_guild(snowflake id) const AEGIS_NOEXCEPT;

    /// Obtain a pointer to a guild by snowflake. If none exists, creates the object.
    /**
     * @param id Snowflake of guild to search for
     * @param _shard Shard this guild will exist on
     * @returns Pointer to guild
     */
    AEGIS_DECL guild * guild_create(snowflake id, shard * _shard) AEGIS_NOEXCEPT;

    /// Called by CHANNEL_CREATE (DirectMessage)
    /**
     * @param obj json obj of DM channel
     * @param _shard Shard this channel will exist on
     * @returns Pointer to channel
     */
    AEGIS_DECL channel * dm_channel_create(const json & obj, shard * _shard);

    /// Return bot uptime as {days hours minutes seconds}
    /**
     * @returns std::string of 'formatted' time
     */
    AEGIS_DECL std::string uptime() const AEGIS_NOEXCEPT;

    snowflake get_id() const AEGIS_NOEXCEPT
    {
        return member_id;
    }

    int16_t get_discriminator() const AEGIS_NOEXCEPT
    {
        return discriminator;
    }

    /// Performs a GET request on the path
    /**
     * @see rest_reply
     * @param path A string of the uri path to get
     * @returns Response object
     */
    AEGIS_DECL rest::rest_reply get(const std::string & path);

    /// Performs a GET request on the path with content as the request body
    /**
     * @see rest_reply
     * @param path A string of the uri path to get
     * @param content JSON formatted string to send as the body
     * @param host Provide only if the call should go to a different host
     * @returns Response object
     */
    AEGIS_DECL rest::rest_reply get(const std::string & path, const std::string & content, const std::string & host = "");

    /// Performs a GET request on the path with content as the request body
    /**
     * @see rest_reply
     * @param path A string of the uri path to get
     * @param content JSON formatted string to send as the body
     * @param host Provide only if the call should go to a different host
     * @returns Response object
     */
    AEGIS_DECL rest::rest_reply post(const std::string & path, const std::string & content, const std::string & host = "");

    /// Performs an HTTP request on the path with content as the request body using the method method
    /**
     * @see rest_reply
     * @param path A string of the uri path to get
     * @param content JSON formatted string to send as the body
     * @param method The HTTP method of the request
     * @param host Provide only if the call should go to a different host
     * @returns Response object
     */
    AEGIS_DECL rest::rest_reply call(const std::string & path, const std::string & content, const std::string & method, const std::string & host = "");

    std::mutex m;
    std::condition_variable cv;
    std::shared_ptr<asio::io_context> _io_context;

    uint32_t shard_max_count;

    std::string ws_gateway;

    std::vector<std::unique_ptr<shard>> shards;
    std::map<snowflake, std::unique_ptr<channel>> channels;
    std::map<snowflake, std::unique_ptr<guild>> guilds;
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    std::map<snowflake, std::unique_ptr<member>> members;
#endif
    std::map<std::string, uint64_t> message_count;

    // DEBUG CODE ONLY
#ifdef REDIS
    std::unique_ptr<redisclient::RedisSyncClient> redis;
#endif

    std::string self_presence;
    int16_t force_shard_count;
    std::string mention;
    bool wsdbg = false;
    std::unique_ptr<asio::io_context::strand> ws_open_strand;
    std::shared_ptr<spdlog::logger> log;

#ifdef AEGIS_PROFILING
    std::function<void(std::chrono::steady_clock::time_point, const std::string&)> message_end;
    std::function<void(std::chrono::steady_clock::time_point)> call_end;
    std::function<void(std::chrono::steady_clock::time_point, const std::string&)> js_end;
#endif

    std::function<void(gateway::events::typing_start obj)> i_typing_start;/**< TYPING_START callback */
    std::function<void(gateway::events::message_create obj)> i_message_create;/**< MESSAGE_CREATE callback */
    std::function<void(gateway::events::message_create obj)> i_message_create_dm;/**< MESSAGE_CREATE callback for direct messages */
    std::function<void(gateway::events::message_update obj)> i_message_update;/**< MESSAGE_UPDATE callback */
    std::function<void(gateway::events::message_delete obj)> i_message_delete;/**< MESSAGE_DELETE callback */
    std::function<void(gateway::events::message_delete_bulk obj)> i_message_delete_bulk;/**<\todo MESSAGE_DELETE_BULK callback */
    std::function<void(gateway::events::guild_create obj)> i_guild_create;/**< GUILD_CREATE callback */
    std::function<void(gateway::events::guild_update obj)> i_guild_update;/**< GUILD_UPDATE callback */
    std::function<void(gateway::events::guild_delete obj)> i_guild_delete;/**< GUILD_DELETE callback */
    std::function<void(gateway::events::user_update obj)> i_user_update;/**< USER_UPDATE callback */
    std::function<void(gateway::events::ready obj)> i_ready;/**< READY callback */
    std::function<void(gateway::events::resumed obj)> i_resumed;/**< RESUME callback */
    std::function<void(gateway::events::channel_create obj)> i_channel_create;/**<\todo CHANNEL_CREATE callback */
    std::function<void(gateway::events::channel_update obj)> i_channel_update;/**<\todo CHANNEL_UPDATE callback */
    std::function<void(gateway::events::channel_delete obj)> i_channel_delete;/**<\todo CHANNEL_DELETE callback */
    std::function<void(gateway::events::guild_ban_add obj)> i_guild_ban_add;/**< GUILD_BAN_ADD callback */
    std::function<void(gateway::events::guild_ban_remove obj)> i_guild_ban_remove;/**< GUILD_BAN_REMOVE callback */
    std::function<void(gateway::events::guild_emojis_update obj)> i_guild_emojis_update;/**<\todo GUILD_EMOJIS_UPDATE callback */
    std::function<void(gateway::events::guild_integrations_update obj)> i_guild_integrations_update;/**<\todo GUILD_INTEGRATIONS_UPDATE callback */
    std::function<void(gateway::events::guild_member_add obj)> i_guild_member_add;/**< GUILD_MEMBER_ADD callback */
    std::function<void(gateway::events::guild_member_remove obj)> i_guild_member_remove;/**< GUILD_MEMBER_REMOVE callback */
    std::function<void(gateway::events::guild_member_update obj)> i_guild_member_update;/**< GUILD_MEMBER_UPDATE callback */
    std::function<void(gateway::events::guild_members_chunk obj)> i_guild_member_chunk;/**< GUILD_MEMBERS_CHUNK callback */
    std::function<void(gateway::events::guild_role_create obj)> i_guild_role_create;/**<\todo GUILD_ROLE_CREATE callback */
    std::function<void(gateway::events::guild_role_update obj)> i_guild_role_update;/**<\todo GUILD_ROLE_UPDATE callback */
    std::function<void(gateway::events::guild_role_delete obj)> i_guild_role_delete;/**<\todo GUILD_ROLE_DELETE callback */
    std::function<void(gateway::events::presence_update obj)> i_presence_update;/**< PRESENCE_UPDATE callback */
    std::function<void(gateway::events::voice_state_update obj)> i_voice_state_update;/**<\todo VOICE_STATE_UPDATE callback */
    std::function<void(gateway::events::voice_server_update obj)> i_voice_server_update;/**<\todo VOICE_SERVER_UPDATE callback */

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

private:

    AEGIS_DECL void _init();
    AEGIS_DECL void _run(std::size_t count = 0, std::function<void(void)> f = {});
    AEGIS_DECL void setup_gateway(std::error_code & ec);

    AEGIS_DECL void reset_shard(shard * _shard);

    friend class guild;
    friend class channel;
    friend class shard;

    AEGIS_DECL void ws_presence_update(const json & result, shard * _shard);
    AEGIS_DECL void ws_typing_start(const json & result, shard * _shard);
    AEGIS_DECL void ws_message_create(const json & result, shard * _shard);
    AEGIS_DECL void ws_message_update(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_create(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_update(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_delete(const json & result, shard * _shard);
    AEGIS_DECL void ws_message_delete(const json & result, shard * _shard);
    AEGIS_DECL void ws_message_delete_bulk(const json & result, shard * _shard);
    AEGIS_DECL void ws_user_update(const json & result, shard * _shard);
    AEGIS_DECL void ws_voice_state_update(const json & result, shard * _shard);
    AEGIS_DECL void ws_resumed(const json & result, shard * _shard);
    AEGIS_DECL void ws_ready(const json & result, shard * _shard);
    AEGIS_DECL void ws_channel_create(const json & result, shard * _shard);
    AEGIS_DECL void ws_channel_update(const json & result, shard * _shard);
    AEGIS_DECL void ws_channel_delete(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_ban_add(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_ban_remove(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_emojis_update(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_integrations_update(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_member_add(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_member_remove(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_member_update(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_members_chunk(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_role_create(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_role_update(const json & result, shard * _shard);
    AEGIS_DECL void ws_guild_role_delete(const json & result, shard * _shard);
    AEGIS_DECL void ws_voice_server_update(const json & result, shard * _shard);

    AEGIS_DECL void on_message(websocketpp::connection_hdl hdl, message_ptr msg, shard * _shard);
    AEGIS_DECL void on_connect(websocketpp::connection_hdl hdl, shard * _shard);
    AEGIS_DECL void on_close(websocketpp::connection_hdl hdl, shard * _shard);
    AEGIS_DECL void process_ready(const json & d, shard * _shard);
    AEGIS_DECL void keep_alive(const asio::error_code & error, const int32_t ms, shard * _shard);
    AEGIS_DECL void ws_status(const asio::error_code & ec);

    AEGIS_DECL void load_config();

    AEGIS_DECL void remove_channel(snowflake channel_id) AEGIS_NOEXCEPT;

    AEGIS_DECL void remove_member(snowflake member_id) AEGIS_NOEXCEPT;

    AEGIS_DECL void queue_reconnect(shard * _shard);

    std::chrono::steady_clock::time_point starttime;

    snowflake member_id;
    int16_t discriminator;
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    std::string username;
    bool mfa_enabled;
#endif

    // Gateway URL for the Discord Websocket
    std::string gateway_url;

    // Websocket++ object
    websocket websocket_o;

    // Bot's token
    std::string token;

    bot_status _status;

    std::shared_ptr<rest::rest_controller> _rest;

    rest::ratelimiter ratelimit_o;

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    member * _self = nullptr;
#endif

    std::shared_ptr<asio::steady_timer> ws_timer;
    std::shared_ptr<asio::steady_timer> ws_connect_timer;
    std::unordered_map<std::string, std::function<void(const json &, shard *)>> ws_handlers;
    spdlog::level::level_enum _loglevel = spdlog::level::level_enum::info;
    std::chrono::time_point<std::chrono::steady_clock> _last_ready;
    std::chrono::time_point<std::chrono::steady_clock> _connect_time;
    std::deque<shard*> _shards_to_connect;
    shard * _connecting_shard;
    mutable shared_mutex _shard_m;
    mutable shared_mutex _guild_m;
    mutable shared_mutex _channel_m;
    mutable shared_mutex _member_m;

#if defined(REDIS)
    std::string redis_address;
    uint16_t redis_port;
#endif
    bool file_logging = false;
};

}
