//
// aegis_impl.hpp
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
#include "objects/message.hpp"
#include "events/message_create.hpp"
#include "events/typing_start.hpp"

namespace aegiscpp
{

using namespace std::chrono;
namespace spd = spdlog;
using json = nlohmann::json;
using namespace std::literals;

inline void aegis::processReady(const json & d, shard * _shard)
{
    _shard->session_id = d["session_id"];

    const json & guilds = d["guilds"];

    if (self() == nullptr)
    {
        const json & userdata = d["user"];
        discriminator = static_cast<uint16_t>(std::stoi(userdata["discriminator"].get<std::string>()));
        member_id = userdata["id"];
        username = userdata["username"];
        mfa_enabled = userdata["mfa_enabled"];
        if (mention.size() == 0)
        {
            std::stringstream ss;
            ss << "<@" << member_id << ">";
            mention = ss.str();
        }

        auto ptr = std::make_shared<member>(member_id);
        members.emplace(member_id, ptr);
        ptr->member_id = member_id;
        ptr->isbot = true;
        ptr->name = username;
        ptr->discriminator = discriminator;
        ptr->status = member::Online;
        state.user.id = member_id;
        state.user.name = username;
    }

    for (auto & guildobj : guilds)
    {
        snowflake id = guildobj["id"];

        bool unavailable = false;
        if (guildobj.count("unavailable"))
            unavailable = guildobj["unavailable"];

        guild * _guild = get_guild_create(id, _shard);
        _guild->unavailable = unavailable;


        if (!unavailable)
        {
            _guild->load(guildobj, _shard);
            log->info("Shard#{} : CREATED Guild: {} [T:{}] [{}]", _shard->get_id(), _guild->guild_id, guilds.size(), guildobj["name"].get<std::string>());
        }
    }
}

inline void aegis::channel_create(const json & obj, shard * _shard)
{
    snowflake channel_id = obj["id"];
    auto _channel = get_channel_create(channel_id);

    try
    {
        //log->debug("Shard#{} : Channel[{}] created for DirectMessage", _shard.m_shardid, channel_id);
        if (!obj["name"].is_null()) _channel->name = obj["name"];
        _channel->type = static_cast<channel_type>(obj["type"].get<int>());// 0 = text, 2 = voice
        _channel->guild_id = 0;

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

inline void aegis::keepAlive(const asio::error_code & ec, const int ms, shard * _shard)
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
            _shard->keepalivetimer = websocket_o.set_timer(ms, std::bind(&aegis::keepAlive, this, std::placeholders::_1, ms, _shard));
        }
        catch (websocketpp::exception & e)
        {
            log->error("Websocket exception : {0}", e.what());
        }
    }
}

inline std::optional<rest_reply> aegis::get(std::string_view path)
{
    return call(path, "", "GET");
}

inline std::optional<rest_reply> aegis::get(std::string_view path, std::string_view content)
{
    return call(path, content, "GET");
}

std::optional<rest_reply> aegis::post(std::string_view path, std::string_view content)
{
    return call(path, content, "POST");
}

inline std::optional<rest_reply> aegis::call(std::string_view path, std::string_view content, std::string_view method)
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
        if (auto test = hresponse.get_header("X-RateLimit-Limit"); test.size() > 0)
            limit = std::stoul(test);
        if (auto test = hresponse.get_header("X-RateLimit-Remaining"); test.size() > 0)
            remaining = std::stoul(test);
        if (auto test = hresponse.get_header("X-RateLimit-Reset"); test.size() > 0)
            reset = std::stoul(test);
        if (auto test = hresponse.get_header("Retry-After"); test.size() > 0)
            retry = std::stoul(test);

        log->debug("status: {} limit: {} remaining: {} reset: {}", hresponse.get_status_code(), limit, remaining, reset);

        bool global = !(hresponse.get_header("X-RateLimit-Global").size() == 0);

        //TODO: check this with OpenSSL 1.1.0+ 
        if (error != asio::error::eof && error != asio::ssl::error::stream_truncated)
            throw asio::system_error(error);
        return { { hresponse.get_status_code(), global, limit, remaining, reset, retry, hresponse.get_body().size() > 0 ? hresponse.get_body() : "" } };
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }
    return {};
}

