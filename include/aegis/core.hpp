//
// core.hpp
// ********
//
// Copyright (c) 2020 Sharon Fox (sharon at xandium dot io)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#pragma once

#include "aegis/fwd.hpp"
#include "aegis/config.hpp"

#include <asio/io_context.hpp>
#include <asio/bind_executor.hpp>
#include <asio/executor_work_guard.hpp>

#ifdef WIN32
# include "aegis/push.hpp"
# include "websocketpp/config/asio_client.hpp"
# include "websocketpp/client.hpp"
# include "aegis/pop.hpp"
#endif

#include "aegis/utility.hpp"
#include "aegis/snowflake.hpp"
#include "aegis/futures.hpp"
//#include "aegis/ratelimit/ratelimit.hpp"
//#include "aegis/ratelimit/bucket.hpp"
#include "aegis/rest/rest_controller.hpp"
#include "aegis/shards/shard_mgr.hpp"
#include "aegis/gateway/objects/role.hpp"
#include "aegis/gateway/objects/member.hpp"
#include "aegis/gateway/objects/channel.hpp"
#include "aegis/gateway/objects/guild.hpp"
#include "aegis/gateway/objects/activity.hpp"

#include <spdlog/spdlog.h>

#include <thread>
#include <condition_variable>
#include <shared_mutex>

namespace aegis
{
using namespace nlohmann;
#if (AEGIS_HAS_STD_SHARED_MUTEX == 1)
using shared_mutex = std::shared_mutex;
#else
using shared_mutex = std::shared_timed_mutex;
#endif

struct create_message_t;

/// Type of a work guard executor for keeping Asio services alive
using asio_exec = asio::executor_work_guard<asio::io_context::executor_type>;

/// Type of a shared pointer to an io_context work object
using work_ptr = std::unique_ptr<asio_exec>;

//using rest_call = std::function<rest::rest_reply(rest::request_params)>;

/// Type of a pointer to the Websocket++ TLS connection
using connection_ptr = websocketpp::client<websocketpp::config::asio_tls_client>::connection_type::ptr;

/// Type of a pointer to the Websocket++ message payload
using message_ptr = websocketpp::config::asio_client::message_type::ptr;

using ratelimit_mgr_t = aegis::ratelimit::ratelimit_mgr;

struct thread_state
{
    std::thread thd;
    bool active = false;
    std::chrono::steady_clock::time_point start_time;
    std::function<void(void)> fn;
};

/// Gateway intents for masking out events on the websocket.
/// Use as a bitfield with create_bot_t::intents().
// https://github.com/discordapp/discord-api-docs/pull/1307
enum intent : uint32_t {
	IntentsDisabled = 0xffffffff,	/* Special case, disables intents if none have been defined */
	Guilds = (1 << 0),
	GuildMembers = (1 << 1),
	GuildBans = (1 << 2),
	GuildEmojis = (1 << 3),
	GuildIntegrations = (1 << 4),
	GuildWebhooks = (1 << 5),
	GuildInvites = (1 << 6),
	GuildVoiceStates = (1 << 7),
	GuildPresences = (1 << 8),
	GuildMessages = (1 << 9),
	GuildMessageReactions = (1 << 10),
	GuildMessageTyping = (1 << 11),
	DirectMessages = (1 << 12),
	DirectMessageReactions = (1 << 13),
	DirectMessageTyping = (1 << 14)
};

struct create_guild_t
{
    create_guild_t & name(const std::string & param) { _name = param; return *this; }
    create_guild_t & voice_region(const std::string & param) { _voice_region = param; return *this; }
    create_guild_t & verification_level(int param) { _verification_level = param; return *this; }
    create_guild_t & default_message_notifications(int param) { _default_message_notifications = param; return *this; }
    create_guild_t & explicit_content_filter(int param) { _explicit_content_filter = param; return *this; }
    create_guild_t & roles(const std::vector<gateway::objects::role> & param)
    { _roles = param; return *this; }
    create_guild_t & channels(const std::vector<std::tuple<std::string, int>> & param)
    { _channels = param; return *this; }
    std::string _name;
    lib::optional<std::string> _voice_region;
    lib::optional<int> _verification_level;
    lib::optional<int> _default_message_notifications;
    lib::optional<int> _explicit_content_filter;
    lib::optional<std::string> _icon;
    lib::optional<std::vector<gateway::objects::role>> _roles;
    lib::optional<std::vector<std::tuple<std::string, int>>> _channels;
};

/// Class for fluent definition of bot parameters
/// The create_bot_t class allows for fluent initialisation of aegis::core objects
struct create_bot_t
{
    create_bot_t & token(const std::string & param) noexcept { _token = param; return *this; }
    create_bot_t & thread_count(const uint32_t param) noexcept { _thread_count = param; return *this; }
    create_bot_t & force_shard_count(const uint32_t param) noexcept { _force_shard_count = param; return *this; }
    create_bot_t & file_logging(const bool param) noexcept { _file_logging  = param; return *this; }
    create_bot_t & log_level(const spdlog::level::level_enum param) noexcept { _log_level = param; return *this; }
    create_bot_t & log_format(const std::string & param) noexcept { _log_format = param; return *this; }
    create_bot_t & io_context(std::shared_ptr<asio::io_context> param) noexcept { _io = param; return *this; }
    /**
     * Defines logging parameters
     * @param param A shard pointer to a custom spdlog::logger object
     */
    create_bot_t & logger(std::shared_ptr<spdlog::logger> param) noexcept { _log = param; return *this; }
    /**
     * Sets up clustering for large bots.
     * Clustering splits the bot's shards across multiple process, where each process takes on an equal subset of the bot's shard count.
     * For example if max_clusters is set to 2 on an 8 shard bot, then cluster_id 0 will contain shards 0, 2, 4 and 6 while cluster_id 1
     * would contain shards 1, 3, 5 and 7. It is the responsibility of your bot to marshall information across clusters if needed, (for
     * example using redis, SQL, etc ) as aegis will only see the shards that are part of the cluster for which it is authoritative.
     * @param cluster_id The cluster ID of this bot, zero-based
     * @param max_clusters The number of clusters that the bot has
     */
    create_bot_t & clustering(uint32_t cluster_id, uint32_t max_clusters) noexcept { _cluster_id = cluster_id; _max_clusters = max_clusters; return *this; }
    /**
     * Defines which events your bot will receive, events that you don't set here will be filtered out from the websocket at discord's side.
     * @param param A bit mask defined by one or more aegis::intents.
     * @returns reference to self
     */
    create_bot_t & intents(uint32_t param) noexcept { _intents = param; return *this; }
private:
    friend aegis::core;
    std::string _token;
    uint32_t _intents{ intent::IntentsDisabled };
    uint32_t _thread_count{ std::thread::hardware_concurrency() };
    uint32_t _force_shard_count{ 0 };
    uint32_t _cluster_id { 0 };
    uint32_t _max_clusters { 0 };
    bool _file_logging{ false };
    spdlog::level::level_enum _log_level{ spdlog::level::level_enum::info };
    std::string _log_format{ "%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v" };
    std::shared_ptr<asio::io_context> _io;
    std::shared_ptr<spdlog::logger> _log;
};

/// Primary class for managing a bot interface
/**
 * Only one instance of this object can exist safely
 */
class core
{
public:
    /// Constructs the aegis object that tracks all of the shards, guilds, channels, and members
    /// This constructor creates its own spdlog::logger and asio::io_context
    /**
     * @param loglevel The level of logging to use
     * @param count Amount of threads to start
     */
    AEGIS_DECL explicit core(create_bot_t bot_config);

