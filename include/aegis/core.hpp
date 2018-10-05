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
#include "aegis/ratelimit/ratelimit.hpp"
#include "aegis/shards/shard_mgr.hpp"
#include "aegis/objects/role.hpp"
#include "aegis/guild.hpp"

#include <vector>
#include <iostream>
#include <string>

#include "spdlog/spdlog.h"

#ifdef WIN32
# include "aegis/push.hpp"
# include "websocketpp/config/asio_client.hpp"
# include "websocketpp/client.hpp"
# include "aegis/pop.hpp"
#endif

#include "aegis/fwd.hpp"

#include <asio/bind_executor.hpp>

namespace aegis
{

/// Type of a work guard executor for keeping Asio services alive
using asio_exec = asio::executor_work_guard<asio::io_context::executor_type>;

/// Type of a shared pointer to an io_context work object
using work_ptr = std::unique_ptr<asio_exec>;

using json = nlohmann::json;
using namespace std::literals;

#if !defined(AEGIS_CXX17)
template<class T>
struct V
{
    static core * bot;
    static std::unique_ptr<asio::io_context> _io_context;
    static std::vector<std::thread> threads;
    static work_ptr wrk;
    static std::condition_variable cv;
    static void shutdown()
    {
        cv.notify_all();
    }
};
#endif

#if defined(AEGIS_CXX17)
struct internal
{
    static inline core * bot = nullptr;
    static inline std::unique_ptr<asio::io_context> _io_context = nullptr;
    static inline std::vector<std::thread> threads;
    static inline work_ptr wrk = nullptr;
    static inline std::condition_variable cv;
    static inline void shutdown()
    {
        cv.notify_all();
    }
};
#endif

#if !defined(AEGIS_CXX17)
template<class T>
core * V<T>::bot = nullptr;
template<class T>
std::unique_ptr<asio::io_context> V<T>::_io_context = nullptr;
template<class T>
std::vector<std::thread> V<T>::threads;
template<class T>
work_ptr V<T>::wrk = nullptr;
template<class T>
std::condition_variable V<T>::cv;

using internal = V<void>;
#endif

#if (AEGIS_HAS_STD_SHARED_MUTEX == 1)
using shared_mutex = std::shared_mutex;
#else
using shared_mutex = std::shared_timed_mutex;
#endif

/// Primary class for managing a bot interface
/**
 * Only one instance of this object can exist safely
 */
class core
{
public:
    /// Type of a pointer to the Websocket++ TLS connection
    using connection_ptr = websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr;

    /// Type of a pointer to the Websocket++ message payload
    using message_ptr = websocketpp::config::asio_client::message_type::ptr;

    using ratelimit_mgr_t = aegis::ratelimit::ratelimit_mgr<rest_call, aegis::rest::rest_reply>;
  
    /// Constructs the aegis object that tracks all of the shards, guilds, channels, and members
    /**
     * @see shard
     * @see guild
     * @see channel
     * @see member
     * @see spdlog::level::level_enum
     * @param loglevel The level of logging to use
     */
    AEGIS_DECL explicit core(spdlog::level::level_enum loglevel = spdlog::level::level_enum::info, std::size_t count = 2);

    AEGIS_DECL explicit core(std::unique_ptr<asio::io_context> _io, spdlog::level::level_enum loglevel = spdlog::level::level_enum::info);

    AEGIS_DECL explicit core(std::shared_ptr<spdlog::logger> _log, std::size_t count = 2);

    AEGIS_DECL explicit core(std::unique_ptr<asio::io_context> _io, std::shared_ptr<spdlog::logger> _log);

    /// Destroys the shards, stops the asio::work object, destroys the websocket object
    AEGIS_DECL ~core();

    core(const core &) = delete;
    core(core &&) = delete;
    core & operator=(const core &) = delete;

    /// Outputs the last 5 messages received from the gateway
    /**
     * @param _shard Pointer to the shard object to dump recent messages
     */
    AEGIS_DECL void debug_trace(shards::shard * _shard);

    AEGIS_DECL void setup_logging();

    AEGIS_DECL void setup_context();

    AEGIS_DECL void setup_shard_mgr();

    /// Get the internal (or external) io_service object
//     asio::io_context & get_io_context()
//     {
//         return *_io_context;
//     }

    static asio::io_context & get_io_context()
    {
        return *internal::_io_context;
    }

