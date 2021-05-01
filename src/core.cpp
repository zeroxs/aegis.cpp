//
// core.hpp
// ********
//
// Copyright (c) 2021 Sharon Fox (sharon at sharonfox dot dev)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#include "aegis/core.hpp"

#include "aegis/vendor/spdlog/spdlog.h"
#include "aegis/vendor/spdlog/sinks/stdout_color_sinks.h"
#include "aegis/vendor/spdlog/sinks/rotating_file_sink.h"
#include "aegis/vendor/spdlog/async.h"

namespace aegis
{

core::core(create_bot_t bot_config)
    : config(bot_config)
{

}

void core::shutdown()
{
    spdlog::shutdown();
}

void core::setup_logging()
{
    spdlog::init_thread_pool(8192, 2);

    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
    std::vector<spdlog::sink_ptr> sinks;// { stdout_sink, rotating_sink };
    sinks.push_back(stdout_sink);

    if (config._file_logging)
    {
        // 5MB max filesize and 10 max log files
        auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(fmt::format("log/{}", config._log_name), 1024 * 1024 * 5, 10);
        sinks.push_back(rotating);
    }

    log = std::make_shared<spdlog::async_logger>("aegis", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    spdlog::register_logger(log);
    spdlog::set_default_logger(log);

    log->set_pattern(config._log_format);
    log->set_level(config._log_level);
}

}