inline void aegis::onMessage(websocketpp::connection_hdl hdl, message_ptr msg, shard * _shard)
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
        if (log->level() > spdlog::level::level_enum::trace && !result.is_null()
            && (result["t"].is_null() || (result["t"] != "GUILD_CREATE" && result["t"] != "PRESENCE_UPDATE")))
            log->trace("Shard#{}: {}", _shard->shardid, payload);


        //////////////////////////////////////////////////////////////////////////
        int64_t t_time = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        _shard->debug_messages[t_time] = payload;

        ///check old messages and remove

        for (auto iter = _shard->debug_messages.begin(); iter != _shard->debug_messages.end(); ++iter)
            if (iter->first < t_time - 10)
                _shard->debug_messages.erase(iter++);
        //////////////////////////////////////////////////////////////////////////

        if (!result["s"].is_null())
            if (result["s"].get<int64_t>() > _shard->sequence)
                _shard->sequence = result["s"];

        if (!result.is_null())
        {
            if (!result["t"].is_null())
            {
                if (result["t"] == "PRESENCE_UPDATE")
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
                    else if (result["d"]["status"] == "offline")
                        status = member::Offline;

                    auto _member = get_member_create(member_id);
                    auto  _guild = get_guild(guild_id);
                    _member->load(_guild, result["d"], _shard);
                    _member->status = status;

                    presence_update obj;
                    obj.bot = this;
                    obj._shard = _shard;
                    obj._user = result["d"]["user"];

                    if (i_presence_update)
                        i_presence_update(obj);
                    return;
                }

                const std::string & cmd = result["t"].get<std::string>();

                if (cmd == "TYPING_START")
                {
                    auto _channel = get_channel(result["d"]["channel_id"]);
                    auto _member = get_member(result["d"]["user_id"]);
                    typing_start obj;
                    obj._channel = _channel.get();
                    obj._member = _member.get();
                    obj.timestamp = result["d"]["timestamp"];
                    obj.bot = this;
                    obj._shard = _shard;

                    if (i_typing_start)
                        i_typing_start(std::move(obj));
                    return;
                }
                if (cmd == "MESSAGE_CREATE")
                {
                    _shard->counters.messages++;

                    message_create obj;
                    obj._member = get_member(result["d"]["author"]["id"]).get();
                    obj._channel = get_channel(result["d"]["channel_id"]).get();
                    obj.msg = result["d"];
                    obj.bot = this;
                    obj._shard = _shard;

                    if (obj._channel == nullptr)
                        //catch this
                        return;

                    if (obj._channel->guild_id == 0)//DM
                    {
                        if (i_message_create_dm)
                            i_message_create_dm(obj);
                    }
                    else
                    {
                        if (i_message_create)
                            i_message_create(obj);
                    }
                    return;
                }
                if (cmd == "MESSAGE_UPDATE")
                {
                    message_update obj;
                    if (result["d"].count("author") && result["d"]["author"].count("id"))
                        obj._member = get_member(result["d"]["author"]["id"]).get();
                    obj._channel = get_channel(result["d"]["channel_id"]).get();
                    obj.msg = result["d"];
                    obj.bot = this;
                    obj._shard = _shard;

                    if (i_message_update)
                        i_message_update(obj);
                    return;
                }
                else if (cmd == "GUILD_CREATE")
                {
                    _shard->counters.guilds++;
                    
                    snowflake guild_id = result["d"]["id"];

                    auto _guild = get_guild_create(guild_id, _shard);
                    if (_guild->unavailable)
                    {
                        //outage
                        _shard->counters.guilds_outage--;
                    }
                    if (_guild->is_init)
                    {
                        std::string guild_msg = fmt::format("Shard#{} : CREATED Guild: {} [T:{}] [{}]", _shard->get_id(), _guild->guild_id, guilds.size(), result["d"]["name"].get<std::string>());
                        log->info(guild_msg);
                        if (control_channel)
                            get_channel(control_channel)->create_message(guild_msg);
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

                    if (i_guild_create)
                        i_guild_create(obj);
                    return;
                }
                else if (cmd == "GUILD_UPDATE")
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

                    if (i_guild_update)
                        i_guild_update(obj);
                    return;
                }
                else if (cmd == "GUILD_DELETE")
                {
                    _shard->counters.guilds--;
                    
                    guild_delete obj;
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
                        _shard->counters.guilds_outage++;
                    }
                    else
                    {
                        snowflake guild_id = result["d"]["id"];

                        auto _guild = get_guild(guild_id);
                        if (_guild == nullptr)
                        {
                            log->error("Guild Delete: [{}] does not exist", guild_id);
                            //this should never happen
                            return;
                        }

                        _guild->unavailable = obj.unavailable;

                        std::string guild_msg = fmt::format("Shard#{} : DELETED Guild: {} [T:{}] [{}]", _shard->get_id(), _guild->guild_id, guilds.size(), _guild->name);
                        log->info(guild_msg);
                        if (control_channel)
                            get_channel(control_channel)->create_message(guild_msg);

                        //kicked or left
                        //websocket_o.set_timer(5000, [this, id, _shard](const asio::error_code & ec)
                        //{
                            guilds.erase(guild_id);
                        //});
                    }
                    return;
                }
                else if (cmd == "MESSAGE_DELETE")
                {
                    message_delete obj;
                    obj._channel = get_channel(result["d"]["channel_id"]).get();
                    obj.bot = this;
                    obj.message_id = result["d"]["id"];
                    obj.channel_id = result["d"]["channel_id"];

                    if (i_message_delete)
                        i_message_delete(obj);
                    return;
                }
                else if (cmd == "MESSAGE_DELETE_BULK")
                {
                    if (i_message_delete_bulk)
                        i_message_delete_bulk(result, _shard, *this);
                    return;
                }
                else if (cmd == "USER_UPDATE")
                {
                    snowflake member_id = result["d"]["user"]["id"];

                    auto _member = get_member_create(member_id);

                    const json & user = result["d"]["user"];
                    if (user.count("username") && !user["username"].is_null()) _member->name = user["username"];
                    if (user.count("avatar") && !user["avatar"].is_null()) _member->avatar = user["avatar"];
                    if (user.count("discriminator") && !user["discriminator"].is_null()) _member->discriminator = static_cast<uint16_t>(std::stoi(user["discriminator"].get<std::string>()));
                    if (user.count("mfa_enabled") && !user["mfa_enabled"].is_null()) _member->mfa_enabled = user["mfa_enabled"];
                    if (user.count("bot") && !user["bot"].is_null()) _member->isbot = user["bot"];
                    //if (!user["verified"].is_null()) _member.m_verified = user["verified"];
                    //if (!user["email"].is_null()) _member.m_email = user["email"];

                    user_update obj;
                    obj._member = _member.get();
                    obj.bot = this;
                    obj._shard = _shard;
                    obj = result["d"];

                    if (i_user_update)
                        i_user_update(obj);

                    return;
                }
                else if (cmd == "VOICE_STATE_UPDATE")
                {
                    if (i_voice_state_update)
                        i_voice_state_update(result, _shard, *this);
                    return;
                }
                else if (cmd == "RESUMED")
                {
                    _shard->connection_state = Online;
                    log->info("Shard#[{}] RESUMED Processed", _shard->shardid);
                    if (_shard->keepalivetimer)
                        _shard->keepalivetimer->cancel();
                    _shard->keepalivetimer = websocket_o.set_timer(_shard->heartbeattime, std::bind(&aegis::keepAlive, this, std::placeholders::_1, _shard->heartbeattime, _shard));
                    return;
                }
                else if (cmd == "READY")
                {
                    processReady(result["d"], _shard);
                    _shard->connection_state = Online;
                    log->info("Shard#[{}] READY Processed", _shard->shardid);

                    ready obj;
                    obj = result["d"];
                    obj.bot = this;
                    obj._shard = _shard;

                    if (i_ready)
                        i_ready(obj);

                    return;
                }
                else if (cmd == "CHANNEL_CREATE")
                {
                    if (i_channel_create)
                        i_channel_create(result, _shard, *this);

                    snowflake channel_id = result["d"]["id"];

                    if (!result["d"]["guild_id"].is_null())
                    {
                        //guild channel
                        snowflake guild_id = result["d"]["guild_id"];
                        auto _guild = get_guild(guild_id);
                        auto _channel = _guild->get_channel_create(channel_id, _shard);
                        _channel->load_with_guild(*_guild, result["d"], _shard);
                        _shard->counters.channels++;
                    }
                    else
                    {
                        //dm
                        auto _channel = get_channel_create(channel_id);
                        channel_create(result["d"], _shard);
                    }

                    return;
                }

                //////////////////////////////////////////////////////////////////////////
                //start of guild_snowflake events
                //everything beyond here has a guild_snowflake

                if (result["d"].count("guild_id"))
                {
                    if (cmd == "CHANNEL_UPDATE")
                    {
                        if (i_channel_update)
                            i_channel_update(result, _shard, *this);

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
                        return;
                    }
                    else if (cmd == "CHANNEL_DELETE")
                    {
                        _shard->counters.channels--;
                       
                        if (i_channel_delete)
                            i_channel_delete(result, _shard, *this);
                        return;
                    }
                    else if (cmd == "GUILD_BAN_ADD")
                    {
                        guild_ban_add obj;
                        obj.bot = this;
                        obj._shard = _shard;

                        if (i_guild_ban_add)
                            i_guild_ban_add(obj);
                        return;
                    }
                    else if (cmd == "GUILD_BAN_REMOVE")
                    {
                        guild_ban_remove obj;
                        obj.bot = this;
                        obj._shard = _shard;

                        if (i_guild_ban_remove)
                            i_guild_ban_remove(obj);
                        return;
                    }
                    else if (cmd == "GUILD_EMOJIS_UPDATE")
                    {
                        if (i_guild_emojis_update)
                            i_guild_emojis_update(result, _shard, *this);
                        return;
                    }
                    else if (cmd == "GUILD_INTEGRATIONS_UPDATE")
                    {
                        if (i_guild_integrations_update)
                            i_guild_integrations_update(result, _shard, *this);
                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_ADD")
                    {
                        _shard->counters.members++;

                        snowflake member_id = result["d"]["user"]["id"];
                        snowflake guild_id = result["d"]["guild_id"];
                       
                        auto _member = get_member_create(member_id);
                        auto _guild = get_guild(guild_id);
                       
                        _member->load(_guild, result["d"], _shard);

                        guild_member_add obj;
                        obj.bot = this;
                        obj._shard = _shard;

                        if (i_guild_member_add)
                            i_guild_member_add(obj);
                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_REMOVE")
                    {
                        _shard->counters.members--;

                        snowflake member_id = result["d"]["user"]["id"];
                        snowflake guild_id = result["d"]["guild_id"];
                        
                        auto _member = get_member(member_id);
                        auto _guild = get_guild(guild_id);
                       
                        _guild->remove_member(member_id);

                        guild_member_remove obj;
                        obj.bot = this;
                        obj._shard = _shard;

                        if (i_guild_member_remove)
                            i_guild_member_remove(obj);
                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_UPDATE")
                    {
                        snowflake member_id = result["d"]["user"]["id"];
                        snowflake guild_id = result["d"]["guild_id"];

                        auto _member = get_member_create(member_id).get();
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
                        obj.bot = this;
                        obj._shard = _shard;

                        if (i_guild_member_update)
                            i_guild_member_update(obj);
                        return;
                    }
                    else if (cmd == "GUILD_MEMBERS_CHUNK")
                    {
                        snowflake guild_id = result["d"]["guild_id"];
                        auto _guild = get_guild(guild_id);
                        if (_guild == nullptr)
                            return;
                        auto & members = result["d"]["members"];
                        //log->info("GUILD_MEMBERS_CHUNK: {1} {0}", guild_id, members.size());
                        if (members.size())
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

                        if (i_guild_member_chunk)
                            i_guild_member_chunk(obj);
                        return;
                    }
                    else if (cmd == "GUILD_ROLE_CREATE")
                    {
                        snowflake guild_id = result["d"]["guild_id"];
                       
                        auto _guild = get_guild(guild_id);
                        _guild->load_role(result["d"]["role"]);

                        if (i_guild_role_create)
                            i_guild_role_create(result, _shard, *this);
                        return;
                    }
                    else if (cmd == "GUILD_ROLE_UPDATE")
                    {
                        snowflake guild_id = result["d"]["guild_id"];

                        auto _guild = get_guild(guild_id);
                        _guild->load_role(result["d"]["role"]);

                        if (i_guild_role_update)
                            i_guild_role_update(result, _shard, *this);
                        return;
                    }
                    else if (cmd == "GUILD_ROLE_DELETE")
                    {
                        snowflake guild_id = result["d"]["guild_id"];
                        snowflake role_id = result["d"]["role_id"];

                        auto _guild = get_guild(guild_id);
                        _guild->remove_role(role_id);

                        if (i_guild_role_delete)
                            i_guild_role_delete(result, _shard, *this);
                        return;
                    }
                    else if (cmd == "VOICE_SERVER_UPDATE")
                    {
                        if (i_voice_server_update)
                            i_voice_server_update(result, _shard, *this);
                        return;
                    }
                }
            }
            if (result["op"] == 9)
            {
                _shard->sequence = 0;
                log->error("Shard#{} : Unable to resume or invalid connection. Starting new", _shard->shardid);
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
                            { "presence",{ { "game",{ { "name", self_presence },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } }
                        }
                    }
                };
                log->trace("Shard#{}: {}", _shard->shardid, obj.dump());
                if (_shard->connection_state == Connecting && _shard->connection != nullptr)
                {
                    _shard->connection->send(obj.dump(), websocketpp::frame::opcode::text);
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
                int heartbeat = result["d"]["heartbeat_interval"];
                _shard->keepalivetimer = websocket_o.set_timer(heartbeat, std::bind(&aegis::keepAlive, this, std::placeholders::_1, heartbeat, _shard));
                _shard->heartbeattime = heartbeat;
            }
            if (result["op"] == 11)
            {
                //heartbeat ACK
                _shard->heartbeat_ack = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
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

inline void aegis::onConnect(websocketpp::connection_hdl hdl, shard * _shard)
{
    log->info("Connection established");
    _shard->connection_state = Connecting;

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
                        { "presence",{ { "game",{ { "name", self_presence },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } }
                    }
                }
            };
        }
        else
        {
            log->info("Attemping RESUME with id : {}", _shard->session_id);
            obj = {
                { "op", 6 },
                {
                    "d",
                    {
                        { "token", token },
                        { "session_id", _shard->session_id },
                        { "seq", _shard->sequence }
                    }
                }
            };
        }
        log->trace("Shard#{}: {}", _shard->shardid, obj.dump());
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
                    { "presence",{ { "game",{ { "name", self_presence },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } }
                }
            }
        };
        log->trace("Shard#{}: {}", _shard->shardid, obj.dump());
        if (_shard->connection)
            _shard->connection->send(obj.dump(), websocketpp::frame::opcode::text);
    }
}

inline void aegis::onClose(websocketpp::connection_hdl hdl, shard * _shard)
{
    log->info("Connection closed");
    if (status == Shutdown)
        return;
    _shard->connection_state = Reconnecting;
    _shard->do_reset();
    _shard->reconnect_timer = websocket_o.set_timer(10000, [_shard, this](const asio::error_code & ec)
    {
        if (ec == asio::error::operation_aborted)
            return;
        _shard->connection_state = Connecting;
        asio::error_code wsec;
        _shard->connection = websocket_o.get_connection(gateway_url, wsec);
        setup_callbacks(_shard);
        websocket_o.connect(_shard->connection);

    });
}

inline void aegis::rest_thread()
{
    using namespace std::chrono_literals;
    while (status != Shutdown)
    {
        try
        {
            ratelimit_o.process_queue();


            std::this_thread::sleep_for(5ms);
        }
        catch (std::exception & e)
        {
            log->error("rest_thread() error : {}", e.what());
        }
        catch (...)
        {
            log->error("rest_thread() error : unknown");
        }
    }
}


}