    static core & get_bot()
    {
        return *internal::bot;
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
        lib::optional<std::string> voice_region = {}, lib::optional<int> verification_level = {},
        lib::optional<int> default_message_notifications = {}, lib::optional<int> explicit_content_filter = {},
        lib::optional<std::string> icon = {}, lib::optional<std::vector<gateway::objects::role>> roles = {},
        lib::optional<std::vector<std::tuple<std::string, int>>> channels = {}
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
     * @returns rest::rest_reply
     */
    AEGIS_DECL rest::rest_reply create_guild(
        std::string name,
        lib::optional<std::string> voice_region = {}, lib::optional<int> verification_level = {},
        lib::optional<int> default_message_notifications = {}, lib::optional<int> explicit_content_filter = {},
        lib::optional<std::string> icon = {}, lib::optional<std::vector<gateway::objects::role>> roles = {},
        lib::optional<std::vector<std::tuple<std::string, int>>> channels = {}
    );

    /// Spawns and starts the default hardware hinted threads on the io_context
    /// and executes the provided functor prior to connecting the gateway
    /**
     * @param f Run f() after gateway information is obtained but before connection
     */
    AEGIS_DECL void run(std::function<void(void)> f);

    /**
     * Yields operation of the current thread until library shutdown is detected
     */
    void yield() const noexcept
    {
        std::mutex m;
        std::unique_lock<std::mutex> l(m);
        internal::cv.wait(l);

        log->info("Closing bot");
    
        if (!external_io_context)
        {
            internal::wrk.reset();
            internal::_io_context->stop();
        }
        for (auto & t : internal::threads)
            t.join();
    }

    rest::rest_controller & get_rest_controller() noexcept { return *_rest; }
    ratelimit_mgr_t & get_ratelimit() noexcept { return *_ratelimit; }
    shards::shard_mgr & get_shard_mgr() noexcept { return *_shard_mgr; }
    bot_status get_state() const noexcept { return _status; }
    void set_state(bot_status s) noexcept { _status = s; }

    /// Helper function for posting tasks to asio
    /**
     * @param f A callable to execute within asio - signature should be void(void)
     */
    template<typename T>
    void async(T f)
    {
        asio::post(*internal::_io_context, std::move(f));
    }

    template<typename T = aegis::rest::rest_reply, typename P>
    AEGIS_DECL std::future<T> post_task(P fn);

#if !defined(AEGIS_DISABLE_ALL_CACHE)

    member * self() const
    {
        if (_self == nullptr)
            throw exception("Self not found", make_error_code(error::member_not_found));
        return _self;
    }

    AEGIS_DECL int64_t get_member_count() const noexcept;

    /// Obtain a pointer to a member by snowflake
    /**
     * @param id Snowflake of member to search for
     * @returns Pointer to member or nullptr
     */
    AEGIS_DECL member * find_member(snowflake id) const noexcept;

    /// Obtain a pointer to a member by snowflake. If none exists, creates the object.
    /**
     * @param id Snowflake of member to search for
     * @returns Pointer to member
     */
    AEGIS_DECL member * member_create(snowflake id) noexcept;
#endif

    /// Get the snowflake of the bot
    /**
    * @returns A snowflake of the bot
    */
    const snowflake get_id() const noexcept
    {
        return member_id;
    }

    /// Obtain a pointer to a channel by snowflake
    /**
     * @param id Snowflake of channel to search for
     * @returns Pointer to channel or nullptr
     */
    AEGIS_DECL channel * find_channel(snowflake id) const noexcept;

    /// Obtain a pointer to a channel by snowflake. If none exists, creates the object.
    /**
     * @param id Snowflake of channel to search for
     * @returns Pointer to channel
     */
    AEGIS_DECL channel * channel_create(snowflake id) noexcept;

    /// Obtain a pointer to a guild by snowflake
    /**
     * @param id Snowflake of guild to search for
     * @returns Pointer to guild or nullptr
     */
    AEGIS_DECL guild * find_guild(snowflake id) const noexcept;

    /// Obtain a pointer to a guild by snowflake. If none exists, creates the object.
    /**
     * @param id Snowflake of guild to search for
     * @param _shard Shard this guild will exist on
     * @returns Pointer to guild
     */
    AEGIS_DECL guild * guild_create(snowflake id, shards::shard * _shard) noexcept;

    /// Called by CHANNEL_CREATE (DirectMessage)
    /**
     * @param obj json obj of DM channel
     * @param _shard Shard this channel will exist on
     * @returns Pointer to channel
     */
    AEGIS_DECL channel * dm_channel_create(const json & obj, shards::shard * _shard);

    AEGIS_DECL std::future<rest::rest_reply> create_dm_message(snowflake member_id, const std::string & content, int64_t nonce = 0) noexcept;

    /// Return bot uptime as {days hours minutes seconds}
    /**
     * @returns std::string of `##h ##m ##s` formatted time
     */
    AEGIS_DECL std::string uptime_str() const noexcept
    {
        return utility::uptime_str(starttime);
    }

    /// Return shard uptime as {days hours minutes seconds}
    /**
     * @returns Time in milliseconds since shard received ready
     */
    int64_t uptime() const noexcept
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - starttime).count();
    }