    /// Constructs the aegis object that tracks all of the shards, guilds, channels, and members
    /// This constructor creates its own spdlog::logger and asio::io_context
    /**\deprecated
     * @param loglevel The level of logging to use
     * @param count Amount of threads to start
     */
    AEGIS_DECL explicit core(spdlog::level::level_enum loglevel = spdlog::level::level_enum::trace, std::size_t count = 10);

    /// Constructs the aegis object that tracks all of the shards, guilds, channels, and members
    /// This constructor creates its own spdlog::logger and expects you to create the asio::io_context.
    /// It also expects you to manage the event loop or start threads on the io_context.
    /**\deprecated
     * @param loglevel The level of logging to use
     */
    AEGIS_DECL explicit core(std::shared_ptr<asio::io_context> _io, spdlog::level::level_enum loglevel = spdlog::level::level_enum::trace);

    /// Constructs the aegis object that tracks all of the shards, guilds, channels, and members
    /// This constructor creates its own asio::io_context and expects you to create the spdlog::logger
    /**\deprecated
     * @param _log Your pre-constructed spdlog::logger object
     * @param count Amount of threads to start
     */
    AEGIS_DECL explicit core(std::shared_ptr<spdlog::logger> _log, std::size_t count = 2);

    /// Constructs the aegis object that tracks all of the shards, guilds, channels, and members
    /// This constructor accepts a logger and io_context that you create. It expects you to
    /// manage the event loop or start threads on the io_context.
    /**\deprecated
     * @param _io Your pre-constructed asio::io_context object
     * @param _log Your pre-constructed spdlog::logger object
     */
    AEGIS_DECL explicit core(std::shared_ptr<asio::io_context> _io, std::shared_ptr<spdlog::logger> _log);

    /// Destroys the shards, stops the asio::work object, destroys the websocket object
    AEGIS_DECL ~core();

    core(const core &) = delete;
    core(core &&) = delete;
    core & operator=(const core &) = delete;

    /// Outputs the last 5 messages received from the gateway
    /**
     * @param _shard Pointer to the shard object to dump recent messages
     */
    AEGIS_DECL void debug_trace(shards::shard * _shard) noexcept;

    AEGIS_DECL void setup_logging();

    AEGIS_DECL void setup_context();

    AEGIS_DECL void setup_shard_mgr();

    /// Get the internal (or external) io_service object
    asio::io_context & get_io_context() noexcept
    {
        return *_io_context;
    }

    /// Invokes a shutdown on the entire lib. Sets internal state to `Shutdown` and propagates the
    /// Shutdown state along with closing all websockets within the shard vector
    AEGIS_DECL void shutdown() noexcept;

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
     * \todo
     * @param icon Set icon 
     * 
     * @param roles vector of roles to create
     * @param channels vector of channels to create
     * @returns aegis::future<gateway::objects::guild>
     */
    AEGIS_DECL aegis::future<gateway::objects::guild> create_guild(
        std::string name, lib::optional<std::string> voice_region = {}, lib::optional<int> verification_level = {},
        lib::optional<int> default_message_notifications = {}, lib::optional<int> explicit_content_filter = {},
        lib::optional<std::string> icon = {}, lib::optional<std::vector<gateway::objects::role>> roles = {},
        lib::optional<std::vector<std::tuple<std::string, int>>> channels = {}
    );

