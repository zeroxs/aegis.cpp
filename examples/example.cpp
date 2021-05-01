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
#include <sstream>
//using json = nlohmann::json;

#include "aegis/vendor/ixwebsocket/IXNetSystem.h"
#include "aegis/vendor/ixwebsocket/IXWebSocket.h"

#include "aegis/vendor/spdlog/spdlog.h"
#include "aegis/vendor/zstr/zstr.hpp"
#include "aegis/vendor/simdjson/simdjson.hpp"

int main(int argc, char *argv[])
{
    using namespace std::chrono_literals;
    try
    {
        aegis::create_bot_t config;
        config.token("TOKEN");
        config.force_shard_count(3);
        aegis::core bot(config);

        bot.setup_logging();

        std::cout << "Press any key to continue...\n";
        std::cin.ignore();
    }
    catch (std::exception &e)
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