    /// Performs an HTTP request on the path with content as the request body using the method method
    /**
     * @see rest_reply
     * @see rest::request_params
     * @param params A struct of HTTP parameters to perform the request
     * @returns Response object
     */
    AEGIS_DECL rest::rest_reply call(rest::request_params params);

    std::unordered_map<snowflake, std::unique_ptr<channel>> channels;
    std::unordered_map<snowflake, std::unique_ptr<guild>> guilds;
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    std::unordered_map<snowflake, std::unique_ptr<member>> members;
#endif
    std::map<std::string, uint64_t> message_count;

    std::string self_presence;
    uint32_t force_shard_count = 0;
    uint32_t shard_max_count = 0;
    std::string mention;
    bool wsdbg = false;
    std::unique_ptr<asio::io_context::strand> ws_open_strand;
    std::shared_ptr<spdlog::logger> log;

#ifdef AEGIS_PROFILING
    using message_end_t = std::function<void(std::chrono::steady_clock::time_point, const std::string&)>;
    using call_end_t = std::function<void(std::chrono::steady_clock::time_point)>;
    using js_end_t = std::function<void(std::chrono::steady_clock::time_point, const std::string&)>;
    void set_on_message_end(message_end_t cb) { message_end = cb; }
    void set_on_call_end(call_end_t cb) { call_end = cb; }
    void set_on_js_end(js_end_t cb) { js_end = cb; }
    message_end_t message_end;
    call_end_t call_end;
    js_end_t js_end;
#endif

    using typing_start_t = std::function<void(gateway::events::typing_start obj)>;
    using message_create_t = std::function<void(gateway::events::message_create obj)>;
    using message_update_t = std::function<void(gateway::events::message_update obj)>;
    using message_delete_t = std::function<void(gateway::events::message_delete obj)>;
    using message_delete_bulk_t = std::function<void(gateway::events::message_delete_bulk obj)>;
    using guild_create_t = std::function<void(gateway::events::guild_create obj)>;
    using guild_update_t = std::function<void(gateway::events::guild_update obj)>;
    using guild_delete_t = std::function<void(gateway::events::guild_delete obj)>;
    using user_update_t = std::function<void(gateway::events::user_update obj)>;
    using ready_t = std::function<void(gateway::events::ready obj)>;
    using resumed_t = std::function<void(gateway::events::resumed obj)>;
    using channel_create_t = std::function<void(gateway::events::channel_create obj)>;
    using channel_update_t = std::function<void(gateway::events::channel_update obj)>;
    using channel_delete_t = std::function<void(gateway::events::channel_delete obj)>;
    using guild_ban_add_t = std::function<void(gateway::events::guild_ban_add obj)>;
    using guild_ban_remove_t = std::function<void(gateway::events::guild_ban_remove obj)>;
    using guild_emojis_update_t = std::function<void(gateway::events::guild_emojis_update obj)>;
    using guild_integrations_update_t = std::function<void(gateway::events::guild_integrations_update obj)>;
    using guild_member_add_t = std::function<void(gateway::events::guild_member_add obj)>;
    using guild_member_remove_t = std::function<void(gateway::events::guild_member_remove obj)>;
    using guild_member_update_t = std::function<void(gateway::events::guild_member_update obj)>;
    using guild_members_chunk_t = std::function<void(gateway::events::guild_members_chunk obj)>;
    using guild_role_create_t = std::function<void(gateway::events::guild_role_create obj)>;
    using guild_role_update_t = std::function<void(gateway::events::guild_role_update obj)>;
    using guild_role_delete_t = std::function<void(gateway::events::guild_role_delete obj)>;
    using presence_update_t = std::function<void(gateway::events::presence_update obj)>;
    using voice_state_update_t = std::function<void(gateway::events::voice_state_update obj)>;
    using voice_server_update_t = std::function<void(gateway::events::voice_server_update obj)>;

