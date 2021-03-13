//
// core.hpp
// ********
//
// Copyright (c) 2021 Sharon Fox (sharon at sharonfox dot dev)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include <memory>

#include "aegis/structs.hpp"

namespace spdlog
{
class logger;
}

namespace aegis
{

class core
{
public:

    /// Constructs the aegis object that tracks all of the shards, guilds, channels, and members
    /// This constructor creates its own spdlog::logger and asio::io_context
    /**
     * @param loglevel The level of logging to use
     * @param count Amount of threads to start
     */
    explicit core(create_bot_t bot_config);

    void shutdown();

    void setup_logging();

    std::shared_ptr<spdlog::logger> log;


private:
    create_bot_t config;
};

}