    /// Create new guild - Unique case. Does not belong to any ratelimit bucket so it is run
    /// directly on the same thread and does not attempt to manage ratelimits due to the already
    /// existing requirement that the bot must be in less than 10 guilds for this call to succeed
    /**
     * @see aegis::create_guild_t
     * @param obj Struct of the contents of the request
     * @returns aegis::future<gateway::objects::guild>
     */
    AEGIS_DECL aegis::future<gateway::objects::guild> create_guild(create_guild_t obj);

    /// Changes bot's username (not implemented yet. username can be changed in developer panel)
    /**
     * @param username String of the username to set
     * @returns aegis::future<gateway::objects::member>
     */
    AEGIS_DECL aegis::future<gateway::objects::member> modify_bot_username(const std::string & username);

    /// Changes bot's avatar (not implemented yet. avatar can be changed in developer panel)
    /**
     * @param avatar String of the avatar to set
     * @returns aegis::future<gateway::objects::member>
     */
    AEGIS_DECL aegis::future<gateway::objects::member> modify_bot_avatar(const std::string & avatar);

    /// Starts the shard manager, creates the shards, and connects to the gateway
    AEGIS_DECL void run();

    /**
     * Yields operation of the current thread until library shutdown is detected
     */
    void yield() noexcept
    {
        if (_status == bot_status::shutdown)
            return;
        std::mutex m;
        std::unique_lock<std::mutex> l(m);
        cv.wait(l);

        log->info("Closing bot");
    }

    /// Get the rest controller object
    /**
     * @returns Reference to the internal rest controller
     */
    rest::rest_controller & get_rest_controller() noexcept { return *_rest; }

    /// Get the ratelimit object
    /**
     * @returns Reference to the internal rate limiter
     */
    ratelimit_mgr_t & get_ratelimit() noexcept { return *_ratelimit; }

    /// Get the shard manager
    /**
     * @returns Reference to the internal shard manager
     */
    shards::shard_mgr & get_shard_mgr() noexcept { return *_shard_mgr; }

    /// Get current state of the bot
    /**
     * @see bot_status
     * @returns Current bot status
     */
    bot_status get_state() const noexcept { return _status; }

    /// Set the bot status
    /**
     * @see bot_status
     * @param s Status to set
     */
    void set_state(bot_status s) noexcept { _status = s; }

    /// Get the timezone offset
    /**
     * @returns std::chrono::hours of timezone bias
     */
    std::chrono::hours get_tz_bias() const noexcept { return _tz_bias; }

#if !defined(AEGIS_DISABLE_ALL_CACHE)

    /// Obtain pointer to self object
    /**
     * @returns Pointer to the user object of the bot
     */
    user * self() const
    {
        if (_self == nullptr)
            throw exception("Self not found", make_error_code(error::member_not_found));
        return _self;
    }


    /// Get count of members tracked (includes duplicates from multiple shared guilds)
    /**
     * @returns int64_t of member count
     */
    AEGIS_DECL int64_t get_member_count() const noexcept;

    /// Get count of unique users tracked
    /**
     * @returns int64_t of user count
     */
    AEGIS_DECL int64_t get_user_count() const noexcept;

    /// Get count of unique channels tracked
    /**
     * @returns int64_t of channel count
     */
    AEGIS_DECL int64_t get_channel_count() const noexcept;

    /// Get count of unique guilds tracked
    /**
     * @returns int64_t of guild count
     */
    AEGIS_DECL int64_t get_guild_count() const noexcept;

    /// Obtain a pointer to a user by snowflake
    /**
     * @param id Snowflake of user to search for
     * @returns Pointer to user or nullptr
     */
    AEGIS_DECL user * find_user(snowflake id) const noexcept;

    /// Obtain a pointer to a user by snowflake without locking - for interal user
    /**
     * @param id Snowflake of user to search for
     * @returns Pointer to user or nullptr
     */
    AEGIS_DECL user * find_user_nolock(snowflake id) const noexcept;

    /// Obtain a pointer to a user by snowflake. If none exists, creates the object.
    /**
     * @param id Snowflake of user to search for
     * @returns Pointer to user
     */
    AEGIS_DECL user * user_create(snowflake id) noexcept;
#endif

    /// Get the snowflake of the bot
    /**
    * @returns A snowflake of the bot
    */
    const snowflake get_id() const noexcept
    {
        return user_id;
    }

    /// Obtain a pointer to a channel by snowflake
    /**
     * @param id Snowflake of channel to search for
     * @returns Pointer to channel or nullptr
     */
    AEGIS_DECL channel * find_channel(snowflake id) const noexcept;

    /// Obtain a pointer to a channel by snowflake without locking - for internal use
    /**
     * @param id Snowflake of channel to search for
     * @returns Pointer to channel or nullptr
     */
    AEGIS_DECL channel * find_channel_nolock(snowflake id) const noexcept;

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

    /// Obtain a pointer to a guild by snowflake without locking - for internal use
    /**
     * @param id Snowflake of guild to search for
     * @returns Pointer to guild or nullptr
     */
    AEGIS_DECL guild * find_guild_nolock(snowflake id) const noexcept;

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

    /// Send a direct message to a user
    /**\deprecated
     * @param id Snowflake of member to message
     * @param content string of message to send
     * @param nonce Unique id to track when message verifies (can be omitted)
     * @returns aegis::future<gateway::objects::message>
     */
    AEGIS_DECL aegis::future<gateway::objects::message> create_dm_message(snowflake member_id, const std::string & content, int64_t nonce = 0);

