//
// minimal.cpp
// ***********
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include <aegis.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char * argv[])
{
    using namespace std::chrono_literals;
    try
    {
        // Create bot object
        aegis::core bot(spdlog::level::trace);

        // Some pieces of the ping test
        std::mutex m_ping_test;
        std::condition_variable cv_ping_test;
        int64_t ws_checktime = 0;

        // These callbacks are what the lib calls when messages come in
        bot.i_message_create = [&](aegis::gateway::events::message_create obj)
        {
            try
            {
                // C++17 version
                //const auto [channel_id, guild_id, message_id, member_id] = obj.msg.get_related_ids();
                auto rets = obj.msg.get_related_ids();

                const aegis::snowflake channel_id = std::get<0>(rets);
                const aegis::snowflake guild_id = std::get<1>(rets);
                const aegis::snowflake message_id = std::get<2>(rets);
                const aegis::snowflake member_id = std::get<3>(rets);

                //ping test
                static aegis::snowflake ping_check(0);
                static int64_t checktime(0);

                // Is message author myself?
                if (obj.msg.author.user_id == obj.bot->get_id())
                {
                    // Does nonce of message match previous ping attempt?
                    if (obj.msg.nonce == checktime)
                    {
                        // Record time and notify original thread that ping came through
                        ws_checktime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - checktime;
                        cv_ping_test.notify_all();
                        return;
                    }
                }

                // Ignore bot messages and DMs
                if (obj.msg.is_bot() || !obj.msg.has_guild())
                    return;

                auto & _channel = obj.msg.get_channel();
                auto & _guild = _channel.get_guild();
                auto & username = obj.msg.author.username;
            
                std::string content{ obj.msg.get_content() };

                // Simple Hi response
                if (content == "~Hi")
                {
                    _channel.create_message("Hello back");
                }
                else if (content == "~React")
                {
                    auto response = obj.msg.create_reaction("success:429554838083207169").get();
                    if (response)
                    {
                        // reaction was a success. maybe chain another?
                        auto response = obj.msg.create_reaction("fail:429554869611921408");

                        // perhaps you'd like to leverage asio to respond
                        bot.async([&_channel, fut = std::move(response)]() mutable
                        {
                            auto response = fut.get();
                            if (response)
                                _channel.create_message("React complete");
                            else
                                _channel.create_message("React failed");
                        });
                    }
                }
                // Send a message, wait for message to successfully be sent, then react to that message
                else if (content == "~Delay")
                {
                    auto reply = _channel.create_message("First message").get();
                    // bool tests true if the http code returned was 200, 201, 202, or 204
                    if (reply)
                    {
                        // parse the message object returned by the first message
                        aegis::gateway::objects::message msg = json::parse(reply.content);
                        // add a reaction to that new message
                        msg.create_reaction("success:429554838083207169");
                    }
                }
                // Complex ping reply. Edits bot's message to display REST response time.
                // Then when the Websocket receives the message, edits it again to show the time
                // the message took to come over the websocket
                else if (content == "~ping")
                {
                    std::unique_lock<std::mutex> lk(m_ping_test);
                    checktime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                    auto apireply = _channel.create_message("Pong", checktime).get();
                    if (apireply)
                    {
                        aegis::gateway::objects::message msg = json::parse(apireply.content);
                        std::string to_edit = fmt::format("Ping reply: REST [{}ms]", (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - checktime));
                        // edit message to show REST reply time
                        msg.edit(to_edit);
                        if (cv_ping_test.wait_for(lk, 20s) == std::cv_status::no_timeout)
                        {
                            // edit message to show REST and websocket reply times
                            msg.edit(fmt::format("{} WS [{}ms]", to_edit, ws_checktime));
                            return;
                        }
                        else
                        {
                            msg.edit(fmt::format("{} WS [timeout20s]", to_edit));
                            return;
                        }
                    }
                }
                else if (content == "~info")
                {
                    fmt::MemoryWriter w;
                    w << "[Latest bot source](https://github.com/zeroxs/aegis.cpp)\n[Official Bot Server](https://discord.gg/Kv7aP5K)\n\nMemory usage: "
                        << double(aegis::utility::getCurrentRSS()) / (1024 * 1024) << "MB\nMax Memory: "
                        << double(aegis::utility::getPeakRSS()) / (1024 * 1024) << "MB";
                    std::string stats = w.str();
                    int64_t eventsseen = 0;
                    for (auto & e : bot.message_count)
                        eventsseen += e.second;

    #if defined(DEBUG) || defined(_DEBUG)
                    std::string build_mode = "DEBUG";
    #else
                    std::string build_mode = "RELEASE";
    #endif
                    std::string misc = fmt::format("I am shard {} of {} running on `{}` in `{}` mode", obj._shard->get_id() + 1, bot.shard_max_count, aegis::utility::platform::get_platform(), build_mode);

    #if !defined(AEGIS_DISABLE_ALL_CACHE)
                    json t = {
                        { "title", "AegisBot" },
                        { "description", stats },
                        { "color", rand() % 0xFFFFFF },
                        { "fields",
                        json::array(
                            {
                                { { "name", "Members" },{ "value", bot.members.size() },{ "inline", true } },
                                { { "name", "Channels" },{ "value", bot.channels.size() },{ "inline", true } },
                                { { "name", "Uptime" },{ "value", bot.uptime() },{ "inline", true } },
                                { { "name", "Guilds" },{ "value", bot.guilds.size() },{ "inline", true } },
                                { { "name", "Events Seen" },{ "value", fmt::format("{}", eventsseen) },{ "inline", true } },
                                { { "name", u8"\u200b" },{ "value", u8"\u200b" },{ "inline", true } },
                                { { "name", "misc" },{ "value", misc },{ "inline", false } }
                            }
                            )
                        },
                        { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", fmt::format("Made in C++{} running {}", CXX_VERSION, AEGIS_VERSION_TEXT) } } }
                    };
    #else
                    json t = {
                        { "title", "AegisBot" },
                        { "description", stats },
                        { "color", rand() % 0xFFFFFF },
                        { "fields",
                        json::array(
                            {
                                { { "name", "Channels" },{ "value", bot.channels.size() },{ "inline", true } },
                                { { "name", "Guilds" },{ "value", bot.guilds.size() },{ "inline", true } },
                                { { "name", "Uptime" },{ "value", bot.uptime() },{ "inline", true } },
                                { { "name", "Events Seen" },{ "value", fmt::format("{}", eventsseen) },{ "inline", true } },
                                { { "name", u8"\u200b" },{ "value", u8"\u200b" },{ "inline", true } },
                                { { "name", u8"\u200b" },{ "value", u8"\u200b" },{ "inline", true } },
                                { { "name", "misc" },{ "value", misc },{ "inline", false } }
                            }
                            )
                        },
                        { "footer",{ { "icon_url", "https://cdn.discordapp.com/emojis/289276304564420608.png" },{ "text", fmt::format("Made in C++{} running {}", CXX_VERSION, AEGIS_VERSION_TEXT) } } }
                    };
    #endif

                    _channel.create_message_embed({}, t);
                    return;
                }
                else if (content == "~exit")
                {
                    bot.shutdown();
                    return;
                }

            }
            catch (std::exception & e)
            {

            }
            return;
        };

        // start the bot. the function passed is executed after the internal logger is set up and the
        // websocket gateway information is collected.
        bot.run(1, [&]
        {
            bot.log->trace("stuff");
            bot.log->debug("stuff");
            bot.log->info("stuff");
            bot.log->warn("stuff");
            bot.log->error("stuff");
            bot.log->critical("stuff");
        });
    }
    catch (std::exception & e)
    {
        std::cout << "Error during initialization: " << e.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cout << "Error during initialization: uncaught\n";
        return 1;
    }
    std::cout << "Press any key to continue...\n";
    std::cin.ignore();
    return 0;
}
