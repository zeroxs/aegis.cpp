//
// example.hpp
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
#include <aegis/config.hpp>
#include <aegis.hpp>
#include <string>
#include <stdint.h>
#include <nlohmann/json.hpp>

namespace example_bot
{

using aegiscpp::shard;
using json = nlohmann::json;
using namespace std::placeholders;
using namespace aegiscpp;

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
    void inject(aegis & bot)
    {
        callbacks cbs;
        cbs.i_message_create = std::bind(&example::MessageCreate, this, _1);
        cbs.i_message_create_dm = std::bind(&example::MessageCreateDM, this, _1);
        bot._callbacks = cbs;
    }

    const snowflake bot_owner_id = 171000788183678976LL;

    // All the hooks into the websocket stream
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

    json make_info_obj(shard * _shard, aegis * bot);
};

}