    /// Send a direct message to a user
    /**
     * @see aegis::create_message_t
     * @param obj Struct of the contents of the request
     * @returns aegis::future<gateway::objects::message>
     */
    AEGIS_DECL aegis::future<gateway::objects::message> create_dm_message(const create_message_t & obj);

    /// Return bot uptime as {days hours minutes seconds}
    /**
     * @returns std::string of `hh mm ss` formatted time
     */
    AEGIS_DECL std::string uptime_str() const noexcept;

    /// Return shard uptime as {days hours minutes seconds}
    /**
     * @returns Time in milliseconds since shard received ready
     */
    AEGIS_DECL int64_t uptime() const noexcept;

    /// Performs an immediate blocking HTTP request on the path with content as the request body using the method method
    /**
     * @see rest::rest_reply
     * @see rest::request_params
     * @param params A struct of HTTP parameters to perform the request
     * @returns Response object
     */
    AEGIS_DECL rest::rest_reply call(rest::request_params && params);

    /// Performs an immediate blocking HTTP request on the path with content as the request body using the method method
    /**
     * @see rest::rest_reply
     * @see rest::request_params
     * @param params A struct of HTTP parameters to perform the request
     * @returns Response object
     */
    AEGIS_DECL rest::rest_reply call(rest::request_params & params);

    /// Update presence across all shards at once
    /**
     * @see aegis::gateway::objects::activity
     * @see aegis::gateway::objects::presence
     * @param text Text of presence message
     * @param type Enum of the activity type
     * @param status Enum of the status
     */
    AEGIS_DECL void update_presence(const std::string& text, gateway::objects::activity::activity_type type = gateway::objects::activity::Game, gateway::objects::presence::user_status status = gateway::objects::presence::Online);

    /// Passes through to Websocket++
    /**
     * @param duration Time until function should be run in milliseconds
     * @param callback Function to run when timer expires
     */
    AEGIS_DECL std::shared_ptr<asio::steady_timer> set_timer(long duration, std::function<void(const asio::error_code &)> callback)
    {
        return get_shard_mgr().get_websocket().set_timer(duration, std::move(callback));
    }

    std::unordered_map<snowflake, std::unique_ptr<channel>> channels;
    std::unordered_map<snowflake, std::unique_ptr<channel>> stale_channels; //<\todo do something with stales - clear them?
    std::unordered_map<snowflake, std::unique_ptr<guild>> guilds;
    std::unordered_map<snowflake, std::unique_ptr<guild>> stale_guilds; //<\todo do something with stales - clear them?
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    std::unordered_map<snowflake, std::unique_ptr<user>> users;
    std::unordered_map<snowflake, std::unique_ptr<user>> stale_users; //<\todo do something with stales - clear them?
#endif
    std::map<std::string, uint64_t> message_count;

    std::string self_presence;
    uint32_t force_shard_count = 0;
    uint32_t shard_max_count = 0; //<\deprecated moving into private
    std::string mention;
    bool wsdbg = false;
    std::unique_ptr<asio::io_context::strand> ws_open_strand;
    std::shared_ptr<spdlog::logger> log;

    void bulk_members_on_connect(bool param) { bulk_members_on_connect_ = param; }
    bool bulk_members_on_connect() { return bulk_members_on_connect_; }
    int64_t shard_count() { return shard_max_count; }

#if defined(AEGIS_PROFILING)
    using message_end_t = std::function<void(std::chrono::steady_clock::time_point, const std::string&)>;
    using rest_end_t = std::function<void(std::chrono::steady_clock::time_point, uint16_t)>;
    using js_end_t = std::function<void(std::chrono::steady_clock::time_point, const std::string&)>;
    void set_on_message_end(message_end_t cb) { message_end = cb; }
    void set_on_rest_end(rest_end_t cb) { _rest->rest_end = cb; }
    void set_on_js_end(js_end_t cb) { js_end = cb; }
    message_end_t message_end;
    js_end_t js_end;
#endif

#if defined(AEGIS_EVENTS)
    //@todo make bool to determine ignoring an event
    using websocket_event_t = std::function<void(std::string, aegis::shards::shard&)>;
    void set_on_websocket_event(websocket_event_t cb) { websocket_event = cb; }
    websocket_event_t websocket_event;
#endif

#pragma region event handlers
    using typing_start_t = std::function<void(gateway::events::typing_start obj)>;
    using message_create_t = std::function<void(gateway::events::message_create obj)>;
    using message_update_t = std::function<void(gateway::events::message_update obj)>;
    using message_delete_t = std::function<void(gateway::events::message_delete obj)>;
    using message_delete_bulk_t = std::function<void(gateway::events::message_delete_bulk obj)>;
    using guild_create_t = std::function<void(gateway::events::guild_create obj)>;
    using guild_update_t = std::function<void(gateway::events::guild_update obj)>;
    using guild_delete_t = std::function<void(gateway::events::guild_delete obj)>;
    using message_reaction_add_t = std::function<void(gateway::events::message_reaction_add obj)>;
    using message_reaction_remove_t = std::function<void(gateway::events::message_reaction_remove obj)>;
    using message_reaction_remove_all_t = std::function<void(gateway::events::message_reaction_remove_all obj)>;
    using user_update_t = std::function<void(gateway::events::user_update obj)>;
    using ready_t = std::function<void(gateway::events::ready obj)>;
    using resumed_t = std::function<void(gateway::events::resumed obj)>;
    using channel_create_t = std::function<void(gateway::events::channel_create obj)>;
    using channel_update_t = std::function<void(gateway::events::channel_update obj)>;
    using channel_delete_t = std::function<void(gateway::events::channel_delete obj)>;
    using channel_pins_update_t = std::function<void(gateway::events::channel_pins_update obj)>;
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
    using webhooks_update_t = std::function<void(gateway::events::webhooks_update obj)>;

