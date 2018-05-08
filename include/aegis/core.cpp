//
// core.cpp
// ********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#include "aegis/core.hpp"
#include <string>
#include <asio/streambuf.hpp>
#include <asio/connect.hpp>
#include "aegis/shard.hpp"
#include "aegis/rest_controller.hpp"
#include "aegis/member.hpp"

#include "aegis/push.hpp"
#include "aegis/zstr/zstr.hpp"
#include "aegis/pop.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/ansicolor_sink.h>

#pragma region websocket events
#include "aegis/events/ready.hpp"
#include "aegis/events/resumed.hpp"
#include "aegis/events/typing_start.hpp"
#include "aegis/events/message_create.hpp"
#include "aegis/events/presence_update.hpp"
#include "aegis/events/channel_create.hpp"
#include "aegis/events/channel_delete.hpp"
#include "aegis/events/channel_pins_update.hpp"
#include "aegis/events/channel_update.hpp"
#include "aegis/events/guild_ban_add.hpp"
#include "aegis/events/guild_ban_remove.hpp"
#include "aegis/events/guild_create.hpp"
#include "aegis/events/guild_delete.hpp"
#include "aegis/events/guild_emojis_update.hpp"
#include "aegis/events/guild_integrations_update.hpp"
#include "aegis/events/guild_member_add.hpp"
#include "aegis/events/guild_member_remove.hpp"
#include "aegis/events/guild_member_update.hpp"
#include "aegis/events/guild_members_chunk.hpp"
#include "aegis/events/guild_role_create.hpp"
#include "aegis/events/guild_role_delete.hpp"
#include "aegis/events/guild_role_update.hpp"
#include "aegis/events/guild_update.hpp"
#include "aegis/events/message_delete.hpp"
#include "aegis/events/message_delete_bulk.hpp"
#include "aegis/events/message_reaction_add.hpp"
#include "aegis/events/message_reaction_remove.hpp"
#include "aegis/events/message_reaction_remove_all.hpp"
#include "aegis/events/message_update.hpp"
#include "aegis/events/user_update.hpp"
#include "aegis/events/voice_server_update.hpp"
#include "aegis/events/voice_state_update.hpp"
#include "aegis/events/webhooks_update.hpp"
#pragma endregion websocket events

