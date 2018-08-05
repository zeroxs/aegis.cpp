//
// core.cpp
// ********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
// 

#include "aegis/config.hpp"
#include "aegis/core.hpp"
#include <string>
#include <asio/streambuf.hpp>
#include <asio/connect.hpp>
#include "aegis/shards/shard.hpp"
#include "aegis/rest/rest_controller.hpp"
#include "aegis/member.hpp"

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
    : force_shard_count(0)
    , shard_max_count(0)
    , member_id(0)
    , discriminator(0)
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    , mfa_enabled(false)
#endif
    , _status{ bot_status::Uninitialized }
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

    load_config();

    // DEBUG CODE ONLY
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

    log->set_pattern("%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v");
    log->set_level(loglevel);

    _io_context = std::make_shared<asio::io_context>();

    _shard_mgr = std::make_unique<shards::shard_mgr>(_token, *_io_context, log);

    std::error_code ec;
    setup_gateway(ec);
    if (ec)
        throw ec;

#if defined(AEGIS_PROFILING)
    _rest = std::make_unique<rest::rest_controller>(_token, "/api/v6", "discordapp.com", this);
#else
    _rest = std::make_unique<rest::rest_controller>(_token, "/api/v6", "discordapp.com");
#endif

    _ratelimit = std::make_unique<ratelimit_mgr_t>(
        std::bind(&aegis::rest::rest_controller::execute,
                  _rest.get(),
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3,
                  std::placeholders::_4),
        get_io_context());

    setup_callbacks();
}

AEGIS_DECL core::~core()
{
    if (_shard_mgr)
        _shard_mgr->shutdown();
    if (_io_context)
        _io_context->stop();
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
        auto g = std::make_unique<channel>(id, 0, this, *_io_context);
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

AEGIS_DECL guild * core::guild_create(snowflake id, shards::shard * _shard) AEGIS_NOEXCEPT
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

    set_state(bot_status::Running);

    starttime = std::chrono::steady_clock::now();
    
    if (f)
        f();

    log->info("Starting shard manager with {} shards", _shard_mgr->shard_max_count);
    _shard_mgr->start(count);


    _io_context->stop();
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
        _token = cfg["token"].get<std::string>();
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
        }
    }

    if (!cfg["file-logging"].is_null())
        file_logging = cfg["file-logging"].get<bool>();
}

