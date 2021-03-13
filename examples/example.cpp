//
// aegis.hpp
// *********
//
// Copyright (c) 2021 Sharon Fox (sharon at sharonfox dot dev)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "example.hpp"
#include <chrono>
#include <thread>
#include <iostream>
//using json = nlohmann::json;

#include "aegis/vendor/spdlog/sinks/stdout_color_sinks.h"
#include "aegis/vendor/spdlog/sinks/rotating_file_sink.h"
#include "aegis/vendor/spdlog/async.h"

int main(int argc, char * argv[])
{
    using namespace std::chrono_literals;
    try
    {
        aegis::core bot;

        example::example commands(&bot);

        //bot.run();
        //bot.yield();

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

namespace example
{

}