    void set_on_typing_start(typing_start_t cb) { i_typing_start = cb; }/**< TYPING_START callback */
    void set_on_message_create(message_create_t cb) { i_message_create = cb; }/**< MESSAGE_CREATE callback */
    void set_on_message_create_dm(message_create_t cb) { i_message_create_dm = cb; }/**< MESSAGE_CREATE callback for direct messages */
    void set_on_message_update(message_update_t cb) { i_message_update = cb; }/**< MESSAGE_UPDATE callback */
    void set_on_message_delete(message_delete_t cb) { i_message_delete = cb; }/**< MESSAGE_DELETE callback */
    void set_on_message_delete_bulk(message_delete_bulk_t cb) { i_message_delete_bulk = cb; }/**< MESSAGE_DELETE_BULK callback */
    void set_on_guild_create(guild_create_t cb) { i_guild_create = cb; }/**< GUILD_CREATE callback */
    void set_on_guild_update(guild_update_t cb) { i_guild_update = cb; }/**< GUILD_UPDATE callback */
    void set_on_guild_delete(guild_delete_t cb) { i_guild_delete = cb; }/**< GUILD_DELETE callback */
    void set_on_user_update(user_update_t cb) { i_user_update = cb; }/**< USER_UPDATE callback */
    void set_on_ready(ready_t cb) { i_ready = cb; }/**< READY callback */
    void set_on_resumed(resumed_t cb) { i_resumed = cb; }/**< RESUME callback */
    void set_on_channel_create(channel_create_t cb) { i_channel_create = cb; }/**< CHANNEL_CREATE callback */
    void set_on_channel_update(channel_update_t cb) { i_channel_update = cb; }/**< CHANNEL_UPDATE callback */
    void set_on_channel_delete(channel_delete_t cb) { i_channel_delete = cb; }/**< CHANNEL_DELETE callback */
    void set_on_guild_ban_add(guild_ban_add_t cb) { i_guild_ban_add = cb; }/**< GUILD_BAN_ADD callback */
    void set_on_guild_ban_remove(guild_ban_remove_t cb) { i_guild_ban_remove = cb; }/**< GUILD_BAN_REMOVE callback */
    void set_on_guild_emojis_update(guild_emojis_update_t cb) { i_guild_emojis_update = cb; }/**< GUILD_EMOJIS_UPDATE callback */
    void set_on_guild_integrations_update(guild_integrations_update_t cb) { i_guild_integrations_update = cb; }/**< GUILD_INTEGRATIONS_UPDATE callback */
    void set_on_guild_member_add(guild_member_add_t cb) { i_guild_member_add = cb; }/**< GUILD_MEMBER_ADD callback */
    void set_on_guild_member_remove(guild_member_remove_t cb) { i_guild_member_remove = cb; }/**< GUILD_MEMBER_REMOVE callback */
    void set_on_guild_member_update(guild_member_update_t cb) { i_guild_member_update = cb; }/**< GUILD_MEMBER_UPDATE callback */
    void set_on_guild_member_chunk(guild_members_chunk_t cb) { i_guild_members_chunk = cb; }/**< GUILD_MEMBERS_CHUNK callback */
    void set_on_guild_role_create(guild_role_create_t cb) { i_guild_role_create = cb; }/**< GUILD_ROLE_CREATE callback */
    void set_on_guild_role_update(guild_role_update_t cb) { i_guild_role_update = cb; }/**< GUILD_ROLE_UPDATE callback */
    void set_on_guild_role_delete(guild_role_delete_t cb) { i_guild_role_delete = cb; }/**< GUILD_ROLE_DELETE callback */
    void set_on_presence_update(presence_update_t cb) { i_presence_update = cb; }/**< PRESENCE_UPDATE callback */
    void set_on_voice_state_update(voice_state_update_t cb) { i_voice_state_update = cb; }/**< VOICE_STATE_UPDATE callback */
    void set_on_voice_server_update(voice_server_update_t cb) { i_voice_server_update = cb; }/**< VOICE_SERVER_UPDATE callback */

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

    shards::shard & get_shard_by_id(uint16_t shard_id)
    {
        return _shard_mgr->get_shard(shard_id);
    }

