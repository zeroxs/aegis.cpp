//
// snowflake.hpp
// *************
//
// Copyright (c) 2018 Sharon W (sharon at aegis dot gg)
//
// Distributed under the MIT License. (See accompanying file LICENSE)
//

#pragma once

#include "aegis/config.hpp"
#include <nlohmann/json.hpp>
#include <stdint.h>
#include <string>

namespace aegis
{

/// Stores creation time and extra data specific to Discord for entities
class snowflake
{
public:
    constexpr snowflake() : snowflake_id(0) {}
    constexpr snowflake(int64_t _snowflake) : snowflake_id(_snowflake) {}

    constexpr operator int64_t() const noexcept
    {
        return snowflake_id;
    }

    constexpr int64_t get() const noexcept
    {
        return snowflake_id;
    };

    constexpr std::tuple<int64_t, int8_t, int8_t, int16_t> get_all() const noexcept
    {
        return std::tuple<int64_t, int8_t, int8_t, int16_t>{ get_timestamp(), get_worker(), get_process(), get_count() };
    };

    constexpr int16_t get_count() const noexcept
    {
        return static_cast<int16_t>(snowflake_id & _countMask);
    };

    constexpr int8_t get_process() const noexcept
    {
        return static_cast<int8_t>((snowflake_id & _workerMask) >> 12);
    };

    constexpr int8_t get_worker() const noexcept
    {
        return static_cast<int8_t>((snowflake_id & _workerMask) >> 17);
    };

    constexpr int64_t get_timestamp() const noexcept
    {
        return (snowflake_id & _timestampMask) >> 22;
    };

    static constexpr std::tuple<int64_t, int8_t, int8_t, int16_t> c_get_all(int64_t snowflake)
    {
        return std::tuple<int64_t, int8_t, int8_t, int16_t>{ c_get_timestamp(snowflake), c_get_worker(snowflake), c_get_process(snowflake), c_get_count(snowflake) };
    };

    static constexpr int16_t c_get_count(int64_t snowflake)
    {
        return static_cast<int16_t>(snowflake & _countMask);
    };

    static constexpr int8_t c_get_process(int64_t snowflake)
    {
        return static_cast<int8_t>((snowflake & _workerMask) >> 12);
    };

    static constexpr int8_t c_get_worker(int64_t snowflake)
    {
        return static_cast<int8_t>((snowflake & _workerMask) >> 17);
    };

    static constexpr int64_t c_get_timestamp(int64_t snowflake)
    {
        return (snowflake & _timestampMask) >> 22;
    };

    constexpr int64_t get_time() const noexcept
    {
        return get_timestamp() + _discordEpoch;
    };

    static constexpr int64_t c_get_time(int64_t snowflake)
    {
        return c_get_timestamp(snowflake) + _discordEpoch;
    };

private:
    uint64_t snowflake_id;
    static constexpr uint64_t _countMask = 0x0000000000000FFFL;
    static constexpr uint64_t _processMask = 0x000000000001F000L;
    static constexpr uint64_t _workerMask = 0x00000000003E0000L;
    static constexpr uint64_t _timestampMask = 0xFFFFFFFFFFC00000L;
    static constexpr uint64_t _discordEpoch = 1420070400000;
};

/**\todo Needs documentation
 */
inline void from_json(const nlohmann::json& j, snowflake& s)
{
    if (j.is_string())
        s = std::stoll(j.get<std::string>());
    else if (j.is_number())
        s = j.get<int64_t>();
}

/**\todo Needs documentation
 */
inline void to_json(nlohmann::json& j, const snowflake& s)
{
    j = nlohmann::json{ static_cast<int64_t>(s) };
}

}

namespace std
{

template <>
struct hash<aegis::snowflake>
{
    std::size_t operator()(const aegis::snowflake& k) const
    {
        return hash<int64_t>()(k.get());
    }
};

}
