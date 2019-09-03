//
// example_main.cpp
// ****************
//
// Copyright (c) 2019 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "example.hpp"

using json = nlohmann::json;

int main(int argc, char * argv[])
{
    using namespace std::chrono_literals;
    try
    {
        aegis::core bot(aegis::create_bot_t().log_level(spdlog::level::trace).token("TOKEN"));

        example_bot::example commands(bot);

        commands.attach();

        bot.run();
        bot.yield();

        std::cout << "Press any key to continue...\n";
        std::cin.ignore();
    }
    catch (std::exception & e)
    {
        std::cout << "Error during initialization: " << e.what() << '\n';
        return -1;
    }
    catch (...)
    {
        std::cout << "Error during initialization: uncaught\n";
        return -1;
    }
    std::this_thread::sleep_for(5ms);
    return 0;
}
