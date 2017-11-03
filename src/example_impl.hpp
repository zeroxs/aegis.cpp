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

inline void example::TypingStart(typing_start obj)
{
    //obj.bot->log->info("Typing start");
    return;
}

inline void example::MessageCreateDM(message_create obj)
{
    std::string username;

    if (obj._member && obj._member->member_id == obj.bot->self()->member_id)
        return;

    auto _member = obj._member;
    if (_member != nullptr)
        username = obj._member->name;

    auto _channel = obj._channel;

    std::string content = obj.msg.content;

    auto toks = split(content, ' ');
    if (toks.size() == 0)
        return;

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
            return;
        if (toks[1] == "help")
        {
            _channel->create_message("This bot is in development. If you'd like more information on it, please join the discord server https://discord.gg/Kv7aP5K");
            return;
        }
        return;
    }

    return;
}

inline void example::MessageCreate(message_create obj)
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
        return;

    // basic stats of the example bot

    if (toks[0] == obj.bot->mention)
    {
        //user is mentioning me
        if (toks.size() < 1)
            return;
        if (toks[1] == "help")
        {
            _channel->create_message("This bot is in development. If you'd like more information on it, please join the discord server https://discord.gg/Kv7aP5K");
            return;
        }
        if (toks[1] == "info")
        {
            _channel->create_message_embed({}, make_info_obj(obj._shard, obj.bot));
            return;
        }
        return;
    }

    if (toks[0] == "?info")
    {
        _channel->create_message_embed({}, make_info_obj(obj._shard, obj.bot));
        return;
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
            return;
        }
        obj.bot->get_channel(channel_id)->create_message("exiting...");
        obj.bot->shutdown();
        return;
    }
    else if (toks[0] == "?test")
    {
        _channel->create_message("test reply");
        return;
    }
    else if (toks[0] == "?shard")
    {
        _channel->create_message(fmt::format("I am shard#[{}]", obj._shard->get_id()));
        return;
    }
    else if (toks[0] == "?shards")
    {
        std::string s = "```\n";
        for (auto & shard : obj.bot->shards)
        {
            s += fmt::format("shard#{} tracking {:4} guilds {:4} channels {:4} members {:4} messages {:4} presence updates\n", shard->get_id(), shard->counters.guilds, shard->counters.channels, shard->counters.members, shard->counters.messages, shard->counters.presence_changes);
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
            return;
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
    else if (toks[0] == "?roles")
    {
        std::stringstream w;
        w << "Server Roles:\n";
        for (auto & r : _guild.roles)
        {
            w << "[" << r.second->role_id << "] : [A:" << r.second->_permission.getAllowPerms() << "] : [D:" << r.second->_permission.getDenyPerms() << "] : [" << r.second->name << "]\n";
        }
        _channel->create_debug_message(w.str());
    }
    else if (toks[0] == "?mroles")
    {
        snowflake mem_id;
        if (toks.size() == 1)
            mem_id = obj._member->member_id;
        else
            mem_id = std::stoll(toks[1]);
        std::stringstream w;
        w << "Member Roles:\n";
        auto hasguildinfo = _guild.members[mem_id]->get_guild_info(_guild.guild_id);
        if (!hasguildinfo.has_value())
            return;
        auto gld = hasguildinfo.value();//->guilds[_guild.guild_id];
        for (auto & rl : gld->roles)
        {
            role & r = _guild.get_role(rl);
            w << "[" << r.role_id << "] : [A:" << r._permission.getAllowPerms() << "] : [D:" << r._permission.getDenyPerms() << "] : [" << r.name << "]\n";

        }
        _channel->create_debug_message(w.str());
    }
    else if (toks[0] == "?croles")
    {
        std::stringstream w;
        w << "Channel Roles:\n";
        for (auto & r : _channel->overrides)
        {
            auto & a = r.second;
            std::string name;
            if (a.type == overwrite_type::User)
                name = _guild.members[a.id]->getFullName();
            else
                name = _guild.roles[a.id]->name;
            w << "[" << ((a.type == overwrite_type::User) ? "user" : "role") << "] : [A:" << a.allow << "] : [D:" << a.deny << "] : [" << name << "]\n";
        }
        _channel->create_debug_message(w.str());
    }

    return;
}

inline void example::MessageUpdate(message_update obj)
{
    return;
}

inline void example::MessageDelete(message_delete obj)
{
    return;
}

// inline void example::MessageDeleteBulk(message_delete_bulk obj)
// {
//     return;
// }

inline void example::GuildCreate(guild_create)
{
    return;
}

inline void example::GuildUpdate(guild_update obj)
{
    return;
}

inline void example::GuildDelete(guild_delete obj)
{
    return;
}

inline void example::UserUpdate(user_update obj)
{
    return;
}

inline void example::Ready(ready obj)
{
    return;
}

inline void example::Resumed(resumed obj)
{
    return;
}

inline void example::ChannelCreate(channel_create obj)
{
    return;
}

inline void example::ChannelUpdate(channel_update obj)
{
    return;
}

inline void example::ChannelDelete(channel_delete obj)
{
    return;
}

inline void example::GuildBanAdd(guild_ban_add obj)
{
    return;
}

inline void example::GuildBanRemove(guild_ban_remove obj)
{
    return;
}

// inline void example::GuildEmojisUpdate(guild_emojis_update obj)
// {
//     return;
// }
// 
// inline void example::GuildIntegrationsUpdate(guild_integrations_update obj)
// {
//     return;
// }

inline void example::GuildMemberAdd(guild_member_add obj)
{
    return;
}

inline void example::GuildMemberRemove(guild_member_remove obj)
{
    return;
}

inline void example::GuildMemberUpdate(guild_member_update obj)
{
    return;
}

inline void example::GuildMemberChunk(guild_members_chunk obj)
{
    return;
}

// inline void example::GuildRoleCreate(guild_role_create obj)
// {
//     return;
// }
// 
// inline void example::GuildRoleUpdate(guild_role_update obj)
// {
//     return;
// }
// 
// inline void example::GuildRoleDelete(guild_role_delete obj)
// {
//     return;
// }

inline void example::PresenceUpdate(presence_update obj)
{
    return;
}

// inline void example::VoiceStateUpdate(voice_state_update obj)
// {
//     return;
// }
// 
// inline void example::VoiceServerUpdate(voice_server_update obj)
// {
//     return;
// }

inline json example::make_info_obj(shard * _shard, aegis * bot)
{
    int64_t guild_count = bot->guilds.size();
    int64_t member_count = 0;
    int64_t member_count_unique = bot->members.size();
    int64_t member_online_count = 0;
    int64_t member_idle_count = 0;
    int64_t member_dnd_count = 0;
    int64_t channel_count = bot->channels.size();
    int64_t channel_text_count = 0;
    int64_t channel_voice_count = 0;
    int64_t member_count_active = 0;

    int64_t eventsseen = 0;

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