    using raw_event_t = std::function<void(json obj, shards::shard * _shard)>;

    /// TYPING_START callback
    void set_on_typing_start(typing_start_t cb) { i_typing_start = cb; }
    void set_on_typing_start_raw(raw_event_t cb) { i_typing_start_raw = cb; }

    /// MESSAGE_CREATE callback
    void set_on_message_create(message_create_t cb) { i_message_create = cb; }
    void set_on_message_create_raw(raw_event_t cb) { i_message_create_raw = cb; }

    /// MESSAGE_CREATE callback for direct messages
    void set_on_message_create_dm(message_create_t cb) { i_message_create_dm = cb; }
    void set_on_message_create_dm_raw(raw_event_t cb) { i_message_create_dm_raw = cb; }

    /// MESSAGE_UPDATE callback
    void set_on_message_update(message_update_t cb) { i_message_update = cb; }
    void set_on_message_update_raw(raw_event_t cb) { i_message_update_raw = cb; }

    /// MESSAGE_DELETE callback
    void set_on_message_delete(message_delete_t cb) { i_message_delete = cb; }
    void set_on_message_delete_raw(raw_event_t cb) { i_message_delete_raw = cb; }

    /// MESSAGE_DELETE_BULK callback
    void set_on_message_delete_bulk(message_delete_bulk_t cb) { i_message_delete_bulk = cb; }
    void set_on_message_delete_bulk_raw(raw_event_t cb) { i_message_delete_bulk_raw = cb; }

    /// GUILD_CREATE callback
    void set_on_guild_create(guild_create_t cb) { i_guild_create = cb; }
    void set_on_guild_create_raw(raw_event_t cb) { i_guild_create_raw = cb; }

    /// GUILD_UPDATE callback
    void set_on_guild_update(guild_update_t cb) { i_guild_update = cb; }
    void set_on_guild_update_raw(raw_event_t cb) { i_guild_update_raw = cb; }

    /// GUILD_DELETE callback
    void set_on_guild_delete(guild_delete_t cb) { i_guild_delete = cb; }
    void set_on_guild_delete_raw(raw_event_t cb) { i_guild_delete_raw = cb; }

    /// MESSAGE_REACTION_ADD callback
    void set_on_message_reaction_add(message_reaction_add_t cb) { i_message_reaction_add = cb; }
    void set_on_message_reaction_add_raw(raw_event_t cb) { i_message_reaction_add_raw = cb; }

    /// MESSAGE_REACTION_REMOVE callback
    void set_on_message_reaction_remove(message_reaction_remove_t cb) { i_message_reaction_remove = cb; }
    void set_on_message_reaction_remove_raw(raw_event_t cb) { i_message_reaction_remove_raw = cb; }

    /// MESSAGE_REACTION_REMOVE_ALL callback
    void set_on_message_reaction_remove_all(message_reaction_remove_all_t cb) { i_message_reaction_remove_all = cb; }
    void set_on_message_reaction_remove_all_raw(raw_event_t cb) { i_message_reaction_remove_all_raw = cb; }

    /// USER_UPDATE callback
    void set_on_user_update(user_update_t cb) { i_user_update = cb; }
    void set_on_user_update_raw(raw_event_t cb) { i_user_update_raw = cb; }

    /// READY callback
    void set_on_ready(ready_t cb) { i_ready = cb; }
    void set_on_ready_raw(raw_event_t cb) { i_ready_raw = cb; }

    /// RESUME callback
    void set_on_resumed(resumed_t cb) { i_resumed = cb; }
    void set_on_resumed_raw(raw_event_t cb) { i_resumed_raw = cb; }

    /// CHANNEL_CREATE callback
    void set_on_channel_create(channel_create_t cb) { i_channel_create = cb; }
    void set_on_channel_create_raw(raw_event_t cb) { i_channel_create_raw = cb; }

    /// CHANNEL_UPDATE callback
    void set_on_channel_update(channel_update_t cb) { i_channel_update = cb; }
    void set_on_channel_update_raw(raw_event_t cb) { i_channel_update_raw = cb; }

    /// CHANNEL_DELETE callback
    void set_on_channel_delete(channel_delete_t cb) { i_channel_delete = cb; }
    void set_on_channel_delete_raw(raw_event_t cb) { i_channel_delete_raw = cb; }

    /// CHANNEL_PINS_UPDATE callback
    void set_on_channel_pins_update(channel_pins_update_t cb) { i_channel_pins_update = cb; }
    void set_on_channel_pins_update_raw(raw_event_t cb) { i_channel_pins_update_raw = cb; }

    /// GUILD_BAN_ADD callback
    void set_on_guild_ban_add(guild_ban_add_t cb) { i_guild_ban_add = cb; }
    void set_on_guild_ban_add_raw(raw_event_t cb) { i_guild_ban_add_raw = cb; }

