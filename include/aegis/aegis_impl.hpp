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
#include <unordered_map>
#include <optional>
#include <set>

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <spdlog/spdlog.h>
#include <websocketpp/common/random.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/connection_hdl.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <json.hpp>

namespace aegis
{

namespace spd = spdlog;
using json = nlohmann::json;
using namespace std::literals;

template<typename bottype>
inline void Aegis<bottype>::set_option(AegisOption opt, bool val)
{

}

template<typename bottype>
inline void Aegis<bottype>::processReady(json & d)
{
}

template<typename bottype>
inline void Aegis<bottype>::keepAlive(const asio::error_code& error, const int ms)
{
    if (error != asio::error::operation_aborted)
    {
        try
        {
            if (m_lastheartbeat > m_heartbeatack)
            {
                m_log->error("Heartbeat ack not received. Reconnecting.");
                m_websocket.close(m_connection, 1002, "");
                return;
            }


            json obj;
            obj["d"] = m_sequence;
            obj["op"] = 1;

            m_websocket.send(m_connection, obj.dump(), websocketpp::frame::opcode::text);
            m_lastheartbeat = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            m_keepalivetimer = m_websocket.set_timer(ms, std::bind(&Aegis::keepAlive, this, std::placeholders::_1, ms));
        }
        catch (websocketpp::exception & e)
        {
            m_log->error("Websocket exception : {0}", e.what());
        }
    }
}

template<typename bottype>
inline std::optional<rest_reply> Aegis<bottype>::get(std::string_view path)
{
    return call(path, "", "GET");
}

template<typename bottype>
inline std::optional<rest_reply> Aegis<bottype>::get(std::string_view path, std::string_view content)
{
    return call(path, content, "GET");
}

template<typename bottype>
std::optional<aegis::rest_reply> Aegis<bottype>::post(std::string_view path, std::string_view content)
{
    return call(path, content, "POST");
}

template<typename bottype>
inline std::optional<rest_reply> Aegis<bottype>::call(std::string_view path, std::string_view content, std::string_view method)
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
        if constexpr (std::is_same<bottype, basebot>::value)
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

template<typename Out>
void split(const std::string &s, char delim, Out result)
{
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        if (!item.empty())
            *(result++) = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

template<typename bottype>
inline void Aegis<bottype>::onMessage(const websocketpp::connection_hdl hdl, const message_ptr msg)
{
    json result;
    std::string_view payload = msg->get_payload();

    try
    {
        if (payload[0] == (char)0x78 && (payload[1] == (char)0x01 || payload[1] == (char)0x9C || payload[1] == (char)0xDA))
        {
            //TODO use boost.iostreams for zlib? this code is so easy to use...
//             boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
//             std::stringstream origin(payload);
//             in.push(boost::iostreams::zlib_decompressor());
//             in.push(origin);
//             payload.clear();
//             std::stringstream ss;
//             boost::iostreams::copy(in, ss);
//             payload = ss.str();
        }

        result = json::parse(payload);

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
                    //do we care? it'd easily be the most sent event
                    return;
                }
                if (cmd == "MESSAGE_CREATE")
                {
                    //userMessage(result["d"]);

                    json author = result["d"]["author"];

                    uint64_t userid = std::stoull(author["id"].get<std::string>());
                    std::string username = author["username"];

                    uint64_t channel_id = std::stoull(result["d"]["channel_id"].get<std::string>());
                    uint64_t id = std::stoull(result["d"]["id"].get<std::string>());
                    std::string content = result["d"]["content"];

                    std::string avatar = author["avatar"].is_string() ? author["avatar"] : "";
                    uint16_t discriminator = std::stoi(author["discriminator"].get<std::string>());

                    uint64_t msgid = std::stoull(result["d"]["id"].get<std::string>());

                    if (userid == 171000788183678976)
                    {
                        auto toks = split(content, ' ');
                        if (toks[0] == "?info")
                        {

                            fmt::MemoryWriter w;
                            w << "[Latest bot source](https://github.com/zeroxs/aegis)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)\n\nMemory usage: "
                                << double(utility::getCurrentRSS()) / (1024 * 1024) << "MB\nMax Memory: "
                                << double(utility::getPeakRSS()) / (1024 * 1024) << "MB";
                            std::string stats = w.str();


                            json obj;
                            json t = {
                                { "title", "AegisBot" },
                                { "description", stats },
                                { "color", rand() % 0xFFFFFF },
                                { "fields",
                                json::array(
                            {
                                { { "name", "Members" },{ "value", 0 },{ "inline", true } },
                                { { "name", "Channels" },{ "value", 0 },{ "inline", true } },
                                { { "name", "Uptime" },{ "value", 0 },{ "inline", true } },
                                { { "name", "Guilds" },{ "value", 0 },{ "inline", true } },
                                { { "name", "Events Seen" },{ "value", 0 },{ "inline", true } },
                                { { "name", u8"\u200b" },{ "value", u8"\u200b" },{ "inline", true } },
                                { { "name", "misc" },{ "value", 0 },{ "inline", false } }
                            }
                                    )
                                },
                                { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", "Made in c++ running aegis library" } } }
                            };
                            obj["embed"] = t;

                            post(fmt::format("/channels/{}/messages", channel_id), obj.dump());
                        }
                        else if (toks[0] == "?exit")
                        {
                            json obj =
                            {
                                { "content", "exiting..." }
                            };
                            post(fmt::format("/channels/{}/messages", channel_id), obj.dump());
                            m_state = SHUTDOWN;
                            m_websocket.close(m_connection, 1002, "");
                            m_work.reset();
                        }
                        else if (toks[0] == "?fake")
                        {
                            m_websocket.close(m_connection, 1002, "");
                            std::error_code ec;
                            connect(ec);
                        }
                        else if (toks[0] == "?test")
                        {
                            auto sendMessage = [&](/*remove this later*/const uint64_t channel_id, const std::string message)
                            {
                                auto & factory = m_ratelimit.get(rest_limits::bucket_type::CHANNEL);
                                //m_log->info("Posting:\n\n{}\n\nTo:\n\n{}", message, fmt::format("/channels/{}/messages", channel_id));
                                factory.push(channel_id, fmt::format("/channels/{}/messages", channel_id), json({ { "content", message } }).dump(), "POST");
                            };

                            auto userfunc = [&](const json & message)
                            {
                                /*channel->*/sendMessage(message["channel_id"], message["content"]);
                            };


                            rest_message<bottype> test;
                            test.endpoint = std::move(fmt::format("/channels/{}/messages", channel_id));
                            test.content = content;
                            test.method = "POST";
                            test.query = "";
                            test.cmd = "testcommand";
                            //test._channel = channel

                            json obj;
                            obj["content"] = "Adding your shit onto mine\n```" + content + "```";
                            obj["channel_id"] = channel_id;

                            userfunc(obj);
                        }
                        else if (toks[0] == "?process")
                        {
                            m_ratelimit.process_queue();
                        }
                    }
                }

                if (cmd == "MESSAGE_UPDATE")
                {
                    json message = result["d"];

                    if (message["embeds"].size() > 0)
                    {
                    }
                    else
                    {
                    }
                }
                else if (cmd == "GUILD_CREATE")
                {
                    //guildCreate(result["d"]);
                }
                else if (cmd == "GUILD_UPDATE")
                {
                    //guildCreate(result["d"]);
                }
                else if (cmd == "GUILD_DELETE")
                {
                    if (result["d"]["unavailable"] == true)
                    {
                        //outage
                    }
                    else
                    {
                        uint64_t id = std::stoull(result["d"]["id"].get<std::string>());
                        //kicked or left
                    }
                }
                else if (cmd == "MESSAGE_DELETE")
                {
                }
                else if (cmd == "MESSAGE_DELETE_BULK")
                {
                }
                else if (cmd == "USER_SETTINGS_UPDATE")
                {
                }
                else if (cmd == "USER_UPDATE")
                {
                }
                else if (cmd == "VOICE_STATE_UPDATE")
                {
                }
                else if (cmd == "READY")
                {
                    processReady(result["d"]);
                    m_state = ONLINE;
                    m_log->info("Bot online");
                    return;
                }
                else if (cmd == "CHANNEL_CREATE")
                {
                    return;
                }

                //////////////////////////////////////////////////////////////////////////
                //start of guild_id events
                //everything beyond here has a guild_id

                if (result["d"].count("guild_id"))
                {
                    if (cmd == "CHANNEL_CREATE")
                    {
                         return;
                    }
                    if (cmd == "CHANNEL_UPDATE")
                    {
                    }
                    else if (cmd == "CHANNEL_DELETE")
                    {
                    }
                    else if (cmd == "GUILD_BAN_ADD")
                    {
                    }
                    else if (cmd == "GUILD_BAN_REMOVE")
                    {
                    }
                    else if (cmd == "GUILD_EMOJIS_UPDATE")
                    {
                    }
                    else if (cmd == "GUILD_INTEGRATIONS_UPDATE")
                    {
                    }
                    else if (cmd == "GUILD_MEMBER_ADD")
                    {
                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_REMOVE")
                    {
                    }
                    else if (cmd == "GUILD_MEMBER_UPDATE")
                    {
                    }
                    else if (cmd == "GUILD_MEMBER_CHUNK")
                    {
                    }
                    else if (cmd == "GUILD_ROLE_CREATE")
                    {
                    }
                    else if (cmd == "GUILD_ROLE_UPDATE")
                    {
                    }
                    else if (cmd == "GUILD_ROLE_DELETE")
                    {
                    }
                    else if (cmd == "VOICE_SERVER_UPDATE")
                    {
                    }
                }
            }
            if (!result["s"].is_null())
                m_sequence = result["s"].get<uint64_t>();

            if (result["op"] == 9)
            {
                if constexpr (std::is_same<bottype, basebot>::value)
                {
                    json obj = {
                        { "op", 2 },
                        {
                            "d",
                            {
                                { "token", m_token },
                                { "properties",
                                {
                                    { "$os", utility::platform::m_platform.data() },
                                    { "$browser", "aegis" },
                                    { "$device", "aegis" }
                                }
                                },
                                { "shard", json::array({ m_shardid, m_shardidmax }) },
                                { "compress", false },
                                { "large_threshhold", 250 },
                                { "presence",{ { "game",{ { "name", "@Aegis help" },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } }
                            }
                        }
                    };
                    m_websocket.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
                }
            }
            if (result["op"] == 10)
            {
                int heartbeat = result["d"]["heartbeat_interval"];
                m_keepalivetimer = m_websocket.set_timer(heartbeat, std::bind(&Aegis::keepAlive, this, std::placeholders::_1, heartbeat));
            }
            if (result["op"] == 11)
            {
                //heartbeat ACK
                m_heartbeatack = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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

template<typename bottype>
inline void Aegis<bottype>::onConnect(const websocketpp::connection_hdl hdl)
{
    m_log->info("Connection established");
    m_state = CONNECTED;

    if constexpr (std::is_same<bottype, basebot>::value)
    {
        json obj = {
            { "op", 2 },
            {
                "d",
                {
                    { "token", m_token },
                    { "properties",
                    {
                        { "$os", utility::platform::m_platform.data() },
                        { "$browser", "aegis" },
                        { "$device", "aegis" }
                    }
                    },
                    { "shard", json::array({ m_shardid, m_shardidmax }) },
                    { "compress", false },
                    { "large_threshhold", 250 },
                    { "presence",{ { "game",{ { "name", "@Aegis help" },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } }
                }
            }
        };
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
                        { "$os", utility::platform::m_platform.data() },
                        { "$browser", "aegis" },
                        { "$device", "aegis" }
                    }
                    },
                    { "compress", false },
                    { "large_threshhold", 250 }
                }
            }
        };
        m_websocket.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
    }
}

template<typename bottype>
inline void Aegis<bottype>::onClose(const websocketpp::connection_hdl hdl)
{
    m_log->info("Connection closed");
    m_keepalivetimer->cancel();
    m_state = RECONNECTING;
}

template<typename bottype>
inline void Aegis<bottype>::rest_thread()
{
    while (m_state != SHUTDOWN)
    {
        m_ratelimit.process_queue();


        std::this_thread::yield();
    }
}


}
