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
#include "structs.hpp"
#include "utility.hpp"

#include <cstdio>
#include <iostream>
#include <streambuf>
#include <sstream>
#include <functional>
#include <memory>
#include <optional>
#include <set>

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <spdlog/spdlog.h>
#include <websocketpp/common/random.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/connection_hdl.hpp>
#include <websocketpp/roles/client_endpoint.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <json.hpp>
#include <zstr.hpp>

namespace aegis
{

using namespace std::chrono;
namespace spd = spdlog;
using json = nlohmann::json;
using namespace std::literals;

inline void Aegis::processReady(json & d, client & shard)
{
    shard.m_sessionId = d["session_id"].get<std::string>();
    if (m_isuserset)
        return;
    json & userdata = d["user"];
    m_discriminator = std::stoi(userdata["discriminator"].get<std::string>());
    m_snowflake = std::stoull(userdata["id"].get<std::string>());
    m_username = userdata["username"].get<std::string>();
    m_mfa_enabled = userdata["mfa_enabled"];
    if (m_mention.size() == 0)
    {
        std::stringstream ss;
        ss << "<@" << m_snowflake << ">";
        m_mention = ss.str();
    }
    m_isuserset = true;
}

inline void Aegis::keepAlive(const asio::error_code & ec, const int ms, client & shard)
{
    if (ec != asio::error::operation_aborted)
    {
        try
        {
            if (shard.m_heartbeatack != 0 && shard.m_lastheartbeat > shard.m_heartbeatack)
            {
                m_log->error("Heartbeat ack not received. Reconnecting.");
                m_websocket.close(shard.m_connection, 1001, "");
                shard.m_state = RECONNECTING;
                shard.do_reset();
                shard.m_reconnect_timer = m_websocket.set_timer(10000, [&shard, this](const asio::error_code & ec)
                {
                    if (ec == asio::error::operation_aborted)
                        return;
                    shard.m_state = CONNECTING;
                    asio::error_code wsec;
                    shard.m_connection = m_websocket.get_connection(m_gatewayurl, wsec);
                    setup_callbacks(shard);
                    m_websocket.connect(shard.m_connection);
                });

                return;
            }


            json obj;
            obj["d"] = shard.m_sequence;
            obj["op"] = 1;

            m_websocket.send(shard.m_connection, obj.dump(), websocketpp::frame::opcode::text);
            shard.m_lastheartbeat = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            shard.m_keepalivetimer = m_websocket.set_timer(ms, std::bind(&Aegis::keepAlive, this, std::placeholders::_1, ms, shard));
        }
        catch (websocketpp::exception & e)
        {
            m_log->error("Websocket exception : {0}", e.what());
        }
    }
}

inline std::optional<rest_reply> Aegis::get(std::string_view path)
{
    return call(path, "", "GET");
}

inline std::optional<rest_reply> Aegis::get(std::string_view path, std::string_view content)
{
    return call(path, content, "GET");
}

std::optional<aegis::rest_reply> Aegis::post(std::string_view path, std::string_view content)
{
    return call(path, content, "POST");
}

inline std::optional<rest_reply> Aegis::call(std::string_view path, std::string_view content, std::string_view method)
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
        if constexpr (!check_setting<settings>::selfbot::test())
            request_stream << "Authorization: Bot " << m_token << "\r\n";
        else
            request_stream << "Authorization: " << m_token << "\r\n";

        request_stream << "User-Agent: DiscordBot (https://github.com/zeroxs/aegis.cpp 0.1.0)\r\n";
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

        m_log->info("status: {} limit: {} remaining: {} reset: {}", hresponse.get_status_code(), limit, remaining, reset);

        bool global = !(hresponse.get_header("X-RateLimit-Global").size() == 0);

        if (error != asio::error::eof && ERR_GET_REASON(error.value()) != SSL_R_SHORT_READ)
            throw asio::system_error(error);
        return { { hresponse.get_status_code(), global, limit, remaining, reset, retry, hresponse.get_body().size() > 0 ? hresponse.get_body() : "" } };
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }
    return {};
}