    /// GUILD_BAN_REMOVE callback
    void set_on_guild_ban_remove(guild_ban_remove_t cb) { i_guild_ban_remove = cb; }
    void set_on_guild_ban_remove_raw(raw_event_t cb) { i_guild_ban_remove_raw = cb; }

    /// GUILD_EMOJIS_UPDATE callback
    void set_on_guild_emojis_update(guild_emojis_update_t cb) { i_guild_emojis_update = cb; }
    void set_on_guild_emojis_update_raw(raw_event_t cb) { i_guild_emojis_update_raw = cb; }

    /// GUILD_INTEGRATIONS_UPDATE callback
    void set_on_guild_integrations_update(guild_integrations_update_t cb) { i_guild_integrations_update = cb; }
    void set_on_guild_integrations_update_raw(raw_event_t cb) { i_guild_integrations_update_raw = cb; }

    /// GUILD_MEMBER_ADD callback
    void set_on_guild_member_add(guild_member_add_t cb) { i_guild_member_add = cb; }
    void set_on_guild_member_add_raw(raw_event_t cb) { i_guild_member_add_raw = cb; }

    /// GUILD_MEMBER_REMOVE callback
    void set_on_guild_member_remove(guild_member_remove_t cb) { i_guild_member_remove = cb; }
    void set_on_guild_member_remove_raw(raw_event_t cb) { i_guild_member_remove_raw = cb; }

    /// GUILD_MEMBER_UPDATE callback
    void set_on_guild_member_update(guild_member_update_t cb) { i_guild_member_update = cb; }
    void set_on_guild_member_update_raw(raw_event_t cb) { i_guild_member_update_raw = cb; }

    /// GUILD_MEMBERS_CHUNK callback
    void set_on_guild_member_chunk(guild_members_chunk_t cb) { i_guild_members_chunk = cb; }
    void set_on_guild_member_chunk_raw(raw_event_t cb) { i_guild_members_chunk_raw = cb; }

    /// GUILD_ROLE_CREATE callback
    void set_on_guild_role_create(guild_role_create_t cb) { i_guild_role_create = cb; }
    void set_on_guild_role_create_raw(raw_event_t cb) { i_guild_role_create_raw = cb; }

    /// GUILD_ROLE_UPDATE callback
    void set_on_guild_role_update(guild_role_update_t cb) { i_guild_role_update = cb; }
    void set_on_guild_role_update_raw(raw_event_t cb) { i_guild_role_update_raw = cb; }

    /// GUILD_ROLE_DELETE callback
    void set_on_guild_role_delete(guild_role_delete_t cb) { i_guild_role_delete = cb; }
    void set_on_guild_role_delete_raw(raw_event_t cb) { i_guild_role_delete_raw = cb; }

    /// PRESENCE_UPDATE callback
    void set_on_presence_update(presence_update_t cb) { i_presence_update = cb; }
    void set_on_presence_update_raw(raw_event_t cb) { i_presence_update_raw = cb; }

    /// VOICE_STATE_UPDATE callback
    void set_on_voice_state_update(voice_state_update_t cb) { i_voice_state_update = cb; }
    void set_on_voice_state_update_raw(raw_event_t cb) { i_voice_state_update_raw = cb; }

    /// VOICE_SERVER_UPDATE callback
    void set_on_voice_server_update(voice_server_update_t cb) { i_voice_server_update = cb; }
    void set_on_voice_server_update_raw(raw_event_t cb) { i_voice_server_update_raw = cb; }

    /// WEBHOOKS_UPDATE callback
    void set_on_webhooks_update(webhooks_update_t cb) { i_webhooks_update = cb; }
    void set_on_webhooks_update_raw(raw_event_t cb) { i_webhooks_update_raw = cb; }

    /// Shard disconnect callback
    void set_on_shard_disconnect(std::function<void(aegis::shards::shard*)> cb)
    {
        _shard_mgr->i_shard_disconnect = cb;
    }

    /// Shard connect callback
    void set_on_shard_connect(std::function<void(aegis::shards::shard*)> cb)
    {
        _shard_mgr->i_shard_connect = cb;
    }
#pragma endregion

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

    /// Get a reference to the shard by shard id
    /**
     * @param shard_id Shard id
     * @returns Reference to shard
     */
    AEGIS_DECL shards::shard & get_shard_by_id(uint16_t shard_id);

    /// Get a reference to the shard that the provided guild is on
    /**
     * @param guild_id Snowflake of guild
     * @returns Reference to shard
     */
    AEGIS_DECL shards::shard & get_shard_by_guild(snowflake guild_id);

    /// Get total transfer of bot in bytes
    /**
     * @returns uint64_t
     */
    AEGIS_DECL uint64_t get_shard_transfer() const noexcept;

    /// Get total transfer of bot in bytes after decompression
    /**
     * @returns uint64_t
     */
    AEGIS_DECL uint64_t get_shard_u_transfer() const noexcept;

    /// Obtain bot's token
    /**
     * @returns std::string
     */
    const std::string & get_token() const noexcept { return _token; }

    /// Interrupt and end threads
    /**
     * @param count Amount of threads to shutdown
     */
    AEGIS_DECL std::size_t add_run_thread() noexcept;

    /// Interrupt and end threads
    /**
     * @param count Amount of threads to shutdown
     */
    AEGIS_DECL void reduce_threads(std::size_t count) noexcept;

    AEGIS_DECL aegis::future<gateway::objects::message> create_message(snowflake channel_id, const std::string& msg, int64_t nonce = 0, bool perform_lookup = false) noexcept;

