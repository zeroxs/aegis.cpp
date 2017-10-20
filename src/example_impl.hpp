//
// example_impl.hpp
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


namespace example_bot
{

inline bool example::TypingStart(typing_start obj)
{
    obj.bot->log->info("Typing start");
    return true;
}

inline bool example::MessageCreate(message_create obj)
{
//     if constexpr (check_setting<settings>::disable_cache::test())
//         return extremely_simplified_message_handler(msg);


    std::string username;
    auto _member = obj._member;
    if (_member != nullptr)
        username = obj._member->name;

    auto _channel = obj._channel;
    auto & _guild = _channel->get_guild();

    snowflake channel_id = _channel->channel_id;
    snowflake message_id = obj.msg.message_id;
    std::string content = obj.msg.content;

    auto toks = split(content, ' ');
    if (toks.size() == 0)
        return true;

    // basic stats of the example bot
    if (toks[0] == "?info")
    {
        uint64_t guild_count = obj.bot->guilds.size();
        uint64_t member_count = 0;
        uint64_t member_count_unique = obj.bot->members.size();
        uint64_t member_online_count = 0;
        uint64_t member_idle_count = 0;
        uint64_t member_dnd_count = 0;
        uint64_t channel_count = obj.bot->channels.size();
        uint64_t channel_text_count = 0;
        uint64_t channel_voice_count = 0;
        uint64_t member_count_active = 0;

        uint64_t eventsseen = 0;

        {
            for (auto & bot_ptr : obj.bot->shards)
                eventsseen += bot_ptr->sequence;

            for (auto &[k, v] : obj.bot->members)
            {
                if (v->status == member::member_status::Online)
                    member_online_count++;
                else if (v->status == member::member_status::Idle)
                    member_idle_count++;
                else if (v->status == member::member_status::DoNotDisturb)
                    member_dnd_count++;
            }

            for (auto & [k,v] : obj.bot->channels)
            {
                if (v->m_type == channel::ChannelType::Text)
                    channel_text_count++;
                else
                    channel_voice_count++;
            }

            member_count = obj.bot->get_member_count();
        }

        std::string members = fmt::format("{} seen\n{} unique\n{} online\n{} idle\n{} dnd", member_count, member_count_unique, member_online_count, member_idle_count, member_dnd_count);
        std::string channels = fmt::format("{} total\n{} text\n{} voice", channel_count, channel_text_count, channel_voice_count);
        std::string guilds = fmt::format("{}", guild_count);
        std::string events = fmt::format("{}", eventsseen);
#if defined(DEBUG) || defined(_DEBUG)
        std::string build_mode = "DEBUG";
#else
        std::string build_mode = "RELEASE";
#endif
        std::string misc = fmt::format("I am shard {} of {} running on `{}` in `{}` mode", obj._shard->shardid + 1, obj.bot->shard_max_count, utility::platform::get_platform(), build_mode);

        fmt::MemoryWriter w;
        w << "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)\n\nMemory usage: "
            << double(utility::getCurrentRSS()) / (1024 * 1024) << "MB\nMax Memory: "
            << double(utility::getPeakRSS()) / (1024 * 1024) << "MB";
        std::string stats = w.str();


        json t = {
            { "title", "AegisBot" },
            { "description", stats },
            { "color", rand() % 0xFFFFFF },
            { "fields",
            json::array(
        {
            { { "name", "Members" },{ "value", members },{ "inline", true } },
            { { "name", "Channels" },{ "value", channels },{ "inline", true } },
            { { "name", "Uptime" },{ "value", obj.bot->uptime() },{ "inline", true } },
            { { "name", "Guilds" },{ "value", guilds },{ "inline", true } },
            { { "name", "Events Seen" },{ "value", events },{ "inline", true } },
            { { "name", u8"\u200b" },{ "value", u8"\u200b" },{ "inline", true } },
            { { "name", "misc" },{ "value", misc },{ "inline", false } }
        }
                )
            },
            { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", "Made in c++ running aegis library" } } }
        };

        _channel->create_message_embed({}, t);
        return true;
    }
    else if (toks[0] == "?source")
    {
        json t = {
            { "title", "AegisBot" },
            { "description", "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)" },
            { "color", rand() % 0xFFFFFF }
        };

        _channel->create_message_embed({}, t);
    }
    else if (toks[0] == "?exit")
    {
        json t =
        {
            { "content", "exiting..." }
        };
        obj.bot->get_channel(channel_id)->create_message(t.dump());
        obj.bot->set_state(Shutdown);
        obj.bot->get_websocket().close(obj._shard->connection, 1001, "");
        obj.bot->stop_work();
        return true;
    }
    else if (toks[0] == "?test")
    {
        _channel->create_message("test reply");
        return true;
    }
    else if (toks[0] == "?shard")
    {
        _channel->create_message(fmt::format("I am shard#[{}]", obj._shard->shardid));
        return true;
    }
    else if (toks[0] == "?shards")
    {
        std::string s = "```\n";
        for (auto & shard : obj.bot->shards)
        {
            s += fmt::format("shard#{} tracking {:4} guilds {:4} channels {:4} members {:4} messages {:4} presence updates\n", obj._shard->shardid, obj._shard->counters.guilds, obj._shard->counters.channels, obj._shard->counters.members, obj._shard->counters.messages, obj._shard->counters.presence_changes);
        }
        s += "```";
        _channel->create_message(s);
    }
    else if (toks[0] == "?createchannel")
    {
        if (!_guild.create_text_channel(toks[1], 0, false, {}))
        {
            _channel->create_message("No perms CREATE_CHANNEL");
        }
    }
    else if (toks[0] == "?deletechannel")
    {
        if (_member->member_id == 171000788183678976)
        {
            if (!_channel->delete_channel())
            {
                _channel->create_message("No perms DELETE_CHANNEL");
            }
        }
    }
    else if (toks[0] == "?serverlist")
    {
        std::stringstream w;
        for (auto & g : obj.bot->guilds)
        {
            auto gld = g.second.get();
            w << "*" << gld->get_name() << "*  :  " << gld->guild_id << "\n";
        }


        json t = {
            { "title", "Server List" },
            { "description", w.str() },
            { "color", 10599460 }
        };
        _channel->create_message_embed("", t);
    }
    else if (toks[0] == "?mroles")
    {
        snowflake role_check;
        if (toks.size() == 1)
            role_check = _member->member_id;
        else
            role_check = std::stoll(toks[1]);

        std::stringstream w;
        auto gld = _guild.get_member(role_check)->get_guild_info(_guild.guild_id);
        for (auto & rl : gld->roles)
        {
            role & r = _guild.get_role(rl);
            w << "[" << r.role_id << "] : [A:" << r._permission.getAllowPerms() << "] : [D:" << r._permission.getDenyPerms() << "] : [" << r.name << "]\n";

        }
        _channel->create_message(w.str());
    }

    return true;
}

inline bool example::extremely_simplified_message_handler(json & msg, shard * shard, aegis & bot)
{
    json _member = msg["d"]["author"];

    snowflake guild_id = _member["id"];
    std::string username = _member["username"];

    snowflake channel_id = msg["d"]["channel_id"];
    snowflake message_id = msg["d"]["id"];
    std::string content = msg["d"]["content"];

    std::string avatar = _member["avatar"].is_string() ? _member["avatar"] : "";
    uint16_t discriminator = std::stoi(_member["discriminator"].get<std::string>());

    auto toks = split(content, ' ');
    if (toks.size() == 0)
        return true;

    if constexpr (check_setting<settings>::disable_cache::test())
    {
        if (toks[0] == "?info")
        {
            uint64_t eventsseen = 0;

            for (auto & bot_ptr : bot.shards)
                eventsseen += bot_ptr->sequence;

            std::string events = fmt::format("{}", eventsseen);
#if defined(DEBUG) || defined(_DEBUG)
            std::string build_mode = "DEBUG";
#else
            std::string build_mode = "RELEASE";
#endif
            std::string misc = fmt::format("I am shard {} of {} running on `{}` in `{}` mode", shard->shardid + 1, bot.shard_max_count, utility::platform::get_platform(), build_mode);

            fmt::MemoryWriter w;
            w << "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)\n\nMemory usage: "
                << double(utility::getCurrentRSS()) / (1024 * 1024) << "MB\nMax Memory: "
                << double(utility::getPeakRSS()) / (1024 * 1024) << "MB";
            std::string stats = w.str();


            json t = {
                { "title", "AegisBot" },
                { "description", stats },
                { "color", rand() % 0xFFFFFF },
                { "fields",
                json::array(
            {
                { { "name", "Uptime" },{ "value", bot.uptime() },{ "inline", true } },
                { { "name", "Events Seen" },{ "value", events },{ "inline", true } },
                { { "name", "misc" },{ "value", misc },{ "inline", false } }
            }
                    )
                },
                { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", "Made in c++ running aegis library" } } }
            };

            json obj;
            if (!content.empty())
                obj["content"] = "";
            obj["embed"] = t;

            //manual call with instant response
//             std::optional<rest_reply> reply = bot.call(fmt::format("/channels/{:d}/messages", channel_id), t.dump(), "POST");
//             if (reply.has_value() && reply->reply_code == 200)
//             {
//                 //success
//                 bot.log->info("?info responded in channel [{}] by user [{}]", channel_id, username);
//             }

            //ratelimit controlled call with callback response
            bot.ratelimit().get(rest_limits::bucket_type::CHANNEL).push(channel_id, fmt::format("/channels/{:d}/messages", channel_id), obj.dump(), "POST", [&bot, channel_id, username](rest_reply reply)
            {
                if (reply.reply_code == 200)
                {
                    //success
                    bot.log->info("?info responded in channel [{}] by user [{}]", channel_id, username);
                }
            });

        }
        //internal handling is disabled so return value is irrelevant
        return true;
    }
}

inline bool example::message_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::message_delete(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::message_delete_bulk(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_create(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_delete(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::user_settings_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::user_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::ready(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::resumed(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::channel_create(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::channel_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::channel_delete(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_ban_add(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_ban_remove(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_emojis_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_integrations_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_member_add(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_member_remove(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_member_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_member_chunk(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_role_create(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_role_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::guild_role_delete(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::presence_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::voice_state_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

inline bool example::voice_server_update(json & msg, shard & shard, aegis & bot)
{
    return true;
}

}