inline void Aegis::onMessage(websocketpp::connection_hdl hdl, message_ptr msg, client & shard)
{
    json result;
    std::string payload = msg->get_payload();

    try
    {
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
        
        result = json::parse(payload);

        if (!result["s"].is_null())
            shard.m_sequence = result["s"].get<uint64_t>();

        if (!result.is_null())
        {
            if (!result["t"].is_null())
            {
                if (result["t"] == "PRESENCE_UPDATE")
                {
                    return;
                }

                const std::string & cmd = result["t"].get<std::string>();

                if (cmd == "TYPING_START")
                {
                    if (i_typing_start)
                        if (!i_typing_start(result, shard, *this))
                            return;
                    return;
                }
                if (cmd == "MESSAGE_CREATE")
                {
                    if (i_message_create)
                        if (!i_message_create(result, shard, *this))
                            return;
                    return;
                }
                if (cmd == "MESSAGE_UPDATE")
                {
                    if (i_message_update)
                        if (!i_message_update(result, shard, *this))
                            return;
                    return;
                }
                else if (cmd == "GUILD_CREATE")
                {
                    snowflake guild_id = std::stoll(result["d"]["id"].get<std::string>());

                    m_guilds.try_emplace(guild_id, new guild(shard, guild_id, ratelimit().get(rest_limits::bucket_type::GUILD)));


                    if (result["d"].count("channels"))
                    {
                        json channels = result["d"]["channels"];

                        for (auto & channel_r : channels)
                        {
                            snowflake channel_id = std::stoll(channel_r["id"].get<std::string>());
                            m_channels.try_emplace(channel_id, new channel(channel_id, guild_id, ratelimit().get(rest_limits::bucket_type::CHANNEL), ratelimit().get(rest_limits::bucket_type::EMOJI)));
                        }
                    }


                    if (result["d"].count("members"))
                    {
                        json members = result["d"]["members"];

                        for (auto & member_r : members)
                        {
                            snowflake member_id = std::stoll(member_r["user"]["id"].get<std::string>());
                            m_members.try_emplace(member_id, new member(member_id));
                        }
                    }

                    if (i_guild_create)
                        if (!i_guild_create(result, shard, *this))
                            return;
                    return;
                }
                else if (cmd == "GUILD_UPDATE")
                {
                    if (i_guild_update)
                        if (!i_guild_update(result, shard, *this))
                            return;
                    return;
                }
                else if (cmd == "GUILD_DELETE")
                {
                    if (i_guild_delete)
                        if (!i_guild_delete(result, shard, *this))
                            return;
                    if (result["d"]["unavailable"] == true)
                    {
                        //outage
                    }
                    else
                    {
                        uint64_t id = std::stoull(result["d"]["id"].get<std::string>());
                        //kicked or left
                    }
                    return;
                }
                else if (cmd == "MESSAGE_DELETE")
                {
                    if (i_message_delete)
                        if (!i_message_delete(result, shard, *this))
                            return;
                    return;
                }
                else if (cmd == "MESSAGE_DELETE_BULK")
                {
                    if (i_message_delete_bulk)
                        if (!i_message_delete_bulk(result, shard, *this))
                            return;
                    return;
                }
                else if (cmd == "USER_SETTINGS_UPDATE")
                {
                    if (i_user_settings_update)
                        if (!i_user_settings_update(result, shard, *this))
                            return;
                    return;
                }
                else if (cmd == "USER_UPDATE")
                {
                    if (i_user_update)
                        if (!i_user_update(result, shard, *this))
                            return;
                    return;
                }
                else if (cmd == "VOICE_STATE_UPDATE")
                {
                    if (i_voice_state_update)
                        if (!i_voice_state_update(result, shard, *this))
                            return;
                    return;
                }
                else if (cmd == "READY")
                {
                    if (i_ready)
                        i_ready(result, shard, *this);
                    processReady(result["d"], shard);
                    shard.m_state = ONLINE;
                    m_log->info("Shard#[{}] READY Processed", shard.m_shardid);
                    return;
                }
                else if (cmd == "CHANNEL_CREATE")
                {
                    if (i_channel_create)
                        if (!i_channel_create(result, shard, *this))
                            return;
                    return;
                }

                //////////////////////////////////////////////////////////////////////////
                //start of guild_snowflake events
                //everything beyond here has a guild_snowflake

                if (result["d"].count("guild_id"))
                {
                    if (cmd == "CHANNEL_CREATE")
                    {
                        if (i_channel_create)
                            if (!i_channel_create(result, shard, *this))
                                return;
                        return;
                    }
                    if (cmd == "CHANNEL_UPDATE")
                    {
                        if (i_channel_update)
                            if (!i_channel_update(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "CHANNEL_DELETE")
                    {
                        if (i_channel_delete)
                            if (!i_channel_delete(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_BAN_ADD")
                    {
                        if (i_guild_ban_add)
                            if (!i_guild_ban_add(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_BAN_REMOVE")
                    {
                        if (i_guild_ban_remove)
                            if (!i_guild_ban_remove(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_EMOJIS_UPDATE")
                    {
                        if (i_guild_emojis_update)
                            if (!i_guild_emojis_update(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_INTEGRATIONS_UPDATE")
                    {
                        if (i_guild_integrations_update)
                            if (!i_guild_integrations_update(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_ADD")
                    {
                        if (i_guild_member_add)
                            if (!i_guild_member_add(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_REMOVE")
                    {
                        if (i_guild_member_remove)
                            if (!i_guild_member_remove(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_UPDATE")
                    {
                        if (i_guild_member_update)
                            if (!i_guild_member_update(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_CHUNK")
                    {
                        if (i_guild_member_chunk)
                            if (!i_guild_member_chunk(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_ROLE_CREATE")
                    {
                        if (i_guild_role_create)
                            if (!i_guild_role_create(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_ROLE_UPDATE")
                    {
                        if (i_guild_role_update)
                            if (!i_guild_role_update(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "GUILD_ROLE_DELETE")
                    {
                        if (i_guild_role_delete)
                            if (!i_guild_role_delete(result, shard, *this))
                                return;
                        return;
                    }
                    else if (cmd == "VOICE_SERVER_UPDATE")
                    {
                        if (i_voice_server_update)
                            if (!i_voice_server_update(result, shard, *this))
                                return;
                        return;
                    }
                }
            }
            if (result["op"] == 9)
            {
                m_log->error("Unable to resume or invalid connection. Starting new");
                json obj = {
                    { "op", 2 },
                    {
                        "d",
                        {
                            { "token", m_token },
                            { "properties",
                            {
                                { "$os", utility::platform::get_platform() },
                                { "$browser", "aegis.cpp" },
                                { "$device", "aegis.cpp" }
                            }
                            },
                            { "shard", json::array({ shard.m_shardid, m_shardidmax }) },
                            { "compress", true },
                            { "large_threshhold", 250 },
                            { "presence",{ { "game",{ { "name", self_presence },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } }
                        }
                    }
                };
                m_websocket.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
            }
            if (result["op"] == 10)
            {
                int heartbeat = result["d"]["heartbeat_interval"];
                shard.m_keepalivetimer = m_websocket.set_timer(heartbeat, std::bind(&Aegis::keepAlive, this, std::placeholders::_1, heartbeat, shard));
            }
            if (result["op"] == 11)
            {
                //heartbeat ACK
                shard.m_heartbeatack = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            }
        }
    }
    catch (std::exception& e)
    {
        m_log->error("Failed to process object: {0}", e.what());
        m_log->error(msg->get_payload());
    }
    catch (...)
    {
        m_log->error("Failed to process object: Unknown error");
    }

}

inline void Aegis::onConnect(websocketpp::connection_hdl hdl, client & shard)
{
    m_log->info("Connection established");
    shard.m_state = CONNECTING;

    if constexpr(!check_setting<settings>::selfbot::test())
    {
        json obj;
        if (shard.m_sessionId.empty())
        {
            obj = {
                { "op", 2 },
                {
                    "d",
                    {
                        { "token", m_token },
                        { "properties",
                        {
                            { "$os", utility::platform::get_platform() },
                            { "$browser", "aegis.cpp" },
                            { "$device", "aegis.cpp" }
                        }
                        },
                        { "shard", json::array({ shard.m_shardid, m_shardidmax }) },
                        { "compress", true },
                        { "large_threshhold", 250 },
                        { "presence",{ { "game",{ { "name", self_presence },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } }
                    }
                }
            };
        }
        else
        {
            m_log->info("Attemping RESUME with id : {}", shard.m_sessionId);
            obj = {
                { "op", 6 },
                {
                    "d",
                    {
                        { "token", m_token },
                        { "session_id", shard.m_sessionId },
                        { "seq", shard.m_sequence }
                    }
                }
            };
        }
        m_log->info(obj.dump());
        m_websocket.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
    }
    else
    {
        json obj = {
            { "op", 2 },
            {
                "d",
                {
                    { "token", m_token },
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
        m_websocket.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
    }
}

inline void Aegis::onClose(websocketpp::connection_hdl hdl, client & shard)
{
    m_log->info("Connection closed");
    shard.m_state = RECONNECTING;
    shard.do_reset();
    shard.m_reconnect_timer = m_websocket.set_timer(10000, [&](const asio::error_code & ec)
    {
        if (ec == asio::error::operation_aborted)
            return;
        shard.m_state = CONNECTING;
        asio::error_code wsec;
        shard.m_connection = m_websocket.get_connection(m_gatewayurl, wsec);
        setup_callbacks(shard);
        m_websocket.connect(shard.m_connection);

    });
}

inline void Aegis::onFail(websocketpp::connection_hdl hdl, client & shard)
{
    m_log->info("Connection failed");
    shard.m_state = RECONNECTING;
    shard.do_reset();
    shard.m_reconnect_timer = m_websocket.set_timer(10000, [&](const asio::error_code & ec)
    {
        if (ec == asio::error::operation_aborted)
            return;
        shard.m_state = CONNECTING;
        asio::error_code wsec;
        shard.m_connection = m_websocket.get_connection(m_gatewayurl, wsec);
        setup_callbacks(shard);
        m_websocket.connect(shard.m_connection);
    });
}

inline void Aegis::rest_thread()
{
    using namespace std::chrono_literals;
    while (m_state != SHUTDOWN)
    {
        try
        {
            m_ratelimit.process_queue();


            std::this_thread::sleep_for(5ms);
        }
        catch (std::exception & e)
        {
            m_log->error("rest_thread() error : {}", e.what());
        }
        catch (...)
        {
            m_log->error("rest_thread() error : unknown");
        }
    }
}


}