    AEGIS_DECL aegis::future<gateway::objects::message> create_message_embed(snowflake channel_id, const std::string& msg, const json& _obj, int64_t nonce = 0, bool perform_lookup = false) noexcept;

    /// Run async task
    /**
     * This function will queue your task (a lambda or std::function) within Asio for execution at a later time
     * This version will return an aegis::future of your type that the passed function returns that you may
     * chain a continuation onto and receive that value within in
     *
     * Example:
     * @code{.cpp}
     * aegis::future<std::string> message = async([]{
     *     return 5;
     * }).then([](int value){
     *     return std::string("result received");
     * });
     * @endcode
     *
     * @param f Function to run async
     * @returns aegis::future<V>
     */
    template<typename T, typename V = std::result_of_t<T()>, typename = std::enable_if_t<!std::is_void<V>::value>>
    aegis::future<V> async(T f) noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        aegis::promise<V> pr(_io_context.get(), &_global_m);
        auto fut = pr.get_future();

        asio::post(*_io_context, [pr = std::move(pr), f = std::move(f)]() mutable
        {
            try
            {
                pr.set_value(f());
            }
            catch (std::exception &)
            {
                pr.set_exception(std::current_exception());
            }
        });
        std::atomic_thread_fence(std::memory_order_release);
        return fut;
    }

    /// Run async task
    /**
     * This function will queue your task (a lambda or std::function) within Asio for execution at a later time
     * This version will return an aegis::future<void> that will still allow chaining continuations on
     *
     * Example:
     * @code{.cpp}
     * aegis::future<std::string> message = async([]{
     *     return;
     * }).then([](){
     *     return std::string("continuation executed");
     * });
     * @endcode
     *
     * @param f Function to run async
     * @returns aegis::future<V>
     */
    template<typename T, typename V = std::enable_if_t<std::is_void<std::result_of_t<T()>>::value>>
    aegis::future<V> async(T f) noexcept
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        aegis::promise<V> pr(_io_context.get(), &_global_m);
        auto fut = pr.get_future();

        asio::post(*_io_context, [pr = std::move(pr), f = std::move(f)]() mutable
        {
            try
            {
                f();
                pr.set_value();
            }
            catch (std::exception & e)
            {
                pr.set_exception(std::current_exception());
            }
        });
        std::atomic_thread_fence(std::memory_order_release);
        return fut;
    }

    /// Get the internal guild mutex
    shared_mutex & get_guild_mutex() { return _guild_m; }

    /// Get the internal channel mutex
    shared_mutex & get_channel_mutex() { return _channel_m; }

    /// Get the internal user mutex
    shared_mutex & get_user_mutex() { return _user_m; }

    /// Get the guild map
    /**
     * This will return the internal unordered_map of all the guilds currently tracked. Does not
     * include stale items. You MUST lock the appropriate mutex BEFORE accessing it to prevent
     * potential race conditions and possible crashes.
     * 
     * Example:
     * @code{.cpp}
     * std::shared_lock<aegis::shared_mutex> l(get_guild_mutex());
     * @endcode
     * 
     * @returns std::unordered_map<snowflake, std::unique_ptr<guild>>
     */
    std::unordered_map<snowflake, std::unique_ptr<guild>> & get_guild_map() { return guilds; };

    /// Get the channel map
    /**
     * This will return the internal unordered_map of all the channels currently tracked. Does not
     * include stale items. You MUST lock the appropriate mutex BEFORE accessing it to prevent
     * potential race conditions and possible crashes.
     *
     * Example:
     * @code{.cpp}
     * std::shared_lock<aegis::shared_mutex> l(get_channel_mutex());
     * @endcode
     *
     * @returns std::unordered_map<snowflake, std::unique_ptr<channel>>
     */
    std::unordered_map<snowflake, std::unique_ptr<channel>> & get_channel_map() { return channels; };

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    /// Get the user map
    /**
     * This will return the internal unordered_map of all the users currently tracked. Does not
     * include stale items. You MUST lock the appropriate mutex BEFORE accessing it to prevent
     * potential race conditions and possible crashes.
     *
     * Example:
     * @code{.cpp}
     * std::shared_lock<aegis::shared_mutex> l(get_user_mutex());
     * @endcode
     *
     * @returns std::unordered_map<snowflake, std::unique_ptr<user>>
     */
    std::unordered_map<snowflake, std::unique_ptr<user>> & get_user_map() { return users; };
#endif

private:

    AEGIS_DECL void _thread_track(thread_state * t_state);

