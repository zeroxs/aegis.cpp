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
    //obj.bot->log->info("Typing start");
    return true;
}

inline bool example::MessageCreateDM(message_create obj)
{
    std::string username;

    if (obj._member && obj._member->member_id == obj.bot->self()->member_id)
        return true;

    auto _member = obj._member;
    if (_member != nullptr)
        username = obj._member->name;

    auto _channel = obj._channel;

    std::string content = obj.msg.content;

    auto toks = split(content, ' ');
    if (toks.size() == 0)
        return true;

    if (obj.bot->control_channel)
    {
        auto ctrl_channel = obj.bot->get_channel(obj.bot->control_channel);
        if (ctrl_channel)
            ctrl_channel->create_message(fmt::format("User: {}\nMessage: {}", username, content));
    }

    if (toks[0] == "help")
        _channel->create_message("This bot is in development. If you'd like more information on it, please join the discord server https://discord.gg/Kv7aP5K");

    if (toks[0] == obj.bot->mention)
    {
        //user is mentioning me
        if (toks.size() < 1)
            return true;
        if (toks[1] == "help")
        {
            _channel->create_message("This bot is in development. If you'd like more information on it, please join the discord server https://discord.gg/Kv7aP5K");
        }
        return true;
    }

    return true;
}

inline bool example::MessageCreate(message_create obj)
{
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

    if (toks[0] == obj.bot->mention)
    {
        //user is mentioning me
        if (toks.size() < 1)
            return true;
        if (toks[1] == "help")
        {
            _channel->create_message("This bot is in development. If you'd like more information on it, please join the discord server https://discord.gg/Kv7aP5K");
        }
        return true;
    }

    if (toks[0] == "?info")
    {
        _channel->create_message_embed({}, make_info_obj(obj._shard, obj.bot));
        return true;
    }
    else if (toks[0] == "?source")
    {
        json t = {
            { "title", "AegisBot" },
            { "description", "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/Kv7aP5K)" },
            { "color", rand() % 0xFFFFFF }
        };

        _channel->create_message_embed({}, t);
    }
    else if (toks[0] == "?exit")
    {
        if (_member->member_id != obj.bot->owner_id)
        {
            _channel->create_message("No perms `exit`");
            return true;
        }
        obj.bot->get_channel(channel_id)->create_message("exiting...");
        obj.bot->shutdown();
        return true;
    }
    else if (toks[0] == "?test")
    {
        _channel->create_message("test reply");
        return true;
    }
    else if (toks[0] == "?shard")
    {
        _channel->create_message(fmt::format("I am shard#[{}]", obj._shard->get_id()));
        return true;
    }
    else if (toks[0] == "?shards")
    {
        std::string s = "```\n";
        for (auto & shard : obj.bot->shards)
        {
            s += fmt::format("shard#{} tracking {:4} guilds {:4} channels {:4} members {:4} messages {:4} presence updates\n", obj._shard->get_id(), obj._shard->counters.guilds, obj._shard->counters.channels, obj._shard->counters.members, obj._shard->counters.messages, obj._shard->counters.presence_changes);
        }
        s += "```";
        _channel->create_message(s);
    }
    else if (toks[0] == "?createchannel")
    {
        if (!_guild.create_text_channel(toks[1], 0, false, {}))
        {
            _channel->create_message("No perms `CREATE_CHANNEL`");
        }
    }
    else if (toks[0] == "?deletechannel")
    {
        if (_member->member_id == obj.bot->owner_id)
        {
            if (!_channel->delete_channel())
            {
                _channel->create_message("No perms `DELETE_CHANNEL`");
            }
        }
    }
    else if (toks[0] == "?serverlist")
    {
        if (_member->member_id != obj.bot->owner_id)
        {
            _channel->create_message("No perms `serverlist`");
            return true;
        }
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

inline bool example::message_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::message_delete(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::message_delete_bulk(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_create(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_delete(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::user_settings_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::user_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::ready(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::resumed(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::channel_create(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::channel_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::channel_delete(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_ban_add(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_ban_remove(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_emojis_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_integrations_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_member_add(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_member_remove(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_member_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_member_chunk(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_role_create(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_role_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::guild_role_delete(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::presence_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::voice_state_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline bool example::voice_server_update(json & msg, shard & _shard, aegis & bot)
{
    return true;
}

inline json example::make_info_obj(shard * _shard, aegis * bot)
{
    uint64_t guild_count = bot->guilds.size();
    uint64_t member_count = 0;
    uint64_t member_count_unique = bot->members.size();
    uint64_t member_online_count = 0;
    uint64_t member_idle_count = 0;
    uint64_t member_dnd_count = 0;
    uint64_t channel_count = bot->channels.size();
    uint64_t channel_text_count = 0;
    uint64_t channel_voice_count = 0;
    uint64_t member_count_active = 0;

    uint64_t eventsseen = 0;

    {
        for (auto & bot_ptr : bot->shards)
            eventsseen += bot_ptr->get_sequence();

        for (auto &[k, v] : bot->members)
        {
            if (v->status == member::member_status::Online)
                member_online_count++;
            else if (v->status == member::member_status::Idle)
                member_idle_count++;
            else if (v->status == member::member_status::DoNotDisturb)
                member_dnd_count++;
        }

        for (auto &[k, v] : bot->channels)
        {
            if (v->type == channel_type::Text)
                channel_text_count++;
            else
                channel_voice_count++;
        }

        member_count = bot->get_member_count();
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
    std::string misc = fmt::format("I am shard {} of {} running on `{}` in `{}` mode", _shard->get_id() + 1, bot->shard_max_count, utility::platform::get_platform(), build_mode);

    fmt::MemoryWriter w;
    w << "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/Kv7aP5K)\n\nMemory usage: "
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
        { { "name", "Uptime" },{ "value", bot->uptime() },{ "inline", true } },
        { { "name", "Guilds" },{ "value", guilds },{ "inline", true } },
        { { "name", "Events Seen" },{ "value", events },{ "inline", true } },
        { { "name", u8"\u200b" },{ "value", u8"\u200b" },{ "inline", true } },
        { { "name", "misc" },{ "value", misc },{ "inline", false } }
    }
            )
        },
        { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", "Made in c++ running aegis library" } } }
    };
    return std::move(t);
}

}
