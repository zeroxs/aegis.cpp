//
// core.cpp
// ********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#include "aegis/config.hpp"
#include "aegis/core.hpp"
#include <string>
#include <asio/streambuf.hpp>
#include <asio/connect.hpp>
#include "aegis/shards/shard.hpp"
#include "aegis/guild.hpp"
#include "aegis/channel.hpp"
#include "aegis/user.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/ansicolor_sink.h>

#pragma region websocket events
#include "aegis/gateway/events/ready.hpp"
#include "aegis/gateway/events/resumed.hpp"
#include "aegis/gateway/events/typing_start.hpp"
#include "aegis/gateway/events/message_create.hpp"
#include "aegis/gateway/events/presence_update.hpp"
#include "aegis/gateway/events/channel_create.hpp"
#include "aegis/gateway/events/channel_delete.hpp"
#include "aegis/gateway/events/channel_pins_update.hpp"
#include "aegis/gateway/events/channel_update.hpp"
#include "aegis/gateway/events/guild_ban_add.hpp"
#include "aegis/gateway/events/guild_ban_remove.hpp"
#include "aegis/gateway/events/guild_create.hpp"
#include "aegis/gateway/events/guild_delete.hpp"
#include "aegis/gateway/events/guild_emojis_update.hpp"
#include "aegis/gateway/events/guild_integrations_update.hpp"
#include "aegis/gateway/events/guild_member_add.hpp"
#include "aegis/gateway/events/guild_member_remove.hpp"
#include "aegis/gateway/events/guild_member_update.hpp"
#include "aegis/gateway/events/guild_members_chunk.hpp"
#include "aegis/gateway/events/guild_role_create.hpp"
#include "aegis/gateway/events/guild_role_delete.hpp"
#include "aegis/gateway/events/guild_role_update.hpp"
#include "aegis/gateway/events/guild_update.hpp"
#include "aegis/gateway/events/message_delete.hpp"
#include "aegis/gateway/events/message_delete_bulk.hpp"
#include "aegis/gateway/events/message_reaction_add.hpp"
#include "aegis/gateway/events/message_reaction_remove.hpp"
#include "aegis/gateway/events/message_reaction_remove_all.hpp"
#include "aegis/gateway/events/message_update.hpp"
#include "aegis/gateway/events/user_update.hpp"
#include "aegis/gateway/events/voice_server_update.hpp"
#include "aegis/gateway/events/voice_state_update.hpp"
#include "aegis/gateway/events/webhooks_update.hpp"
#pragma endregion websocket events

namespace aegis
{

AEGIS_DECL void core::setup_logging()
{
    std::vector<spdlog::sink_ptr> sinks;
#ifdef _WIN32
    auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
    auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
    sinks.push_back(color_sink);

    if (file_logging)
    {
        // 5MB max filesize and 10 max log files
        auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("log/aegis.log", 1024 * 1024 * 5, 10);
        sinks.push_back(rotating);
    }

    // Add more sinks here, if needed.
    log = std::make_shared<spdlog::logger>("aegis", begin(sinks), end(sinks));
    spdlog::register_logger(log);

    log->set_pattern(log_formatting);
    log->set_level(_loglevel);
}

AEGIS_DECL void core::setup_context()
{
    // ensure any sort of single blocking call in message processing usercode doesn't block everything
    // this will not protect against faulty usercode entirely, but will at least provide some leeway
    // to allow a single blocking message to not halt operations
    // recommended amount would depend on how much your bot is interacted with multiplied with how
    // extensive you make use of the async and futures.
    if (thread_count <= 2)
        thread_count = 10;

    external_io_context = false;
    _io_context = std::make_shared<asio::io_context>();

    wrk = std::make_unique<asio_exec>(asio::make_work_guard(*_io_context));
    for (std::size_t i = 0; i < thread_count; ++i)
        add_run_thread();
}

AEGIS_DECL void core::setup_shard_mgr()
{
    _shard_mgr = std::make_shared<shards::shard_mgr>(_token, *_io_context, log);

    _rest = std::make_shared<rest::rest_controller>(_token, "/api/v6", "discordapp.com", &get_io_context());

    setup_gateway();

    _ratelimit = std::make_shared<ratelimit_mgr_t>(
        std::bind(&aegis::rest::rest_controller::execute,
                  _rest.get(),
                  std::placeholders::_1),
        get_io_context(), this);

    setup_callbacks();
}

AEGIS_DECL core::core(spdlog::level::level_enum loglevel, std::size_t count)
    : _loglevel(loglevel)
    , thread_count(count)
{
    if (thread_count == 0)
        thread_count = std::thread::hardware_concurrency();

    load_config();

    setup_logging();
    setup_context();
    setup_shard_mgr();
}

AEGIS_DECL core::core(std::shared_ptr<asio::io_context> _io, spdlog::level::level_enum loglevel)
    : _io_context(_io)
    , _loglevel(loglevel)
{
    load_config();

    setup_logging();
    //setup_context();
    setup_shard_mgr();
}

AEGIS_DECL core::core(std::shared_ptr<spdlog::logger> _log, std::size_t count)
    : thread_count(count)
{
    if (thread_count == 0)
        thread_count = std::thread::hardware_concurrency();

    load_config();

    log = _log;
    _loglevel = log->level();
    setup_context();
    setup_shard_mgr();
}

AEGIS_DECL core::core(std::shared_ptr<asio::io_context> _io, std::shared_ptr<spdlog::logger> _log)
{
    load_config();

    log = _log;
    _loglevel = log->level();
    _io_context = std::move(_io);
    setup_shard_mgr();
}

AEGIS_DECL core::~core()
{
    if (_shard_mgr)
        _shard_mgr->shutdown();
    wrk.reset();
    if (!external_io_context)
        if (_io_context)
            _io_context->stop();
    for (auto & t : threads)
        t->thd.join();
    _shard_mgr.reset();
}

#if !defined(AEGIS_DISABLE_ALL_CACHE)
AEGIS_DECL int64_t core::get_member_count() const noexcept
{
    int64_t count = 0;
    for (auto & kv : guilds)
        count += kv.second->get_member_count();
    return count;
}

AEGIS_DECL int64_t core::get_user_count() const noexcept
{
    return members.size();
}

AEGIS_DECL user * core::find_user(snowflake id) const noexcept
{
    std::shared_lock<shared_mutex> l(_member_m);
    auto it = members.find(id);
    if (it == members.end())
        return nullptr;
    return it->second.get();
}

AEGIS_DECL user * core::user_create(snowflake id) noexcept
{
    std::unique_lock<shared_mutex> l(_member_m);
    auto it = members.find(id);
    if (it == members.end())
    {
        auto g = std::make_unique<user>(id);
        auto ptr = g.get();
        members.emplace(id, std::move(g));
        return ptr;
    }
    return it->second.get();
}
#endif

AEGIS_DECL aegis::future<gateway::objects::message> core::create_dm_message(snowflake member_id, const std::string & content, int64_t nonce)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    channel * c = nullptr;
    auto m = find_user(member_id);
    if (!m)
        return aegis::make_exception_future<gateway::objects::message>(aegis::error::member_error);
    if (m->get_dm_id())
        c = channel_create(m->get_dm_id());
    if (c)
        return c->create_message(content, nonce);
    else
#endif
    {
        rest::request_params params{ "/users/@me/channels", rest::Post, fmt::format(R"({{ "recipient_id": "{}" }})", member_id), "", {}, {} };
        return get_ratelimit().post_task<gateway::objects::message>(params)
            .then([=](gateway::objects::message && reply)
            {
                auto c = channel_create(reply.get_channel_id());
                if (!c) //return aegis::make_exception_future<gateway::objects::message>(aegis::error::general)
                    throw aegis::exception(make_error_code(error::member_error));
#if !defined(AEGIS_DISABLE_ALL_CACHE)
                m->set_dm_id(reply.get_channel_id());
#endif
                return c->create_message(content, nonce).get();
            });


//         return async([=]() -> rest::rest_reply
//         {
//             try
//             {
//                 rest::request_params params{ "/users/@me/channels", rest::Post, fmt::format(R"({{ "recipient_id": "{}" }})", member_id), "", {}, {} };
//                 auto res = json::parse(get_ratelimit()
//                                        .post_task(params)
//                                        .get()
//                                        .content);
//                 snowflake channel_id = std::stoull(res["id"].get<std::string>());
//                 auto c = channel_create(channel_id);
//                 if (!c) return {};
// #if !defined(AEGIS_DISABLE_ALL_CACHE)
//                 m->set_dm_id(channel_id);
// #endif
//                 return c->create_message(content, nonce).get();
//             }
//             catch (std::exception & e)
//             {
//             	
//             }
//             return {};
//         });
    }
}

