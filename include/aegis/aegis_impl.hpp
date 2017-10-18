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

#include "aegis/config.hpp"


namespace aegis
{

using namespace std::chrono;
namespace spd = spdlog;
using json = nlohmann::json;
using namespace std::literals;

inline void aegis_core::processReady(json & d, aegis_shard & shard)
{
    shard.session_id = d["session_id"].get<std::string>();

    json guilds = d["guilds"];
    size_t connectguilds = guilds.size();

    if (self() == nullptr)
    {
        json & userdata = d["user"];
        discriminator = std::stoi(userdata["discriminator"].get<std::string>());
        member_id = userdata["id"];
        username = userdata["username"].get<std::string>();
        mfa_enabled = userdata["mfa_enabled"];
        if (m_mention.size() == 0)
        {
            std::stringstream ss;
            ss << "<@" << member_id << ">";
            m_mention = ss.str();
        }

        auto ptr = std::make_shared<aegis_member>(member_id);
        members.emplace(member_id, ptr);
        ptr->member_id = member_id;
        ptr->isbot = true;
        ptr->name = username;
        ptr->discriminator = discriminator;
        ptr->status = aegis_member::Online;
    }

    for (auto & guildobj : guilds)
    {
        snowflake id = guildobj["id"];

        bool unavailable = false;
        if (guildobj.count("unavailable"))
            unavailable = guildobj["unavailable"];

        aegis_guild & _guild = get_guild_create(id, shard);
        _guild.unavailable = unavailable;

        if (!unavailable)
            guild_create(_guild, guildobj, shard);
    }
}

inline void aegis_core::guild_create(aegis_guild & _guild, json & obj, aegis_shard & shard)
{
    //uint64_t application_id = obj->get("application_id").convert<uint64_t>();
    snowflake g_id = obj["id"];

    try
    {
        json voice_states;

        if (!obj["name"].is_null()) _guild.name = obj["name"].get<std::string>();
        if (!obj["icon"].is_null()) _guild.m_icon = obj["icon"].get<std::string>();
        if (!obj["splash"].is_null()) _guild.m_splash = obj["splash"].get<std::string>();
        _guild.m_owner_id = obj["owner_id"];
        _guild.m_region = obj["region"].get<std::string>();
        if (!obj["afk_channel_id"].is_null()) _guild.m_afk_channel_id = obj["afk_channel_id"];
        _guild.m_afk_timeout = obj["afk_timeout"];//in seconds
        if (!obj["embed_enabled"].is_null()) _guild.m_embed_enabled = obj["embed_enabled"].get<bool>();
        //_guild.m_embed_channel_id = obj->get("embed_channel_id").convert<uint64_t>();
        _guild.m_verification_level = obj["verification_level"];
        _guild.m_default_message_notifications = obj["default_message_notifications"];
        _guild.m_mfa_level = obj["mfa_level"];
        if (!obj["joined_at"].is_null()) _guild.joined_at = obj["joined_at"].get<std::string>();
        if (!obj["large"].is_null()) _guild.m_large = obj["large"];
        if (!obj["unavailable"].is_null()) _guild.unavailable = obj["unavailable"].get<bool>();
        if (!obj["member_count"].is_null()) _guild.m_member_count = obj["member_count"];
        if (!obj["voice_states"].is_null()) voice_states = obj["voice_states"];

        if (obj.count("members"))
        {
            json members = obj["members"];

            for (auto & member : members)
            {
                member_create(_guild, member, shard);
                //++counters.members;
            }
        }

        if (obj.count("channels"))
        {
            json channels = obj["channels"];

            for (auto & channel : channels)
            {
                channel_guild_create(_guild, channel, shard);
                //++counters.channels;
            }
        }

        if (obj.count("roles"))
        {
            json roles = obj["roles"];

            for (auto & role : roles)
            {
                load_role(role, _guild);
            }
        }

        if (obj.count("presences"))
        {
            json presences = obj["presences"];

            for (auto & presence : presences)
            {
                load_presence(presence, _guild);
            }
        }

        if (obj.count("emojis"))
        {
            json emojis = obj["emojis"];

            for (auto & emoji : emojis)
            {
                //loadEmoji(emoji, _guild);
            }
        }

        if (obj.count("features"))
        {
            json features = obj["features"];

        }

        //if (shardloaded)
            //_guild.UpdatePermissions();

        //bot commands

        //for (auto cmd : defaultcmdlist)
            //_guild.cmdlist[cmd.first] = ABCallbackPair(ABCallbackOptions(), cmd.second);




        /*
        for (auto & feature : features)
        {
        //??
        }

        for (auto & voicestate : voice_states)
        {
        //no voice yet
        }*/


        log->info("Shard#{} : Guild created: {} [{}]", shard.shardid, g_id, _guild.name);

    }
    catch (std::exception&e)
    {
        log->error("Shard#{} : Error processing guild[{}] {}", shard.shardid, g_id, (std::string)e.what());
    }
}

inline void aegis_core::channel_guild_create(aegis_guild & _guild, json & obj, aegis_shard & shard)
{
    snowflake channel_id = obj["id"];
    aegis_channel & _channel = get_guild_channel_create(channel_id, _guild.guild_id, shard);
    _channel.guild_id = _guild.guild_id;

    try
    {
        //log->debug("Shard#{} : Channel[{}] created for guild[{}]", shard.m_shardid, channel_id, _channel.m_guild_id);
        if (!obj["name"].is_null()) _channel.name = obj["name"].get<std::string>();
        _channel.m_position = obj["position"];
        _channel.m_type = static_cast<aegis_channel::ChannelType>(obj["type"].get<int>());// 0 = text, 2 = voice

        //voice channels
        if (!obj["bitrate"].is_null())
        {
            _channel.m_bitrate = obj["bitrate"];
            _channel.m_user_limit = obj["user_limit"];
        }
        else
        {
            //not a voice channel, so has a topic field and last message id
            if (!obj["topic"].is_null()) _channel.m_topic = obj["topic"].get<std::string>();
            if (!obj["last_message_id"].is_null()) _channel.m_last_message_id = obj["last_message_id"];
        }


        json permission_overwrites = obj["permission_overwrites"];
        for (auto & permission : permission_overwrites)
        {
            uint32_t allow = permission["allow"];
            uint32_t deny = permission["deny"];
            snowflake p_id = permission["id"];
            std::string p_type = permission["type"];

            _channel.m_overrides[p_id].allow = allow;
            _channel.m_overrides[p_id].deny = deny;
            _channel.m_overrides[p_id].id = p_id;
            if (p_type == "role")
                _channel.m_overrides[p_id].type = perm_overwrite::Role;
            else
                _channel.m_overrides[p_id].type = perm_overwrite::User;
        }

        //_channel.update_permission_cache();
    }
    catch (std::exception&e)
    {
        log->error("Shard#{} : Error processing channel[{}] of guild[{}] {}", shard.shardid, channel_id, _channel.guild_id, e.what());
    }
}

inline void aegis_core::channel_create(json & obj, aegis_shard & shard)
{
    snowflake channel_id = obj["id"];
    aegis_channel & _channel = get_channel_create(channel_id);

    try
    {
        //log->debug("Shard#{} : Channel[{}] created for DirectMessage", shard.m_shardid, channel_id);
        if (!obj["name"].is_null()) _channel.name = obj["name"].get<std::string>();
        _channel.m_position = obj["position"];
        _channel.m_type = static_cast<aegis_channel::ChannelType>(obj["type"].get<int>());// 0 = text, 2 = voice

        //not a voice channel, so has a topic field and last message id
        if (!obj["topic"].is_null()) _channel.m_topic = obj["topic"].get<std::string>();
        if (!obj["last_message_id"].is_null()) _channel.m_last_message_id = obj["last_message_id"];

        //owner_id DirectMessage creator group DirectMessage
        //application_id DirectMessage creator if bot group DirectMessage
        //recipients user objects
    }
    catch (std::exception&e)
    {
        log->error("Shard#{} : Error processing channel[{}] of guild[{}] {}", shard.shardid, channel_id, _channel.guild_id, e.what());
    }
}

inline void aegis_core::member_create(aegis_guild & _guild, json & obj, aegis_shard & shard)
{
    json & user = obj["user"];
    snowflake member_id = user["id"];
    aegis_member & _member = get_member_create(member_id);

    try
    {
        //log->debug("Shard#{} : Member[{}] created for guild[{}]", shard.m_shardid, member_id, _guild.guild_id);
        
        auto g_member = members[member_id];

        if (!_guild.members.count(member_id))
        {
            //new member
            _guild.members.emplace(member_id, g_member);
        }
        //member update
        if (!user["username"].is_null()) g_member->name = user["username"].get<std::string>();
        if (!user["avatar"].is_null()) g_member->avatar = user["avatar"].get<std::string>();
        if (!user["discriminator"].is_null()) g_member->discriminator = std::stoi(user["discriminator"].get<std::string>());
        g_member->isbot = user["bot"].is_null() ? false : true;

        auto g_info = _member.get_guild_info(_guild.guild_id);
        if (g_info == nullptr)
        {
            g_info = _member.join(_guild.guild_id);
        }

        if (!obj["deaf"].is_null()) g_info->deaf = obj["deaf"];
        if (!obj["mute"].is_null()) g_info->mute = obj["mute"];

        if (!obj["joined_at"].is_null()) g_info->joined_at = obj["joined_at"].get<std::string>();

        if (!obj["roles"].is_null())
        {
            if (!_member.guilds.count(_guild.guild_id))
            {
                _member.guilds.emplace(_guild.guild_id, std::make_unique<aegis_member::guild_info>());
            }
            g_info->roles.clear();
            g_info->guild_id = _guild.guild_id;
            g_info->roles.push_back(_guild.guild_id);//default everyone role

            json roles = obj["roles"];
            for (auto & r : roles)
                g_info->roles.push_back(r);
        }

        if (!obj["nick"].is_null())
            g_info->nickname = obj["nick"].get<std::string>();
    }
    catch (std::exception&e)
    {
        log->error("Shard#{} : Error processing member[{}] of guild[{}] {}", shard.shardid, member_id, _guild.guild_id, e.what());
    }
}

inline void aegis_core::load_presence(json & obj, aegis_guild & _guild)
{
    json user = obj["user"];

    aegis_member::member_status status;
    if (obj["status"] == "idle")
        status = aegis_member::Idle;
    else if (obj["status"] == "dnd")
        status = aegis_member::DoNotDisturb;
    else if (obj["status"] == "online")
        status = aegis_member::Online;
    else if (obj["status"] == "offline")
        status = aegis_member::Offline;

    aegis_member & _member = get_member(user["id"]);
    _member.status = status;
    return;
}

inline void aegis_core::load_role(json & obj, aegis_guild & _guild)
{
    snowflake role_id = obj["id"];
    if (!_guild.m_roles.count(role_id))
    {
        auto r = std::make_unique<role>();
        auto & _role = *r.get();
        _guild.m_roles.emplace(role_id, std::move(r));
    }
    auto & _role = *_guild.m_roles[role_id].get();
    _role.role_id = role_id;
    _role.hoist = obj["hoist"];
    _role.managed = obj["managed"];
    _role.mentionable = obj["mentionable"];
    _role._permission = permission(obj["permissions"].get<uint64_t>());
    _role.position = obj["position"];
    if (!obj["name"].is_null()) _role.name = obj["name"].get<std::string>();
    _role.color = obj["color"];
    return;
}

inline void aegis_core::keepAlive(const asio::error_code & ec, const int ms, aegis_shard & shard)
{
    if (ec != asio::error::operation_aborted)
    {
        try
        {
            if (shard.heartbeat_ack != 0 && shard.lastheartbeat > shard.heartbeat_ack)
            {
                log->error("Heartbeat ack not received. Reconnecting.");
                websocket_o.close(shard.connection, 1001, "");
                shard.state_o = Reconnecting;
                shard.do_reset();
                shard.reconnect_timer = websocket_o.set_timer(10000, [&shard, this](const asio::error_code & ec)
                {
                    if (ec == asio::error::operation_aborted)
                        return;
                    shard.state_o = Connecting;
                    asio::error_code wsec;
                    shard.connection = websocket_o.get_connection(gateway_url, wsec);
                    setup_callbacks(shard);
                    websocket_o.connect(shard.connection);
                });

                return;
            }


            json obj;
            obj["d"] = shard.sequence;
            obj["op"] = 1;

            log->debug("Shard#{}: {}", shard.shardid, obj.dump());
            shard.connection->send(obj.dump(), websocketpp::frame::opcode::text);
            shard.lastheartbeat = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            shard.keepalivetimer = websocket_o.set_timer(ms, std::bind(&aegis_core::keepAlive, this, std::placeholders::_1, ms, shard));
        }
        catch (websocketpp::exception & e)
        {
            log->error("Websocket exception : {0}", e.what());
        }
    }
}

inline std::optional<rest_reply> aegis_core::get(std::string_view path)
{
    return call(path, "", "GET");
}

inline std::optional<rest_reply> aegis_core::get(std::string_view path, std::string_view content)
{
    return call(path, content, "GET");
}

std::optional<aegis::rest_reply> aegis_core::post(std::string_view path, std::string_view content)
{
    return call(path, content, "POST");
}

inline std::optional<rest_reply> aegis_core::call(std::string_view path, std::string_view content, std::string_view method)
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
            request_stream << "Authorization: Bot " << token << "\r\n";
        else
            request_stream << "Authorization: " << token << "\r\n";

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

