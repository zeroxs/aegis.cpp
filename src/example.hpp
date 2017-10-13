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

#include <aegis/aegis.hpp>
#include <aegis/client.hpp>
#include <json.hpp>
#include <functional>


namespace example_bot
{

using aegis::client;
using json = nlohmann::json;
using namespace std::placeholders;
using namespace aegis;


template<typename bottype>
class example
{
public:

    //////////////////////////////////////////////////////////////////////////

    class member
    {
    public:
        member() {}
        member(int64_t id) : member_id(id) {}
        int64_t member_id = 0;

    };

    class channel
    {
    public:
        channel() {}
        channel(int64_t id, int64_t g_id) : channel_id(id), guild_id(g_id) {}
        int64_t channel_id = 0;
        int64_t guild_id = 0;
        void sendMessage(std::string content, Aegis<bottype> & bot)
        {
            bot.ratelimit().get(rest_limits::bucket_type::CHANNEL).push(channel_id, fmt::format("/channels/{}/messages", channel_id), content, "POST");
        }
        void sendMessageEmbed(std::string content, std::string embed, Aegis<bottype> & bot)
        {
            json obj;
            if (!content.empty())
                obj["content"] = content;
            obj["embed"] = embed;

            bot.ratelimit().get(rest_limits::bucket_type::CHANNEL).push(channel_id, fmt::format("/channels/{}/messages", channel_id), obj.dump(), "POST");
        }
    };

    class guild
    {
    public:
        guild() {}
        guild(int64_t id) : guild_id(id) {}
        int64_t guild_id = 0;

    };

    //////////////////////////////////////////////////////////////////////////

    using c_inject = std::function<bool(json & msg, client & shard, Aegis<bottype> & bot)>;

    example() = default;
    ~example() = default;

    std::map<int64_t, member> m_members;
    std::map<int64_t, guild> m_guilds;
    std::map<int64_t, channel> m_channels;

    // Functions you wish you hook into
    void inject(aegis::Aegis<bottype> & bot)
    {
        bot.i_message_create = std::bind(&example::message_create, this, _1, _2, _3);
        bot.i_guild_create = std::bind(&example::guild_create, this, _1, _2, _3);
        bot.i_guild_delete = std::bind(&example::guild_delete, this, _1, _2, _3);
        bot.i_ready = std::bind(&example::ready, this, _1, _2, _3);
        bot.i_resumed = std::bind(&example::resumed, this, _1, _2, _3);
    }