AEGIS_DECL std::string core::uptime_str() const noexcept
{
    return utility::uptime_str(starttime);
}

AEGIS_DECL int64_t core::uptime() const noexcept
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - starttime).count();
}

AEGIS_DECL aegis::rest::rest_reply core::call(rest::request_params & params)
{
    return _rest->execute(std::forward<rest::request_params>(params));
}

AEGIS_DECL aegis::rest::rest_reply core::call(rest::request_params && params)
{
    return _rest->execute(std::forward<rest::request_params>(params));
}

AEGIS_DECL channel * core::find_channel(snowflake id) const noexcept
{
    std::shared_lock<shared_mutex> l(_channel_m);
    auto it = channels.find(id);
    if (it == channels.end())
        return nullptr;
    return it->second.get();
}

AEGIS_DECL channel * core::channel_create(snowflake id) noexcept
{
    std::unique_lock<shared_mutex> l(_channel_m);
    auto it = channels.find(id);
    if (it == channels.end())
    {
        auto g = std::make_unique<channel>(id, 0, this, *_io_context);
        auto ptr = g.get();
        channels.emplace(id, std::move(g));
        return ptr;
    }
    return it->second.get();
}

AEGIS_DECL guild * core::find_guild(snowflake id) const noexcept
{
    std::shared_lock<shared_mutex> l(_guild_m);
    auto it = guilds.find(id);
    if (it == guilds.end())
        return nullptr;
    return it->second.get();
}

AEGIS_DECL guild * core::guild_create(snowflake id, shards::shard * _shard) noexcept
{
    std::unique_lock<shared_mutex> l(_guild_m);
    auto it = guilds.find(id);
    if (it == guilds.end())
    {
        auto g = std::make_unique<guild>(_shard->get_id(), id, this, *_io_context);
        auto ptr = g.get();
        guilds.emplace(id, std::move(g));
        return ptr;
    }
    return it->second.get();
}

AEGIS_DECL void core::remove_channel(snowflake channel_id) noexcept
{
    auto it = channels.find(channel_id);
    if (it == channels.end())
    {
        AEGIS_DEBUG(log, "Unable to remove channel [{}] (does not exist)", channel_id);
        return;
    }
    channels.erase(it);
}


#if !defined(AEGIS_DISABLE_ALL_CACHE)
AEGIS_DECL void core::remove_member(snowflake member_id) noexcept
{
    auto it = members.find(member_id);
    if (it == members.end())
    {
        AEGIS_DEBUG(log, "Unable to remove member [{}] (does not exist)", member_id);
        return;
    }
    members.erase(it);
}
#endif

AEGIS_DECL void core::run()
{
    if (!state_valid)
    {
        std::cout << "Library state invalid. Errors occurred on creation. Exiting core::run()\n";
        return;
    }

    set_state(bot_status::running);

    starttime = std::chrono::steady_clock::now();
    
    log->info("Starting shard manager with {} shards", _shard_mgr->shard_max_count);
    _shard_mgr->start();
}


AEGIS_DECL void core::load_config()
{
    try
    {
        std::ifstream config_file("config.json");

        
        if (!config_file.is_open())
        {
            std::perror("File opening failed");
            throw std::runtime_error("config.json does not exist");
        }

        json cfg;
        config_file >> cfg;

        if (!cfg["token"].is_null())
        {
            if (cfg["token"].is_string())
                _token = cfg["token"].get<std::string>();
            else
                throw std::runtime_error("\"token\" must be string");
        }
        else
            throw std::runtime_error("\"token\" not set");

        if (!cfg["force-shard-count"].is_null())
        {
            if (cfg["force-shard-count"].is_number_integer())
                force_shard_count = cfg["force-shard-count"].get<int16_t>();
            else
                std::cout << "Cannot read \"force-shard-count\" from config.json.\n";
        }

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
                _loglevel = l;
            }
            else if (cfg["log-level"].is_string())
            {
                std::string s = cfg["log-level"].get<std::string>();
                if (s == "trace")
                    _loglevel = spdlog::level::level_enum::trace;
                else if (s == "debug")
                    _loglevel = spdlog::level::level_enum::debug;
                else if (s == "info")
                    _loglevel = spdlog::level::level_enum::info;
                else if (s == "warn")
                    _loglevel = spdlog::level::level_enum::warn;
                else if (s == "err")
                    _loglevel = spdlog::level::level_enum::err;
                else if (s == "critical")
                    _loglevel = spdlog::level::level_enum::critical;
                else if (s == "off")
                    _loglevel = spdlog::level::level_enum::off;
                else
                    _loglevel = spdlog::level::level_enum::info;
            }
            else
            {
                std::cout << "Cannot read \"log-level\" from config.json. Default: info\n";
                _loglevel = spdlog::level::level_enum::info;
            }
        }
        else
            _loglevel = spdlog::level::level_enum::info;

        if (!cfg["file-logging"].is_null())
            file_logging = cfg["file-logging"].get<bool>();

        if (!cfg["log-format"].is_null())
            log_formatting = cfg["log-format"].get<std::string>();
        else
            log_formatting = "%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v";
    }
    catch (std::exception & e)
    {
        std::cout << "Error loading config.json : " << e.what() << '\n';
        state_valid = false;
    }
}