namespace aegis
{

AEGIS_DECL core::core(spdlog::level::level_enum loglevel)
    : shard_max_count(0)
    , force_shard_count(0)
    , member_id(0)
    , discriminator(0)
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    , mfa_enabled(false)
#endif
    , _status{ Uninitialized }
#if defined(AEGIS_PROFILING)
    , _rest{ std::make_shared<rest::rest_controller>(token, this) }
#else
    , _rest{ std::make_shared<rest::rest_controller>(token) }
#endif
    , ratelimit_o{ std::bind(&rest::rest_controller::call, _rest, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4) }
    , _loglevel(loglevel)
{
#if !defined(AEGIS_CXX17)
    if (gateway::objects::BOT::bot != nullptr)
        throw std::runtime_error("Instance of bot already exists");
    gateway::objects::BOT::bot = this;
#else
    if (gateway::objects::message::bot != nullptr)
        throw std::runtime_error("Instance of bot already exists");
    gateway::objects::message::bot = this;
#endif
    //gateway::objects::message::bot = this;
    ratelimit_o.add(rest::bucket_type::GUILD);
    ratelimit_o.add(rest::bucket_type::CHANNEL);
    ratelimit_o.add(rest::bucket_type::EMOJI);
}

AEGIS_DECL core::~core()
{
    if (_io_context)
        _io_context->stop();
    for (auto & s : shards)
    {
        if (s->keepalivetimer != nullptr)
        {
            s->keepalivetimer->cancel();
            s->keepalivetimer.reset();
        }
        s->connection_state = Shutdown;
        if (s->_connection != nullptr)
        {
            s->_connection->close(1001, "");
            s->_connection.reset();
        }
    }
}

#if !defined(AEGIS_DISABLE_ALL_CACHE)
AEGIS_DECL int64_t core::get_member_count() const AEGIS_NOEXCEPT
{
    int64_t count = 0;
    for (auto & kv : guilds)
        count += kv.second->get_member_count();
    return count;
}

AEGIS_DECL member * core::find_member(snowflake id) const AEGIS_NOEXCEPT
{
    std::shared_lock<shared_mutex> l(_member_m);
    auto it = members.find(id);
    if (it == members.end())
        return nullptr;
    return it->second.get();
}

AEGIS_DECL member * core::member_create(snowflake id) AEGIS_NOEXCEPT
{
    std::unique_lock<shared_mutex> l(_member_m);
    auto it = members.find(id);
    if (it == members.end())
    {
        auto g = std::make_unique<member>(id);
        auto ptr = g.get();
        members.emplace(id, std::move(g));
        return ptr;
    }
    return it->second.get();
}
#endif

AEGIS_DECL channel * core::find_channel(snowflake id) const AEGIS_NOEXCEPT
{
    std::shared_lock<shared_mutex> l(_channel_m);
    auto it = channels.find(id);
    if (it == channels.end())
        return nullptr;
    return it->second.get();
}

AEGIS_DECL channel * core::channel_create(snowflake id) AEGIS_NOEXCEPT
{
    std::unique_lock<shared_mutex> l(_channel_m);
    auto it = channels.find(id);
    if (it == channels.end())
    {
        auto g = std::make_unique<channel>(id, 0, ratelimit(), *_io_context);
        auto ptr = g.get();
        channels.emplace(id, std::move(g));
        return ptr;
    }
    return it->second.get();
}

AEGIS_DECL guild * core::find_guild(snowflake id) const AEGIS_NOEXCEPT
{
    std::shared_lock<shared_mutex> l(_guild_m);
    auto it = guilds.find(id);
    if (it == guilds.end())
        return nullptr;
    return it->second.get();
}

AEGIS_DECL guild * core::guild_create(snowflake id, shard * _shard) AEGIS_NOEXCEPT
{
    std::unique_lock<shared_mutex> l(_guild_m);
    auto it = guilds.find(id);
    if (it == guilds.end())
    {
        auto g = std::make_unique<guild>(_shard->shardid, id, this, ratelimit(), *_io_context);
        auto ptr = g.get();
        guilds.emplace(id, std::move(g));
        return ptr;
    }
    return it->second.get();
}

AEGIS_DECL void core::remove_channel(snowflake channel_id) AEGIS_NOEXCEPT
{
    auto it = channels.find(channel_id);
    if (it == channels.end())
    {
        log->debug("Unable to remove channel [{}] (does not exist)", channel_id);
        return;
    }
    channels.erase(it);
}


#if !defined(AEGIS_DISABLE_ALL_CACHE)
AEGIS_DECL void core::remove_member(snowflake member_id) AEGIS_NOEXCEPT
{
    auto it = members.find(member_id);
    if (it == members.end())
    {
        log->debug("Unable to remove member [{}] (does not exist)", member_id);
        return;
    }
    members.erase(it);
}
#endif

AEGIS_DECL void core::_run(std::size_t count, std::function<void(void)> f)
{
    if (count == 0)
        count = std::thread::hardware_concurrency();

    // ensure any sort of single blocking call in message processing usercode doesn't block everything
    // this will not protect against faulty usercode entirely, but will at least provide some leeway
    // to allow a single blocking message to not halt operations
    if (count == 1)
        count = 2;

    std::vector<std::thread> threads;

    if (_status != Ready)
    {
        //needs setup
        std::error_code ec;
        initialize(ec);
        if (ec) throw exception(ec);
    }

    {
        std::error_code ec;
        setup_gateway(ec);
        if (ec) throw exception(ec);
    }

    work_ptr wrk = std::make_shared<asio_exec>(asio::make_work_guard(io_service()));
    for (std::size_t i = 0; i < count; ++i)
        threads.emplace_back(std::bind(static_cast<asio::io_context::count_type(asio::io_context::*)()>(&asio::io_context::run), &io_service()));

    starttime = std::chrono::steady_clock::now();
    
    if (f)
        f();

    log->info("Starting bot with {} shards", shard_max_count);
    {
        log->info("Websocket[s] connecting");
        for (uint32_t k = 0; k < shard_max_count; ++k)
        {
            std::error_code ec;
            auto _shard = std::make_unique<shard>(*_io_context, websocket_o);
            _shard->_connection = websocket_o.get_connection(gateway_url, ec);
            _shard->shardid = k;

            if (ec)
            {
                log->critical("Websocket connection failed: {}", ec.message());
                return;
            }

            setup_callbacks(_shard.get());

            log->trace("Shard#{}: added to connect list", _shard->get_id());
            _shards_to_connect.push_back(_shard.get());
            shards.push_back(std::move(_shard));
        }

        ws_timer = websocket_o.set_timer(100, std::bind(&core::ws_status, this, std::placeholders::_1));

        std::unique_lock<std::mutex> l(m);
        cv.wait(l);

        log->info("Closing bot");
    }

    wrk.reset();
    io_service().stop();
    for (auto & t : threads)
        t.join();
}

AEGIS_DECL void core::run(std::size_t count)
{
    _run(count);
}

AEGIS_DECL void core::run(std::size_t count, std::function<void(void)> f)
{
    _run(count, f);
}

AEGIS_DECL void core::run(std::function<void(void)> f)
{
    _run(0, f);
}

AEGIS_DECL void core::_init()
{
    // DEBUG CODE ONLY
    std::vector<spdlog::sink_ptr> sinks;
#ifdef _WIN32
    auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
    auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
    sinks.push_back(color_sink);

#ifdef REDIS
    redis = std::make_unique<redisclient::RedisSyncClient>(io_service());
    auto redis_sink = std::make_shared<redis_sink>(io_service(), *redis);
    sinks.push_back(redis_sink);
#endif
    // Add more sinks here, if needed.
    log = std::make_shared<spdlog::logger>("aegis", begin(sinks), end(sinks));
    spdlog::register_logger(log);

    log->set_pattern("%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v");
    log->set_level(_loglevel);

    load_config();

#ifdef REDIS
    std::string errmsg;
    if (!redis->connect(asio::ip::make_address(redis_address), redis_port, errmsg))
    {
        log->error("Can't connect to redis: {}", errmsg);
        return;
    }

    log->info("Redis connected");
#endif
}

AEGIS_DECL void core::load_config()
{
    auto configfile = std::fopen("config.json", "r+");

    if (!configfile)
    {
        std::perror("File opening failed");
        throw std::runtime_error("config.json does not exist");
    }

    std::fseek(configfile, 0, SEEK_END);
    std::size_t filesize = std::ftell(configfile);

    std::fseek(configfile, 0, SEEK_SET);
    std::vector<char> buffer(filesize+1);
    std::memset(buffer.data(), 0, filesize+1);
    size_t rd = std::fread(buffer.data(), sizeof(char), buffer.size()-1, configfile);

    std::fclose(configfile);

    json cfg = json::parse(buffer.data());

    if (!cfg["token"].is_null())
        token = cfg["token"].get<std::string>();
    else
        throw std::runtime_error("\"token\" not set in config.json");

    if (!cfg["force-shard-count"].is_null())
        force_shard_count = cfg["force-shard-count"].get<int16_t>();

    if (!cfg["log-level"].is_null())
    {
        if (cfg["log-level"].is_number_integer())
        {
            auto l = static_cast<spdlog::level::level_enum>(cfg["log-level"].get<int32_t>());
            std::string s;
            switch (cfg["log-level"].get<int32_t>())
            {
                case 0: s = "trace"; break;
                case 1: s = "debug"; break;
                case 2: s = "info"; break;
                case 3: s = "warn"; break;
                case 4: s = "err"; break;
                case 5: s = "critical"; break;
                case 6: s = "off"; break;
                default: s = "info"; l = spdlog::level::level_enum::info; break;
            }
            log->set_level(l);
            log->info("Logging level set to {}", s);
        }
        else if (cfg["log-level"].is_string())
        {
            std::string s = cfg["log-level"].get<std::string>();
            if (s == "trace")
                log->set_level(spdlog::level::level_enum::trace);
            else if (s == "debug")
                log->set_level(spdlog::level::level_enum::debug);
            else if (s == "info")
                log->set_level(spdlog::level::level_enum::info);
            else if (s == "warn")
                log->set_level(spdlog::level::level_enum::warn);
            else if (s == "err")
                log->set_level(spdlog::level::level_enum::err);
            else if (s == "critical")
                log->set_level(spdlog::level::level_enum::critical);
            else if (s == "off")
                log->set_level(spdlog::level::level_enum::off);

            log->info("Logging level set to {}", s);
        }
    }

#if defined(REDIS)
    redis_address = cfg["redis-address"].get<std::string>();
    if (!redis_address.empty())
    {
        redis_port = cfg["redis-port"];
    }
#endif
}

AEGIS_DECL void core::setup_callbacks(shard * _shard)
{
    _shard->_connection->set_message_handler(
        std::bind(&core::on_message, this, std::placeholders::_1, std::placeholders::_2, _shard));

    _shard->_connection->set_open_handler(
        std::bind(&core::on_connect, this, std::placeholders::_1, _shard));

    _shard->_connection->set_close_handler(
        std::bind(&core::on_close, this, std::placeholders::_1, _shard));
}

AEGIS_DECL void core::shutdown()
{
    set_state(Shutdown);
    websocket_o.stop();
    cv.notify_all();
    for (auto & _shard : shards)
    {
        _shard->connection_state = Shutdown;
        if (_shard->_connection && _shard->_connection->get_socket().lowest_layer().is_open())
            _shard->_connection->close(1001, "");
        _shard->do_reset();
    }
}

AEGIS_DECL void core::setup_gateway(std::error_code & ec)
{
    log->info("Creating websocket");
    if (_status != Ready)
    {
        log->error("core::websocketcreate() called in the wrong state");
        ec = make_error_code(error::invalid_state);
        return;
    }

    rest::rest_reply res = _rest->get("/gateway/bot");

    if (res.content.empty())
    {
        ec = make_error_code(error::get_gateway);
        return;
    }

    if (res.reply_code == 401)
    {
        ec = make_error_code(error::invalid_token);
        return;
    }

    using json = nlohmann::json;

    json ret = json::parse(res.content);
    if (ret.count("message"))
    {
        if (ret["message"] == "401: Unauthorized")
        {
            ec = make_error_code(error::invalid_token);
            return;
        }
    }

    ws_handlers.emplace("PRESENCE_UPDATE", std::bind(&core::ws_presence_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("TYPING_START", std::bind(&core::ws_typing_start, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_CREATE", std::bind(&core::ws_message_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_DELETE", std::bind(&core::ws_message_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_CREATE", std::bind(&core::ws_guild_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_UPDATE", std::bind(&core::ws_guild_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_DELETE", std::bind(&core::ws_guild_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_DELETE_BULK", std::bind(&core::ws_message_delete_bulk, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("USER_UPDATE", std::bind(&core::ws_user_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("RESUMED", std::bind(&core::ws_resumed, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("READY", std::bind(&core::ws_ready, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("CHANNEL_CREATE", std::bind(&core::ws_channel_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("CHANNEL_UPDATE", std::bind(&core::ws_channel_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("CHANNEL_DELETE", std::bind(&core::ws_channel_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_BAN_ADD", std::bind(&core::ws_guild_ban_add, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_BAN_REMOVE", std::bind(&core::ws_guild_ban_remove, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_EMOJIS_UPDATE", std::bind(&core::ws_guild_emojis_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_INTEGRATIONS_UPDATE", std::bind(&core::ws_guild_integrations_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_MEMBER_ADD", std::bind(&core::ws_guild_member_add, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_MEMBER_REMOVE", std::bind(&core::ws_guild_member_remove, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_MEMBER_UPDATE", std::bind(&core::ws_guild_member_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_MEMBERS_CHUNK", std::bind(&core::ws_guild_members_chunk, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_ROLE_CREATE", std::bind(&core::ws_guild_role_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_ROLE_UPDATE", std::bind(&core::ws_guild_role_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_ROLE_DELETE", std::bind(&core::ws_guild_role_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("VOICE_STATE_UPDATE", std::bind(&core::ws_voice_state_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("VOICE_SERVER_UPDATE", std::bind(&core::ws_voice_server_update, this, std::placeholders::_1, std::placeholders::_2));

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

    ws_gateway = ret["url"].get<std::string>();
    gateway_url = ws_gateway + "/?compress=zlib-stream&encoding=json&v=6";

    websocket_o.clear_access_channels(websocketpp::log::alevel::all);

    websocket_o.set_tls_init_handler([](websocketpp::connection_hdl)
    {
        return websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv1);
    });

    ec = std::error_code();
}


AEGIS_DECL void core::process_ready(const json & d, shard * _shard)
{
    _shard->session_id = d["session_id"].get<std::string>();

    const json & guilds = d["guilds"];

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_self == nullptr)
    {
        const json & userdata = d["user"];
        discriminator = static_cast<int16_t>(std::stoi(userdata["discriminator"].get<std::string>()));
        member_id = userdata["id"];
        username = userdata["username"].get<std::string>();
        mfa_enabled = userdata["mfa_enabled"];
        if (mention.empty())
        {
            std::stringstream ss;
            ss << "<@" << member_id << ">";
            mention = ss.str();
        }

        auto m = std::make_unique<member>(member_id);
        _self = m.get();
        members.emplace(member_id, std::move(m));
        _self->member_id = member_id;
        _self->is_bot = true;
        _self->name = username;
        _self->discriminator = discriminator;
        _self->status = member::Online;
    }
#else
    const json & userdata = d["user"];
    discriminator = static_cast<int16_t>(std::stoi(userdata["discriminator"].get<std::string>()));
    member_id = userdata["id"];
#endif

    for (auto & guildobj : guilds)
    {
        snowflake id = guildobj["id"];

        bool unavailable = false;
        if (guildobj.count("unavailable"))
            unavailable = guildobj["unavailable"];

        guild * _guild = guild_create(id, _shard);

        if (!unavailable)
        {
            std::unique_lock<shared_mutex> l(_guild->mtx());
            _guild->load(guildobj, _shard);
            log->debug("Shard#{} : CREATED Guild: {} [T:{}] [{}]"
                      , _shard->get_id()
                      , _guild->guild_id
                      , guilds.size()
                      , guildobj["name"].get<std::string>());
        }
    }
}

AEGIS_DECL rest::rest_reply core::create_guild(
    std::string name,
    std::optional<std::string> voice_region, std::optional<int> verification_level,
    std::optional<int> default_message_notifications, std::optional<int> explicit_content_filter,
    std::optional<std::string> icon, std::optional<std::vector<gateway::objects::role>> roles,
    std::optional<std::vector<std::tuple<std::string, int>>> channels
)
{
    std::error_code ec;
    auto res = create_guild(ec, name, voice_region, verification_level, default_message_notifications,
                            explicit_content_filter, icon, roles, channels);
    if (ec)
        throw ec;
    return res;
}

AEGIS_DECL std::string core::uptime() const AEGIS_NOEXCEPT
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

///\todo
AEGIS_DECL channel * core::dm_channel_create(const json & obj, shard * _shard)
{
    snowflake channel_id = obj["id"];
    auto _channel = channel_create(channel_id);

    try
    {
        std::unique_lock<shared_mutex> l(_channel->_m);
#if !defined(AEGIS_DISABLE_ALL_CACHE)
        log->debug("Shard#{} : Channel[{}] created for DirectMessage", _shard->get_id(), channel_id);
        if (obj.count("name") && !obj["name"].is_null()) _channel->name = obj["name"].get<std::string>();
        _channel->type = static_cast<gateway::objects::channel_type>(obj["type"].get<int>());// 0 = text, 2 = voice

        if (!obj["last_message_id"].is_null()) _channel->last_message_id = obj["last_message_id"];
#endif

        //owner_id DirectMessage creator group DirectMessage
        //application_id DirectMessage creator if bot group DirectMessage
        //recipients user objects
        return _channel;
    }
    catch (std::exception&e)
    {
        log->error("Shard#{} : Error processing DM channel[{}] {}", _shard->shardid, channel_id, e.what());
    }
    return nullptr;
}

AEGIS_DECL void core::keep_alive(const asio::error_code & ec, const int32_t ms, shard * _shard)
{
    if (ec == asio::error::operation_aborted)
        return;
    try
    {
        if (_shard->connection_state == bot_status::Shutdown || _shard->_connection == nullptr)
            return;

        auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(_shard->heartbeat_ack.time_since_epoch()).count() > 0
            && std::chrono::duration_cast<std::chrono::milliseconds>(now - _shard->lastheartbeat) >= std::chrono::milliseconds(int32_t(ms * 1.5)))
        {
            log->error("Shard#{}: Heartbeat ack timeout. Reconnecting. Last Ack:{}ms Last Sent:{}ms Timeout:{}ms",
                        _shard->get_id(),
                        std::chrono::duration_cast<std::chrono::milliseconds>(now - _shard->heartbeat_ack).count(),
                        std::chrono::duration_cast<std::chrono::milliseconds>(now - _shard->lastheartbeat).count(),
                        int32_t(ms * 1.5));
            websocket_o.close(_shard->_connection, 1001, "");
            reset_shard(_shard);
            queue_reconnect(_shard);
            return;
        }
        json obj;
        obj["d"] = _shard->sequence;
        obj["op"] = 1;
        _shard->send_now(obj.dump());
        _shard->lastheartbeat = std::chrono::steady_clock::now();
        _shard->keepalivetimer = websocket_o.set_timer(
            ms,
            std::bind(&core::keep_alive, this, std::placeholders::_1, ms, _shard)
        );
    }
    catch (websocketpp::exception & e)
    {
        log->error("Websocket exception : {0}", e.what());
    }
    catch (...)
    {
        log->critical("Uncaught websocket exception");
    }
}

AEGIS_DECL void core::on_message(websocketpp::connection_hdl hdl, message_ptr msg, shard * _shard)
{
    std::string payload;

    _shard->transfer_bytes += msg->get_header().size() + msg->get_payload().size();
    _shard->transfer_bytes_u += msg->get_header().size();

#ifdef AEGIS_PROFILING
    auto s_t = std::chrono::steady_clock::now();
#endif

    try
    {
        //zlib detection and decoding
        _shard->ws_buffer << msg->get_payload();

        const std::string & pld = msg->get_payload();
        if (std::strcmp((pld.data() + pld.size() - 4), "\x00\x00\xff\xff"))
        {
            log->trace("Shard#{}: zlib-stream incomplete", _shard->shardid);
            return;
        }

        std::stringstream ss;
        std::string s;
        while (getline(_shard->zlib_ctx, s))
            ss << s;
        ss << '\0';
        _shard->zlib_ctx.seekg(0, std::ios::beg);
        _shard->zlib_ctx.clear();
        payload = ss.str();
        _shard->ws_buffer.str("");
        _shard->transfer_bytes_u += payload.size();

       const json result = json::parse(payload);

       if (!result.is_null())
        {
        
           if ((log->level() == spdlog::level::level_enum::trace && wsdbg)
               && (result["t"].is_null()
                   || (result["t"] != "GUILD_CREATE"
                       && result["t"] != "PRESENCE_UPDATE"
                       && result["t"] != "GUILD_MEMBERS_CHUNK")))
               log->trace("Shard#{}: {}", _shard->shardid, payload);


#if defined(AEGIS_PROFILING)
           if (!result.is_null())
               if (!result["t"].is_null())
                   if (js_end)
                       js_end(s_t, result["t"]);

           s_t = std::chrono::steady_clock::now();
#endif
        
           int64_t t_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

           _shard->lastwsevent = std::chrono::steady_clock::now();

#if defined(AEGIS_DEBUG_HISTORY)
            //////////////////////////////////////////////////////////////////////////
           _shard->debug_messages.push_front({ t_time, payload });

           if (_shard->debug_messages.size() >= 5)
               _shard->debug_messages.pop_back();
            //////////////////////////////////////////////////////////////////////////
#endif
        
            if (!result["s"].is_null())
                _shard->sequence = result["s"];
        
            if (!result.is_null())
            {
                //does string message exist
                if (!result["t"].is_null())
                {
        
                    std::string cmd = result["t"];
        
                    const auto it = ws_handlers.find(cmd);
                    if (it != ws_handlers.end())
                    {
                        //message id found
                        ++message_count[cmd];
#ifdef AEGIS_PROFILING
                        asio::post(io_service(), [this, it, res = std::move(result), _shard, s_t, cmd]()
                        {
                            try
                            {
                                (it->second)(res, _shard);
                                if (message_end)
                                    message_end(s_t, cmd);
                            }
                            catch (std::exception& e)
                            {
                                log->error("Failed to process object: {0}", e.what());
                                log->error(res.dump());
                                debug_trace(_shard);
                            }
                            catch (...)
                            {
                                log->error("Failed to process object: Unknown error");
                                debug_trace(_shard);
                            }
                        });
#else
                        asio::post(io_service(), [this, it, res = std::move(result), _shard, cmd]()
                        {
                            try
                            {
                                (it->second)(res, _shard);
                            }
                            catch (std::exception& e)
                            {
                                log->error("Failed to process object: {0}", e.what());
                                log->error(res.dump());
                                debug_trace(_shard);
                            }
                            catch (...)
                            {
                                log->error("Failed to process object: Unknown error");
                                debug_trace(_shard);
                            }
                        });
#endif
                    }
                    else
                    {
                        //message id exists but not found
                    }
                }
        
                //no message. check opcodes
        
                if (result["op"] == 9)
                {
                    if (result["d"] == false)
                    {
                        _shard->sequence = 0;
                        log->error("Shard#{} : Unable to resume or invalid connection. Starting new", _shard->shardid);
                        _shard->session_id.clear();
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 4000) + 1000));
                        json obj = {
                            { "op", 2 },
                            {
                                "d",
                                {
                                    { "token", token },
                                    { "properties",
                                        {
                                            { "$os", utility::platform::get_platform() },
                                            { "$browser", "aegis.cpp" },
                                            { "$device", "aegis.cpp" }
                                        }
                                    },
                                    { "shard", json::array({ _shard->shardid, shard_max_count }) },
                                    { "compress", true },
                                    { "large_threshhold", 250 },
                                    { "presence",
                                        {
                                            { "game",
                                                {
                                                    { "name", self_presence },
                                                    { "type", 0 }
                                                }
                                            },
                                            { "status", "online" },
                                            { "since", 1 },
                                            { "afk", false }
                                        }
                                    }
                                }
                            }
                        };
                        //log->trace("Shard#{}: {}", _shard->shardid, obj.dump());
                        if (_shard->_connection != nullptr)
                        {
                            _shard->send(obj.dump());
                        }
                        else
                        {
                            //debug?
                            log->error("Shard#{} : Invalid session received with an invalid connection state state: {}", _shard->shardid, static_cast<int32_t>(_shard->connection_state));
                            websocket_o.close(_shard->_connection, 1001, "");
                            reset_shard(_shard);
                            _shard->session_id.clear();
                        }
                    }
                    else
                    {
        
                    }
                    debug_trace(_shard);
                }
                if (result["op"] == 1)
                {
                    //requested heartbeat
                    json obj;
                    obj["d"] = _shard->sequence;
                    obj["op"] = 1;

                    _shard->send(obj.dump());
                }
                if (result["op"] == 10)
                {
                    int32_t heartbeat = result["d"]["heartbeat_interval"];
                    _shard->keepalivetimer = websocket_o.set_timer(
                        heartbeat,
                        std::bind(&core::keep_alive, this, std::placeholders::_1, heartbeat, _shard)
                    );
                    _shard->heartbeattime = heartbeat;
                }
                if (result["op"] == 11)
                {
                    //heartbeat ACK
                    _shard->lastheartbeat = _shard->heartbeat_ack = std::chrono::steady_clock::now();
                }
        
            }
        }
    }
    catch (std::exception& e)
    {
        log->error("Failed to process object: {0}", e.what());
        log->error(payload);

        debug_trace(_shard);
    }
    catch (...)
    {
        log->error("Failed to process object: Unknown error");
        debug_trace(_shard);
    }

}

AEGIS_DECL void core::on_connect(websocketpp::connection_hdl hdl, shard * _shard)
{
    log->info("Shard#{}: connection established", _shard->shardid);
    _shard->set_connected();
    if (_shards_to_connect.empty())
    {
        log->error("Shard#{}: _shards_to_connect empty", _shard->shardid);
    }
    else
    {
        auto * _s = _shards_to_connect.front();
        assert(_s != nullptr);
        if (_s != _shard)
            log->error("Shard#{}: _shards_to_connect.front wrong shard id:{} status:{}",
                       _shard->shardid,
                       _s->shardid,
                       _s->is_connected()?"connected":"not connected");
        if (_s != _connecting_shard)
            log->error("Shard#{}: _connecting_shard wrong shard id:{} status:{}",
                       _shard->shardid,
                       _s->shardid,
                       _s->is_connected() ? "connected" : "not connected");
        _shards_to_connect.pop_front();
    }

    try
    {
        json obj;
        if (_shard->session_id.empty())
        {
            obj = {
                { "op", 2 },
                {
                    "d",
                    {
                        { "token", token },
                        { "properties",
                            {
                                { "$os", utility::platform::get_platform() },
                                { "$browser", "aegis.cpp" },
                                { "$device", "aegis.cpp" }
                            }
                        },
                        { "shard", json::array({ _shard->shardid, shard_max_count }) },
                        { "compress", true },
                        { "large_threshhold", 250 },
                        { "presence",
                            {
                                { "game",
                                    {
                                        { "name", self_presence },
                                        { "type", 0 }
                                    }
                                },
                                { "status", "online" },
                                { "since", 1 },
                                { "afk", false }
                            }
                        }
                    }
                }
            };
        }
        else
        {
            log->info("Attempting RESUME with id : {}", _shard->session_id);
            obj = {
                { "op", 6 },
                { "d",
                    {
                        { "token", token },
                        { "session_id", _shard->session_id },
                        { "seq", _shard->sequence }
                    }
                }
            };
        }
        _shard->send(obj.dump());
    }
    catch (std::exception & e)
    {
        log->error("Shard#{}: on_connect exception : {}", e.what());
    }
    catch (...)
    {
        log->error("Shard#{}: Uncaught on_connect exception");
    }
}

AEGIS_DECL void core::send_all_shards(std::string & msg) AEGIS_NOEXCEPT
{
    for (auto & s : shards)
        s->send(msg);
}

AEGIS_DECL void core::send_all_shards(const json & msg) AEGIS_NOEXCEPT
{
    for (auto & s : shards)
        s->send(msg.dump());
}

AEGIS_DECL void core::on_close(websocketpp::connection_hdl hdl, shard * _shard)
{
    if (_status == Shutdown)
        return;
    log->error("Shard#{}: disconnected. lastheartbeat({}) lastwsevent({}) lastheartbeatack({})",
               _shard->get_id(),
               std::chrono::duration_cast<std::chrono::milliseconds>(_shard->lastheartbeat.time_since_epoch()).count(),
               std::chrono::duration_cast<std::chrono::milliseconds>(_shard->lastwsevent.time_since_epoch()).count(),
               std::chrono::duration_cast<std::chrono::milliseconds>(_shard->heartbeat_ack.time_since_epoch()).count());
    reset_shard(_shard);
    //TODO debug only auto reconnect 50 times per session
    if (_shard->counters.reconnects < 50)
        queue_reconnect(_shard);
}

AEGIS_DECL void core::reset_shard(shard * _shard)
{
    _shard->do_reset();
    _shard->connection_state = bot_status::Reconnecting;
    asio::error_code wsec;
    _shard->_connection = websocket_o.get_connection(gateway_url, wsec);
    setup_callbacks(_shard);
}

AEGIS_DECL void core::ws_status(const asio::error_code & ec)
{
    if ((ec == asio::error::operation_aborted) || (_status == Shutdown))
        return;

    using namespace std::chrono_literals;

    try
    {
        auto now = std::chrono::steady_clock::now();

        // do basic shard timer actions such as timing out potential ghost sockets
        for (auto & _shard : shards)
        {
            if (_shard == nullptr)
                continue;

            if (_shard->is_connected())
            {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(_shard->lastwsevent.time_since_epoch()).count() > 0)
                {
                    // heartbeat system should typically pick up any dead sockets. this is sort of redundant at the moment
                    if (_shard->lastwsevent < (now - 90s))
                    {
                        log->error("Shard#{}: Websocket had no events in last 90s - reconnecting", _shard->shardid);
                        debug_trace(_shard.get());
                        if (_shard->_connection->get_state() < websocketpp::session::state::closing)
                        {
                            websocket_o.close(_shard->_connection, 1001, "");
                            _shard->connection_state = bot_status::Reconnecting;
                        }
                        else
                            reset_shard(_shard.get());
                    }
                }
            }
            _shard->last_status_time = now;
        }

        // check if not all shards connected
        if (!_shards_to_connect.empty())
        {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - _last_ready) >= 6s)
            {
                if (_connect_time != std::chrono::steady_clock::time_point())
                {
                    if (std::chrono::duration_cast<std::chrono::seconds>(now - _connect_time) >= 10s)
                    {
                        log->error("Shard#{}: timeout while connecting (10s)", _connecting_shard->get_id());
                        _shards_to_connect.pop_front();
                        reset_shard(_connecting_shard);
                        queue_reconnect(_connecting_shard);
                        _connecting_shard = nullptr;
                        _connect_time = std::chrono::steady_clock::time_point();
                    }
                }
                else
                {
                    log->info("Shards to connect : {}", _shards_to_connect.size());
                    auto * _shard = _shards_to_connect.front();

                    if (!_shard->is_connected())
                    {
                        log->info("Shard#{}: connecting", _shard->get_id());
                        _connecting_shard = _shard;
                        _connect_time = now;
                        _shard->connect();
                        _shard->connection_state = bot_status::Connecting;
                    }
                    else
                    {
                        log->debug("Shard#{}: already connected {} {} {} {}",
                                    _shard->get_id(),
                                    _shard->_connection->get_state(),
                                    _shard->connection_state,
                                    std::chrono::duration_cast<std::chrono::milliseconds>(now - _shard->lastwsevent).count(),
                                    std::chrono::duration_cast<std::chrono::milliseconds>(now - _shard->last_status_time).count());
                        _shards_to_connect.pop_front();
                    }
                }
            }
        }
    }
    catch (std::exception & e)
    {
        log->error("ws_status() error : {}", e.what());
    }
    catch (...)
    {
        log->error("ws_status() error : unknown");
    }
    ws_timer = websocket_o.set_timer(1000, std::bind(&core::ws_status, this, std::placeholders::_1));
}

AEGIS_DECL void core::queue_reconnect(shard * _shard)
{
    auto it = std::find(_shards_to_connect.cbegin(), _shards_to_connect.cend(), _shard);
    if (it != _shards_to_connect.cend())
    {
        log->error("Shard#{}: shard to be connected already on connect list", _shard->shardid);
        return;
    }
    log->trace("Shard#{}: added to connect list", _shard->get_id());
    _shards_to_connect.push_back(_shard);
}

AEGIS_DECL void core::debug_trace(shard * _shard, bool extended)
{
    fmt::MemoryWriter w;

    w << "~~ERROR~~ extended(" << extended << ")"
        << "\n==========<Start Error Trace>==========\n"
        << "Shard: " << _shard->shardid << '\n'
        << "Seq: " << _shard->sequence << '\n';
    int i = 0;

    for (auto & msg : _shard->debug_messages)
        w << std::get<0>(msg) << " - " << std::get<1>(msg) << '\n';

    /// in most cases the entire shard list shouldn't be dumped
    if (extended)
    for (auto & c : shards)
    {
        if (c == nullptr)
            w << fmt::format("Shard#{} INVALID OBJECT\n",
                             _shard->shardid);

        else
        {
            if (c->_connection == nullptr)
                w << fmt::format("Shard#{} not connected\n",
                                 c->shardid);
            else
                w << fmt::format("Shard#{} connected Seq:{} LastWS:{}ms ago Total Xfer:{}\n",
                                 c->shardid, c->sequence, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - c->lastwsevent).count());
        }
    }
    w << "==========<End Error Trace>==========";
    log->error(w.str());
}

AEGIS_DECL void core::ws_presence_update(const json & result, shard * _shard)
{
    _shard->counters.presence_changes++;


    member::member_status status;
    std::string sts = result["d"]["status"];
    if (sts == "idle")
        status = member::Idle;
    else if (sts == "dnd")
        status = member::DoNotDisturb;
    else if (sts == "online")
        status = member::Online;
    else
        status = member::Offline;

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    json user = result["d"]["user"];
    snowflake guild_id = result["d"]["guild_id"];
    snowflake member_id = user["id"];
    auto _member = member_create(member_id);
    auto  _guild = find_guild(guild_id);
    if (_guild == nullptr)
    {
        log->error("Shard#{}: member without guild M:{} G:{} null:{}", _shard->get_id(), member_id, guild_id, _member == nullptr);
        return;
    }
    std::unique_lock<shared_mutex> l(_member->mtx(), std::defer_lock);
    std::unique_lock<shared_mutex> l2(_guild->mtx(), std::defer_lock);
    std::lock(l, l2);
    _member->load(_guild, result["d"], _shard);
    _member->status = status;
#endif

    gateway::events::presence_update obj;
    obj.bot = this;
    obj._shard = _shard;
    obj._user = result["d"]["user"];

    if (i_presence_update)
        i_presence_update(obj);
}

AEGIS_DECL void core::ws_typing_start(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    gateway::events::typing_start obj(static_cast<int64_t>(result["d"]["timestamp"])
                                      , find_channel(result["d"]["channel_id"])
                                      , find_member(result["d"]["user_id"]));
#else
    gateway::events::typing_start obj(static_cast<int64_t>(result["d"]["timestamp"]));
#endif
    obj.bot = this;
    obj._shard = _shard;

    if (i_typing_start)
        i_typing_start(obj);
}


AEGIS_DECL void core::ws_message_create(const json & result, shard * _shard)
{
    _shard->counters.messages++;
    snowflake c_id = result["d"]["channel_id"];
    auto c = find_channel(c_id);
    //assert(c != nullptr);
    if (c == nullptr)
    {
        log->error("Shard#{} - channel == nullptr - {} {} {}", _shard->get_id(), c_id, result["d"]["author"]["id"].get<std::string>(), result["d"]["content"].get<std::string>());
    }
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    else if (c->get_guild_id() == 0)//DM
    {
        auto m = find_member(result["d"]["author"]["id"]);
        gateway::events::message_create obj(result["d"], c, m);
        obj.bot = this;
        obj._shard = _shard;

        if (i_message_create_dm)
            i_message_create_dm(obj);
    }
    else
    {
        auto m = find_member(result["d"]["author"]["id"]);
        auto g = &c->get_guild();
        gateway::events::message_create obj(result["d"], g, c, m);
        obj.bot = this;
        obj._shard = _shard;
        if (i_message_create)
            i_message_create(obj);
    }
#else
    auto g = &c->get_guild();
    gateway::events::message_create obj(result["d"], g, c);
    obj.bot = this;
    obj._shard = _shard;
    if (i_message_create)
        i_message_create(obj);
#endif

}

AEGIS_DECL void core::ws_message_update(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    gateway::events::message_update obj(result["d"]
        , find_channel(result["d"]["channel_id"])
        , (result["d"].count("author") && result["d"]["author"].count("id"))? find_member(result["d"]["author"]["id"]) : nullptr );
#else
    gateway::events::message_update obj(result["d"]);
#endif
    obj.bot = this;
    obj._shard = _shard;

    if (i_message_update)
        i_message_update(obj);
}

AEGIS_DECL void core::ws_guild_create(const json & result, shard * _shard)
{
    snowflake guild_id = result["d"]["id"];

    auto _guild = guild_create(guild_id, _shard);
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_guild->unavailable && _guild->get_owner())
    {
        //outage
    }
#endif

    std::unique_lock<shared_mutex> l(_guild->mtx());
    _guild->load(result["d"], _shard);

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    //TODO: abide by rate limits (120/60)
    json chunk;
    chunk["d"]["guild_id"] = std::to_string(guild_id);
    chunk["d"]["query"] = "";
    chunk["d"]["limit"] = 0;
    chunk["op"] = 8;
    _shard->send(chunk.dump());
#endif

    gateway::events::guild_create obj;
    obj.bot = this;
    obj._guild = result["d"];
    obj._shard = _shard;

    if (i_guild_create)
        i_guild_create(obj);
}

AEGIS_DECL void core::ws_guild_update(const json & result, shard * _shard)
{
    snowflake guild_id = result["d"]["id"];

    auto _guild = find_guild(guild_id);
    if (_guild == nullptr)
    {
        log->error("Guild Update: [{}] does not exist", guild_id);
        //this should never happen
        return;
    }

    std::unique_lock<shared_mutex> l(_guild->mtx());
    _guild->load(result["d"], _shard);

    gateway::events::guild_update obj;
    obj.bot = this;
    obj._shard = _shard;
    obj._guild = result["d"];

    if (i_guild_update)
        i_guild_update(obj);
}

AEGIS_DECL void core::ws_guild_delete(const json & result, shard * _shard)
{
    gateway::events::guild_delete obj;
    obj.bot = this;
    obj._shard = _shard;
    obj.guild_id = result["d"]["id"];
    if (result["d"].count("unavailable"))
        obj.unavailable = result["d"]["unavailable"];
    else
        obj.unavailable = false;

    if (i_guild_delete)
        i_guild_delete(obj);

    if (obj.unavailable == true)
    {
        //outage
    }
    else
    {
        snowflake guild_id = result["d"]["id"];

        auto _guild = find_guild(guild_id);
        if (_guild == nullptr)
        {
            log->critical("Guild Delete: [{}] does not exist", guild_id);
            //this should never happen
            return;
        }

#if !defined(AEGIS_DISABLE_ALL_CACHE)
        _guild->unavailable = obj.unavailable;
#endif

        std::unique_lock<shared_mutex> l(_guild_m);
        //kicked or left
        //websocket_o.set_timer(5000, [this, id, _shard](const asio::error_code & ec)
        //{
        guilds.erase(guild_id);
        //});
    }
}

AEGIS_DECL void core::ws_message_delete(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    gateway::events::message_delete obj(static_cast<snowflake>(std::stoll(result["d"]["id"].get<std::string>()))
                                        , find_channel(result["d"]["channel_id"]));
#else
    gateway::events::message_delete obj(static_cast<snowflake>(std::stoll(result["d"]["id"].get<std::string>())));
#endif
    obj.bot = this;

    if (i_message_delete)
        i_message_delete(obj);
}

AEGIS_DECL void core::ws_message_delete_bulk(const json & result, shard * _shard)
{
    gateway::events::message_delete_bulk obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_message_delete_bulk)
        i_message_delete_bulk(obj);
}

AEGIS_DECL void core::ws_user_update(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake member_id = result["d"]["user"]["id"];

    auto _member = member_create(member_id);

    const json & user = result["d"]["user"];
    if (user.count("username") && !user["username"].is_null())
        _member->name = user["username"].get<std::string>();
    if (user.count("avatar") && !user["avatar"].is_null())
        _member->avatar = user["avatar"].get<std::string>();
    if (user.count("discriminator") && !user["discriminator"].is_null())
        _member->discriminator = static_cast<uint16_t>(std::stoi(user["discriminator"].get<std::string>()));
    if (user.count("mfa_enabled") && !user["mfa_enabled"].is_null())
        _member->mfa_enabled = user["mfa_enabled"];
    if (user.count("bot") && !user["bot"].is_null())
        _member->is_bot = user["bot"];
    //if (!user["verified"].is_null()) _member.m_verified = user["verified"];
    //if (!user["email"].is_null()) _member.m_email = user["email"];
    gateway::events::user_update obj(_member);
#else
    gateway::events::user_update obj;
#endif

    obj.bot = this;
    obj._shard = _shard;
    obj._user = result["d"];

    if (i_user_update)
        i_user_update(obj);
}

AEGIS_DECL void core::ws_voice_state_update(const json & result, shard * _shard)
{
    gateway::events::voice_state_update obj;
    obj.bot = this;
    obj._shard = _shard;
    obj = result["d"];
    
    if (i_voice_state_update)
        i_voice_state_update(obj);
}

AEGIS_DECL void core::ws_resumed(const json & result, shard * _shard)
{
    _connecting_shard = nullptr;
    _shard->connection_state = bot_status::Online;
    _connect_time = std::chrono::steady_clock::time_point();
    _shard->_ready_time = _last_ready = std::chrono::steady_clock::now();
    log->info("Shard#{} RESUMED Processed", _shard->shardid);
    if (_shard->keepalivetimer)
        _shard->keepalivetimer->cancel();
    _shard->keepalivetimer = websocket_o.set_timer(
        _shard->heartbeattime
        , std::bind(&core::keep_alive, this, std::placeholders::_1, _shard->heartbeattime, _shard)
    );

    gateway::events::resumed obj;
    obj.bot = this;
    obj._shard = _shard;
    obj = result["d"];

    if (i_resumed)
        i_resumed(obj);
}

AEGIS_DECL void core::ws_ready(const json & result, shard * _shard)
{
    _connecting_shard = nullptr;
    _shard->connection_state = bot_status::Online;
    _connect_time = std::chrono::steady_clock::time_point();
    _shard->_ready_time = _last_ready = std::chrono::steady_clock::now();
    process_ready(result["d"], _shard);
    log->info("Shard#{} READY Processed", _shard->shardid);

    gateway::events::ready obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_ready)
        i_ready(obj);
}

AEGIS_DECL void core::ws_channel_create(const json & result, shard * _shard)
{
    snowflake channel_id = result["d"]["id"];

    if (result["d"].count("guild_id") && !result["d"]["guild_id"].is_null())
    {
        //guild channel
        snowflake guild_id = result["d"]["guild_id"];
        auto _guild = find_guild(guild_id);
        if (_guild == nullptr)//TODO: errors
            return;
        auto _channel = channel_create(channel_id);
        std::unique_lock<shared_mutex> l(_channel->mtx(), std::defer_lock);
        std::unique_lock<shared_mutex> l2(_channel_m, std::defer_lock);
        std::unique_lock<shared_mutex> l3(_guild->mtx(), std::defer_lock);
        std::lock(l, l2, l3);
        _channel->load_with_guild(*_guild, result["d"], _shard);
        _guild->channels.emplace(channel_id, _channel);
        _channel->guild_id = guild_id;
        _channel->_guild = _guild;
    }
    else
    {
        //dm
        //auto _channel = channel_create(channel_id);
        auto _channel = dm_channel_create(result["d"], _shard);
    }

    gateway::events::channel_create obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_channel_create)
        i_channel_create(obj);
}

AEGIS_DECL void core::ws_channel_update(const json & result, shard * _shard)
{
    snowflake channel_id = result["d"]["id"];

    if (!result["d"]["guild_id"].is_null())
    {
        //guild channel
        snowflake guild_id = result["d"]["guild_id"];
        auto _guild = find_guild(guild_id);
        if (_guild == nullptr)//TODO: errors
            return;
        auto _channel = channel_create(channel_id);
        std::unique_lock<shared_mutex> l(_channel->mtx(), std::defer_lock);
        std::unique_lock<shared_mutex> l2(_channel_m, std::defer_lock);
        std::lock(l, l2);
        _channel->load_with_guild(*_guild, result["d"], _shard);
        _guild->channels.emplace(channel_id, _channel);
    }
    else
    {
        //dm
        //auto _channel = channel_create(channel_id);
        auto _channel = dm_channel_create(result["d"], _shard);
    }

    gateway::events::channel_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_channel_update)
        i_channel_update(obj);
}

AEGIS_DECL void core::ws_channel_delete(const json & result, shard * _shard)
{
    if (!result["d"]["guild_id"].is_null())
    {
        //guild channel
        snowflake channel_id = result["d"]["id"];
        snowflake guild_id = result["d"]["guild_id"];
        auto _guild = find_guild(guild_id);
        if (_guild == nullptr)//TODO: errors
            return;
        auto _channel = find_channel(channel_id);
        if (_channel == nullptr)//TODO: errors
            return;
        std::unique_lock<shared_mutex> l(_channel->mtx(), std::defer_lock);
        std::unique_lock<shared_mutex> l2(_channel_m, std::defer_lock);
        std::lock(l, l2);
        _guild->remove_channel(channel_id);
        remove_channel(channel_id);
    }

    gateway::events::channel_delete obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_channel_delete)
        i_channel_delete(obj);
}

AEGIS_DECL void core::ws_guild_ban_add(const json & result, shard * _shard)
{
    gateway::events::guild_ban_add obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_ban_add)
        i_guild_ban_add(obj);
}

AEGIS_DECL void core::ws_guild_ban_remove(const json & result, shard * _shard)
{
    gateway::events::guild_ban_remove obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_ban_remove)
        i_guild_ban_remove(obj);
}

AEGIS_DECL void core::ws_guild_emojis_update(const json & result, shard * _shard)
{
    gateway::events::guild_emojis_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_emojis_update)
        i_guild_emojis_update(obj);
}

AEGIS_DECL void core::ws_guild_integrations_update(const json & result, shard * _shard)
{
    gateway::events::guild_integrations_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_integrations_update)
        i_guild_integrations_update(obj);
}

AEGIS_DECL void core::ws_guild_member_add(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake member_id = result["d"]["user"]["id"];
    snowflake guild_id = result["d"]["guild_id"];

    auto _member = member_create(member_id);
    auto _guild = find_guild(guild_id);

    std::unique_lock<shared_mutex> l(_member->mtx(), std::defer_lock);
    std::unique_lock<shared_mutex> l2(_guild->mtx(), std::defer_lock);
    std::lock(l, l2);
    _member->load(_guild, result["d"], _shard);
#endif

    gateway::events::guild_member_add obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_member_add)
        i_guild_member_add(obj);
}

AEGIS_DECL void core::ws_guild_member_remove(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake member_id = result["d"]["user"]["id"];
    snowflake guild_id = result["d"]["guild_id"];

    auto _member = find_member(member_id);
    auto _guild = find_guild(guild_id);

    if (_guild != nullptr)
    {
        std::unique_lock<shared_mutex> l(_guild->mtx());
        //if user was self, guild may already be deleted
        _guild->remove_member(member_id);
    }
#endif

    gateway::events::guild_member_remove obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_member_remove)
        i_guild_member_remove(obj);
}

AEGIS_DECL void core::ws_guild_member_update(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake member_id = result["d"]["user"]["id"];
    snowflake guild_id = result["d"]["guild_id"];

    auto _member = member_create(member_id);
    auto _guild = find_guild(guild_id);

    if (_member == nullptr)
    {
#ifdef WIN32
        log->critical("Shard#{} : Error in [{}] _member == nullptr", _shard->shardid, __FUNCSIG__);
#else
        log->critical("Shard#{} : Error in [{}] _member == nullptr", _shard->shardid, __PRETTY_FUNCTION__);
#endif
        return;
    }
    if (_guild == nullptr)
    {
#ifdef WIN32
        log->critical("Shard#{} : Error in [{}] _guild == nullptr", _shard->shardid, __FUNCSIG__);
#else
        log->critical("Shard#{} : Error in [{}] _guild == nullptr", _shard->shardid, __PRETTY_FUNCTION__);
#endif
        return;
    }

    std::unique_lock<shared_mutex> l(_member->mtx(), std::defer_lock);
    std::unique_lock<shared_mutex> l2(_guild->mtx(), std::defer_lock);
    std::lock(l, l2);
    _member->load(_guild, result["d"], _shard);
#endif

    gateway::events::guild_member_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_member_update)
        i_guild_member_update(obj);
}

AEGIS_DECL void core::ws_guild_members_chunk(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake guild_id = result["d"]["guild_id"];
    auto _guild = find_guild(guild_id);
    if (_guild == nullptr)
        return;
    auto & members = result["d"]["members"];
    if (!members.empty())
    {
        for (auto & _member : members)
        {
            snowflake member_id = _member["user"]["id"];

            auto _member_ptr = member_create(member_id);

            std::unique_lock<shared_mutex> l(_member_ptr->mtx(), std::defer_lock);
            std::unique_lock<shared_mutex> l2(_guild->mtx(), std::defer_lock);
            std::lock(l, l2);
            _member_ptr->load(_guild, _member, _shard);
        }
    }
#endif

    gateway::events::guild_members_chunk obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_member_chunk)
        i_guild_member_chunk(obj);
}

AEGIS_DECL void core::ws_guild_role_create(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake guild_id = result["d"]["guild_id"];

    auto _guild = find_guild(guild_id);
    _guild->load_role(result["d"]["role"]);
#endif

    gateway::events::guild_role_create obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_role_create)
        i_guild_role_create(obj);
}