    shards::shard & get_shard_by_guild(snowflake guild_id)
    {
        auto g = find_guild(guild_id);
        if (g == nullptr)
            throw std::out_of_range("get_shard_by_guild error - guild does not exist");
        return _shard_mgr->get_shard(g->shard_id);
    }

    uint64_t get_shard_transfer()
    {
        uint64_t count = 0;
        for (auto & s : _shard_mgr->_shards)
            count += s->get_transfer();
        return count;
    }

private:

    typing_start_t i_typing_start;
    message_create_t i_message_create;
    message_create_t i_message_create_dm;
    message_update_t i_message_update;
    message_delete_t i_message_delete;
    message_delete_bulk_t i_message_delete_bulk;
    guild_create_t i_guild_create;
    guild_update_t i_guild_update;
    guild_delete_t i_guild_delete;
    user_update_t i_user_update;
    ready_t i_ready;
    resumed_t i_resumed;
    channel_create_t i_channel_create;
    channel_update_t i_channel_update;
    channel_delete_t i_channel_delete;
    guild_ban_add_t i_guild_ban_add;
    guild_ban_remove_t i_guild_ban_remove;
    guild_emojis_update_t i_guild_emojis_update;
    guild_integrations_update_t i_guild_integrations_update;
    guild_member_add_t i_guild_member_add;
    guild_member_remove_t i_guild_member_remove;
    guild_member_update_t i_guild_member_update;
    guild_members_chunk_t i_guild_members_chunk;
    guild_role_create_t i_guild_role_create;
    guild_role_update_t i_guild_role_update;
    guild_role_delete_t i_guild_role_delete;
    presence_update_t i_presence_update;
    voice_state_update_t i_voice_state_update;
    voice_server_update_t i_voice_server_update;

    AEGIS_DECL void setup_gateway(std::error_code & ec);
    AEGIS_DECL void keep_alive(const asio::error_code & error, const std::chrono::milliseconds ms, shards::shard * _shard);

    AEGIS_DECL void reset_shard(shards::shard * _shard);

    /// Assign the message, connect, and close callbacks to the websocket object
    AEGIS_DECL void setup_callbacks();

    friend class guild;
    friend class channel;
    //friend class shard;

    AEGIS_DECL void ws_presence_update(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_typing_start(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_message_create(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_message_update(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_create(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_update(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_delete(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_message_delete(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_message_delete_bulk(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_user_update(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_voice_state_update(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_resumed(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_ready(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_channel_create(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_channel_update(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_channel_delete(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_ban_add(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_ban_remove(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_emojis_update(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_integrations_update(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_member_add(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_member_remove(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_member_update(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_members_chunk(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_role_create(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_role_update(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_guild_role_delete(const json & result, shards::shard * _shard);
    AEGIS_DECL void ws_voice_server_update(const json & result, shards::shard * _shard);

    AEGIS_DECL void on_message(websocketpp::connection_hdl hdl, std::string msg, shards::shard * _shard);
    AEGIS_DECL void on_connect(websocketpp::connection_hdl hdl, shards::shard * _shard);
    AEGIS_DECL void on_close(websocketpp::connection_hdl hdl, shards::shard * _shard);
    AEGIS_DECL void process_ready(const json & d, shards::shard * _shard);

    AEGIS_DECL void load_config();

    AEGIS_DECL void remove_channel(snowflake channel_id) noexcept;

    AEGIS_DECL void remove_member(snowflake member_id) noexcept;

    std::chrono::steady_clock::time_point starttime;

    snowflake member_id;
    int16_t discriminator;
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    std::string username;
    bool mfa_enabled;
#endif

    // Bot's token
    std::string _token;

    bot_status _status;

    std::unique_ptr<rest::rest_controller> _rest;
    std::unique_ptr<ratelimit_mgr_t > _ratelimit;
    std::unique_ptr<shards::shard_mgr> _shard_mgr;

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    member * _self = nullptr;
#endif

    std::unordered_map<std::string, std::function<void(const json &, shards::shard *)>> ws_handlers;
    spdlog::level::level_enum _loglevel = spdlog::level::level_enum::info;
    mutable shared_mutex _shard_m;
    mutable shared_mutex _guild_m;
    mutable shared_mutex _channel_m;
    mutable shared_mutex _member_m;

    bool file_logging = false;
    bool external_io_context = true;
    std::size_t thread_count = 0;
    std::string log_formatting;
};

}