AEGIS_DECL void core::setup_callbacks()
{
    _shard_mgr->set_on_message(std::bind(&core::on_message, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _shard_mgr->set_on_connect(std::bind(&core::on_connect, this, std::placeholders::_1, std::placeholders::_2));
    _shard_mgr->set_on_close(std::bind(&core::on_close, this, std::placeholders::_1, std::placeholders::_2));
}

AEGIS_DECL void core::shutdown()
{
    set_state(bot_status::shutdown);
    _shard_mgr->shutdown();
    cv.notify_all();
}

AEGIS_DECL void core::setup_gateway()
{
    log->info("Creating websocket");

    rest::rest_reply res = _rest->execute({ "/gateway/bot", rest::Get });

    _rest->tz_bias = tz_bias = std::chrono::hours(int(std::round(double((std::chrono::duration_cast<std::chrono::minutes>(res.date - std::chrono::system_clock::now()) / 60).count()))));

    if (res.content.empty())
        throw make_error_code(error::get_gateway);

    if (res.reply_code == 401)
        throw make_error_code(error::invalid_token);

    using json = nlohmann::json;

    json ret = json::parse(res.content);
    if (ret.count("message"))
        if (ret["message"] == "401: Unauthorized")
            throw make_error_code(error::invalid_token);

    ws_handlers.emplace("PRESENCE_UPDATE", std::bind(&core::ws_presence_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("TYPING_START", std::bind(&core::ws_typing_start, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_CREATE", std::bind(&core::ws_message_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_UPDATE", std::bind(&core::ws_message_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_DELETE", std::bind(&core::ws_message_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_CREATE", std::bind(&core::ws_guild_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_UPDATE", std::bind(&core::ws_guild_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_DELETE", std::bind(&core::ws_guild_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_REACTION_ADD", std::bind(&core::ws_message_reaction_add, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_REACTION_REMOVE", std::bind(&core::ws_message_reaction_remove, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_REACTION_REMOVE_ALL", std::bind(&core::ws_message_reaction_remove_all, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_DELETE_BULK", std::bind(&core::ws_message_delete_bulk, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("USER_UPDATE", std::bind(&core::ws_user_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("RESUMED", std::bind(&core::ws_resumed, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("READY", std::bind(&core::ws_ready, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("CHANNEL_CREATE", std::bind(&core::ws_channel_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("CHANNEL_UPDATE", std::bind(&core::ws_channel_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("CHANNEL_DELETE", std::bind(&core::ws_channel_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("CHANNEL_PINS_UPDATE", std::bind(&core::ws_channel_pins_update, this, std::placeholders::_1, std::placeholders::_2));
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
    ws_handlers.emplace("WEBHOOKS_UPDATE", std::bind(&core::ws_webhooks_update, this, std::placeholders::_1, std::placeholders::_2));

    if (force_shard_count)
    {
        _shard_mgr->shard_max_count = shard_max_count = force_shard_count;
        log->info("Forcing Shard count by config: {}", _shard_mgr->shard_max_count);
    }
    else
    {
        shard_max_count = _shard_mgr->shard_max_count = ret["shards"];
        log->info("Shard count: {}", _shard_mgr->shard_max_count);
    }

    _shard_mgr->ws_gateway = ret["url"].get<std::string>();
    _shard_mgr->set_gateway_url(_shard_mgr->ws_gateway + "/?compress=zlib-stream&encoding=json&v=6");
}


AEGIS_DECL void core::process_ready(const json & d, shards::shard * _shard)
{
    _shard->session_id = d["session_id"].get<std::string>();

    const json & guilds = d["guilds"];

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    if (_self == nullptr)
    {
        const json & userdata = d["user"];
        discriminator = static_cast<int16_t>(std::stoi(userdata["discriminator"].get<std::string>()));
        user_id = userdata["id"];
        username = userdata["username"].get<std::string>();
        mfa_enabled = userdata["mfa_enabled"];
        if (mention.empty())
        {
            std::stringstream ss;
            ss << "<@" << user_id << ">";
            mention = ss.str();
        }

        auto m = std::make_unique<user>(user_id);
        _self = m.get();
        members.emplace(user_id, std::move(m));
        _self->_member_id = user_id;
        _self->_is_bot = true;
        _self->_name = username;
        _self->_discriminator = discriminator;
        _self->_status = aegis::gateway::objects::presence::Online;
    }
#else
    const json & userdata = d["user"];
    discriminator = static_cast<int16_t>(std::stoi(userdata["discriminator"].get<std::string>()));
    user_id = userdata["id"];
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
            AEGIS_DEBUG(log, "Shard#{} : CREATED Guild: {} [T:{}] [{}]"
                      , _shard->get_id()
                      , _guild->guild_id
                      , guilds.size()
                      , guildobj["name"].get<std::string>());
        }
    }
}

AEGIS_DECL aegis::future<gateway::objects::guild> core::create_guild(create_guild_t obj)
{
    return create_guild(obj._name, obj._voice_region, obj._verification_level, obj._default_message_notifications,
                        obj._explicit_content_filter, obj._icon, obj._roles, obj._channels);
}

AEGIS_DECL aegis::future<gateway::objects::member> core::modify_bot_username(const std::string & username)
{
    if (!username.empty())
    {
        return make_ready_future<gateway::objects::member>();
    }
    return make_ready_future<gateway::objects::member>();
}

AEGIS_DECL aegis::future<gateway::objects::member> core::modify_bot_avatar(const std::string & avatar)
{
    if (!avatar.empty())
    {
        return make_ready_future<gateway::objects::member>();
    }
    return make_ready_future<gateway::objects::member>();
}

///\todo
AEGIS_DECL channel * core::dm_channel_create(const json & obj, shards::shard * _shard)
{
    snowflake channel_id = obj["id"];
    auto _channel = channel_create(channel_id);

    try
    {
        std::unique_lock<shared_mutex> l(_channel->mtx());
#if !defined(AEGIS_DISABLE_ALL_CACHE)
        AEGIS_DEBUG(log, "Shard#{} : Channel[{}] created for DirectMessage", _shard->get_id(), channel_id);
        if (obj.count("name") && !obj["name"].is_null()) _channel->name = obj["name"].get<std::string>();
        _channel->type = static_cast<gateway::objects::channel::channel_type>(obj["type"].get<int>());// 0 = text, 2 = voice

        if (!obj["last_message_id"].is_null()) _channel->last_message_id = obj["last_message_id"];
#endif

        //owner_id DirectMessage creator group DirectMessage
        //application_id DirectMessage creator if bot group DirectMessage
        //recipients user objects
        return _channel;
    }
    catch (std::exception&e)
    {
        log->error("Shard#{} : Error processing DM channel[{}] {}", _shard->get_id(), channel_id, e.what());
    }
    return nullptr;
}

AEGIS_DECL void core::on_message(websocketpp::connection_hdl hdl, std::string msg, shards::shard * _shard)
{
#if defined(AEGIS_PROFILING)
    auto s_t = std::chrono::steady_clock::now();
#endif
    try
    {
        json result = json::parse(msg);

        if (!result.is_null())
        {
            if (!result["s"].is_null())
                _shard->set_sequence(result["s"]);

            if (!result["t"].is_null())
            {
                const std::string & cmd = result["t"];

#if defined(AEGIS_PROFILING)
                if (js_end)
                    js_end(s_t, result["t"]);
#endif

                if ((wsdbg && log->level() == spdlog::level::level_enum::trace)
                    && ((result["t"] != "GUILD_CREATE"
                           && result["t"] != "PRESENCE_UPDATE"
                           && result["t"] != "GUILD_MEMBERS_CHUNK")))
                    AEGIS_TRACE(log, "Shard#{}: {}", _shard->get_id(), msg);

                int64_t t_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

                _shard->lastwsevent = std::chrono::steady_clock::now();

#if defined(AEGIS_DEBUG_HISTORY)
                if (_shard->debug_messages.size() > 5)
                    _shard->debug_messages.pop_front();
#endif
                //log->info("Shard#{}: {}", _shard->get_id(), cmd);

                const auto it = ws_handlers.find(cmd);
                if (it != ws_handlers.end())
                {
                    //message id found
                    ++message_count[cmd];
                    asio::post(*_io_context, [=, res = std::move(result)]()
                    {
                        if (get_state() == aegis::bot_status::shutdown)
                            return;

                        try
                        {
#if defined(AEGIS_PROFILING)
                            auto s_t = std::chrono::steady_clock::now();
                            (it->second)(res, _shard);
                            if (message_end)
                                message_end(s_t, cmd);
#else
                            (it->second)(res, _shard);
#endif
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
                }
                else
                {
                    //message id exists but not found
                }
                return;
            }

            //no message. check opcodes

            if (result["op"] == 9)
            {
                if (result["d"] == false)
                {
                    _shard->set_sequence(0);
                    log->warn("Shard#{} : Unable to resume or invalid connection. Starting new", _shard->get_id());
                    _shard->session_id.clear();

                    _shard->delayedauth.expires_after(std::chrono::milliseconds((rand() % 2000) + 5000));
                    _shard->delayedauth.async_wait(asio::bind_executor(*_shard->get_connection()->get_strand(), [=](const asio::error_code & ec)
                    {
                        if (ec == asio::error::operation_aborted)
                            return;

                        if (_shard->get_connection() == nullptr)
                        {
                            //debug?
                            log->error("Shard#{} : Invalid session received with an invalid connection state: {}", _shard->get_id(), static_cast<int32_t>(_shard->connection_state));
                            _shard_mgr->reset_shard(_shard);
                            _shard->session_id.clear();
                            return;
                        }

                        json obj = {
                            { "op", 2 },
                            {
                                "d",
                                {
                                    { "token", _token },
                                    { "properties",
                                        {
                                            { "$os", utility::platform::get_platform() },
                                            { "$browser", "aegis.cpp" },
                                            { "$device", "aegis.cpp" }
                                        }
                                    },
                                    { "shard", json::array({ _shard->get_id(), _shard_mgr->shard_max_count }) },
                                    { "compress", false },
                                    { "large_threshold", 250 }
                                }
                            }
                        };
                        _shard_mgr->_last_identify = std::chrono::steady_clock::now();
                        if (!self_presence.empty())
                        {
                            obj["d"]["presence"] = json({
                                                            { "game", {
                                                                { "name", self_presence },
                                                                { "type", 0 } }
                                                            },
                                                            { "status", "online" },
                                                            { "since", 1 },
                                                            { "afk", false }
                                                        });
                        }
                        _shard->send_now(obj.dump());
                    }));


                }
                else
                {
                    //
                }
                return;
            }
            if (result["op"] == 1)
            {
                //requested heartbeat
                json obj;
                obj["d"] = _shard->get_sequence();
                obj["op"] = 1;

                _shard->send(obj.dump());
                return;
            }
            if (result["op"] == 10)
            {
                _shard->_heartbeat_status = heartbeat_status::normal;
                _shard->heartbeat_ack = std::chrono::steady_clock::now();
                int32_t heartbeat = result["d"]["heartbeat_interval"];
                _shard->set_heartbeat(std::bind(&core::keep_alive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _shard->start_heartbeat(heartbeat);
                return;
            }
            if (result["op"] == 11)
            {
                //heartbeat ACK
                _shard->_heartbeat_status = heartbeat_status::normal;
                _shard->heartbeat_ack = std::chrono::steady_clock::now();
                return;
            }

            log->error("unhandled op({})", result["op"].get<int>());
            debug_trace(_shard);
            return;
        }
    }
    catch (std::exception& e)
    {
        log->error("Failed to process object: {0}", e.what());
        log->error(msg);

        debug_trace(_shard);
    }
    catch (...)
    {
        log->error("Failed to process object: Unknown error");
        debug_trace(_shard);
    }
}

AEGIS_DECL void core::debug_trace(shards::shard * _shard)
{
    _shard_mgr->debug_trace(_shard);
}

AEGIS_DECL void core::keep_alive(const asio::error_code & ec, const std::chrono::milliseconds ms, shards::shard * _shard)
{
    if (ec == asio::error::operation_aborted)
        return;
    try
    {
        if ((_shard->connection_state != shard_status::preready
             && _shard->connection_state != shard_status::online)
            || !_shard->is_connected())
            return;

        json obj;
        obj["d"] = _shard->get_sequence();
        obj["op"] = 1;
        _shard->send_now(obj.dump());
        _shard->_heartbeat_status = heartbeat_status::waiting;
        _shard->lastheartbeat = std::chrono::steady_clock::now();
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

AEGIS_DECL void core::on_connect(websocketpp::connection_hdl hdl, shards::shard * _shard)
{
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
                        { "token", _token },
                        { "properties",
                            {
                                { "$os", utility::platform::get_platform() },
                                { "$browser", "aegis.cpp" },
                                { "$device", "aegis.cpp" }
                            }
                        },
                        { "shard", json::array({ _shard->get_id(), _shard_mgr->shard_max_count }) },
                        { "compress", false },
                        { "large_threshold", 250 },
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
            _shard_mgr->_last_identify = std::chrono::steady_clock::now();
        }
        else
        {
            log->debug("Attempting RESUME with id : {}", _shard->session_id);
            obj = {
                { "op", 6 },
                { "d",
                    {
                        { "token", _token },
                        { "session_id", _shard->session_id },
                        { "seq", _shard->get_sequence() }
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

AEGIS_DECL void core::send_all_shards(const std::string & msg)
{
    _shard_mgr->send_all_shards(msg);
}

AEGIS_DECL void core::send_all_shards(const json & msg)
{
    _shard_mgr->send_all_shards(msg);
}

AEGIS_DECL aegis::shards::shard & core::get_shard_by_id(uint16_t shard_id)
{
    return _shard_mgr->get_shard(shard_id);
}

AEGIS_DECL aegis::shards::shard & core::get_shard_by_guild(snowflake guild_id)
{
    auto g = find_guild(guild_id);
    if (g == nullptr)
        throw std::out_of_range("get_shard_by_guild error - guild does not exist");
    return _shard_mgr->get_shard(g->shard_id);
}

AEGIS_DECL uint64_t core::get_shard_transfer()
{
    uint64_t count = 0;
    for (auto & s : _shard_mgr->_shards)
        count += s->get_transfer();
    return count;
}

AEGIS_DECL void core::_thread_track(thread_state * t_state)
{
    try
    {
//        while (t_state->active)
        t_state->fn();
    }
    catch (std::exception & e)
    {
        log->critical("Scheduler thread exit due to exception: {}", e.what());
    }
    catch (int)
    {
    }
    t_state->active = false;
}

AEGIS_DECL std::size_t core::add_run_thread() noexcept
{
    std::unique_ptr<thread_state> t = std::make_unique<thread_state>();
    t->active = true;
    t->start_time = std::chrono::steady_clock::now();
    t->fn = std::bind(static_cast<asio::io_context::count_type(asio::io_context::*)()>(&asio::io_context::run),
                      _io_context.get());
    t->thd = std::thread(std::bind(&core::_thread_track, this, t.get()));
    threads.emplace_back(std::move(t));
    return threads.size();
}

AEGIS_DECL void core::reduce_threads(std::size_t count) noexcept
{
    for (int i = 0; i < count; ++i)
        asio::post(*_io_context, []
        {
            throw 1;
        });
}

AEGIS_DECL void core::on_close(websocketpp::connection_hdl hdl, shards::shard * _shard)
{
    if (_status == bot_status::shutdown)
        return;
}

AEGIS_DECL void core::reset_shard(shards::shard * _shard)
{
    _shard->do_reset();
}

AEGIS_DECL void core::ws_presence_update(const json & result, shards::shard * _shard)
{
    _shard->counters.presence_changes++;

#if !defined(AEGIS_DISABLE_ALL_CACHE)
    json user = result["d"]["user"];
    snowflake guild_id = result["d"]["guild_id"];
    snowflake member_id = user["id"];
    auto _member = user_create(member_id);
    auto  _guild = find_guild(guild_id);
    if (_guild == nullptr)
    {
        log->warn("Shard#{}: member without guild M:{} G:{} null:{}", _shard->get_id(), member_id, guild_id, _member == nullptr);
        return;
    }
    std::unique_lock<shared_mutex> l(_member->mtx(), std::defer_lock);
    std::unique_lock<shared_mutex> l2(_guild->mtx(), std::defer_lock);
    std::lock(l, l2);
    _member->load(_guild, result["d"], _shard);

    using user_status = aegis::gateway::objects::presence::user_status;

    const std::string & sts = result["d"]["status"];

    if (sts == "idle")
        _member->_status = user_status::Idle;
    else if (sts == "dnd")
        _member->_status = user_status::DoNotDisturb;
    else if (sts == "online")
        _member->_status = user_status::Online;
    else
        _member->_status = user_status::Offline;

    //TODO: this is where rich presence might be stored if it's relevant to do so
    //_member->rich_presence = result["d"]["game"]; //activity object
#endif

    gateway::events::presence_update obj{*_shard};

    const json & j = result["d"];

    obj.user = j["user"];
    obj.guild_id = j["guild_id"];

    if (j.count("game") && !j["game"].is_null())
        obj.game = j["game"];

    if (j.count("roles") && !j["roles"].is_null())
        for (const auto & _role : j["roles"])
            obj.roles.push_back(_role);


    if (i_presence_update)
        i_presence_update(obj);
}

AEGIS_DECL void core::ws_typing_start(const json & result, shards::shard * _shard)
{
    gateway::events::typing_start obj{ *_shard
                                      , *channel_create(result["d"]["channel_id"])
                                      , *user_create(result["d"]["user_id"]) };
    obj.timestamp = static_cast<int64_t>(result["d"]["timestamp"]);

    if (i_typing_start)
        i_typing_start(obj);
}


AEGIS_DECL void core::ws_message_create(const json & result, shards::shard * _shard)
{
    _shard->counters.messages++;
    snowflake c_id = result["d"]["channel_id"];
    auto c = find_channel(c_id);
    //assert(c != nullptr);
    if (c == nullptr)
    {
        log->warn("Shard#{} - channel == nullptr - {} {} {}", _shard->get_id(), c_id, result["d"]["author"]["id"].get<std::string>(), result["d"]["content"].get<std::string>());
    }
    else if (c->get_guild_id() == 0)//DM
    {
        auto m = find_user(result["d"]["author"]["id"]);
        gateway::events::message_create obj{ *_shard, std::ref(*m), std::ref(*c) };

        obj.msg = result["d"];
        obj.msg._bot = this;

        if (i_message_create_dm)
            i_message_create_dm(obj);
    }
    else
    {
        auto m = find_user(result["d"]["author"]["id"]);
        auto g = &c->get_guild();
        gateway::events::message_create obj{ *_shard, std::ref(*m), std::ref(*c)/*, std::make_optional(std::ref(*g))*/ };

        obj.msg = result["d"];
        obj.msg._bot = this;

        if (i_message_create)
            i_message_create(obj);
    }
}

AEGIS_DECL void core::ws_message_update(const json & result, shards::shard * _shard)
{
    auto _channel = channel_create(result["d"]["channel_id"]);
    lib::optional<std::reference_wrapper<aegis::user>> _user;
    if (result["d"].count("author"))
    {
        const json & author = result["d"]["author"];
        _user = std::ref(*user_create(author["id"]));
    }
    gateway::events::message_update obj{ *_shard, *_channel, std::ref(*_user) };
    obj.msg = result["d"];

    if (i_message_update)
        i_message_update(obj);
}

AEGIS_DECL void core::ws_guild_create(const json & result, shards::shard * _shard)
{
    snowflake guild_id = result["d"]["id"];

    auto _guild = guild_create(guild_id, _shard);
    if (_guild->unavailable && _guild->get_owner())
    {
        //outage
    }

    {
        std::unique_lock<shared_mutex> l(_guild->mtx());
        _guild->load(result["d"], _shard);
    }

    json chunk;
    chunk["d"]["guild_id"] = std::to_string(guild_id);
    chunk["d"]["query"] = "";
    chunk["d"]["limit"] = 0;
    chunk["op"] = 8;
    _shard->send(chunk.dump());

    gateway::events::guild_create obj{ *_shard };
    obj.guild = result["d"];

    if (i_guild_create)
        i_guild_create(obj);
}

AEGIS_DECL void core::ws_guild_update(const json & result, shards::shard * _shard)
{
    snowflake guild_id = result["d"]["id"];

    auto _guild = find_guild(guild_id);
    if (_guild == nullptr)
    {
        log->error("Guild Update: [{}] does not exist", guild_id);
        //this should never happen
        return;
    }

    {
        std::unique_lock<shared_mutex> l(_guild->mtx());
        _guild->load(result["d"], _shard);
    }

    gateway::events::guild_update obj{ *_shard };
    obj.guild = result["d"];

    if (i_guild_update)
        i_guild_update(obj);
}

AEGIS_DECL void core::ws_guild_delete(const json & result, shards::shard * _shard)
{
    gateway::events::guild_delete obj{ *_shard };
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

AEGIS_DECL void core::ws_message_delete(const json & result, shards::shard * _shard)
{
    gateway::events::message_delete obj{ *_shard, *channel_create(result["d"]["channel_id"]) };
    obj.id = static_cast<snowflake>(std::stoll(result["d"]["id"].get<std::string>()));

    if (i_message_delete)
        i_message_delete(obj);
}

AEGIS_DECL void core::ws_message_delete_bulk(const json & result, shards::shard * _shard)
{
    gateway::events::message_delete_bulk obj{ *_shard };

    const json & j = result["d"];

    obj.channel_id = j["channel_id"];
    obj.guild_id = j["guild_id"];
    for (const auto & id : j["ids"])
        obj.ids.push_back(id);

    if (i_message_delete_bulk)
        i_message_delete_bulk(obj);
}

AEGIS_DECL void core::ws_user_update(const json & result, shards::shard * _shard)
{
    snowflake member_id = result["d"]["id"];

    auto _member = user_create(member_id);

    const json & user = result["d"];
    if (user.count("username") && !user["username"].is_null())
        _member->_name = user["username"].get<std::string>();
    if (user.count("avatar") && !user["avatar"].is_null())
        _member->_avatar = user["avatar"].get<std::string>();
    if (user.count("discriminator") && !user["discriminator"].is_null())
        _member->_discriminator = static_cast<uint16_t>(std::stoi(user["discriminator"].get<std::string>()));
    if (user.count("mfa_enabled") && !user["mfa_enabled"].is_null())
        _member->_mfa_enabled = user["mfa_enabled"];
    if (user.count("bot") && !user["bot"].is_null())
        _member->_is_bot = user["bot"];
    //if (!user["verified"].is_null()) _member.m_verified = user["verified"];
    //if (!user["email"].is_null()) _member.m_email = user["email"];
    gateway::events::user_update obj{ *_shard };

    const json & j = result["d"];

    obj._user = j;

    if (i_user_update)
        i_user_update(obj);
}

AEGIS_DECL void core::ws_voice_state_update(const json & result, shards::shard * _shard)
{
    gateway::events::voice_state_update obj{ *_shard };

    const json & j = result["d"];

    if (j.count("guild_id") && !j["guild_id"].is_null())
        obj.guild_id = j["guild_id"];
    obj.channel_id = j["channel_id"];
    obj.user_id = j["user_id"];
    obj.session_id = j["session_id"].get<std::string>();
    obj.deaf = j["deaf"];
    obj.mute = j["mute"];
    obj.self_deaf = j["self_deaf"];
    obj.self_mute = j["self_mute"];
    obj.suppress = j["suppress"];

    if (i_voice_state_update)
        i_voice_state_update(obj);
}

AEGIS_DECL void core::ws_resumed(const json & result, shards::shard * _shard)
{
    _shard_mgr->_connecting_shard = nullptr;
    _shard->connection_state = shard_status::online;
    _shard_mgr->_connect_time = std::chrono::steady_clock::time_point();
    //_shard->_ready_time = _shard_mgr->_last_ready = std::chrono::steady_clock::now();
    log->info("Shard#{} RESUMED Processed", _shard->get_id());
    _shard->keepalivetimer.cancel();
    _shard->set_heartbeat(std::bind(&core::keep_alive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _shard->start_heartbeat(_shard->heartbeattime);

    gateway::events::resumed obj{ *_shard };

    const json & j = result["d"];

    if (j.count("_trace") && !j["_trace"].is_null())
        for (const auto & i : j["_trace"])
            obj._trace.push_back(i);

    _shard->_trace = obj._trace;

    if (i_resumed)
        i_resumed(obj);
}

AEGIS_DECL void core::ws_ready(const json & result, shards::shard * _shard)
{
    _shard_mgr->_connecting_shard = nullptr;
    _shard->connection_state = shard_status::online;
    _shard_mgr->_connect_time = std::chrono::steady_clock::time_point();
    _shard->_ready_time = _shard_mgr->_last_ready = std::chrono::steady_clock::now();
    process_ready(result["d"], _shard);
    log->info("Shard#{} READY Processed", _shard->get_id());

    gateway::events::ready obj{ *_shard };

    const json & j = result["d"];

    obj.user = j["user"];
    if (j.count("private_channels") && !j["private_channels"].is_null())
        for (const auto & i : j["private_channels"])
            obj.private_channels.push_back(i);
    if (j.count("guilds") && !j["guilds"].is_null())
        for (const auto & i : j["guilds"])
            obj.guilds.push_back(i);
    if (j.count("_trace") && !j["_trace"].is_null())
        for (const auto & i : j["_trace"])
            obj._trace.push_back(i);

    _shard->_trace = obj._trace;

    if (i_ready)
        i_ready(obj);
}

AEGIS_DECL void core::ws_channel_create(const json & result, shards::shard * _shard)
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
        auto _channel = dm_channel_create(result["d"], _shard);

        const json & r = result["d"]["recipients"];
        if (result["d"]["type"] == gateway::objects::channel::channel_type::DirectMessage)//as opposed to a GroupDirectMessage
        {
            snowflake user_id = std::stoull(r.at(0)["id"].get<std::string>());
            snowflake dm_id = std::stoull(result["d"]["id"].get<std::string>());
#if !defined(AEGIS_DISABLE_ALL_CACHE)
            auto user = find_user(user_id);
            if (user)
                user->set_dm_id(dm_id);
#endif
        }
    }

    gateway::events::channel_create obj{ *_shard };

    const json & j = result["d"];

    obj.channel = j;

    if (i_channel_create)
        i_channel_create(obj);
}

AEGIS_DECL void core::ws_channel_update(const json & result, shards::shard * _shard)
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

    gateway::events::channel_update obj{ *_shard };

    const json & j = result["d"];

    obj.channel = j;

    if (i_channel_update)
        i_channel_update(obj);
}

AEGIS_DECL void core::ws_channel_delete(const json & result, shards::shard * _shard)
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

    gateway::events::channel_delete obj{ *_shard };

    const json & j = result["d"];

    obj.channel = j;

    if (i_channel_delete)
        i_channel_delete(obj);
}

AEGIS_DECL void core::ws_guild_ban_add(const json & result, shards::shard * _shard)
{
    gateway::events::guild_ban_add obj{ *_shard };

    const json & j = result["d"];

    obj.guild_id = j["guild_id"];
    obj.user = j["user"];

    if (i_guild_ban_add)
        i_guild_ban_add(obj);
}

AEGIS_DECL void core::ws_guild_ban_remove(const json & result, shards::shard * _shard)
{
    gateway::events::guild_ban_remove obj{ *_shard };

    const json & j = result["d"];

    obj.guild_id = j["guild_id"];
    obj.user = j["user"];

    if (i_guild_ban_remove)
        i_guild_ban_remove(obj);
}

AEGIS_DECL void core::ws_guild_emojis_update(const json & result, shards::shard * _shard)
{
    gateway::events::guild_emojis_update obj{ *_shard };

    const json & j = result["d"];

    obj.guild_id = j["guild_id"];
    if (j.count("emojis") && !j["emojis"].is_null())
        for (const auto & _emoji : j["emojis"])
            obj.emojis.push_back(_emoji);

    if (i_guild_emojis_update)
        i_guild_emojis_update(obj);
}

AEGIS_DECL void core::ws_guild_integrations_update(const json & result, shards::shard * _shard)
{
    gateway::events::guild_integrations_update obj{ *_shard };

    const json & j = result["d"];

    obj.guild_id = j["guild_id"];

    if (i_guild_integrations_update)
        i_guild_integrations_update(obj);
}

AEGIS_DECL void core::ws_guild_member_add(const json & result, shards::shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake member_id = result["d"]["user"]["id"];
    snowflake guild_id = result["d"]["guild_id"];

    auto _member = user_create(member_id);
    auto _guild = find_guild(guild_id);

    std::unique_lock<shared_mutex> l(_member->mtx(), std::defer_lock);
    std::unique_lock<shared_mutex> l2(_guild->mtx(), std::defer_lock);
    std::lock(l, l2);
    _member->load(_guild, result["d"], _shard);
#endif

    gateway::events::guild_member_add obj{ *_shard };

    const json & j = result["d"];

    obj.member = j;

    if (i_guild_member_add)
        i_guild_member_add(obj);
}

AEGIS_DECL void core::ws_guild_member_remove(const json & result, shards::shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake member_id = result["d"]["user"]["id"];
    snowflake guild_id = result["d"]["guild_id"];

    auto _member = find_user(member_id);
    auto _guild = find_guild(guild_id);

    if (_guild != nullptr)
    {
        std::unique_lock<shared_mutex> l(_guild->mtx());
        //if user was self, guild may already be deleted
        _guild->remove_member(member_id);
    }
#endif

    gateway::events::guild_member_remove obj{ *_shard };

    const json & j = result["d"];

    obj.user = j["user"];
    if (j.count("guild_id") && !j["guild_id"].is_null())
        obj.guild_id = j["guild_id"];

    if (i_guild_member_remove)
        i_guild_member_remove(obj);
}

AEGIS_DECL void core::ws_guild_member_update(const json & result, shards::shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake member_id = result["d"]["user"]["id"];
    snowflake guild_id = result["d"]["guild_id"];

    auto _member = user_create(member_id);
    auto _guild = find_guild(guild_id);

    if (_member == nullptr)
    {
#ifdef WIN32
        log->critical("Shard#{} : Error in [{}] _member == nullptr", _shard->get_id(), __FUNCSIG__);
#else
        log->critical("Shard#{} : Error in [{}] _member == nullptr", _shard->get_id(), __PRETTY_FUNCTION__);
#endif
        return;
    }
    if (_guild == nullptr)
    {
#ifdef WIN32
        log->critical("Shard#{} : Error in [{}] _guild == nullptr", _shard->get_id(), __FUNCSIG__);
#else
        log->critical("Shard#{} : Error in [{}] _guild == nullptr", _shard->get_id(), __PRETTY_FUNCTION__);
#endif
        return;
    }

    std::unique_lock<shared_mutex> l(_member->mtx(), std::defer_lock);
    std::unique_lock<shared_mutex> l2(_guild->mtx(), std::defer_lock);
    std::lock(l, l2);
    _member->load(_guild, result["d"], _shard);
#endif

    gateway::events::guild_member_update obj{ *_shard };

    const json & j = result["d"];

    obj.user = j["user"];
    if (j.count("nick") && !j["nick"].is_null())
        obj.nick = j["nick"].get<std::string>();
    if (j.count("guild_id") && !j["guild_id"].is_null())
        obj.guild_id = j["guild_id"];
    if (j.count("roles") && !j["roles"].is_null())
        for (const auto & i : j["roles"])
            obj.roles.push_back(i);

    if (i_guild_member_update)
        i_guild_member_update(obj);
}

AEGIS_DECL void core::ws_guild_members_chunk(const json & result, shards::shard * _shard)
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

            auto _member_ptr = user_create(member_id);

            std::unique_lock<shared_mutex> l(_member_ptr->mtx(), std::defer_lock);
            std::unique_lock<shared_mutex> l2(_guild->mtx(), std::defer_lock);
            std::lock(l, l2);
            _member_ptr->load(_guild, _member, _shard);
        }
    }
#endif

    gateway::events::guild_members_chunk obj{ *_shard };

    const json & j = result["d"];

    obj.guild_id = j["guild_id"];
    if (j.count("members") && !j["members"].is_null())
        for (const auto & i : j["members"])
            obj.members.push_back(i);

    if (i_guild_members_chunk)
        i_guild_members_chunk(obj);
}

AEGIS_DECL void core::ws_guild_role_create(const json & result, shards::shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake guild_id = result["d"]["guild_id"];

    auto _guild = find_guild(guild_id);
    _guild->load_role(result["d"]["role"]);
#endif

    gateway::events::guild_role_create obj{ *_shard };

    const json & j = result["d"];

    obj.guild_id = j["guild_id"];
    obj.role = j["role"];

    if (i_guild_role_create)
        i_guild_role_create(obj);
}

AEGIS_DECL void core::ws_guild_role_update(const json & result, shards::shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake guild_id = result["d"]["guild_id"];

    auto _guild = find_guild(guild_id);
    _guild->load_role(result["d"]["role"]);
#endif

    gateway::events::guild_role_update obj{ *_shard };

    const json & j = result["d"];

    obj.guild_id = j["guild_id"];
    obj.role = j["role"];

    if (i_guild_role_update)
        i_guild_role_update(obj);
}

AEGIS_DECL void core::ws_guild_role_delete(const json & result, shards::shard * _shard)
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

    gateway::events::guild_role_delete obj{ *_shard };

    const json & j = result["d"];

    obj.guild_id = j["guild_id"];
    obj.role_id = j["role_id"];

    if (i_guild_role_delete)
        i_guild_role_delete(obj);
}

AEGIS_DECL void core::ws_voice_server_update(const json & result, shards::shard * _shard)
{
    gateway::events::voice_server_update obj{ *_shard };

    const json & j = result["d"];

    obj.token = j["token"].get<std::string>();
    obj.guild_id = j["guild_id"];
    if (!j["endpoint"].is_null())
        obj.endpoint = j["endpoint"].get<std::string>();


    if (i_voice_server_update)
        i_voice_server_update(obj);
}

AEGIS_DECL void core::ws_message_reaction_add(const json & result, shards::shard * _shard)
{
    gateway::events::message_reaction_add obj{ *_shard };

    const json & j = result["d"];

    obj.user_id = j["user_id"];
    obj.channel_id = j["channel_id"];
    obj.message_id = j["message_id"];
    obj.guild_id = j["guild_id"];
    obj.emoji = j["emoji"];

    if (i_message_reaction_add)
        i_message_reaction_add(obj);
}

AEGIS_DECL void core::ws_message_reaction_remove(const json & result, shards::shard * _shard)
{
    gateway::events::message_reaction_remove obj{ *_shard };

    const json & j = result["d"];

    obj.user_id = j["user_id"];
    obj.channel_id = j["channel_id"];
    obj.message_id = j["message_id"];
    obj.guild_id = j["guild_id"];
    obj.emoji = j["emoji"];

    if (i_message_reaction_remove)
        i_message_reaction_remove(obj);
}

AEGIS_DECL void core::ws_message_reaction_remove_all(const json & result, shards::shard * _shard)
{
    gateway::events::message_reaction_remove_all obj{ *_shard };

    const json & j = result["d"];

    obj.channel_id = j["channel_id"];
    obj.message_id = j["message_id"];
    obj.guild_id = j["guild_id"];

    if (i_message_reaction_remove_all)
        i_message_reaction_remove_all(obj);
}

AEGIS_DECL void core::ws_channel_pins_update(const json & result, shards::shard * _shard)
{
    gateway::events::channel_pins_update obj{ *_shard };

    const json & j = result["d"];

    obj.channel_id = j["channel_id"];
    if (j.count("last_pin_timestamp") && !j["last_pin_timestamp"].is_null())
        obj.last_pin_timestamp = j["last_pin_timestamp"].get<std::string>();

    if (i_channel_pins_update)
        i_channel_pins_update(obj);
}

AEGIS_DECL void core::ws_webhooks_update(const json & result, shards::shard * _shard)
{
    gateway::events::webhooks_update obj{ *_shard };

    const json & j = result["d"];

    obj.guild_id = j["guild_id"];
    obj.channel_id = j["channel_id"];

    if (i_webhooks_update)
        i_webhooks_update(obj);
}

AEGIS_DECL aegis::future<gateway::objects::guild> core::create_guild(
    std::string name, lib::optional<std::string> voice_region, lib::optional<int> verification_level,
    lib::optional<int> default_message_notifications, lib::optional<int> explicit_content_filter,
    lib::optional<std::string> icon, lib::optional<std::vector<gateway::objects::role>> roles,
    lib::optional<std::vector<std::tuple<std::string, int>>> channels
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

    return aegis::make_ready_future(gateway::objects::guild(json::parse(get_ratelimit().post_task({ "/guilds", rest::Post, obj.dump() }).get().content)));
}

AEGIS_DECL void aegis::core::update_presence(const std::string& text, gateway::objects::activity::activity_type type, gateway::objects::presence::user_status status)
{
    json j = {
        { "op", 3 },
        {
            "d",
            {
                { "game",
                    {
                        { "name", text },
                        { "type", static_cast<int32_t>(type) }
                    }
                },
                { "status", gateway::objects::presence::to_string(status) },
                { "since", json::value_t::null },
                { "afk", false }
            }
        }
    };
    send_all_shards(j.dump());
    return;
}

}
