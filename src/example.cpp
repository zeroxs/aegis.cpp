//
// example.cpp
// ***********
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "example.hpp"

namespace example_bot
{

void example::MessageCreate(message_create obj)
{
    using aegis::rest::rest_reply;

    auto rets = obj.msg.get_related_ids();

    const aegis::snowflake channel_id = std::get<0>(rets);
    const aegis::snowflake guild_id = std::get<1>(rets);
    const aegis::snowflake message_id = std::get<2>(rets);
    const aegis::snowflake member_id = std::get<3>(rets);

    if (obj.msg.is_bot() || !obj.has_user() || !obj.msg.has_guild())
        return;

    auto & _member = obj.get_user();
    auto & _channel = obj.channel;
    auto & _guild = _channel.get_guild();

    const auto & username = _member.get_username();

    std::string content{ obj.msg.get_content() };

    // check if mentioning bot
    if (content.substr(0, bot.mention.size()) == bot.mention)
    {
        // remove mention
        content.erase(0, bot.mention.size());
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
            if (_member.get_id() != bot_owner_id)
            {
                _channel.create_message("No perms `exit`");
                return;
            }
            bot.find_channel(channel_id)->create_message("exiting...");
            bot.shutdown();
            return;
        }
        else if (toks[0] == "kick")
        {
            snowflake tar = get_snowflake(toks[1], _guild);
            if (tar == 0)
            {
                _channel.create_message(fmt::format("User not found: {}", toks[1]));
            }

            _guild.remove_guild_member(tar).then([&](rest_reply reply)
            {
                if (!reply)
                    _channel.create_message(fmt::format("Unable to kick: {}", tar));
                else
                {
                    auto tmem = bot.find_user(tar);
                    if (!tmem)
                        _channel.create_message(fmt::format("Kicked: {}", tar));
                    else
                        _channel.create_message(fmt::format("Kicked: <@{}>", tar));
                }
            });
        }
        else if (toks[0] == "shard")
        {
            _channel.create_message(fmt::format("I am shard#[{}]", obj.shard.get_id()));
            return;
        }
        /*else*/ if (toks[0] == "?stats")
        {
            _channel.create_message_embed("", make_info_obj(bot, &obj.shard));
        }
    }
    catch (std::out_of_range & e)
    {
        bot.log->error("std::out_of_range - {}", e.what());
    }
}

const json example::make_info_obj(aegis::core & bot, aegis::shards::shard * _shard)
{
    int64_t guild_count = bot.guilds.size();
    int64_t member_count_unique = bot.members.size();
    int64_t channel_count = bot.channels.size();

    int64_t eventsseen = 0;

    for (auto & e : bot.message_count)
        eventsseen += e.second;

    std::string members = fmt::format("{}", member_count_unique);
    std::string channels = fmt::format("{}", channel_count);
    std::string guilds = fmt::format("{}", guild_count);
    std::string events = fmt::format("{}", eventsseen);

    std::stringstream w;

#if defined(DEBUG) || defined(_DEBUG)
    std::string misc = fmt::format("I am shard # {} of {} running on `{}` in `DEBUG` mode", _shard->get_id() + 1, bot.shard_max_count, aegis::utility::platform::get_platform());

    w << "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/Kv7aP5K)\n\nMemory usage: "
        << double(aegis::utility::getCurrentRSS()) / (1024 * 1024) << "MB\nMax Memory: "
        << double(aegis::utility::getPeakRSS()) / (1024 * 1024) << "MB";
#else
    std::string misc = fmt::format("I am shard # {} of {} running on `{}`", _shard->get_id(), bot.shard_max_count, aegis::utility::platform::get_platform());

    w << "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/Kv7aP5K)\n\nMemory usage: "
        << double(aegis::utility::getCurrentRSS()) / (1024 * 1024) << "MB";
#endif

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
                { { "name", "Uptime" },{ "value", bot.uptime_str() },{ "inline", true } },
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
