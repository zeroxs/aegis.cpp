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
#include <future>
#include <aegis.hpp>

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

    using c_inject = std::function<bool(json & msg, shard & _shard, aegis & bot)>;

    // Messages you want to process
    void inject(aegis & bot)
    {
        bot.i_typing_start = std::bind(&example::TypingStart, this, _1);
        bot.i_message_create = std::bind(&example::MessageCreate, this, _1);
        //bot.i_guild_create = std::bind(&example::guild_create, this, _1, _2, _3);
        //bot.i_guild_delete = std::bind(&example::guild_delete, this, _1, _2, _3);
        //bot.i_ready = std::bind(&example::ready, this, _1, _2, _3);
        //bot.i_resumed = std::bind(&example::resumed, this, _1, _2, _3);
    }


    // All the hooks into the websocket stream
    bool TypingStart(typing_start obj);

    bool MessageCreate(message_create msg);
    
    bool extremely_simplified_message_handler(json & msg, shard * shard, aegis & bot);

    bool message_update(json & msg, shard & shard, aegis & bot);

    bool message_delete(json & msg, shard & shard, aegis & bot);

    bool message_delete_bulk(json & msg, shard & shard, aegis & bot);

    bool guild_create(json & msg, shard & shard, aegis & bot);

    bool guild_update(json & msg, shard & shard, aegis & bot);

    bool guild_delete(json & msg, shard & shard, aegis & bot);

    bool user_settings_update(json & msg, shard & shard, aegis & bot);

    bool user_update(json & msg, shard & shard, aegis & bot);

    bool ready(json & msg, shard & shard, aegis & bot);

    bool resumed(json & msg, shard & shard, aegis & bot);

    bool channel_create(json & msg, shard & shard, aegis & bot);

    bool channel_update(json & msg, shard & shard, aegis & bot);

    bool channel_delete(json & msg, shard & shard, aegis & bot);

    bool guild_ban_add(json & msg, shard & shard, aegis & bot);

    bool guild_ban_remove(json & msg, shard & shard, aegis & bot);

    bool guild_emojis_update(json & msg, shard & shard, aegis & bot);

    bool guild_integrations_update(json & msg, shard & shard, aegis & bot);

    bool guild_member_add(json & msg, shard & shard, aegis & bot);

    bool guild_member_remove(json & msg, shard & shard, aegis & bot);

    bool guild_member_update(json & msg, shard & shard, aegis & bot);

    bool guild_member_chunk(json & msg, shard & shard, aegis & bot);

    bool guild_role_create(json & msg, shard & shard, aegis & bot);

    bool guild_role_update(json & msg, shard & shard, aegis & bot);

    bool guild_role_delete(json & msg, shard & shard, aegis & bot);

    bool presence_update(json & msg, shard & shard, aegis & bot);

    bool voice_state_update(json & msg, shard & shard, aegis & bot);

    bool voice_server_update(json & msg, shard & shard, aegis & bot);
};

}

#include "example_impl.hpp"

