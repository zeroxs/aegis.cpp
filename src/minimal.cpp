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

        // These callbacks are what the lib calls when messages come in
        bot.set_on_message_create([&](aegis::gateway::events::message_create obj)
        {
            try
            {
                // C++17 version
                //const auto [channel_id, guild_id, message_id, member_id] = obj.msg.get_related_ids();
                const aegis::snowflake channel_id = obj.msg.get_channel().get_id();
                const aegis::snowflake guild_id = obj.msg.get_guild().get_id();
                const aegis::snowflake message_id = obj.msg.get_id();
#if !defined(AEGIS_DISABLE_ALL_CACHE)
                const aegis::snowflake member_id = obj.msg.get_member().get_id();
#else
                const aegis::snowflake member_id = obj.msg.author.id;
#endif

                // Is message author myself?
                if (member_id == obj.bot->get_id())
                    return;
     
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
                        // without any sleep/wait, an instant reaction may fail due to ratelimits
                        std::this_thread::sleep_for(250ms);
                        auto response = obj.msg.create_reaction("fail:429554869611921408");

                        // perhaps you'd like to leverage asio to respond
                        aegis::async([&_channel, fut = std::move(response)]() mutable
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
        });

        // start the bot
        bot.run();
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
