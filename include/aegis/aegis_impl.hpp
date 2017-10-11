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

#include <cstdio>
#include <iostream>
#include <streambuf>
#include <sstream>
#include <functional>
#include <memory>
#include <unordered_map>
#include <optional>
#include <set>

#include "asio.hpp"
#include "asio/ssl.hpp"
#include "websocketpp/common/random.hpp"
#include "websocketpp/common/thread.hpp"
#include "websocketpp/common/connection_hdl.hpp"

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <json.hpp>

namespace aegis
{

namespace spd = spdlog;
using json = nlohmann::json;
using namespace std::literals;

inline void Aegis::set_option(AegisOption opt, bool val)
{

}

inline void Aegis::processReady(json & d)
{

}

inline void Aegis::keepAlive(const asio::error_code& error, const long ms)
{
    if (error != asio::error::operation_aborted)
    {
        try
        {
            json obj;
            obj["d"] = m_sequence;
            obj["op"] = 1;

            m_websocket.send(m_connection, obj.dump(), websocketpp::frame::opcode::text);
            m_websocket.set_timer(ms, std::bind(&Aegis::keepAlive, this, std::placeholders::_1, ms));
        }
        catch (websocketpp::exception & e)
        {
            m_log->error(fmt::format("Websocket exception : {0}", e.what()));
        }
    }
}

inline std::optional<std::string> Aegis::get(const std::string & path)
{
    return call(path, ""s, "GET");
}

inline std::optional<std::string> Aegis::get(const std::string & path, const std::string & content)
{
    return call(path, content, "GET");
}

inline std::optional<std::string> Aegis::post(const std::string & path, const std::string & content)
{
    return call(path, content, "POST");
}

inline std::optional<std::string> Aegis::call(const std::string & path, const std::string & content, const std::string method)
{
    try
    {
        asio::ip::tcp::resolver resolver(*m_io_service);
        asio::ip::tcp::resolver::query query("discordapp.com", "443");
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        asio::ssl::context ctx(asio::ssl::context::tlsv12);

        ctx.set_options(
            asio::ssl::context::default_workarounds
            | asio::ssl::context::no_sslv2
            | asio::ssl::context::no_sslv3);

        asio::ssl::stream<asio::ip::tcp::socket> socket(*m_io_service, ctx);
        SSL_set_tlsext_host_name(socket.native_handle(), "discordapp.com");

        asio::connect(socket.lowest_layer(), endpoint_iterator);

        asio::error_code handshake_ec;
        socket.handshake(asio::ssl::stream_base::client, handshake_ec);

        asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << method << " " << "/api/v6" << path << " HTTP/1.0\r\n";
        request_stream << "Host: discordapp.com\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Authorization: Bot " << m_token << "\r\n";
        request_stream << "User-Agent: DiscordBot (https://github.com/aegis-collaboratory/aegis.cpp 0.1)\r\n";
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

        if (error != asio::error::eof && ERR_GET_REASON(error.value()) != SSL_R_SHORT_READ)
            throw asio::system_error(error);
        return hresponse.get_body();
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

inline void Aegis::onMessage(websocketpp::connection_hdl hdl, message_ptr msg)
{
    json result;
    std::string payload = msg->get_payload();

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
                            json obj;
                            json t = {
                                { "title", "AegisBot" },
                                { "description", "Test data" },
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
                json obj = {
                    { "op", 2 },
                    {
                        "d",
                        {
                            { "token", m_token },
                            { "properties",
                            {
#ifdef WIN32
                                { "$os", "windows" },
#else
                                { "$os", "linux" },
#endif
                                { "$browser", "aegis" },
                                { "$device", "aegis" },
                                { "$referrer", "" },
                                { "$referring_domain", "" }
                            }
                            },
                            { "shard", json::array({ m_shardid, m_shardidmax }) },
                            { "compress", true },
                            { "large_threshhold", 250 },
                            { "presence",{ { "game",{ { "name", "@Aegis help" },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } }
                        }
                    }
                };
                m_websocket.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
            }
            if (result["op"] == 10)
            {
                long heartbeat = result["d"]["heartbeat_interval"];
                m_websocket.set_timer(heartbeat, std::bind(&Aegis::keepAlive, this, std::placeholders::_1, heartbeat));
            }
            if (result["op"] == 11)
            {
                //heartbeat ACK
            }
        }
    }
    catch (std::exception& e)
    {
        m_log->error(fmt::format("Failed to process object: {0}", e.what()));
        m_log->error(msg->get_payload());
    }
    catch (...)
    {
        m_log->error("Failed to process object: Unknown error");
    }

}

inline void Aegis::onConnect(websocketpp::connection_hdl hdl)
{
    m_log->info("Connection established.");

    json obj;
    obj = {
        { "op", 2 },
        {
            "d",
            {
                { "token", m_token },
                {
                    "properties",
                    {
                        { "$os", "linux" },
                        { "$browser", "aegis" },
                        { "$device", "aegis" }
                    }
                },
                { "shard", json::array({ 0, 1 }) },
                { "compress", false },
                { "large_threshhold", 250 },
                { "presence",{ { "game",{ { "name", "@Aegis help" },{ "type", 0 } } },{ "status", "online" },{ "since", nullptr },{ "afk", false } } }
            }
        }
    };
    std::cout << "Send: " << obj.dump() << '\n';
    m_websocket.send(hdl, obj.dump(), websocketpp::frame::opcode::text);
}

inline void Aegis::onClose(websocketpp::connection_hdl hdl)
{
    std::cout << "Connection closed.\n";
}


}
