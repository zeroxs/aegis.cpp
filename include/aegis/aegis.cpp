//
// aegis.cpp
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

#include "aegis/config.hpp"
#include "aegis/objects/message.hpp"
#include "aegis/events/message_create.hpp"
#include "aegis/events/typing_start.hpp"
#include "aegis/guild.hpp"
#include "aegis/channel.hpp"
#include "aegis/member.hpp"
#include "aegis/ratelimit.hpp"
#include <zstr.hpp>


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



namespace aegiscpp
{


AEGIS_DECL int64_t aegis::get_member_count() const noexcept
{
    int64_t count = 0;
    for (auto & kv : guilds)
        count += kv.second->get_member_count();
    return count;
}

AEGIS_DECL member * aegis::get_member(snowflake id) const noexcept
{
    auto it = members.find(id);
    if (it == members.end())
        return nullptr;
    return it->second.get();
}

AEGIS_DECL channel * aegis::get_channel(snowflake id) const noexcept
{
    auto it = channels.find(id);
    if (it == channels.end())
        return nullptr;
    return it->second.get();
}

AEGIS_DECL guild * aegis::get_guild(snowflake id) const noexcept
{
    auto it = guilds.find(id);
    if (it == guilds.end())
        return nullptr;
    return it->second.get();
}

AEGIS_DECL member * aegis::get_member_create(snowflake id) noexcept
{
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

AEGIS_DECL channel * aegis::get_channel_create(snowflake id) noexcept
{
    auto it = channels.find(id);
    if (it == channels.end())
    {
        auto g = std::make_unique<channel>(id, 0, this
                                           , ratelimit().get(rest_limits::bucket_type::CHANNEL)
                                           , ratelimit().get(rest_limits::bucket_type::EMOJI));
        auto ptr = g.get();
        channels.emplace(id, std::move(g));
        return ptr;
    }
    return it->second.get();
}

AEGIS_DECL guild * aegis::get_guild_create(snowflake id, shard * _shard) noexcept
{
    auto it = guilds.find(id);
    if (it == guilds.end())
    {
        auto g = std::make_unique<guild>(_shard->shardid, id, this
                                         , ratelimit().get(rest_limits::bucket_type::GUILD));
        auto ptr = g.get();
        guilds.emplace(id, std::move(g));
        return ptr;
    }
    return it->second.get();
}

AEGIS_DECL void aegis::setup_callbacks(shard * _shard)
{
    _shard->connection->set_message_handler(
        std::bind(&aegis::on_message, this, std::placeholders::_1, std::placeholders::_2, _shard));

    _shard->connection->set_open_handler(
        std::bind(&aegis::on_connect, this, std::placeholders::_1, _shard));

    _shard->connection->set_close_handler(
        std::bind(&aegis::on_close, this, std::placeholders::_1, _shard));
}

AEGIS_DECL void aegis::shutdown()
{
    set_state(Shutdown);
    rest_work.reset();
    websocket_o.stop_perpetual();
    websocket_o.stop();
    for (auto & _shard : shards)
    {
        _shard->connection_state = Shutdown;
        _shard->connection->close(1001, "");
        _shard->do_reset();
    }
}


AEGIS_DECL void aegis::create_websocket(std::error_code & ec)
{
    log->info("Creating websocket");
    if (status != Ready)
    {
        log->critical("aegis::websocketcreate() called in the wrong state");
        ec = make_error_code(aegiscpp::invalid_state);
        return;
    }

    std::optional<rest_reply> res;

    if (!selfbot)
        res = get("/gateway/bot");
    else
        res = get("/gateway");

    if (!res.has_value() || res->content.size() == 0)
    {
        ec = make_error_code(aegiscpp::get_gateway);
        return;
    }

    if (res->reply_code == 401)
    {
        ec = make_error_code(aegiscpp::invalid_token);
        return;
    }

    ws_handlers.emplace("PRESENCE_UPDATE",          std::bind(&aegis::ws_presence_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("TYPING_START",             std::bind(&aegis::ws_typing_start, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_CREATE",           std::bind(&aegis::ws_message_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_DELETE",           std::bind(&aegis::ws_message_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_CREATE",             std::bind(&aegis::ws_guild_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_UPDATE",             std::bind(&aegis::ws_guild_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_DELETE",             std::bind(&aegis::ws_guild_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("MESSAGE_DELETE_BULK",      std::bind(&aegis::ws_message_delete_bulk, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("USER_UPDATE",              std::bind(&aegis::ws_user_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("RESUMED",                  std::bind(&aegis::ws_resumed, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("READY",                    std::bind(&aegis::ws_ready, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("CHANNEL_CREATE",           std::bind(&aegis::ws_channel_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("CHANNEL_UPDATE",           std::bind(&aegis::ws_channel_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("CHANNEL_DELETE",           std::bind(&aegis::ws_channel_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_BAN_ADD",            std::bind(&aegis::ws_guild_ban_add, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_BAN_REMOVE",         std::bind(&aegis::ws_guild_ban_remove, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_EMOJIS_UPDATE",      std::bind(&aegis::ws_guild_emojis_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_INTEGRATIONS_UPDATE",std::bind(&aegis::ws_guild_integrations_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_MEMBER_ADD",         std::bind(&aegis::ws_guild_member_add, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_MEMBER_REMOVE",      std::bind(&aegis::ws_guild_member_remove, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_MEMBER_UPDATE",      std::bind(&aegis::ws_guild_member_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_MEMBERS_CHUNK",      std::bind(&aegis::ws_guild_members_chunk, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_ROLE_CREATE",        std::bind(&aegis::ws_guild_role_create, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_ROLE_UPDATE",        std::bind(&aegis::ws_guild_role_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("GUILD_ROLE_DELETE",        std::bind(&aegis::ws_guild_role_delete, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("VOICE_STATE_UPDATE",       std::bind(&aegis::ws_voice_state_update, this, std::placeholders::_1, std::placeholders::_2));
    ws_handlers.emplace("VOICE_SERVER_UPDATE",      std::bind(&aegis::ws_voice_server_update, this, std::placeholders::_1, std::placeholders::_2));

    json ret = json::parse(res->content);
    if (ret.count("message"))
    {
        if (ret["message"] == "401: Unauthorized")
        {
            ec = make_error_code(aegiscpp::invalid_token);
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

AEGIS_DECL void aegis::process_ready(const json & d, shard * _shard)
{
    _shard->session_id = d["session_id"];

    const json & guilds = d["guilds"];

    if (_self == nullptr)
    {
        const json & userdata = d["user"];
        discriminator = static_cast<int16_t>(std::stoi(userdata["discriminator"].get<std::string>()));
        member_id = userdata["id"];
        username = userdata["username"];
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

    for (auto & guildobj : guilds)
    {
        snowflake id = guildobj["id"];

        bool unavailable = false;
        if (guildobj.count("unavailable"))
            unavailable = guildobj["unavailable"];

        guild * _guild = get_guild_create(id, _shard);
        _guild->unavailable = unavailable;
        _guild->is_init = false;


        if (!unavailable)
        {
            _guild->load(guildobj, _shard);
            log->info("Shard#{} : CREATED Guild: {} [T:{}] [{}]"
                      , _shard->get_id()
                      , _guild->guild_id
                      , guilds.size()
                      , guildobj["name"].get<std::string>());
        }
    }
}

AEGIS_DECL void aegis::channel_create(const json & obj, shard * _shard)
{
    snowflake channel_id = obj["id"];
    auto _channel = get_channel_create(channel_id);

    try
    {
        //log->debug("Shard#{} : Channel[{}] created for DirectMessage", _shard.m_shardid, channel_id);
        if (obj.count("name") && !obj["name"].is_null()) _channel->name = obj["name"];
        _channel->type = static_cast<channel_type>(obj["type"].get<int>());// 0 = text, 2 = voice

        if (!obj["last_message_id"].is_null()) _channel->last_message_id = obj["last_message_id"];

        //owner_id DirectMessage creator group DirectMessage
        //application_id DirectMessage creator if bot group DirectMessage
        //recipients user objects
    }
    catch (std::exception&e)
    {
        log->error("Shard#{} : Error processing DM channel[{}] {}", _shard->shardid, channel_id, e.what());
    }
}

AEGIS_DECL void aegis::keep_alive(const asio::error_code & ec, const int32_t ms, shard * _shard)
{
    if (ec != asio::error::operation_aborted)
    {
        try
        {
            if (_shard->connection_state == Shutdown || _shard->connection == nullptr)
                return;

            if (_shard->heartbeat_ack != 0 && _shard->lastheartbeat - _shard->heartbeat_ack > ms*2)
            {
                log->error("Heartbeat ack not received. Reconnecting. {} {}", _shard->heartbeat_ack, _shard->lastheartbeat);
                websocket_o.close(_shard->connection, 1001, "");
                _shard->connection_state = Reconnecting;
                _shard->do_reset();
//                 _shard->reconnect_timer = websocket_o.set_timer(10000, [_shard, this](const asio::error_code & ec)
//                 {
//                     if (ec == asio::error::operation_aborted)
//                         return;
//                     _shard->connection_state = Connecting;
//                     asio::error_code wsec;
//                     _shard->connection = websocket_o.get_connection(gateway_url, wsec);
//                     setup_callbacks(_shard);
//                     websocket_o.connect(_shard->connection);
//                 });

                return;
            }
            json obj;
            obj["d"] = _shard->sequence;
            obj["op"] = 1;
            _shard->connection->send(obj.dump(), websocketpp::frame::opcode::text);
            _shard->lastheartbeat = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            _shard->keepalivetimer = websocket_o.set_timer(
                ms,
                std::bind(&aegis::keep_alive, this, std::placeholders::_1, ms, _shard)
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
}

AEGIS_DECL std::optional<rest_reply> aegis::get(std::string_view path)
{
    return call(path, "", "GET");
}

AEGIS_DECL std::optional<rest_reply> aegis::get(std::string_view path, std::string_view content)
{
    return call(path, content, "GET");
}

AEGIS_DECL std::optional<rest_reply> aegis::post(std::string_view path, std::string_view content)
{
    return call(path, content, "POST");
}

AEGIS_DECL std::optional<rest_reply> aegis::call(std::string_view path, std::string_view content, std::string_view method)
{
    try
    {
        asio::ip::tcp::resolver resolver(io_service());
        asio::ip::tcp::resolver::query query("discordapp.com", "443");
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        asio::ssl::context ctx(asio::ssl::context::tlsv12);

        ctx.set_options(
            asio::ssl::context::default_workarounds
            | asio::ssl::context::no_sslv2
            | asio::ssl::context::no_sslv3);

        asio::ssl::stream<asio::ip::tcp::socket> socket(io_service(), ctx);
        SSL_set_tlsext_host_name(socket.native_handle(), "discordapp.com");

        asio::connect(socket.lowest_layer(), endpoint_iterator);

        asio::error_code handshake_ec;
        socket.handshake(asio::ssl::stream_base::client, handshake_ec);

        asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << method << " " << "/api/v6" << path << " HTTP/1.0\r\n";
        request_stream << "Host: discordapp.com\r\n";
        request_stream << "Accept: */*\r\n";
        if (!selfbot)
            request_stream << "Authorization: Bot " << token << "\r\n";
        else
            request_stream << "Authorization: " << token << "\r\n";

        request_stream << "User-Agent: DiscordBot (https://github.com/zeroxs/aegis.cpp " << AEGIS_VERSION_LONG << ")\r\n";
        request_stream << "Content-Length: " << content.size() << "\r\n";
        request_stream << "Content-Type: application/json\r\n";
        request_stream << "Connection: close\r\n\r\n";
        request_stream << content;

        asio::write(socket, request);
        asio::streambuf response;
        asio::read_until(socket, response, "\r\n");
        std::stringstream response_content;
        response_content << &response;

        asio::error_code error;
        while (asio::read(socket, response, asio::transfer_at_least(1), error))
            response_content << &response;

        std::istream response_stream(&response);
        std::istringstream istrm(response_content.str());
        websocketpp::http::parser::response hresponse;
        hresponse.consume(istrm);

        int32_t limit = 0;
        int32_t remaining = 0;
        int64_t reset = 0;
        int32_t retry = 0;
        if (auto test = hresponse.get_header("X-RateLimit-Limit"); !test.empty())
            limit = std::stoul(test);
        if (auto test = hresponse.get_header("X-RateLimit-Remaining"); !test.empty())
            remaining = std::stoul(test);
        if (auto test = hresponse.get_header("X-RateLimit-Reset"); !test.empty())
            reset = std::stoul(test);
        if (auto test = hresponse.get_header("Retry-After"); !test.empty())
            retry = std::stoul(test);

        log->debug("status: {} limit: {} remaining: {} reset: {}", hresponse.get_status_code(), limit, remaining, reset);

        bool global = !(hresponse.get_header("X-RateLimit-Global").empty());

        //TODO: check this with OpenSSL 1.1.0+ 
        if (error != asio::error::eof && error != asio::ssl::error::stream_truncated)
            throw asio::system_error(error);
        return { { hresponse.get_status_code(), global, limit, remaining, reset, retry
            , hresponse.get_body() } };
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }
    return {};
}

AEGIS_DECL void aegis::on_message(websocketpp::connection_hdl hdl, message_ptr msg, shard * _shard)
{
    std::string payload = msg->get_payload();

    try
    {
        //zlib detection and decoding
        if (payload[0] == (char)0x78 && (payload[1] == (char)0x01 || payload[1] == (char)0x9C || payload[1] == (char)0xDA))
        {
            std::stringstream origin(msg->get_payload());
            zstr::istream is(origin);
            std::stringstream ss;
            std::string s;
            while (getline(is, s))
                ss << s;
            ss << '\0';
            payload = ss.str();
        }

       const json result = json::parse(payload);

       if (!result.is_null())
        {
        
           if ((log->level() == spdlog::level::level_enum::trace && wsdbg)
               && (result["t"].is_null()
                   || (result["t"] != "GUILD_CREATE"
                       && result["t"] != "PRESENCE_UPDATE"
                       && result["t"] != "GUILD_MEMBERS_CHUNK")))
               log->trace("Shard#{}: {}", _shard->shardid, payload);

            //////////////////////////////////////////////////////////////////////////
            int64_t t_time = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            _shard->debug_messages[t_time] = payload;
        
            _shard->lastwsevent = t_time;
            ///check old messages and remove
        
            for (auto iter = _shard->debug_messages.begin(); iter != _shard->debug_messages.end(); ++iter)
                if (iter->first < t_time - 30)
                    _shard->debug_messages.erase(iter++);
            //////////////////////////////////////////////////////////////////////////
        
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
        
                        io_service().post([this, it, res = std::move(result), _shard, cmd]()
                        {
                            (it->second)(res, _shard);
                        });
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
                        std::this_thread::sleep_for(std::chrono::seconds(6));
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
                        if (_shard->connection_state == Connecting && _shard->connection != nullptr)
                        {
                            _shard->connection->send(obj.dump(), websocketpp::frame::opcode::text);
                        }
                        else
                        {
                            //debug?
                            log->error("Shard#{} : Invalid session received with an invalid connection state state: {} connection exists: {}", _shard->shardid, static_cast<int32_t>(_shard->connection_state), _shard->connection != nullptr);
                            websocket_o.close(_shard->connection, 1001, "");
                            _shard->connection_state = Reconnecting;
                            _shard->do_reset();
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
        
                    _shard->connection->send(obj.dump(), websocketpp::frame::opcode::text);
                }
                if (result["op"] == 10)
                {
                    int32_t heartbeat = result["d"]["heartbeat_interval"];
                    _shard->keepalivetimer = websocket_o.set_timer(
                        heartbeat,
                        std::bind(&aegis::keep_alive, this, std::placeholders::_1, heartbeat, _shard)
                    );
                    _shard->heartbeattime = heartbeat;
                }
                if (result["op"] == 11)
                {
                    //heartbeat ACK
                    _shard->heartbeat_ack = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
                }
        
            }
        }
    }
    catch (std::exception& e)
    {
        log->error("Failed to process object: {0}", e.what());
        log->error(msg->get_payload());

        debug_trace(_shard);
    }
    catch (...)
    {
        log->error("Failed to process object: Unknown error");
        debug_trace(_shard);
    }

}

AEGIS_DECL void aegis::on_connect(websocketpp::connection_hdl hdl, shard * _shard)
{
    log->info("Connection established");
    _shard->connection_state = Connecting;

    try
    {
        if (!selfbot)
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
                log->info("Attemping RESUME with id : {}", _shard->session_id);
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
            //log->trace("Shard#{}: {}", _shard->shardid, obj.dump());
            if (_shard->connection)
                _shard->connection->send(obj.dump(), websocketpp::frame::opcode::text);
        }
        else
        {
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
            if (_shard->connection)
                _shard->connection->send(obj.dump(), websocketpp::frame::opcode::text);
        }
    }
    catch (...)
    {
        log->critical("Uncaught on_connect exception");
    }
}

AEGIS_DECL void aegis::on_close(websocketpp::connection_hdl hdl, shard * _shard)
{
    log->info("Connection closed");
    if (status == Shutdown)
        return;
    _shard->connection_state = Reconnecting;
    _shard->do_reset();
    _shard->start_reconnect();
}

AEGIS_DECL void aegis::ws_status(const asio::error_code & ec)
{
    if (ec == asio::error::operation_aborted)
        return;
    using namespace std::chrono_literals;
    int64_t checktime;
    if (status != Shutdown)
    {
        try
        {
            checktime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            for (auto & _shard : shards)
            {
                if (_shard && _shard->connection)
                {
                    if (_shard->lastwsevent && (_shard->lastwsevent < (checktime - 90)))
                    {
                        log->critical("Shard#{} : Websocket had no events in last 90s", _shard->shardid);
                        debug_trace(_shard.get());
                        if (_shard->connection->get_state() < websocketpp::session::state::closing)
                            websocket_o.close(_shard->connection, 1001, "");
                        else
                            _shard->start_reconnect();
                        _shard->connection_state = Reconnecting;
                        _shard->do_reset();
                    }
                }
                _shard->last_status_time = checktime;

                //find a prettier way to do this
                if (_shard && !_shard->connection && !_shard->reconnect_timer)
                {
                    _shard->start_reconnect();
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
    }
    ws_timer = websocket_o.set_timer(5000, std::bind(&aegis::ws_status, this, std::placeholders::_1));
}

AEGIS_DECL void aegis::debug_trace(shard * _shard)
{
    fmt::MemoryWriter w;

    w << "\n==========<Start Error Trace>==========\n"
        << "Shard: " << _shard->shardid << '\n'
        << "Seq: " << _shard->sequence << '\n';
    int i = 0;
    if (!_shard->debug_messages.size())//no messages even received yet
        return;
    for (auto iter = _shard->debug_messages.rbegin(); (i < 5 && iter != _shard->debug_messages.rend()); ++i, ++iter)
        w << (*iter).second << '\n';


    for (auto & c : shards)
    {
        if (c == nullptr)
            w << fmt::format("Shard#{} shard:{:p}\n",
                             _shard->shardid,
                             static_cast<void*>(c.get()));

        else
            w << fmt::format("Shard#{} shard:{:p} m_connection:{:p}\n",
                             _shard->shardid,
                             static_cast<void*>(c.get()),
                             static_cast<void*>(c->connection.get()));
    }

    w << "==========<End Error Trace>==========";

    log->critical(w.str());
}

AEGIS_DECL void aegis::connect(std::error_code & ec) noexcept
{
    log->info("Websocket[s] connecting");

    for (uint32_t k = 0; k < shard_max_count; ++k)
    {
        auto _shard = std::make_unique<shard>(this);
        _shard->connection = websocket_o.get_connection(gateway_url, ec);
        _shard->shardid = k;

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

AEGIS_DECL void aegis::ws_presence_update(const json & result, shard * _shard)
{
    _shard->counters.presence_changes++;
    json user = result["d"]["user"];
    snowflake guild_id = result["d"]["guild_id"];
    snowflake member_id = user["id"];


    member::member_status status;
    if (result["d"]["status"] == "idle")
        status = member::Idle;
    else if (result["d"]["status"] == "dnd")
        status = member::DoNotDisturb;
    else if (result["d"]["status"] == "online")
        status = member::Online;
    else
        status = member::Offline;

    auto _member = get_member_create(member_id);
    auto  _guild = get_guild(guild_id);
    _member->load(_guild, result["d"], _shard);
    _member->status = status;

    presence_update obj;
    obj.bot = this;
    obj._shard = _shard;
    obj._user = result["d"]["user"];

    if (_callbacks.i_presence_update)
        _callbacks.i_presence_update(obj);
}

AEGIS_DECL void aegis::ws_typing_start(const json & result, shard * _shard)
{
    auto _channel = get_channel(result["d"]["channel_id"]);
    auto _member = get_member(result["d"]["user_id"]);
    typing_start obj;
    obj._channel = _channel;
    obj._member = _member;
    obj.timestamp = result["d"]["timestamp"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_typing_start)
        _callbacks.i_typing_start(obj);
}


AEGIS_DECL void aegis::ws_message_create(const json & result, shard * _shard)
{
    _shard->counters.messages++;

    message_create obj( result["d"]
        , get_channel(result["d"]["channel_id"])
        , get_member(result["d"]["author"]["id"]));
    obj.bot = this;
    obj._shard = _shard;
    obj.msg.init(_shard);

    if (!obj.has_channel())
        //catch this
        return;

    if (obj._channel->guild_id == 0)//DM
    {
        if (_callbacks.i_message_create_dm)
            _callbacks.i_message_create_dm(obj);
    }
    else
    {
        if (_callbacks.i_message_create)
            _callbacks.i_message_create(obj);
    }
}

AEGIS_DECL void aegis::ws_message_update(const json & result, shard * _shard)
{
    message_update obj( result["d"]
        , get_channel(result["d"]["channel_id"])
        , (result["d"].count("author") && result["d"]["author"].count("id"))? get_member(result["d"]["author"]["id"]) : nullptr );
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_message_update)
        _callbacks.i_message_update(obj);
}

AEGIS_DECL void aegis::ws_guild_create(const json & result, shard * _shard)
{
    snowflake guild_id = result["d"]["id"];

    auto _guild = get_guild_create(guild_id, _shard);
    if (_guild->unavailable && _guild->get_owner())
    {
        //outage
    }

    _guild->load(result["d"], _shard);

    //TODO: abide by rate limits (120/60)
    json chunk;
    chunk["d"]["guild_id"] = std::to_string(guild_id);
    chunk["d"]["query"] = "";
    chunk["d"]["limit"] = 0;
    chunk["op"] = 8;
    _shard->connection->send(chunk.dump(), websocketpp::frame::opcode::text);

    guild_create obj;
    obj.bot = this;
    obj._guild = result["d"];
    obj._shard = _shard;

    if (_callbacks.i_guild_create)
        _callbacks.i_guild_create(obj);
}

AEGIS_DECL void aegis::ws_guild_update(const json & result, shard * _shard)
{
    snowflake guild_id = result["d"]["id"];

    auto _guild = get_guild(guild_id);
    if (_guild == nullptr)
    {
        log->error("Guild Update: [{}] does not exist", guild_id);
        //this should never happen
        return;
    }

    _guild->load(result["d"], _shard);

    guild_update obj;
    obj.bot = this;
    obj._shard = _shard;
    obj._guild = result["d"];

    if (_callbacks.i_guild_update)
        _callbacks.i_guild_update(obj);
}

AEGIS_DECL void aegis::ws_guild_delete(const json & result, shard * _shard)
{
    guild_delete obj;
    obj.bot = this;
    obj._shard = _shard;
    obj.guild_id = result["d"]["id"];
    if (result["d"].count("unavailable"))
        obj.unavailable = result["d"]["unavailable"];
    else
        obj.unavailable = false;

    if (_callbacks.i_guild_delete)
        _callbacks.i_guild_delete(obj);

    if (obj.unavailable == true)
    {
        //outage
    }
    else
    {
        snowflake guild_id = result["d"]["id"];

        auto _guild = get_guild(guild_id);
        if (_guild == nullptr)
        {
            log->critical("Guild Delete: [{}] does not exist", guild_id);
            //this should never happen
            return;
        }

        _guild->unavailable = obj.unavailable;

        //kicked or left
        //websocket_o.set_timer(5000, [this, id, _shard](const asio::error_code & ec)
        //{
        guilds.erase(guild_id);
        //});
    }
}

AEGIS_DECL void aegis::ws_message_delete(const json & result, shard * _shard)
{
    message_delete obj( get_channel(result["d"]["channel_id"]) 
    , static_cast<snowflake>(std::stoll(result["d"]["id"].get<std::string>())) );
    obj.bot = this;
    obj.message_id = result["d"]["id"];

    if (_callbacks.i_message_delete)
        _callbacks.i_message_delete(obj);
}

AEGIS_DECL void aegis::ws_message_delete_bulk(const json & result, shard * _shard)
{
    message_delete_bulk obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_message_delete_bulk)
        _callbacks.i_message_delete_bulk(obj);
}

AEGIS_DECL void aegis::ws_user_update(const json & result, shard * _shard)
{
    snowflake member_id = result["d"]["user"]["id"];

    auto _member = get_member_create(member_id);

    const json & user = result["d"]["user"];
    if (user.count("username") && !user["username"].is_null())
        _member->name = user["username"];
    if (user.count("avatar") && !user["avatar"].is_null())
        _member->avatar = user["avatar"];
    if (user.count("discriminator") && !user["discriminator"].is_null())
        _member->discriminator = static_cast<uint16_t>(std::stoi(user["discriminator"].get<std::string>()));
    if (user.count("mfa_enabled") && !user["mfa_enabled"].is_null())
        _member->mfa_enabled = user["mfa_enabled"];
    if (user.count("bot") && !user["bot"].is_null())
        _member->is_bot = user["bot"];
    //if (!user["verified"].is_null()) _member.m_verified = user["verified"];
    //if (!user["email"].is_null()) _member.m_email = user["email"];

    user_update obj;
    obj._member = _member;
    obj.bot = this;
    obj._shard = _shard;
    obj = result["d"];

    if (_callbacks.i_user_update)
        _callbacks.i_user_update(obj);
}

AEGIS_DECL void aegis::ws_voice_state_update(const json & result, shard * _shard)
{
    voice_state_update obj;
    obj.bot = this;
    obj._shard = _shard;
    obj = result["d"];
    
    if (_callbacks.i_voice_state_update)
        _callbacks.i_voice_state_update(obj);
}

AEGIS_DECL void aegis::ws_resumed(const json & result, shard * _shard)
{
    _shard->connection_state = Online;
    log->info("Shard#[{}] RESUMED Processed", _shard->shardid);
    if (_shard->keepalivetimer)
        _shard->keepalivetimer->cancel();
    _shard->keepalivetimer = websocket_o.set_timer(
        _shard->heartbeattime
        , std::bind(&aegis::keep_alive, this, std::placeholders::_1, _shard->heartbeattime, _shard)
    );

    resumed obj;
    obj.bot = this;
    obj._shard = _shard;
    obj = result["d"];

    if (_callbacks.i_resumed)
        _callbacks.i_resumed(obj);
}

AEGIS_DECL void aegis::ws_ready(const json & result, shard * _shard)
{
    process_ready(result["d"], _shard);
    _shard->connection_state = Online;
    log->info("Shard#[{}] READY Processed", _shard->shardid);

    ready obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_ready)
        _callbacks.i_ready(obj);
}

AEGIS_DECL void aegis::ws_channel_create(const json & result, shard * _shard)
{
    snowflake channel_id = result["d"]["id"];

    if (result["d"].count("guild_id") && !result["d"]["guild_id"].is_null())
    {
        //guild channel
        snowflake guild_id = result["d"]["guild_id"];
        auto _guild = get_guild(guild_id);
        auto _channel = _guild->get_channel_create(channel_id, _shard);
        _channel->load_with_guild(*_guild, result["d"], _shard);
    }
    else
    {
        //dm
        auto _channel = get_channel_create(channel_id);
        channel_create(result["d"], _shard);
    }

    aegiscpp::channel_create obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_channel_create)
        _callbacks.i_channel_create(obj);
}

AEGIS_DECL void aegis::ws_channel_update(const json & result, shard * _shard)
{
    snowflake channel_id = result["d"]["id"];

    if (!result["d"]["guild_id"].is_null())
    {
        //guild channel
        snowflake guild_id = result["d"]["guild_id"];
        guild * _guild = get_guild(guild_id);
        if (_guild == nullptr)//TODO: errors
            return;
        channel * _channel = _guild->get_channel_create(channel_id, _shard);
        _channel->load_with_guild(*_guild, result["d"], _shard);
    }
    else
    {
        //dm
        auto _channel = get_channel_create(channel_id);
        channel_create(result["d"], _shard);
    }

    channel_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_channel_update)
        _callbacks.i_channel_update(obj);
}

AEGIS_DECL void aegis::ws_channel_delete(const json & result, shard * _shard)
{
    channel_delete obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_channel_delete)
        _callbacks.i_channel_delete(obj);
}

AEGIS_DECL void aegis::ws_guild_ban_add(const json & result, shard * _shard)
{
    guild_ban_add obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_ban_add)
        _callbacks.i_guild_ban_add(obj);
}

AEGIS_DECL void aegis::ws_guild_ban_remove(const json & result, shard * _shard)
{
    guild_ban_remove obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_ban_remove)
        _callbacks.i_guild_ban_remove(obj);
}

AEGIS_DECL void aegis::ws_guild_emojis_update(const json & result, shard * _shard)
{
    guild_emojis_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_emojis_update)
        _callbacks.i_guild_emojis_update(obj);
}

AEGIS_DECL void aegis::ws_guild_integrations_update(const json & result, shard * _shard)
{
    guild_integrations_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_integrations_update)
        _callbacks.i_guild_integrations_update(obj);
}

AEGIS_DECL void aegis::ws_guild_member_add(const json & result, shard * _shard)
{
    snowflake member_id = result["d"]["user"]["id"];
    snowflake guild_id = result["d"]["guild_id"];

    auto _member = get_member_create(member_id);
    auto _guild = get_guild(guild_id);

    _member->load(_guild, result["d"], _shard);

    guild_member_add obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_member_add)
        _callbacks.i_guild_member_add(obj);
}

AEGIS_DECL void aegis::ws_guild_member_remove(const json & result, shard * _shard)
{
    snowflake member_id = result["d"]["user"]["id"];
    snowflake guild_id = result["d"]["guild_id"];

    auto _member = get_member(member_id);
    auto _guild = get_guild(guild_id);

    if (_guild != nullptr)
    {
        //if user was self, guild may already be deleted
        _guild->remove_member(member_id);
    }

    guild_member_remove obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_member_remove)
        _callbacks.i_guild_member_remove(obj);
}

AEGIS_DECL void aegis::ws_guild_member_update(const json & result, shard * _shard)
{
    snowflake member_id = result["d"]["user"]["id"];
    snowflake guild_id = result["d"]["guild_id"];

    auto _member = get_member_create(member_id);
    auto _guild = get_guild(guild_id);

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

    _member->load(_guild, result["d"], _shard);

    guild_member_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_member_update)
        _callbacks.i_guild_member_update(obj);
}

AEGIS_DECL void aegis::ws_guild_members_chunk(const json & result, shard * _shard)
{
    snowflake guild_id = result["d"]["guild_id"];
    auto _guild = get_guild(guild_id);
    if (_guild == nullptr)
        return;
    auto & members = result["d"]["members"];
    //log->info("GUILD_MEMBERS_CHUNK: {1} {0}", guild_id, members.size());
    if (!members.empty())
    {
        for (auto & _member : members)
        {
            snowflake member_id = _member["user"]["id"];

            auto _member_ptr = get_member_create(member_id);

            _member_ptr->load(_guild, _member, _shard);
        }
    }

    guild_members_chunk obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_member_chunk)
        _callbacks.i_guild_member_chunk(obj);
}

AEGIS_DECL void aegis::ws_guild_role_create(const json & result, shard * _shard)
{
    snowflake guild_id = result["d"]["guild_id"];

    auto _guild = get_guild(guild_id);
    _guild->load_role(result["d"]["role"]);

    guild_role_create obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_role_create)
        _callbacks.i_guild_role_create(obj);
}

AEGIS_DECL void aegis::ws_guild_role_update(const json & result, shard * _shard)
{
    snowflake guild_id = result["d"]["guild_id"];

    auto _guild = get_guild(guild_id);
    _guild->load_role(result["d"]["role"]);

    guild_role_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_role_update)
        _callbacks.i_guild_role_update(obj);
}

AEGIS_DECL void aegis::ws_guild_role_delete(const json & result, shard * _shard)
{
    snowflake guild_id = result["d"]["guild_id"];
    snowflake role_id = result["d"]["role_id"];

    auto _guild = get_guild(guild_id);

    if (_guild != nullptr)
    {
        //if role was own, we may have been kicked and guild may already be deleted
        _guild->remove_role(role_id);
    }

    guild_role_delete obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_guild_role_delete)
        _callbacks.i_guild_role_delete(obj);
}

AEGIS_DECL void aegis::ws_voice_server_update(const json & result, shard * _shard)
{
    voice_server_update obj;
    obj = result["d"];
    obj.bot = this;
    obj._shard = _shard;

    if (_callbacks.i_voice_server_update)
        _callbacks.i_voice_server_update(obj);
}

}
