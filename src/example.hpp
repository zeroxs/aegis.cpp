//
// example.hpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include <aegis.hpp>
#include <string>
#include <stdint.h>
#include <nlohmann/json.hpp>

namespace example_bot
{

using json = nlohmann::json;
using namespace aegis;
using namespace aegis::gateway::events;

class example
{
public:
    example() = default;
    ~example() = default;

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

    // Messages you want to process
    void inject(core & bot)
    {
        bot.set_on_message_create(std::bind(&example::MessageCreate, this, std::placeholders::_1));
    }

    snowflake get_snowflake(const std::string name, aegis::guild & _guild) const noexcept
    {
        if (name.empty())
            return { 0 };
        try
        {
            if (name[0] == '<')
            {
                //mention param

                std::string::size_type pos = name.find_first_of('>');
                if (pos == std::string::npos)
                    return { 0 };
                if (name[2] == '!')//mobile mention. strip <@!
                    return std::stoull(std::string{ name.substr(3, pos - 1) });
                else  if (name[2] == '&')//role mention. strip <@&
                    return std::stoull(std::string{ name.substr(3, pos - 1) });
                else  if (name[1] == '#')//channel mention. strip <#
                    return std::stoull(std::string{ name.substr(2, pos - 1) });
                else//regular mention. strip <@
                    return std::stoull(std::string{ name.substr(2, pos - 1) });
            }
            else if (std::isdigit(name[0]))
            {
                //snowflake param
                return std::stoull(std::string{ name });
            }
            else
            {
                //most likely username#discriminator param
                std::string::size_type n = name.find('#');
                if (n != std::string::npos)
                {
                    //found # separator
                    for (auto & m : _guild.get_members())
                        if (m.second->get_full_name() == name)
                            return { m.second->get_id() };
                    return { 0 };
                }
                return { 0 };//# not found. unknown parameter. unicode may trigger this.
            }
        }
        catch (std::invalid_argument &)
        {
            return { 0 };
        }
    }

    const snowflake bot_owner_id = 171000788183678976LL;

    std::string prefix = "?";

    // All the hooks in the websocket stream
    void TypingStart(typing_start obj);

    void MessageCreate(message_create msg);

    void MessageCreateDM(message_create msg);

    void MessageUpdate(message_update obj);

    void MessageDelete(message_delete obj);

    void MessageDeleteBulk(message_delete_bulk obj);

    void GuildCreate(guild_create);

    void GuildUpdate(guild_update obj);

    void GuildDelete(guild_delete obj);

    void UserUpdate(user_update obj);

    void Ready(ready obj);

    void Resumed(resumed obj);

    void ChannelCreate(channel_create obj);

    void ChannelUpdate(channel_update obj);

    void ChannelDelete(channel_delete obj);

    void GuildBanAdd(guild_ban_add obj);

    void GuildBanRemove(guild_ban_remove obj);

    void GuildEmojisUpdate(guild_emojis_update obj);

    void GuildIntegrationsUpdate(guild_integrations_update obj);

    void GuildMemberAdd(guild_member_add obj);

    void GuildMemberRemove(guild_member_remove obj);

    void GuildMemberUpdate(guild_member_update obj);

    void GuildMemberChunk(guild_members_chunk obj);

    void GuildRoleCreate(guild_role_create obj);

    void GuildRoleUpdate(guild_role_update obj);

    void GuildRoleDelete(guild_role_delete obj);

    void PresenceUpdate(presence_update obj);

    void VoiceStateUpdate(voice_state_update obj);

    void VoiceServerUpdate(voice_server_update obj);

    const json make_info_obj(shard * _shard, core * bot);
};

}
