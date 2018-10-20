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

    const auto & username = _member.get_username();

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
            if (_member.get_id() != bot_owner_id)
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
    }
    catch (std::out_of_range & e)
    {
        bot.log->error("std::out_of_range - {}", e.what());
    }
}

}