AEGIS_DECL void core::ws_guild_role_update(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake guild_id = result["d"]["guild_id"];

    auto _guild = find_guild(guild_id);
    _guild->load_role(result["d"]["role"]);
#endif

    gateway::events::guild_role_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_role_update)
        i_guild_role_update(obj);
}

AEGIS_DECL void core::ws_guild_role_delete(const json & result, shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake guild_id = result["d"]["guild_id"];
    snowflake role_id = result["d"]["role_id"];

    auto _guild = find_guild(guild_id);

    if (_guild != nullptr)
    {
        //if role was own, we may have been kicked and guild may already be deleted
        _guild->remove_role(role_id);
    }
#endif

    gateway::events::guild_role_delete obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_role_delete)
        i_guild_role_delete(obj);
}

AEGIS_DECL void core::ws_voice_server_update(const json & result, shard * _shard)
{
    gateway::events::voice_server_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_voice_server_update)
        i_voice_server_update(obj);
}

AEGIS_DECL rest::rest_reply core::create_guild(
    std::error_code & ec, std::string name,
    std::optional<std::string> voice_region, std::optional<int> verification_level,
    std::optional<int> default_message_notifications, std::optional<int> explicit_content_filter,
    std::optional<std::string> icon, std::optional<std::vector<gateway::objects::role>> roles,
    std::optional<std::vector<std::tuple<std::string, int>>> channels
)
{
    json obj;
    obj["name"] = name;
    if (voice_region.has_value())
        obj["region"] = voice_region.value();
    if (verification_level.has_value())
        obj["verification_level"] = verification_level.value();
    if (default_message_notifications.has_value())
        obj["default_message_notifications"] = default_message_notifications.value();
    if (explicit_content_filter.has_value())
        obj["explicit_content_filter"] = explicit_content_filter.value();
    if (icon.has_value())
        obj["icon"] = icon.value();
    if (roles.has_value())
        obj["roles"] = roles.value();
    if (channels.has_value())
        for (auto & c : channels.value())
            obj["channels"].push_back(json({ { "name", std::get<0>(c) }, { "type", std::get<1>(c) } }));

    try
    {
        auto r = _rest->call("/guilds", obj.dump(), "POST");
        ec = r.code();
        return r;
    }
    catch (nlohmann::json::type_error& e)
    {
        log->critical("json::type_error guild::post_task() exception : {}", e.what());
    }
    catch (...)
    {
        log->critical("Uncaught post_task exception");
    }
    ec = make_error_code(error::general);
    return {};
}

AEGIS_DECL rest::rest_reply core::get(const std::string & path)
{
    return _rest->get(path);
}

AEGIS_DECL rest::rest_reply core::get(const std::string & path, const std::string & content, const std::string & host)
{
    return _rest->get(path, content, host);
}

AEGIS_DECL rest::rest_reply core::post(const std::string & path, const std::string & content, const std::string & host)
{
    return _rest->post(path, content, host);
}

AEGIS_DECL rest::rest_reply core::call(const std::string & path, const std::string & content, const std::string & method, const std::string & host)
{
    return _rest->call(path, content, method, host);
}

}
