//
// structs.hpp
// ***********
//
// Copyright (c) 2021 Sharon Fox (sharon at sharonfox dot dev)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include <string>
#include <optional>
#include <memory>
#include <vector>
#include <thread>
#include <cstdint>

#include "defines.hpp"
#include "aegis/vendor/spdlog/common.h"

namespace aegis
{

class core;

struct create_message_t;

struct create_guild_t
{
    create_guild_t & name(const std::string & param) noexcept { _name = param; return *this; }
    create_guild_t & voice_region(const std::string & param) noexcept { _voice_region = param; return *this; }
    create_guild_t & verification_level(int param) noexcept { _verification_level = param; return *this; }
    create_guild_t & default_message_notifications(int param) noexcept { _default_message_notifications = param; return *this; }
    create_guild_t & explicit_content_filter(int param) noexcept { _explicit_content_filter = param; return *this; }
    /* create_guild_t & roles(const std::vector<gateway::objects::role> & param) noexcept
    {
        _roles = param; return *this;
    } */
    create_guild_t & channels(const std::vector<std::tuple<std::string, int>> & param) noexcept
    {
        _channels = param; return *this;
    }
    std::string _name;
    std::optional<std::string> _voice_region;
    std::optional<int> _verification_level;
    std::optional<int> _default_message_notifications;
    std::optional<int> _explicit_content_filter;
    std::optional<std::string> _icon;
    // std::optional<std::vector<gateway::objects::role>> _roles;
    std::optional<std::vector<std::tuple<std::string, int>>> _channels;
};

/// Class for fluent definition of bot parameters
/// The create_bot_t class allows for fluent initialisation of aegis::core objects
struct create_bot_t
{
    create_bot_t & token(const std::string & param) noexcept { _token = param; return *this; }
    create_bot_t & thread_count(const uint32_t param) noexcept { _thread_count = param; return *this; }
    create_bot_t & force_shard_count(const uint32_t param) noexcept { _force_shard_count = param; return *this; }
    create_bot_t & file_logging(const bool param) noexcept { _file_logging = param; return *this; }
    create_bot_t & bulk_members_on_connect(const bool param) noexcept { _bulk_members_on_connect = param; return *this; }
    create_bot_t & log_level(const spdlog::level::level_enum param) noexcept { _log_level = param; return *this; }
    create_bot_t & log_format(const std::string & param) noexcept { _log_format = param; return *this; }
    /**
     * Sets the base name of the log file.
     * If this is not called, the default is "aegis.log"
     * @param log_name The name of the log to create. Will be created in a subdfolder called "log"
     */
    create_bot_t & log_name(const std::string & param) noexcept { _log_name = param; return *this; }
    /**
     * Sets up clustering for large bots.
     * Clustering splits the bot's shards across multiple process, where each process takes on an equal subset of the bot's shard count.
     * For example if max_clusters is set to 2 on an 8 shard bot, then cluster_id 0 will contain shards 0, 2, 4 and 6 while cluster_id 1
     * would contain shards 1, 3, 5 and 7. It is the responsibility of your bot to marshall information across clusters if needed, (for
     * example using redis, SQL, etc ) as aegis will only see the shards that are part of the cluster for which it is authoritative.
     * @param cluster_id The cluster ID of this bot, zero-based
     * @param max_clusters The number of clusters that the bot has
     */
    create_bot_t & clustering(uint32_t cluster_id, uint32_t max_clusters) noexcept { _cluster_id = cluster_id; _max_clusters = max_clusters; return *this; }
    /**
     * Defines which events your bot will receive, events that you don't set here will be filtered out from the websocket at discord's side.
     * @param param A bit mask defined by one or more aegis::intents.
     * @returns reference to self
     */
    create_bot_t & intents(uint32_t param) noexcept { _intents = param; return *this; }
private:
    friend aegis::core;
    std::string _token;
    uint32_t _intents{ intent::IntentsDisabled };
    uint32_t _thread_count{ std::thread::hardware_concurrency() };
    uint32_t _force_shard_count{ 0 };
    uint32_t _cluster_id{ 0 };
    uint32_t _max_clusters{ 0 };
    bool _bulk_members_on_connect{ false };
    bool _file_logging{ false };
    std::string _log_name{ "aegis.log" };
    spdlog::level::level_enum _log_level{ spdlog::level::level_enum::info };
    std::string _log_format{ "%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v" };
};

}