        log->info("status: {} limit: {} remaining: {} reset: {}", hresponse.get_status_code(), limit, remaining, reset);

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

inline void aegis_core::onMessage(websocketpp::connection_hdl hdl, message_ptr msg, aegis_shard & shard)
{
    json result;
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

        result = json::parse(payload);
        if (log->level() > spdlog::level::level_enum::trace && !result.is_null()
            && (result["t"].is_null() || (result["t"] != "GUILD_CREATE" && result["t"] != "PRESENCE_UPDATE")))
            log->trace("Shard#{}: {}", shard.shardid, payload);


        //////////////////////////////////////////////////////////////////////////
        if constexpr (check_setting<settings>::debugmode::test())
        {
            int64_t t_time = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            shard.debug_messages[t_time] = payload;

            ///check old messages and remove

            for (auto &[k, v] : shard.debug_messages)
                if (k < t_time - 10000)
                    shard.debug_messages.erase(k);
        }
        //////////////////////////////////////////////////////////////////////////

        if (!result["s"].is_null())
            shard.sequence = result["s"].get<uint64_t>();

        if (!result.is_null())
        {
            if (!result["t"].is_null())
            {
                if (result["t"] == "PRESENCE_UPDATE")
                {
                    shard.counters.presence_changes++;

                    if (i_presence_update)
                        if (!i_presence_update(result, shard, *this))
                            return;

                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;

                    json user = result["d"]["user"];
                    snowflake guild_id = result["d"]["guild_id"];
                    snowflake member_id = user["id"];


                    aegis_member::member_status status;
                    if (result["d"]["status"] == "idle")
                        status = aegis_member::Idle;
                    else if (result["d"]["status"] == "dnd")
                        status = aegis_member::DoNotDisturb;
                    else if (result["d"]["status"] == "online")
                        status = aegis_member::Online;
                    else if (result["d"]["status"] == "offline")
                        status = aegis_member::Offline;

                    aegis_member & _member = get_member_create(member_id);
                    aegis_guild & _guild = get_guild(guild_id);
                    member_create(_guild, result["d"], shard);
                    _member.status = status;

                    return;
                }

                const std::string & cmd = result["t"].get<std::string>();

                if (cmd == "TYPING_START")
                {
                    if (i_typing_start)
                        if (!i_typing_start(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;
                    return;
                }
                if (cmd == "MESSAGE_CREATE")
                {
                    shard.counters.messages++;
                    
                    if (i_message_create)
                        if (!i_message_create(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;
                    return;
                }
                if (cmd == "MESSAGE_UPDATE")
                {
                    if (i_message_update)
                        if (!i_message_update(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;
                    return;
                }
                else if (cmd == "GUILD_CREATE")
                {
                    shard.counters.guilds++;
                    
                    if (i_guild_create)
                        if (!i_guild_create(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;

                    snowflake guild_id = result["d"]["id"];

                    aegis_guild & _guild = get_guild_create(guild_id, shard);
                    if (_guild.unavailable)
                    {
                        //outage
                        shard.counters.guilds_outage--;
                    }

                    guild_create(_guild, result["d"], shard);

                    return;
                }
                else if (cmd == "GUILD_UPDATE")
                {
                    if (i_guild_update)
                        if (!i_guild_update(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;

                    snowflake guild_id = result["d"]["id"];

                    aegis_guild & _guild = get_guild(guild_id);
                    guild_create(_guild, result["d"], shard);
                    return;
                }
                else if (cmd == "GUILD_DELETE")
                {
                    shard.counters.guilds--;
                    
                    if (i_guild_delete)
                        if (!i_guild_delete(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;

                    if (result["d"]["unavailable"] == true)
                    {
                        //outage
                        shard.counters.guilds_outage++;
                    }
                    else
                    {
                        snowflake id = result["d"]["id"];
                        //kicked or left - remove from memory
                        guilds.erase(id);
                    }
                    return;
                }
                else if (cmd == "MESSAGE_DELETE")
                {
                    if (i_message_delete)
                        if (!i_message_delete(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;
                    return;
                }
                else if (cmd == "MESSAGE_DELETE_BULK")
                {
                    if (i_message_delete_bulk)
                        if (!i_message_delete_bulk(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;
                    return;
                }
                else if (cmd == "USER_SETTINGS_UPDATE")
                {
                    if (i_user_settings_update)
                        if (!i_user_settings_update(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;
                    return;
                }
                else if (cmd == "USER_UPDATE")
                {
                    if (i_user_update)
                        if (!i_user_update(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;

                    snowflake member_id = result["d"]["user"]["id"];

                    aegis_member & _member = get_member_create(member_id);

                    json & user = result["d"]["user"];
                    if (!user["username"].is_null()) _member.name = user["username"].get<std::string>();
                    if (!user["avatar"].is_null()) _member.avatar = user["avatar"].get<std::string>();
                    if (!user["discriminator"].is_null()) _member.discriminator = std::stoi(user["discriminator"].get<std::string>());
                    if (!user["mfa_enabled"].is_null()) _member.mfa_enabled = user["mfa_enabled"];
                    if (!user["bot"].is_null()) _member.isbot = user["bot"];
                    //if (!user["verified"].is_null()) _member.m_verified = user["verified"];
                    //if (!user["email"].is_null()) _member.m_email = user["email"].get<std::string>();


                    return;
                }
                else if (cmd == "VOICE_STATE_UPDATE")
                {
                    if (i_voice_state_update)
                        if (!i_voice_state_update(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;
                    return;
                }
                else if (cmd == "READY")
                {
                    processReady(result["d"], shard);
                    shard.state_o = Online;
                    log->info("Shard#[{}] READY Processed", shard.shardid);

                    if (i_ready)
                        i_ready(result, shard, *this);

                    return;
                }
                else if (cmd == "CHANNEL_CREATE")
                {
                    if (i_channel_create)
                        if (!i_channel_create(result, shard, *this))
                            return;
                    if constexpr (check_setting<settings>::disable_cache::test())
                        return;

                    snowflake channel_id = result["d"]["id"];

                    if (!result["d"]["guild_id"].is_null())
                    {
                        //guild channel
                        snowflake guild_id = result["d"]["guild_id"];
                        aegis_channel & _channel = get_guild_channel_create(channel_id, guild_id, shard);
                        aegis_guild & _guild = get_guild(guild_id);
                        channel_guild_create(_guild, result["d"], shard);
                        shard.counters.channels++;
                    }
                    else
                    {
                        //dm
                        aegis_channel & _channel = get_channel_create(channel_id);
                        channel_create(result["d"], shard);
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
                            if (!i_channel_update(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;

                        snowflake channel_id = result["d"]["id"];

                        if (!result["d"]["guild_id"].is_null())
                        {
                            //guild channel
                            snowflake guild_id = result["d"]["guild_id"];
                            aegis_channel & _channel = get_guild_channel_create(channel_id, guild_id, shard);
                            aegis_guild & _guild = get_guild(guild_id);
                            channel_guild_create(_guild, result["d"], shard);
                        }
                        else
                        {
                            //dm
                            aegis_channel & _channel = get_channel_create(channel_id);
                            channel_create(result["d"], shard);
                        }
                        return;
                    }
                    else if (cmd == "CHANNEL_DELETE")
                    {
                        shard.counters.channels--;
                       
                        if (i_channel_delete)
                            if (!i_channel_delete(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;
                        return;
                    }
                    else if (cmd == "GUILD_BAN_ADD")
                    {
                        if (i_guild_ban_add)
                            if (!i_guild_ban_add(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;
                        return;
                    }
                    else if (cmd == "GUILD_BAN_REMOVE")
                    {
                        if (i_guild_ban_remove)
                            if (!i_guild_ban_remove(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;
                        return;
                    }
                    else if (cmd == "GUILD_EMOJIS_UPDATE")
                    {
                        if (i_guild_emojis_update)
                            if (!i_guild_emojis_update(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;
                        return;
                    }
                    else if (cmd == "GUILD_INTEGRATIONS_UPDATE")
                    {
                        if (i_guild_integrations_update)
                            if (!i_guild_integrations_update(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;
                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_ADD")
                    {
                        shard.counters.members++;
                        
                        if (i_guild_member_add)
                            if (!i_guild_member_add(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;

                        snowflake member_id = result["d"]["user"]["id"];
                        snowflake guild_id = result["d"]["guild_id"];
                       
                        aegis_member & _member = get_member_create(member_id);
                        aegis_guild & _guild = get_guild(guild_id);
                       
                        member_create(_guild, result["d"], shard);

                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_REMOVE")
                    {
                        shard.counters.members--;

                        if (i_guild_member_remove)
                            if (!i_guild_member_remove(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;

                        snowflake member_id = result["d"]["user"]["id"];
                        snowflake guild_id = result["d"]["guild_id"];
                        
                        aegis_member & _member = get_member(member_id);
                        aegis_guild & _guild = get_guild(guild_id);
                       
                        _guild.remove_member(result["d"]);

                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_UPDATE")
                    {
                        if (i_guild_member_update)
                            if (!i_guild_member_update(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;

                        snowflake member_id = result["d"]["user"]["id"];
                        snowflake guild_id = result["d"]["guild_id"];

                        aegis_member & _member = get_member(member_id);
                        aegis_guild & _guild = get_guild(guild_id);

                        member_create(_guild, result["d"], shard);
                       
                        return;
                    }
                    else if (cmd == "GUILD_MEMBER_CHUNK")
                    {
                        if (i_guild_member_chunk)
                            if (!i_guild_member_chunk(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;
                        return;
                    }
                    else if (cmd == "GUILD_ROLE_CREATE")
                    {
                        if (i_guild_role_create)
                            if (!i_guild_role_create(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;

                        snowflake guild_id = result["d"]["guild_id"];
                       
                        aegis_guild & _guild = get_guild(guild_id);
                        load_role(result["d"]["role"], _guild);
                       
                        return;
                    }
                    else if (cmd == "GUILD_ROLE_UPDATE")
                    {
                        if (i_guild_role_update)
                            if (!i_guild_role_update(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;

                        snowflake guild_id = result["d"]["guild_id"];

                        aegis_guild & _guild = get_guild(guild_id);
                        load_role(result["d"]["role"], _guild);

                        return;
                    }
                    else if (cmd == "GUILD_ROLE_DELETE")
                    {
                        if (i_guild_role_delete)
                            if (!i_guild_role_delete(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;

                        snowflake guild_id = result["d"]["guild_id"];
                        snowflake role_id = result["d"]["role_id"];

                        aegis_guild & _guild = get_guild(guild_id);
                        _guild.remove_role(role_id);

                        return;
                    }
                    else if (cmd == "VOICE_SERVER_UPDATE")
                    {
                        if (i_voice_server_update)
                            if (!i_voice_server_update(result, shard, *this))
                                return;
                        if constexpr (check_setting<settings>::disable_cache::test())
                            return;
                        return;
                    }
                }
            }
            if (result["op"] == 9)
            {
                log->error("Shard#{} : Unable to resume or invalid connection. Starting new", shard.shardid);
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
                            { "shard", json::array({ shard.shardid, shard_max_count }) },
                            { "compress", true },
                            { "large_threshhold", 250 },
                            { "presence",{ { "game",{ { "name", self_presence },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } }
                        }
                    }
                };
                log->debug("Shard#{}: {}", shard.shardid, obj.dump());
                shard.connection->send(obj.dump(), websocketpp::frame::opcode::text);
                debug_trace(shard);
            }
            if (result["op"] == 1)
            {
                //requested heartbeat
                json obj;
                obj["d"] = shard.sequence;
                obj["op"] = 1;

                shard.connection->send(obj.dump(), websocketpp::frame::opcode::text);
            }
            if (result["op"] == 10)
            {
                int heartbeat = result["d"]["heartbeat_interval"];
                shard.keepalivetimer = websocket_o.set_timer(heartbeat, std::bind(&aegis_core::keepAlive, this, std::placeholders::_1, heartbeat, shard));
            }
            if (result["op"] == 11)
            {
                //heartbeat ACK
                shard.heartbeat_ack = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            }
        }
    }
    catch (std::exception& e)
    {
        log->error("Failed to process object: {0}", e.what());
        log->error(msg->get_payload());

        debug_trace(shard);
    }
    catch (...)
    {
        log->error("Failed to process object: Unknown error");
        debug_trace(shard);
    }

}

inline void aegis_core::onConnect(websocketpp::connection_hdl hdl, aegis_shard & shard)
{
    log->info("Connection established");
    shard.state_o = Connecting;

    if constexpr(!check_setting<settings>::selfbot::test())
    {
        json obj;
        if (shard.session_id.empty())
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
                        { "shard", json::array({ shard.shardid, shard_max_count }) },
                        { "compress", true },
                        { "large_threshhold", 250 },
                        { "presence",{ { "game",{ { "name", self_presence },{ "type", 0 } } },{ "status", "online" },{ "since", 1 },{ "afk", false } } }
                    }
                }
            };
        }
        else
        {
            log->info("Attemping RESUME with id : {}", shard.session_id);
            obj = {
                { "op", 6 },
                {
                    "d",
                    {
                        { "token", token },
                        { "session_id", shard.session_id },
                        { "seq", shard.sequence }
                    }
                }
            };
        }
        log->debug("Shard#{}: {}", shard.shardid, obj.dump());
        shard.connection->send(obj.dump(), websocketpp::frame::opcode::text);
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
        log->debug("Shard#{}: {}", shard.shardid, obj.dump());
        shard.connection->send(obj.dump(), websocketpp::frame::opcode::text);
    }
}

inline void aegis_core::onClose(websocketpp::connection_hdl hdl, aegis_shard & shard)
{
    log->info("Connection closed");
    shard.state_o = Reconnecting;
    shard.do_reset();
    shard.reconnect_timer = websocket_o.set_timer(10000, [&](const asio::error_code & ec)
    {
        if (ec == asio::error::operation_aborted)
            return;
        shard.state_o = Connecting;
        asio::error_code wsec;
        shard.connection = websocket_o.get_connection(gateway_url, wsec);
        setup_callbacks(shard);
        websocket_o.connect(shard.connection);

    });
}

inline void aegis_core::rest_thread()
{
    using namespace std::chrono_literals;
    while (bot_state != Shutdown)
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