AEGIS_DECL void core::setup_callbacks()
{
    _shard_mgr->set_on_message(std::bind(&core::on_message, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _shard_mgr->set_on_connect(std::bind(&core::on_connect, this, std::placeholders::_1, std::placeholders::_2));
    _shard_mgr->set_on_close(std::bind(&core::on_close, this, std::placeholders::_1, std::placeholders::_2));
}

AEGIS_DECL void core::shutdown()
{
    set_state(bot_status::Shutdown);
    _shard_mgr->shutdown();
    cv.notify_all();
}

AEGIS_DECL void core::setup_gateway(std::error_code & ec)
{
    log->info("Creating websocket");

#if defined(AEGIS_PROFILING)
    aegis::rest::rest_controller _rest(_token, "/api/v6", "discordapp.com", this);
#else
    aegis::rest::rest_controller _rest(_token, "/api/v6", "discordapp.com");
#endif
    rest::rest_reply res = _rest.get("/gateway/bot");

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
    ws_handlers.emplace("MESSAGE_UPDATE", std::bind(&core::ws_message_update, this, std::placeholders::_1, std::placeholders::_2));
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
        _shard_mgr->shard_max_count = shard_max_count = force_shard_count;
        log->info("Forcing Shard count by config: {}", _shard_mgr->shard_max_count);
    }
    else
    {
        _shard_mgr->shard_max_count = ret["shards"];
        log->info("Shard count: {}", _shard_mgr->shard_max_count);
    }

    _shard_mgr->ws_gateway = ret["url"].get<std::string>();
    _shard_mgr->set_gateway_url(_shard_mgr->ws_gateway + "/?compress=zlib-stream&encoding=json&v=6");

    ec = std::error_code();
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
        _self->_member_id = member_id;
        _self->_is_bot = true;
        _self->_name = username;
        _self->_discriminator = discriminator;
        _self->_status = member::Online;
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
    lib::optional<std::string> voice_region, lib::optional<int> verification_level,
    lib::optional<int> default_message_notifications, lib::optional<int> explicit_content_filter,
    lib::optional<std::string> icon, lib::optional<std::vector<gateway::objects::role>> roles,
    lib::optional<std::vector<std::tuple<std::string, int>>> channels
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
AEGIS_DECL channel * core::dm_channel_create(const json & obj, shards::shard * _shard)
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
        const json result = json::parse(msg);

#if defined(AEGIS_PROFILING)
        if (!result.is_null())
            if (!result["t"].is_null())
                if (js_end)
                    js_end(s_t, result["t"]);
#endif

        if (!result.is_null())
        {
        
           if ((log->level() == spdlog::level::level_enum::trace && wsdbg)
               && (result["t"].is_null()
                   || (result["t"] != "GUILD_CREATE"
                       && result["t"] != "PRESENCE_UPDATE"
                       && result["t"] != "GUILD_MEMBERS_CHUNK")))
               log->trace("Shard#{}: {}", _shard->get_id(), msg);

           int64_t t_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

           _shard->lastwsevent = std::chrono::steady_clock::now();
        
            if (!result["s"].is_null())
                _shard->set_sequence(result["s"]);
        
            if (!result.is_null())
            {
                //does string message exist
                if (!result["t"].is_null())
                {
                    std::string cmd = result["t"];
        
                    //log->info("Shard#{}: {}", _shard->get_id(), cmd);
                   
                    const auto it = ws_handlers.find(cmd);
                    if (it != ws_handlers.end())
                    {
                        //message id found
                        ++message_count[cmd];
                        asio::post(*_io_context, [=, res = std::move(result)]()
                        {
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
                        log->error("Shard#{} : Unable to resume or invalid connection. Starting new", _shard->get_id());
                        _shard->session_id.clear();

                        _shard->delayedauth.expires_after(std::chrono::milliseconds((rand() % 2000) + 5000));
                        _shard->delayedauth.async_wait(asio::bind_executor(*_shard->get_connection()->get_strand(), [=](const asio::error_code & ec)
                        {
                            if (ec == asio::error::operation_aborted)
                                return;

                            if (_shard->get_connection() == nullptr)
                            {
                                //debug?
                                log->error("Shard#{} : Invalid session received with an invalid connection state state: {}", _shard->get_id(), static_cast<int32_t>(_shard->connection_state));
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
                            _shard->send(obj.dump());
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
                    int32_t heartbeat = result["d"]["heartbeat_interval"];
                    _shard->set_heartbeat(std::bind(&core::keep_alive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                    _shard->start_heartbeat(heartbeat);
                    return;
                }
                if (result["op"] == 11)
                {
                    //heartbeat ACK
                    _shard->heartbeat_ack = std::chrono::steady_clock::now();
                    return;
                }
        
                log->error("unhandled op({})", result["op"].get<int>());
                debug_trace(_shard);
                return;
            }
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

AEGIS_DECL void core::keep_alive(const asio::error_code & ec, const int32_t ms, shards::shard * _shard)
{
    if (ec == asio::error::operation_aborted)
        return;
    try
    {
        if (_shard->connection_state == shard_status::Shutdown || !_shard->is_connected())
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
            _shard_mgr->close(_shard, 1001, "");
            _shard_mgr->reset_shard(_shard);
            _shard_mgr->queue_reconnect(_shard);
            return;
        }
        json obj;
        obj["d"] = _shard->get_sequence();
        obj["op"] = 1;
        _shard->send_now(obj.dump());
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

AEGIS_DECL void core::on_close(websocketpp::connection_hdl hdl, shards::shard * _shard)
{
    if (_status == bot_status::Shutdown)
        return;
}

AEGIS_DECL void core::reset_shard(shards::shard * _shard)
{
    _shard->do_reset();
}

AEGIS_DECL void core::ws_presence_update(const json & result, shards::shard * _shard)
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
    _member->_status = status;
#endif

    gateway::events::presence_update obj;
    obj._user = result["d"]["user"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_presence_update)
        i_presence_update(obj);
}

AEGIS_DECL void core::ws_typing_start(const json & result, shards::shard * _shard)
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


AEGIS_DECL void core::ws_message_create(const json & result, shards::shard * _shard)
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

AEGIS_DECL void core::ws_message_update(const json & result, shards::shard * _shard)
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

AEGIS_DECL void core::ws_guild_create(const json & result, shards::shard * _shard)
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
    obj._guild = result["d"];
    obj.bot = this;
    obj._shard = _shard;

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

    std::unique_lock<shared_mutex> l(_guild->mtx());
    _guild->load(result["d"], _shard);

    gateway::events::guild_update obj;
    obj._guild = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_update)
        i_guild_update(obj);
}

AEGIS_DECL void core::ws_guild_delete(const json & result, shards::shard * _shard)
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

AEGIS_DECL void core::ws_message_delete(const json & result, shards::shard * _shard)
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

AEGIS_DECL void core::ws_message_delete_bulk(const json & result, shards::shard * _shard)
{
    gateway::events::message_delete_bulk obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_message_delete_bulk)
        i_message_delete_bulk(obj);
}

AEGIS_DECL void core::ws_user_update(const json & result, shards::shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake member_id = result["d"]["user"]["id"];

    auto _member = member_create(member_id);

    const json & user = result["d"]["user"];
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
    gateway::events::user_update obj(_member);
#else
    gateway::events::user_update obj;
#endif

    obj._user = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_user_update)
        i_user_update(obj);
}

AEGIS_DECL void core::ws_voice_state_update(const json & result, shards::shard * _shard)
{
    gateway::events::voice_state_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;
    
    if (i_voice_state_update)
        i_voice_state_update(obj);
}

AEGIS_DECL void core::ws_resumed(const json & result, shards::shard * _shard)
{
    _shard_mgr->_connecting_shard = nullptr;
    _shard->connection_state = shard_status::Online;
    _shard_mgr->_connect_time = std::chrono::steady_clock::time_point();
    _shard->_ready_time = _shard_mgr->_last_ready = std::chrono::steady_clock::now();
    log->info("Shard#{} RESUMED Processed", _shard->get_id());
    _shard->keepalivetimer.cancel();
    _shard->set_heartbeat(std::bind(&core::keep_alive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _shard->start_heartbeat(_shard->heartbeattime);

    gateway::events::resumed obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_resumed)
        i_resumed(obj);
}

AEGIS_DECL void core::ws_ready(const json & result, shards::shard * _shard)
{
    _shard_mgr->_connecting_shard = nullptr;
    _shard->connection_state = shard_status::Online;
    _shard_mgr->_connect_time = std::chrono::steady_clock::time_point();
    _shard->_ready_time = _shard_mgr->_last_ready = std::chrono::steady_clock::now();
    process_ready(result["d"], _shard);
    log->info("Shard#{} READY Processed", _shard->get_id());

    gateway::events::ready obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

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

    gateway::events::channel_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

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

    gateway::events::channel_delete obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_channel_delete)
        i_channel_delete(obj);
}

AEGIS_DECL void core::ws_guild_ban_add(const json & result, shards::shard * _shard)
{
    gateway::events::guild_ban_add obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_ban_add)
        i_guild_ban_add(obj);
}

AEGIS_DECL void core::ws_guild_ban_remove(const json & result, shards::shard * _shard)
{
    gateway::events::guild_ban_remove obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_ban_remove)
        i_guild_ban_remove(obj);
}

AEGIS_DECL void core::ws_guild_emojis_update(const json & result, shards::shard * _shard)
{
    gateway::events::guild_emojis_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_emojis_update)
        i_guild_emojis_update(obj);
}

AEGIS_DECL void core::ws_guild_integrations_update(const json & result, shards::shard * _shard)
{
    gateway::events::guild_integrations_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_integrations_update)
        i_guild_integrations_update(obj);
}

AEGIS_DECL void core::ws_guild_member_add(const json & result, shards::shard * _shard)
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

AEGIS_DECL void core::ws_guild_member_remove(const json & result, shards::shard * _shard)
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

AEGIS_DECL void core::ws_guild_member_update(const json & result, shards::shard * _shard)
{
#if !defined(AEGIS_DISABLE_ALL_CACHE)
    snowflake member_id = result["d"]["user"]["id"];
    snowflake guild_id = result["d"]["guild_id"];

    auto _member = member_create(member_id);
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

    gateway::events::guild_member_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

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

    gateway::events::guild_role_create obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

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

    gateway::events::guild_role_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

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

    gateway::events::guild_role_delete obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (i_guild_role_delete)
        i_guild_role_delete(obj);
}

AEGIS_DECL void core::ws_voice_server_update(const json & result, shards::shard * _shard)
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
    lib::optional<std::string> voice_region, lib::optional<int> verification_level,
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

    try
    {
        return _rest->execute("/guilds", obj.dump(), "POST");
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
    return _rest->execute(path, content, method, host);
}

}
