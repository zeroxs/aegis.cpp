//
// example.cpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "example.hpp"

namespace example_bot
{

void example::MessageCreate(message_create obj)
{
    auto rets = obj.msg.get_related_ids();

    const aegis::snowflake channel_id = std::get<0>(rets);
    const aegis::snowflake guild_id = std::get<1>(rets);
    const aegis::snowflake message_id = std::get<2>(rets);
    const aegis::snowflake member_id = std::get<3>(rets);

    if (obj.msg.is_bot() || !obj.has_member() || !obj.has_channel() || !obj.msg.has_guild())
        return;

    auto & _member = obj.get_member();
    auto & _channel = obj.get_channel();
    auto & _guild = _channel.get_guild();

    auto & username = _member.name;

    std::string content{ obj.msg.get_content() };

    core & bot = *obj.bot;

    // check if mentioning bot
    if (content.substr(0, obj.bot->mention.size()) == obj.bot->mention)
    {
        // remove mention
        content.erase(0, obj.bot->mention.size());
    }
    else
    {
        if (content.substr(0, prefix.size()) != prefix)
            return;

        // remove prefix
        content.erase(0, prefix.size());
    }

    try
    {
        auto toks = split(content, ' ');
        if (toks.empty())
            return;

        if (toks[0] == "exit")
        {
            if (_member.member_id != bot_owner_id)
            {
                _channel.create_message("No perms `exit`");
                return;
            }
            obj.bot->find_channel(channel_id)->create_message("exiting...");
            obj.bot->shutdown();
            return;
        }
        else if (toks[0] == "kick")
        {
            snowflake tar = get_snowflake(toks[1], _guild);
            if (tar == 0)
            {
                _channel.create_message(fmt::format("User not found: {}", toks[1]));
            }

            auto reply = _guild.remove_guild_member(tar).get();
            if (!reply)
            {
                if (reply.code() == error::no_permission)
                    _channel.create_message(fmt::format("Unable to kick: {}", tar));
            }
            else
            {
                auto tmem = obj.bot->find_member(tar);
                if (!tmem)
                    _channel.create_message(fmt::format("Kicked: {}", tar));
                else
                    _channel.create_message(fmt::format("Kicked: <@{}>", tar));
            }
        }
        else if (toks[0] == "shard")
        {
            _channel.create_message(fmt::format("I am shard#[{}]", obj._shard->get_id()));
            return;
        }
        else if (toks[0] == "shards")
        {
            std::string s = "```\n";

            std::vector<size_t> shard_guild_c(obj.bot->shards.size());
            std::vector<size_t> shard_channel_c(obj.bot->shards.size());
            std::vector<size_t> shard_member_c(obj.bot->shards.size());

            for (auto & v : obj.bot->guilds)
            {
                ++shard_guild_c[v.second->shard_id];
                shard_channel_c[v.second->shard_id] += v.second->channels.size();
                shard_member_c[v.second->shard_id] += v.second->members.size();
            }

            for (auto & shard : obj.bot->shards)
            {
                s += fmt::format("shard#[{:4}] tracking {:4} guilds {:5} channels {:7} members {:9} messages {:10} presence\n", shard->get_id(), shard_guild_c[shard->get_id()], shard_channel_c[shard->get_id()], shard_guild_c[shard->get_id()], shard->counters.messages, shard->counters.presence_changes);
            }
            s += "```";
            _channel.create_message(s);
        }
        else if (toks[0] == "info")
        {
            _channel.create_message_embed({}, make_info_obj(obj._shard, obj.bot));
        }
    }
    catch (std::out_of_range & e)
    {
        bot.log->error("std::out_of_range - {}", e.what());
    }
}

const json example::make_info_obj(aegis::shard * _shard, aegis::core * bot)
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
        for (auto & e : bot->message_count)
            eventsseen += e.second;

        for (auto & v : bot->members)
        {
            if (v.second->status == aegis::member::member_status::Online)
                member_online_count++;
            else if (v.second->status == aegis::member::member_status::Idle)
                member_idle_count++;
            else if (v.second->status == aegis::member::member_status::DoNotDisturb)
                member_dnd_count++;
        }

        for (auto & v : bot->channels)
        {
            if (v.second->get_type() == aegis::gateway::objects::channel_type::Text)
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
    std::string misc = fmt::format("I am shard {} of {} running on `{}` in `{}` mode", _shard->get_id() + 1, bot->shard_max_count, aegis::utility::platform::get_platform(), build_mode);

    fmt::MemoryWriter w;
    w << "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/Kv7aP5K)\n\nMemory usage: "
        << double(aegis::utility::getCurrentRSS()) / (1024 * 1024) << "MB\nMax Memory: "
        << double(aegis::utility::getPeakRSS()) / (1024 * 1024) << "MB";
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
        { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", fmt::format("Made in C++{} running {}", CXX_VERSION, AEGIS_VERSION_TEXT) } } }
    };
    return t;
}

}