#pragma region event handlers
    //preprocessed object events
    typing_start_t i_typing_start;
    message_create_t i_message_create;
    message_create_t i_message_create_dm;
    message_update_t i_message_update;
    message_delete_t i_message_delete;
    message_delete_bulk_t i_message_delete_bulk;
    guild_create_t i_guild_create;
    guild_update_t i_guild_update;
    guild_delete_t i_guild_delete;
    message_reaction_add_t i_message_reaction_add;
    message_reaction_remove_t i_message_reaction_remove;
    message_reaction_remove_all_t i_message_reaction_remove_all;
    user_update_t i_user_update;
    ready_t i_ready;
    resumed_t i_resumed;
    channel_create_t i_channel_create;
    channel_update_t i_channel_update;
    channel_delete_t i_channel_delete;
    channel_pins_update_t i_channel_pins_update;
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
    webhooks_update_t i_webhooks_update;

    //raw json events
    raw_event_t i_typing_start_raw;
    raw_event_t i_message_create_raw;
    raw_event_t i_message_create_dm_raw;
    raw_event_t i_message_update_raw;
    raw_event_t i_message_delete_raw;
    raw_event_t i_message_delete_bulk_raw;
    raw_event_t i_guild_create_raw;
    raw_event_t i_guild_update_raw;
    raw_event_t i_guild_delete_raw;
    raw_event_t i_message_reaction_add_raw;
    raw_event_t i_message_reaction_remove_raw;
    raw_event_t i_message_reaction_remove_all_raw;
    raw_event_t i_user_update_raw;
    raw_event_t i_ready_raw;
    raw_event_t i_resumed_raw;
    raw_event_t i_channel_create_raw;
    raw_event_t i_channel_update_raw;
    raw_event_t i_channel_delete_raw;
    raw_event_t i_channel_pins_update_raw;
    raw_event_t i_guild_ban_add_raw;
    raw_event_t i_guild_ban_remove_raw;
    raw_event_t i_guild_emojis_update_raw;
    raw_event_t i_guild_integrations_update_raw;
    raw_event_t i_guild_member_add_raw;
    raw_event_t i_guild_member_remove_raw;
    raw_event_t i_guild_member_update_raw;
    raw_event_t i_guild_members_chunk_raw;
    raw_event_t i_guild_role_create_raw;
    raw_event_t i_guild_role_update_raw;
    raw_event_t i_guild_role_delete_raw;
    raw_event_t i_presence_update_raw;
    raw_event_t i_voice_state_update_raw;
    raw_event_t i_voice_server_update_raw;
    raw_event_t i_webhooks_update_raw;


    AEGIS_DECL void ws_presence_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_typing_start(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_message_create(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_message_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_create(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_delete(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_message_reaction_add(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_message_reaction_remove(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_message_reaction_remove_all(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_message_delete(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_message_delete_bulk(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_user_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_resumed(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_ready(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_channel_create(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_channel_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_channel_delete(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_channel_pins_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_ban_add(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_ban_remove(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_emojis_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_integrations_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_member_add(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_member_remove(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_member_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_members_chunk(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_role_create(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_role_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_guild_role_delete(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_voice_server_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_voice_state_update(const json& result, shards::shard* _shard);
    AEGIS_DECL void ws_webhooks_update(const json& result, shards::shard* _shard);
#pragma endregion

    AEGIS_DECL void setup_gateway();
    AEGIS_DECL void keep_alive(const asio::error_code & error, const std::chrono::milliseconds ms, shards::shard * _shard);

    AEGIS_DECL void reset_shard(shards::shard * _shard);

    /// Assign the message, connect, and close callbacks to the websocket object
    AEGIS_DECL void setup_callbacks() noexcept;

    friend class guild;
    friend class channel;
    //friend class shard;


    AEGIS_DECL void on_message(websocketpp::connection_hdl hdl, std::string msg, shards::shard * _shard);
    AEGIS_DECL void on_connect(websocketpp::connection_hdl hdl, shards::shard * _shard);
    AEGIS_DECL void on_close(websocketpp::connection_hdl hdl, shards::shard * _shard);
    AEGIS_DECL void process_ready(const json & d, shards::shard * _shard);

    AEGIS_DECL void load_config();

    AEGIS_DECL void remove_guild(snowflake guild_id) noexcept;
    AEGIS_DECL void remove_channel(snowflake channel_id) noexcept;
    AEGIS_DECL void remove_guild_nolock(snowflake guild_id) noexcept;
    AEGIS_DECL void remove_channel_nolock(snowflake channel_id) noexcept;

    AEGIS_DECL void remove_member(snowflake member_id) noexcept;

    std::chrono::steady_clock::time_point starttime;

    bool bulk_members_on_connect_ = true; //<\todo will eventually default to false

    snowflake user_id;
    int16_t discriminator = 0;
    std::string username;
    bool mfa_enabled = false;

    // Bot's token
    std::string _token;

    // Gateway intents
    // If you want to turn these on, you should use the create_bot_t class intents() method
    // This defaults to a special-case value which causes the intents values to not be sent.
    uint32_t _intents = intent::IntentsDisabled;

    uint32_t _cluster_id = 0;
    uint32_t _max_clusters = 0;

    bot_status _status = bot_status::uninitialized;

    std::shared_ptr<rest::rest_controller> _rest;
    std::shared_ptr<ratelimit_mgr_t> _ratelimit;
    std::shared_ptr<shards::shard_mgr> _shard_mgr;

    user * _self = nullptr;

    std::unordered_map<std::string, std::function<void(const json &, shards::shard *)>> ws_handlers;
    spdlog::level::level_enum _loglevel = spdlog::level::level_enum::info;
    mutable shared_mutex _shard_m;
    mutable shared_mutex _guild_m;
    mutable shared_mutex _channel_m;
    mutable shared_mutex _user_m;

    bool file_logging = false;
    bool external_io_context = true;
    std::size_t thread_count = 0;
    std::string log_formatting;
    bool state_valid = true;


    std::shared_ptr<asio::io_context> _io_context = nullptr;
    work_ptr wrk = nullptr;
    std::condition_variable cv;
    std::chrono::hours _tz_bias = 0h;
public:
    std::vector<std::unique_ptr<thread_state>> threads;
    std::recursive_mutex _global_m;
};

}