    // All the hooks into the websocket stream
    // Your hooked functions take priority over the library for processing.
    // Returning a false makes the library skip handling the data when you are done. (except READY)
    bool typing_start(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool message_create(json & msg, client & shard, Aegis<bottype> & bot)
    {
        json author = msg["d"]["author"];

        uint64_t userid = std::stoull(author["id"].get<std::string>());
        std::string username = author["username"];

        uint64_t channel_id = std::stoull(msg["d"]["channel_id"].get<std::string>());
        uint64_t id = std::stoull(msg["d"]["id"].get<std::string>());
        std::string content = msg["d"]["content"];

        std::string avatar = author["avatar"].is_string() ? author["avatar"] : "";
        uint16_t discriminator = std::stoi(author["discriminator"].get<std::string>());

        uint64_t msgid = std::stoull(msg["d"]["id"].get<std::string>());

        if (userid == 171000788183678976)
        {
            auto toks = split(content, ' ');
            if (toks[0] == "?info")
            {
                uint64_t guild_count = m_guilds.size();
                uint64_t member_count = m_members.size();
                uint64_t member_count_unique = 0;
                uint64_t member_online_count = 0;
                uint64_t member_dnd_count = 0;
                uint64_t channel_count = m_channels.size();
                uint64_t channel_text_count = 0;
                uint64_t channel_voice_count = 0;
                uint64_t member_count_active = 0;

                uint64_t eventsseen = 0;

                {
                    for (auto & bot_ptr : bot.m_clients)
                        eventsseen += bot_ptr->m_sequence;

                    for (auto & member : bot.m_members)
                    {
                        /*if (member.second->status == MEMBER_STATUS::ONLINE)
                        member_online_count++;
                        else if (member.second->status == MEMBER_STATUS::DND)
                        member_dnd_count++;*/
                    }

                    for (auto & channel : bot.m_channels)
                    {
                        /*if (channel.second->type == ChannelType::TEXT)
                        channel_text_count++;
                        else
                        channel_voice_count++;*/
                    }

                    /*for (auto & guild : AegisBot::guildlist)
                    member_count_active += guild.second->memberlist.size();*/

                    member_count = m_members.size();
                }

                std::string members = fmt::format("{0} seen online", member_count);
                std::string channels = fmt::format("{0} total\n{1} text\n{2} voice", channel_count, channel_text_count, channel_voice_count);
                std::string guilds = fmt::format("{0}", guild_count);
                std::string events = fmt::format("{0}", eventsseen);
                std::string misc = fmt::format("I am shard {0} of {1} running on `{2}`", shard.m_shardid+1, bot.m_shardidmax, aegis::utility::platform::get_platform());

                fmt::MemoryWriter w;
                w << "[Latest bot source](https://github.com/zeroxs/aegis)\n[Official Bot Server](https://discord.gg/w7Y3Bb8)\n\nMemory usage: "
                    << double(aegis::utility::getCurrentRSS()) / (1024 * 1024) << "MB\nMax Memory: "
                    << double(aegis::utility::getPeakRSS()) / (1024 * 1024) << "MB";
                std::string stats = w.str();


                json obj;
                json t = {
                    { "title", "AegisBot" },
                    { "description", stats },
                    { "color", rand() % 0xFFFFFF },
                    { "fields",
                    json::array(
                {
                    { { "name", "Members" },{ "value", members },{ "inline", true } },
                    { { "name", "Channels" },{ "value", channels },{ "inline", true } },
                    { { "name", "Uptime" },{ "value", bot.uptime() },{ "inline", true } },
                    { { "name", "Guilds" },{ "value", guilds },{ "inline", true } },
                    { { "name", "Events Seen" },{ "value", events },{ "inline", true } },
                    { { "name", u8"\u200b" },{ "value", u8"\u200b" },{ "inline", true } },
                    { { "name", "misc" },{ "value", misc },{ "inline", false } }
                }
                        )
                    },
                    { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", "Made in c++ running aegis library" } } }
                };
                obj["embed"] = t;
                
                m_channels[channel_id].sendMessageEmbed("", obj.dump(), bot);
                return true;
            }
            else if (toks[0] == "?exit")
            {
                json obj =
                {
                    { "content", "exiting..." }
                };
                m_channels[channel_id].sendMessage(obj.dump(), bot);
                bot.set_state(SHUTDOWN);
                bot.websocket().close(shard.m_connection, 1002, "");
                bot.stop_work();
                return true;
            }
            else if (toks[0] == "?test")
            {
                m_channels[channel_id].sendMessage("test message", bot);
                return true;
            }
            else if (toks[0] == "?shard")
            {
                m_channels[channel_id].sendMessage(json({ { "content", fmt::format("I am shard#[{}]", shard.m_shardid) } }).dump(), bot);
                return true;
            }
        }

        return true;
    }

    bool message_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool message_delete(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool message_delete_bulk(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_create(json & msg, client & shard, Aegis<bottype> & bot)
    {
        int64_t g_id = std::stoll(msg["d"]["id"].get<std::string>());

        m_guilds.try_emplace(g_id, guild(g_id));


        if (msg["d"].count("channels"))
        {
            json channels = msg["d"]["channels"];

            for (auto & channel_r : channels)
            {
                int64_t c_id = std::stoll(channel_r["id"].get<std::string>());
                m_channels.try_emplace(c_id, channel( c_id, g_id ));
            }
        }


        if (msg["d"].count("members"))
        {
            json members = msg["d"]["members"];

            for (auto & member_r : members)
            {
                int64_t m_id = std::stoll(member_r["user"]["id"].get<std::string>());
                m_members.try_emplace(m_id, member(m_id));
            }
        }

        return true;
    }

    bool guild_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_delete(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool user_settings_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool user_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool ready(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool resumed(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool channel_create(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool channel_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool channel_delete(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_ban_add(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_ban_remove(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_emojis_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_integrations_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_member_add(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_member_remove(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_member_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_member_chunk(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_role_create(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_role_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool guild_role_delete(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool presence_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool voice_state_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }

    bool voice_server_update(json & msg, client & shard, Aegis<bottype> & bot)
    {
        return true;
    }
};

}

