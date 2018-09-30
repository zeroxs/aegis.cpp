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
    constexpr snowflake() noexcept : snowflake_id(0) {}
    constexpr snowflake(int64_t _snowflake) noexcept : snowflake_id(_snowflake) {}

    constexpr operator int64_t() const noexcept
    {
        return snowflake_id;
    }

    /// Retrieve full snowflake value
    /**
     * @returns int64_t Snowflake
     */
    constexpr int64_t get() const noexcept
    {
        return snowflake_id;
    };

    /// Obtain all the snowflake values as a tuple
    /**
     * @returns std::tuple of all the snowflake parts
     */
    constexpr std::tuple<int64_t, int8_t, int8_t, int16_t> get_all() const noexcept
    {
        return std::tuple<int64_t, int8_t, int8_t, int16_t>{ get_timestamp(), get_worker(), get_process(), get_count() };
    };

    /// 
    /**
     * @returns int16_t Internal counter for snowflakes
     */
    constexpr int16_t get_count() const noexcept
    {
        return static_cast<int16_t>(snowflake_id & _countMask);
    };

    /// 
    /**
     * @returns int8_t Process ID of the snowflake generator
     */
    constexpr int8_t get_process() const noexcept
    {
        return static_cast<int8_t>((snowflake_id & _workerMask) >> 12);
    };

    /// 
    /**
     * @returns int8_t Worker ID of the snowflake generator
     */
    constexpr int8_t get_worker() const noexcept
    {
        return static_cast<int8_t>((snowflake_id & _workerMask) >> 17);
    };

    /// 
    /**
     * @returns int64_t The Discord epoch timestamp this snowflake was generated
     */
    constexpr int64_t get_timestamp() const noexcept
    {
        return (snowflake_id & _timestampMask) >> 22;
    };

    /// Obtain all the snowflake values as a tuple
    /**
     * @returns std::tuple of all the snowflake parts
     */
    static constexpr std::tuple<int64_t, int8_t, int8_t, int16_t> c_get_all(int64_t snowflake) noexcept
    {
        return std::tuple<int64_t, int8_t, int8_t, int16_t>{ c_get_timestamp(snowflake), c_get_worker(snowflake), c_get_process(snowflake), c_get_count(snowflake) };
    };

    /// 
    /**
     * @returns int16_t Internal counter for snowflakes
     */
    static constexpr int16_t c_get_count(int64_t snowflake) noexcept
    {
        return static_cast<int16_t>(snowflake & _countMask);
    };

    /// 
    /**
     * @returns int8_t Process ID of the snowflake generator
     */
    static constexpr int8_t c_get_process(int64_t snowflake) noexcept
    {
        return static_cast<int8_t>((snowflake & _workerMask) >> 12);
    };

    /// 
    /**
     * @returns int8_t Worker ID of the snowflake generator
     */
    static constexpr int8_t c_get_worker(int64_t snowflake) noexcept
    {
        return static_cast<int8_t>((snowflake & _workerMask) >> 17);
    };

    /// 
    /**
     * @returns int64_t The Discord epoch timestamp this snowflake was generated
     */
    static constexpr int64_t c_get_timestamp(int64_t snowflake) noexcept
    {
        return (snowflake & _timestampMask) >> 22;
    };

    /// 
    /**
     * @returns int64_t Unix timestamp this snowflake was generated
     */
    constexpr int64_t get_time() const noexcept
    {
        return get_timestamp() + _discordEpoch;
    };

    /// 
    /**
     * @returns int64_t Unix timestamp this snowflake was generated
     */
    static constexpr int64_t c_get_time(int64_t snowflake) noexcept
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

/// \cond TEMPLATES
inline void from_json(const nlohmann::json& j, snowflake& s)
{
    if (j.is_string())
        s = std::stoll(j.get<std::string>());
    else if (j.is_number())
        s = j.get<int64_t>();
}
/// \endcond

/// \cond TEMPLATES
inline void to_json(nlohmann::json& j, const snowflake& s)
{
    j = nlohmann::json{ static_cast<int64_t>(s) };
}
/// \endcond

}

/// \cond TEMPLATES
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
/// \endcond
